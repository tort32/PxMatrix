[![PlatformIO Registry](https://badges.registry.platformio.org/packages/tort32/library/PxMatrix-1R.svg)](https://registry.platformio.org/libraries/tort32/PxMatrix-1R)
[![arduino-library-badge](https://www.ardu-badge.com/badge/PxMatrix-1R.svg?)](https://www.ardu-badge.com/PxMatrix-1R)

# PxMatrix 1R - monochrome LED matrix panel driver for ESP8266, ESP32 and AVR

This is library version adapted for monocrome displays.

Original PxMatrix library (for RGB displays) - https://github.com/2dom/PxMatrix

## Applied project
Here is an example of matrix array in configuration of 6 by 2:
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

## Wiring diagrams

### Parallel rows
Each row required to have a separate latch signal. Data and scan control lines are shared.
Signal wires from MCU board should be connected to the first column. Next columns are connected with a daisy chaining between modules.

Example for two rows configutation (showing headers of the first column):
![Parallel wiring diagram](https://github.com/tort32/PxMatrix/blob/main/docs/multiple_rows_connection.jpg "Wiring diagram")

If you got issues with noise, ghosting or jittering try to shorten the wiring or/and use dedicated ground wire in pair for DATA and CLOCK signal wires.

Set multiple latch pins in the constructor (example for 2 rows):
``` cpp
#define PIN_A    16
#define PIN_B    17
#define PIN_OE   22
#define PIN_LAT  21
#define PIN_LAT2 5
PxMATRIX display(WIDTH, HEIGHT, { PIN_LAT, PIN_LAT2 }, PIN_OE, { PIN_A, PIN_B });
```

### Zigzag chaining
Alternative connection (popular with [DMD library](https://github.com/freetronics/DMD)) is folded single chain:
![Zigzag chain diagram](https://github.com/tort32/PxMatrix/blob/main/docs/zig_zag_connection.png "Zigzag chain diagram")
Note that each even row is turned 180 forming the continuous connection. The power bus is not shown for simplicity.

Setup zigzag chaining for 3 by 4 display showned:
``` cpp
PxMATRIX display(32 * 3, 16 * 4, PIN_LAT, PIN_OE, PIN_A, PIN_B);
void setup() {
    display.setMatrixSize(3, 4, PxMATRIX::Chain_Mode::ZIGZAG_DOWN); // ZIGZAG_UP for upright stacking
    display.begin(4); // Scan rate 1/4
    ...
```

Rows can be stacked **Top to Bottom** (controller input on top as showned) or **Bottom to Top** (controller at the bottom row - DMD style).

## Performance considerations

The library implements display scanning routine via blocking function which is called by the timer interrpution.
This means that some of the execution time will be spent on these calls depending on the core frequency, timer period, and display active time.
Any additional long blocking interruptions (for example HTTP Server requests) can cause timing jitter for scanning routine that may effect on apparent brightness (display flickering).

For better performance **Hardware SPI** should be used to push data into the display.
AVR microcontrollers (like on Arduino UNO R3 board) have lower clock frequency, smaller memory and sometimes only **Software SPI** implementation via bit-banging which is notoriously slow.
But still small (1-2 modules) display can be driven with lower FPS and grayscale depth.

ESP32 controller is recommended for larger displays.

## Gamma correction and grayscale depth

By default you may notice that grayscale images lack of dark tones.
For the better rendering a gamma correction should be used.

The library provides multiple gamma lookup tables to choose by #define macro:
value | PxMATRIX_GAMMA_PRESET
-----:|:--
1.0 | undefined
1.8 | 1
2.0 | 2
2.2 | 3
2.4 | 4
2.6 | 5
2.8 | 6

Grayscale depth also can be adjusted by macro `PxMATRIX_COLOR_DEPTH` in a range from 1 bit - black/white, to 8 bits maximum - 256 semi-tones (including black and white). Default is 4 (16 tones).

## Double buffer

Double buffering technique can be enabled by macro `PxMATRIX_DOUBLE_BUFFER`.
This helps eliminate Screen Tearing effect when frame is being rendered partially drawned.

``` cpp
#define PxMATRIX_DOUBLE_BUFFER 1
PxMATRIX display(WIDTH, HEIGHT, PIN_LAT, PIN_OE, PIN_A, PIN_B);
void IRAM_ATTR display_updater() {
    // Render pixels (read) from Buffer_Type::ACTIVE
    display.display(50); // Render each scan line for 50 microseconds (total time ~0.2 ms)
}
void setup() {
    display.begin(4); // Scan rate 1/4
    ...
    timer = timerBegin(1000000); // 1MHz
    timerAttachInterrupt(timer, &display_updater);
    timerAlarm(timer, 1000, true, 0); // 1MHz / 1000 = 1 ms period
    ...
}
void loop() {
    // Drawing pixels (write) to Buffer_Type::INACTIVE
    drawFrame(); // <--- Frame update here
    display.showBuffer(); // Swap active buffer
    delay(20); // Limit FPS
}
```

Alternative solution is using of a mutex for critical sections (see [source](https://github.com/tort32/PxMatrix/blob/main/examples/hello/hello.ino#L125) for ESP32)
