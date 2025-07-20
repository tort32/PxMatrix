[![PlatformIO Registry](https://badges.registry.platformio.org/packages/tort32/library/PxMatrix-1R.svg)](https://registry.platformio.org/libraries/tort32/PxMatrix-1R)
[![arduino-library-badge](https://www.ardu-badge.com/badge/PxMatrix-1R.svg?)](https://www.ardu-badge.com/PxMatrix-1R)

# PxMatrix 1R - monochrome LED matrix panel driver for ESP8266, ESP32 and ATMEL

This is library version adapted for monocrome displays.

Original PxMatrix library (for RGB displays) - https://github.com/2dom/PxMatrix

## Applied project
Here is an example of matrix array in configuration of 2 by 6:
![Matrix array](https://github.com/tort32/PxMatrix/blob/main/docs/example_photo_6x2_modules.jpg "Array of 2 by 6 display modules")
(Note that this particular display was acquired from a local dumpster for free so be lenient that some leds are worn out or damaged).

Project sources: https://github.com/hackerembassy/led-matrix-display

## Hardware

Monochrome display matrix 16x32 usally has HUB12 header.
Older single or double color displays can have HUB08 with a different pinout. They should be sofrware compatible but not tested.
Full color displays with HUB75 headers can be driven with ![original PxMatrix library](https://github.com/2dom/PxMatrix).

P10 matrix display hardware with schematics:
https://gr33nonline.wordpress.com/2020/01/16/p10-displays/

Shift registers layout in monochrome P10 displays:
https://forum.arduino.cc/t/p10-led-matrix-panels-16x32/251251/13

## Wiring diagram
Each row required to have a separate latch signal. Data and scan control lines are shared.
Signal wires from MCU board should be connected to the first column. Next columns are connected with a daisy chaining between modules.

Example for two rows configutation (showing headers of the first column):
![Wiring diagram](https://github.com/tort32/PxMatrix/blob/main/docs/multiple_rows_connection.jpg "Wiring diagram")

If you got issues with noise, ghosting or jittering try to shorten the wiring or/and use dedicated ground wire in pair for DATA and CLOCK signal wires.
