# RGBW Cube Firmware Design

## Purpose

This document describes the intended C++ architecture for an ESP32-driven cube
with multiple 8x8 RGBW NeoPixel panels. The design begins with the
current single-panel test hardware but is intended to support six independently
addressable faces, shared cube-wide effects, an absolute orientation sensor,
wireless commands and parameter changes, and over-the-air firmware updates.

The main architectural requirement is that effects must be nonblocking. An
effect renders one frame and returns; it must never own an infinite loop or
sleep while rendering. This allows the application to update several panels,
process wireless commands, read sensors, enforce power limits, and service OTA
operations concurrently.

## Design goals

- Assign an effect object to each panel by pointer.
- Pass the panel index to every effect render call.
- Permit one effect instance to serve one panel, several panels, or the entire
  cube.
- Keep effect rendering independent from the physical LED wiring.
- Allow effect parameters to change safely while the program is running.
- Use one command model for Bluetooth, Wi-Fi, USB serial, and future transports.
- Keep wireless protocols and OTA code separate from effect implementations.
- Avoid allocation and unbounded work in the frame-rendering path.
- Make effect logic testable without an ESP32 or attached LEDs.
- Preserve a global place to enforce brightness and current limits.

## Terminology

- **Panel**: one physical 8x8 RGBW matrix.
- **Panel index**: a stable integer identifying a cube face.
- **Frame buffer**: 64 logical RGBW pixels for one panel.
- **Effect**: an object that produces frames over time.
- **Effect manager**: owns panel assignments and schedules rendering.
- **Panel driver**: translates logical frames into physical LED order.
- **Command**: a transport-independent request to change application state.

## Core value types

The core types should be small value objects without dynamic allocation.

```cpp
using PanelIndex = uint8_t;

struct Rgbw {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t w = 0;
};

class FrameBuffer {
public:
    static constexpr size_t width = 8;
    static constexpr size_t height = 8;
    static constexpr size_t pixelCount = width * height;

    Rgbw& at(size_t x, size_t y);
    const Rgbw& at(size_t x, size_t y) const;
    void clear(Rgbw color = {});

private:
    std::array<Rgbw, pixelCount> pixels_{};
};
```

`FrameBuffer` uses logical coordinates. It must not know whether a panel is
wired in progressive rows, serpentine rows, columns, or a rotated orientation.
Those transformations belong in the panel driver.

## Effect interface

Each effect receives a panel index, timing and shared inputs, and a destination
frame buffer.

```cpp
struct EffectContext {
    uint32_t elapsedMs;
    uint32_t deltaMs;

    // Added when the hardware is integrated:
    // Orientation orientation;
    // InputState inputs;
};

class Effect {
public:
    virtual ~Effect() = default;

    virtual const char* name() const = 0;
    virtual void start(PanelIndex panel) = 0;
    virtual void stop(PanelIndex panel) = 0;

    virtual void render(
        PanelIndex panel,
        const EffectContext& context,
        FrameBuffer& output
    ) = 0;

    virtual bool setParameter(
        uint16_t parameterId,
        float value
    ) = 0;
};
```

The manager calls `render()` once per panel per frame. A render call must:

- finish promptly;
- avoid delays and blocking I/O;
- avoid heap allocation;
- write a complete logical frame;
- derive animation timing from `EffectContext` rather than call frequency.

The effect pointer and panel index relationship is explicit:

```text
panel index -> Effect* -> render(panel index, context, frame buffer)
```

Passing the panel index lets a shared effect render different faces of one
simulation. For example, a world-stabilized field can maintain one 3D model and
project it differently for the front, back, left, right, top, and bottom.

## Effect state

An effect may keep independent state for each panel:

```cpp
class ExpandingRings final : public Effect {
public:
    void render(
        PanelIndex panel,
        const EffectContext& context,
        FrameBuffer& output
    ) override;

private:
    struct State {
        uint8_t activeRing = 0;
        bool movingOutward = true;
        uint32_t phaseStartedMs = 0;
    };

    std::array<State, 6> states_{};
    Parameters parameters_{};
};
```

Alternatively, an effect may intentionally share state across panels:

```cpp
class WorldField final : public Effect {
private:
    SharedSimulation simulation_;
};
```

The choice belongs to the effect. The manager should not assume that assigning
the same pointer to two panels creates two independent animations.

## Effect manager

The effect manager records which effect pointer is assigned to each panel and
when that assignment started.

```cpp
struct PanelAssignment {
    Effect* effect = nullptr;
    uint32_t startedAtMs = 0;
};

class EffectManager {
public:
    void assign(PanelIndex panel, Effect* effect);
    void clear(PanelIndex panel);
    void setParameter(PanelIndex panel, uint16_t parameterId, float value);
    void render(uint32_t nowMs);

private:
    static constexpr size_t panelCount = 6;

    std::array<PanelAssignment, panelCount> assignments_{};
    std::array<FrameBuffer, panelCount> frames_{};
    PanelDriver& panelDriver_;
};
```

A typical render pass is:

```cpp
void EffectManager::render(uint32_t nowMs)
{
    for (PanelIndex panel = 0; panel < assignments_.size(); ++panel) {
        auto& assignment = assignments_[panel];

        if (assignment.effect == nullptr) {
            frames_[panel].clear();
            continue;
        }

        EffectContext context{
            .elapsedMs = nowMs - assignment.startedAtMs,
            .deltaMs = frameIntervalMs,
        };

        assignment.effect->render(panel, context, frames_[panel]);
    }

    panelDriver_.show(frames_);
}
```

Assignment examples:

```cpp
ExpandingRings rings;
DigitalRain rain;

manager.assign(0, &rings);
manager.assign(1, &rain);
manager.assign(2, &rings);
```

Effects should have stable lifetimes. Initially they can be statically allocated
for the life of the application. The manager stores non-owning pointers and
must never outlive the corresponding objects.

## Panel driver

The panel driver owns the ESP-IDF LED-strip handles and all knowledge of physical
wiring. Its responsibilities are:

- map `(panel, x, y)` to a physical LED index;
- apply rotation and mirroring for each cube face;
- convert logical RGBW ordering to the device component order;
- apply color correction and gamma mapping if configured;
- enforce per-pixel and global current limits;
- transmit complete frames to the panels.

```cpp
class PanelDriver {
public:
    void show(std::span<const FrameBuffer> frames);
    void setGlobalBrightness(uint8_t value);
};
```

Power limiting must remain downstream from effects. Effects describe desired
colors; the driver or a dedicated `PowerLimiter` produces a safe output. This
prevents a new or remotely selected effect from bypassing electrical limits.

## Timing and scheduling

The application should use one animation task with a stable target frame rate,
for example 30 or 60 frames per second. It performs this sequence:

1. Read and apply queued commands.
2. Snapshot the latest orientation and input state.
3. Render all assigned effects.
4. Apply brightness and power limiting.
5. Transmit frames.
6. Wait until the next scheduled frame boundary.

Effects implement animation as state machines based on elapsed time. The current
expanding-ring effect must therefore be migrated away from its infinite loop and
FreeRTOS delays. Each render call computes the current ring, direction, dwell,
and crossfade from time and returns immediately.

## Parameters

Parameters should have stable numeric identifiers on the wire. Text names may
be used by local tools but should not be required in Bluetooth packets.

```cpp
enum class RingsParameter : uint16_t {
    InnerDwell,
    StepDwell,
    ActiveBrightness,
    InnerBrightness,
};
```

Each effect validates parameter values before accepting them. Invalid IDs,
non-finite values, and unsafe ranges must be rejected. Persistent parameters can
later be stored in NVS, but persistence should be managed outside the render
path.

For parameters that must change together, the manager should construct a new
validated parameter snapshot and swap it between frames. This avoids rendering
half of a frame using old values and half using new ones.

## Commands and wireless control

Wireless callbacks must not modify effects directly. Bluetooth, Wi-Fi, and USB
serial handlers translate requests into a common bounded command structure and
place it on a queue.

```cpp
enum class CommandType : uint8_t {
    AssignEffect,
    SetParameter,
    SetBrightness,
    Start,
    Stop,
    Restart,
    EnterOtaMode,
};

struct Command {
    CommandType type;
    PanelIndex panel;
    uint16_t effectId;
    uint16_t parameterId;
    float value;
};
```

```text
Bluetooth ---+
Wi-Fi -------+--> bounded CommandQueue --> animation task --> EffectManager
USB serial --+
```

Only the animation task owns and mutates effect-manager state. This avoids locks
inside rendering and prevents races with asynchronous radio callbacks.

The protocol should include a version number, request identifier, operation,
target panel, and payload. Commands should return explicit success or error
responses. Transport authentication and authorization can be added without
changing the effect API.

## Bluetooth and Wi-Fi

The core system should not depend on a choice between Bluetooth and Wi-Fi.
Implement each as an adapter that produces the same `Command` objects.

Bluetooth Low Energy is suitable for local configuration, effect selection,
small parameter updates, and modest power use. Wi-Fi is advantageous for larger
payloads, browser-based control, telemetry, and OTA downloads. The finished
device may reasonably use BLE for everyday control and Wi-Fi only when needed
for updates or richer administration.

No transport choice is required during the initial C++ refactor.

## OTA updates

OTA should be implemented as a service separate from effects:

```cpp
class OtaService {
public:
    void beginUpdate();
    void writeChunk(std::span<const uint8_t> data);
    void finishAndRestart();
    void cancel();
};
```

An OTA operation should:

1. authenticate and validate the request;
2. enter a dedicated low-power update mode;
3. stop or simplify normal effects;
4. write the inactive application partition;
5. verify integrity and authenticity;
6. mark the new partition for boot;
7. restart;
8. support rollback if the new image does not validate after boot.

The effect manager may display a simple progress or status effect during the
update, but effects must not write flash or control OTA state themselves.

## Orientation integration

The Bosch-based Adafruit 9-axis sensor should be read by a dedicated hardware
service. It should publish the latest normalized orientation snapshot rather
than allowing each effect to access I2C independently.

```cpp
struct Orientation {
    Quaternion attitude;
    Vec3 gravity;
    Vec3 magneticNorth;
    uint32_t sampleTimeMs;
    bool valid;
};
```

The animation task copies the latest snapshot into `EffectContext`. This gives
all six panel renders a consistent orientation for the frame and permits host
tests to supply synthetic orientations.

## Memory and C++ constraints

ESP-IDF supports C++ well, but embedded constraints should remain explicit:

- use `extern "C" void app_main()` in a C++ entry point;
- avoid allocation in `render()` and other per-frame paths;
- allocate frame buffers and large simulations statically or once at startup;
- avoid exceptions and RTTI unless their cost is deliberately accepted;
- use `std::array`, `std::span`, `constexpr`, scoped enums, and RAII wrappers;
- avoid deep class hierarchies and prefer composition;
- keep large objects off small FreeRTOS task stacks;
- measure frame time, heap use, and binary size as the design evolves.

Virtual dispatch once per panel per frame is negligible here. If effects become
fully compile-time-selected, `std::variant` or function tables remain possible,
but a simple virtual `Effect` interface is clearer for runtime wireless
selection.

## Testing strategy

Hardware-independent code should compile and run as host unit tests. Tests
should cover:

- framebuffer bounds and clearing;
- logical-to-physical mappings for every panel orientation;
- effect state transitions at exact timing boundaries;
- independent versus shared per-panel effect state;
- parameter validation and atomic updates;
- command parsing, queueing, and error responses;
- power-limit calculations and worst-case frames;
- synthetic orientation inputs;
- OTA state transitions using a fake storage backend.

Hardware tests should separately verify LED component order, panel mapping,
maximum current, sensor communication, wireless operation, and OTA/rollback.

## Proposed source layout

```text
main/
  app_main.cpp

  core/
    color.h
    framebuffer.h
    effect.h
    effect_manager.cpp
    effect_manager.h
    command.h
    command_queue.h

  hardware/
    panel_driver.cpp
    panel_driver.h
    orientation_sensor.cpp
    orientation_sensor.h

  effects/
    expanding_rings.cpp
    expanding_rings.h
    digital_rain.cpp
    digital_rain.h

  services/
    bluetooth_service.cpp
    bluetooth_service.h
    wifi_service.cpp
    wifi_service.h
    ota_service.cpp
    ota_service.h

tests/
  test_framebuffer.cpp
  test_effect_manager.cpp
  test_expanding_rings.cpp
  test_panel_mapping.cpp
  test_power_limiter.cpp
```

## Migration plan

1. Rename the entry point to `app_main.cpp` and verify the existing firmware
   builds under C++ without behavioral changes.
2. Introduce `Rgbw` and `FrameBuffer` with host tests.
3. Wrap the LED-strip handle in `PanelDriver` and move wiring translation and
   power limits into it.
4. Define `Effect`, `EffectContext`, and `EffectManager`.
5. Convert expanding rings into a nonblocking `Effect` state machine and compare
   its output and timing against the current implementation.
6. Add a bounded command queue and serial command adapter for early testing.
7. Add the orientation service and synthetic-orientation tests.
8. Expand the driver and manager from one panel to six.
9. Add BLE and/or Wi-Fi adapters without changing the core command API.
10. Add signed OTA updates, boot validation, and rollback.

This ordering preserves a working visual demonstration during the refactor and
keeps hardware, animation, control, and update concerns independently testable.
