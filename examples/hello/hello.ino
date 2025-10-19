// Configuration defines should go before library include
#define PxMATRIX_OE_INVERT     1
#define PxMATRIX_DATA_INVERT   1
#define PxMATRIX_GAMMA_PRESET  3
#define PxMATRIX_DOUBLE_BUFFER 1

#ifdef ESP8266
// May require lower frequency (try 10MHz or even lower)
#define PxMATRIX_SPI_FREQUENCY 16000000L
#endif

#include <PxMatrix.h>

// Define output pins
#if defined(ESP32)
#define P_A   16
#define P_B   17
#define P_OE  22
#define P_LAT 21
// SPI pins (for the reference)
const uint8_t SPI_MOSI = 23;
const uint8_t SPI_SCK  = 18;
#elif defined(ESP8266)
#define P_A   9
#define P_B   10
#define P_OE  5
#define P_LAT 4
// SPI pins (for the reference)
const uint8_t SPI_MOSI = 13;
const uint8_t SPI_SCK  = 14;
#elif defined(__AVR__)
#define P_A   2
#define P_B   3
#define P_LAT 7
#define P_OE  8
// SPI pins (for the reference)
const uint8_t SPI_MOSI = 11;
const uint8_t SPI_SCK  = 13;
#endif

#ifdef ESP32
hw_timer_t*  timer    = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
#elif defined(ESP8266)
#include <Ticker.h>
Ticker ticker;
#elif defined(__AVR__)
#define USE_TIMER_2 true
#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif
#include "TimerInterrupt.h"
#define IRAM_ATTR
#endif

PxMATRIX display(32, 16, P_LAT, P_OE, P_A, P_B);

const uint8_t frame_scan_counts = (1 << PxMATRIX_COLOR_DEPTH);
uint8_t       scan_counts = 0;

void IRAM_ATTR display_updater() {
#ifdef ESP32
    // Critical section to refresh display
    portENTER_CRITICAL_ISR(&timerMux);
#endif
    display.display(50);
    if(++scan_counts == frame_scan_counts) {
        scan_counts = 0;
        Serial.print("."); // Track frames
    }
#ifdef ESP32
    portEXIT_CRITICAL_ISR(&timerMux);
#endif
}

void setup() {
    Serial.begin(115200);
    while (!Serial);
    Serial.println("Setup");

    display.begin(4); // Scan rate 1/4

    display.flushDisplay();
    display.showBuffer();

    // Register display scanline task
#ifdef ESP32
    #if ESP_ARDUINO_VERSION_MAJOR <= 2
        timer = timerBegin(0, 80, true);
        timerAttachInterrupt(timer, &display_updater, true);
        timerAlarmWrite(timer, 1000, true); // each 1 ms
        timerAlarmEnable(timer);
    #else
        timer = timerBegin(1000000);
        timerAttachInterrupt(timer, &display_updater);
        timerAlarm(timer, 1000, true, 0); // each 1 ms
    #endif
#elif defined(ESP8266)
    ticker.attach(0.001, display_updater); // 1 ms
#elif defined(__AVR__)
    ITimer2.init();
    ITimer2.attachInterruptInterval(1, display_updater); // 1 ms
#endif

    display.clearDisplay();
    display.setTextColor(0xFF);
    display.setCursor(1, 1);
    display.println("Hello");
    display.setCursor(2, 8);
    display.println("world");
    display.showBuffer();

    delay(3000);
    Serial.println();
    Serial.println("Start animation");
}

uint8_t frame = 0;

void loop() {
#if defined(PxMATRIX_DOUBLE_BUFFER) || !defined(ESP32)
    drawFrame();
#else
    // Block display till buffer is updated
    portENTER_CRITICAL_ISR(&timerMux);
    drawFrame();
    portEXIT_CRITICAL_ISR(&timerMux);
#endif

    Serial.print(frame);
    if(++frame == 32) {
        frame = 0;
        Serial.println();
    }
#ifndef __AVR__
    delay(100);
#endif
}

void drawFrame() {
    for(uint8_t y = 0; y < 16; ++y) {
        uint8_t pos = (y + 32 - frame) % 32;
        uint16_t c = (pos < 16) ? 15 - pos : (pos - 16);
        display.drawLine(0, y, 32, y, (c << 4));
    }
    display.setTextColor(0x00);
    display.setCursor(1, 1);
    display.println("Hello");
    display.setCursor(2, 8);
    display.println("world");
    display.showBuffer();
}