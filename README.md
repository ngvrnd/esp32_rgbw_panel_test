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

`expanding_rings_run()` exposes the endpoint/inner dwell, transition-step
dwell, and a color callback. The callback receives index 0 for the background
or 1 through 4 for center through outer ring, plus a flag distinguishing the
active ring from retained inner pixels. Passing zero dwell values or a null
callback selects the current defaults. `expanding_rings_run_default()` is the
convenience wrapper used by the demo.
