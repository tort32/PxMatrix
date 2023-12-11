// Configuration defines should go before include
#define PxMATRIX_OE_INVERT     1
#define PxMATRIX_DATA_INVERT   1
#define PxMATRIX_GAMMA_PRESET  3
#define PxMATRIX_DOUBLE_BUFFER 1

#include <PxMatrix.h>

// For ESP32 based board
// Define output pins
#define P_A   16
#define P_B   17
#define P_OE  22
#define P_LAT 21
// SPI pins (for the reference)
#define SPI_MOSI = 23;
#define SPI_SCK  = 18;

hw_timer_t*  timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

PxMATRIX display(32, 16, P_LAT, P_OE, P_A, P_B);

const uint8_t frame_scan_counts = (1 << PxMATRIX_COLOR_DEPTH);
uint8_t       scan_counts = 0;

void IRAM_ATTR display_updater() {
    // Critical section to refresh display
    portENTER_CRITICAL_ISR(&timerMux);
    display.display(50);
    if(++scan_counts == frame_scan_counts) {
        scan_counts = 0;
        Serial.print("."); // Track frames
    }
    portEXIT_CRITICAL_ISR(&timerMux);
}

void setup() {
    Serial.begin(115200);
    Serial.println("Setup");

    display.begin(4); // Scan rate 1/4
    display.flushDisplay();
    display.showBuffer();

#ifdef ESP32
    // Register display scanline task
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &display_updater, true);
    timerAlarmWrite(timer, 1000, true); // each 1 ms
    timerAlarmEnable(timer);
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
#ifdef PxMATRIX_DOUBLE_BUFFER
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
    delay(100);
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