# ESP32 RGBW panel test

Minimal ESP-IDF 6.1 test for an ESP32-DevKitC V4 / ESP32-WROOM-32D.

Wiring:

- GPIO16 -> 330 ohm series resistor -> panel DIN
- External regulated 5 V -> panel 5 V
- External supply ground -> panel ground and ESP32 ground

The program displays an expanding sequence of four white square rings. The
center four pixels ramp up first; each ring then dims while the next surrounding
ring brightens. Completed inner rings remain on the dedicated white channel at
`W=1` rather than turning fully off; the RGB dies are unused. The center and
outermost rings each hold for 1.5 transition intervals, then the sequence reverses
ring-by-ring to the center before fading out and repeating. Brightness moves in
single 8-bit PWM steps paced at an
average of about 3.33 ms.
All rings are capped at
16/255, with an additional conservative 500 mA current-budget
calculation assuming 20 mA per white die at full scale. Calibration
established that this panel uses progressive left-to-right rows.

The firmware is now C++. `ExpandingRings` implements the nonblocking `Effect`
interface: each call to `render()` writes one complete logical 8x8 `FrameBuffer`
and returns. Its constructor exposes the endpoint/inner dwell, transition-step
dwell, and a color callback. The callback receives index 0 for the background
or 1 through 4 for center through outer ring, plus a flag distinguishing the
active ring from retained inner pixels. Zero dwell values and a null callback
select the current defaults.

`PanelDriver` is the only layer that knows the physical LED ordering and RGBW
strip API. The demo holds the effect through an `Effect*`, renders panel index
0, and passes the resulting logical frame to that driver. This is the first
framework increment described in `design.md`; an effect manager, runtime
parameters, orientation input, wireless control, power limiting, and OTA are
still future work.

Run the host-side unit tests with:

```sh
mkdir -p .cache/tests
g++ -std=c++20 -Wall -Wextra -Werror -Imain \
  main/expanding_rings.cpp tests/test_effects.cpp \
  -o .cache/tests/test_effects
.cache/tests/test_effects
```
