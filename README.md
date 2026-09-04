# ESP32 RGBW panel test

Minimal ESP-IDF 6.1 test for an ESP32-DevKitC V4 / ESP32-WROOM-32D.

Wiring:

- GPIO16 -> 330 ohm series resistor -> panel DIN
- External regulated 5 V -> panel 5 V
- External supply ground -> panel ground and ESP32 ground

The framework currently includes expanding white rings and green digital rain.
The desk demo starts with digital rain: eight independently phased columns descend
at three related speeds, with a bright green head, graded short tail and dark
gap between drops. A short press of the ESP32-DevKitC `BOOT` button on GPIO0
toggles between Digital Rain and Expanding Rings. The active effect restarts
from its beginning when selected. The animation is deterministic and
timestamp-driven. Do not hold `BOOT` while resetting or powering up, because
GPIO0 is also the ESP32 boot-mode strap.

## Effects and controls

The expanding-rings effect displays a sequence of four white square rings. The
center four pixels ramp up first; each ring then dims while the next surrounding
ring brightens. Completed inner rings remain on the dedicated white channel at
`W=1` rather than turning fully off; the RGB dies are unused. The center and
outermost rings each hold for 1.5 transition intervals, then the sequence reverses
ring-by-ring to the center before fading out and repeating. Its phase and
brightness are calculated from elapsed microseconds rather than render-call
count. All rings are capped at 16/255; retained inner pixels use `W=1`.

Digital Rain uses only the RGB green die. Its head is capped at `G=12`, followed
by a three-pixel trail at approximately `G=6`, `G=4`, and `G=3`. Per-column
phase and speed are deterministic, so the same panel and timestamp always
produce the same frame.

The application refreshes the panel at an average interval of about 3.33 ms.
Calibration established that this panel uses progressive left-to-right rows.

| Control | Result |
| --- | --- |
| Reset/power on | Start Digital Rain |
| Short `BOOT` press | Toggle to the other effect and restart its timeline |
| Hold `BOOT` during reset | Enter the ESP32 ROM bootloader; avoid during normal use |

## Framework

The firmware is now C++. `ExpandingRings` implements the nonblocking `Effect`
interface: each call to `render()` writes one complete logical 8x8 `FrameBuffer`
and returns. Animation state is selected from effect-local elapsed microseconds,
so render rate and scheduler delays do not alter its speed. Its constructor
exposes fade, endpoint-hold and transition durations plus a color callback. The callback receives index 0 for the background
or 1 through 4 for center through outer ring, plus a flag distinguishing the
active ring from retained inner pixels. Zero dwell values and a null callback
select the current defaults.

`EffectManager` owns effect assignment timestamps, calculates `elapsed_us` and
`delta_us`, and renders each configured panel through a `FrameSink`. `PanelDriver`
is the hardware sink and the only layer that knows the physical LED ordering
and RGBW strip API. Rotation, mirroring and serpentine transforms are represented
by `PanelMapping`. Runtime parameter commands, orientation input, wireless
control, power limiting, and OTA are still future work described in `design.md`.

## Build and test

Run the host-side unit tests with:

```sh
mkdir -p .cache/tests
g++ -std=c++20 -Wall -Wextra -Werror -Imain \
  main/digital_rain.cpp main/effect_manager.cpp \
  main/expanding_rings.cpp tests/test_effects.cpp \
  -o .cache/tests/test_effects
.cache/tests/test_effects
```

Build the ESP32 firmware with an initialized ESP-IDF 6.1 environment:

```sh
idf.py build
```
