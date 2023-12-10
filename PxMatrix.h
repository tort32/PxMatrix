/*********************************************************************
This is a library for Chinese LED matrix displays

Originally written for RGB panels by Dominic Buchstaller.
Adapted for monochrome HUB12 1R panels by tort32@github.
BSD license, check LICENSE for more information
*********************************************************************/

#ifndef _PxMATRIX_H
#define _PxMATRIX_H

// Color depth (defines the number of grayscale levels)
// NOTE: the more levels - the slower the update
#ifndef PxMATRIX_COLOR_DEPTH
#define PxMATRIX_COLOR_DEPTH 4
#endif
#if PxMATRIX_COLOR_DEPTH > 8 || PxMATRIX_COLOR_DEPTH < 1
#error "PxMATRIX_COLOR_DEPTH must be 1 to 8 bits maximum"
#endif

// Number of color components:
//  1 for monochrome,
//  3 for RGB - not supported, use original PxMatrix library
#ifndef PxMATRIX_COLOR_COMP
#define PxMATRIX_COLOR_COMP 1
#endif
#if PxMATRIX_COLOR_COMP != 1
#error "PxMATRIX_COLOR_COMP only support 1"
#endif

// Defines how long we display things by default in milliseconds
#ifndef PxMATRIX_DEFAULT_SHOWTIME
#define PxMATRIX_DEFAULT_SHOWTIME 30
#endif

// Defines the speed of the SPI bus (reducing this may help if you experience noisy images)
#ifndef PxMATRIX_SPI_FREQUENCY
#define PxMATRIX_SPI_FREQUENCY 20000000L
#endif

// Inverted Enable signal
#ifndef PxMATRIX_OE_INVERT
#define PxMATRIX_OE_INVERT false
#endif

// Inverted Latch signal
#ifndef PxMATRIX_LATCH_INVERT
#define PxMATRIX_LATCH_INVERT false
#endif

// Inverted Data bytes
#ifndef PxMATRIX_DATA_INVERT
#define PxMATRIX_DATA_INVERT false
#endif

#ifndef _BV
#define _BV(x) (1 << (x))
#endif

#if defined(ESP8266) || defined(ESP32)
#define SPI_BUFFER(x, y) SPI.writeBytes(x, y)
#define SPI_BYTE(x)      SPI.write(x)
#endif

#ifdef __AVR__
#define SPI_BUFFER(x, y) SPI.transfer(x, y)
#define SPI_BYTE(x)      SPI.transfer(x)
#endif

#include "Adafruit_GFX.h"
#include "Arduino.h"
#include <SPI.h>

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#ifdef __AVR__
#include <util/delay.h>
#endif

#include <stdlib.h>

// Sometimes some extra width needs to be passed to Adafruit GFX constructor
// to render text close to the end of the display correctly
#ifndef ADAFRUIT_GFX_EXTRA
#define ADAFRUIT_GFX_EXTRA 0
#endif

#ifdef ESP8266
#define GPIO_REG_SET(val)   GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, val)
#define GPIO_REG_CLEAR(val) GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, val)
#endif
#ifdef ESP32
#define GPIO_REG_SET(val)   GPIO.out_w1ts = val
#define GPIO_REG_CLEAR(val) GPIO.out_w1tc = val
#endif
#ifdef __AVR__
#define GPIO_REG_SET(val)   (val < 8) ? PORTD |= _BV(val) : PORTB |= _BV(val - 8)
#define GPIO_REG_CLEAR(val) (val < 8) ? PORTD &= ~_BV(val) : PORTB &= ~_BV(val - 8)
#endif

#ifdef ESP32
#include "esp32-hal-gpio.h"
#include "soc/spi_struct.h"

#ifdef PxMATRIX_GAMMA_PRESET
#include "PxMatrix_gamma.h"
#endif

struct spi_struct_t
{
    spi_dev_t* dev;
#if !CONFIG_DISABLE_HAL_LOCKS
    xSemaphoreHandle lock;
#endif
    uint8_t num;
};
#endif /* ESP32 */

class PxMATRIX : public Adafruit_GFX
{
public:
    inline PxMATRIX(uint16_t width, uint16_t height, uint8_t LATCH, uint8_t OE, uint8_t A, uint8_t B, uint8_t C = 0, uint8_t D = 0, uint8_t E = 0);

    inline void begin(uint8_t row_pattern = 8);

    inline void clearDisplay(void);
    inline void clearDisplay(bool selected_buffer);

    // Updates the display
    inline void display(uint16_t show_time = PxMATRIX_DEFAULT_SHOWTIME);

    // Draw pixel
    inline void drawPixel(int16_t x, int16_t y, uint16_t color = 0xFF) override;

    // Read pixel
    uint8_t getPixel(int8_t x, int8_t y);
    uint8_t getPixel(int8_t x, int8_t y, bool selected_buffer);

    // Flush the hardware display at startup (doesn't clear the buffer, use clearDisplay instead)
    inline void flushDisplay();

    // Rotate display
    inline void setRotate(bool rotate);

    // Flip display
    inline void setFlip(bool flip);

    // Helps to reduce display update latency on larger displays
    inline void setFastUpdate(bool fast_update);

    // When using double buffering, this displays the draw buffer
    inline void showBuffer();

#ifdef PxMATRIX_DOUBLE_BUFFER
    // This copies the display buffer to the drawing buffer (or reverse)
    inline void copyBuffer(bool reverse = false);
#endif

    // Set the time in microseconds that we pause after selecting each mux channel
    // (May help if some rows are missing / the mux chip is too slow)
    inline void setMuxDelay(uint8_t mux_delay_A, uint8_t mux_delay_B, uint8_t mux_delay_C = 0, uint8_t mux_delay_D = 0, uint8_t mux_delay_E = 0);

    // Set the number of panels that make up the display area width (default is 1)
    inline void setPanelsWidth(uint8_t panels);

    // Set the brightness of the panels (default is 255)
    inline void setBrightness(uint8_t brightness);

private:
    // the display buffer for the LED matrix
    uint8_t* PxMATRIX_buffer;
#ifdef PxMATRIX_DOUBLE_BUFFER
    uint8_t* PxMATRIX_buffer2;
#endif

    // GPIO pins
    uint8_t _LATCH_PIN;
    uint8_t _OE_PIN;
    uint8_t _A_PIN;
    uint8_t _B_PIN;
    uint8_t _C_PIN;
    uint8_t _D_PIN;
    uint8_t _E_PIN;

    uint16_t _width;
    uint16_t _height;
    uint8_t  _panels_width;
    
    uint8_t  _rows_per_pattern;
    uint8_t  _panel_width_bytes;

    // Panel Brightness
    uint8_t _brightness;

    // Color pattern that is pushed to the display
    uint8_t _display_color;

    // Holds some pre-computed values for faster pixel drawing
    uint32_t* _row_offset;

    // Holds the display row pattern type
    uint8_t _row_pattern;

    // Number of bytes in one color
    uint8_t _pattern_color_bytes;

    // Total number of bytes that is pushed to the display at a time
    // PxMATRIX_COLOR_COMP * _pattern_color_bytes
    uint16_t _buffer_size;
    uint16_t _send_buffer_size;

    // This is for double buffering
    bool _active_buffer;

    // Display and color engine
    bool _rotate;
    bool _flip;
    bool _fast_update;

    uint8_t _mux_delay_A;
    uint8_t _mux_delay_B;
    uint8_t _mux_delay_C;
    uint8_t _mux_delay_D;
    uint8_t _mux_delay_E;

    static const uint32_t BUFFER_OUT_OF_BOUNDS = UINT32_MAX;

private:
    // Generic function that draw one pixel
    inline void fillMatrixBuffer(int16_t x, int16_t y, uint8_t r, bool selected_buffer);

    inline uint32_t mapBufferIndex(int16_t x, int16_t y, uint8_t* pBit);

    // Light up LEDs and hold for show_time microseconds
    inline void latch(uint16_t show_time);

    // Set row multiplexer
    inline void set_mux(uint8_t value);

    inline void spi_init();
};

inline void PxMATRIX::setMuxDelay(uint8_t mux_delay_A, uint8_t mux_delay_B, uint8_t mux_delay_C, uint8_t mux_delay_D, uint8_t mux_delay_E) {
    _mux_delay_A = mux_delay_A;
    _mux_delay_B = mux_delay_B;
    _mux_delay_C = mux_delay_C;
    _mux_delay_D = mux_delay_D;
    _mux_delay_E = mux_delay_E;
}

inline void PxMATRIX::setPanelsWidth(uint8_t panels) {
    _panels_width = panels;
    _panel_width_bytes = (_width / _panels_width) / 8;
}

inline void PxMATRIX::setRotate(bool rotate) {
    _rotate = rotate;
}

inline void PxMATRIX::setFlip(bool flip) {
    _flip = flip;
}

inline void PxMATRIX::setFastUpdate(bool fast_update) {
    _fast_update = fast_update;
}

inline void PxMATRIX::setBrightness(uint8_t brightness) {
    _brightness = brightness;
}

inline PxMATRIX::PxMATRIX(uint16_t width, uint16_t height, uint8_t LATCH, uint8_t OE,
        uint8_t A, uint8_t B, uint8_t C, uint8_t D, uint8_t E) : Adafruit_GFX(width + ADAFRUIT_GFX_EXTRA, height) {
    _LATCH_PIN = LATCH;
    _OE_PIN = OE;
    _A_PIN = A;
    _B_PIN = B;
    _C_PIN = C;
    _D_PIN = D;
    _E_PIN = E;

    _width = width;
    _height = height;
    _panels_width = 1;
    _row_pattern = 0;

    _active_buffer = false;
    _display_color = 0;
    _brightness = 255;
    _rotate = 0;
    _flip = 0;
    _fast_update = 0;

    _buffer_size = (_width * _height * PxMATRIX_COLOR_COMP / 8);
    _panel_width_bytes = (_width / _panels_width) / 8;

    _mux_delay_A = 0;
    _mux_delay_B = 0;
    _mux_delay_C = 0;
    _mux_delay_D = 0;
    _mux_delay_E = 0;

    PxMATRIX_buffer = new uint8_t[PxMATRIX_COLOR_DEPTH * _buffer_size];
#ifdef PxMATRIX_DOUBLE_BUFFER
    PxMATRIX_buffer2 = new uint8_t[PxMATRIX_COLOR_DEPTH * _buffer_size];
#endif
}

inline void PxMATRIX::drawPixel(int16_t x, int16_t y, uint16_t color) {
    uint8_t r = color & 0xFF;
#ifdef PxMATRIX_DOUBLE_BUFFER
    // Draw into inactive buffer
    fillMatrixBuffer(x, y, r, !_active_buffer);
#else
    fillMatrixBuffer(x, y, r, false);
#endif
}

inline void PxMATRIX::showBuffer() {
    _active_buffer = !_active_buffer;
}

#ifdef PxMATRIX_DOUBLE_BUFFER
inline void PxMATRIX::copyBuffer(bool reverse) {
    // This copies the display buffer to the drawing buffer (or reverse)
    // You may need this in case you rely on the framebuffer to always contain the last frame
    // _active_buffer = true means that PxMATRIX_buffer2 is displayed
    if(_active_buffer ^ reverse) {
        memcpy(PxMATRIX_buffer, PxMATRIX_buffer2, PxMATRIX_COLOR_DEPTH * _buffer_size);
    } else {
        memcpy(PxMATRIX_buffer2, PxMATRIX_buffer, PxMATRIX_COLOR_DEPTH * _buffer_size);
    }
}
#endif /* PxMATRIX_DOUBLE_BUFFER */

inline uint32_t PxMATRIX::mapBufferIndex(int16_t x, int16_t y, uint8_t* pBit) {
    if(_rotate) {
        uint16_t temp_x = x;
        x = y;
        y = (_height - 1) - temp_x;
    }
    // Panels are naturally flipped
    if(!_flip) {
        x = (_width - 1) - x;
    }
    if(x < 0 || x >= _width || y < 0 || y >= _height)
        return BUFFER_OUT_OF_BOUNDS;

    // HUB12 (32x16) register chain (https://forum.arduino.cc/t/p10-led-matrix-panels-16x32/251251/13)
    // Data bytes order:
    //                3  7 11 15 <= data-in
    //                2  6 10 14
    //                1  5  9 13
    // next pannel <= 0  4  8 12
    // Each register has bits in reverse order:
    //             7 6 5 4 3 2 1 0
    *pBit = x % 8;
    uint8_t x_byte = x / 8;
    uint8_t panel_index = x_byte / _panel_width_bytes;
    uint8_t x_index = x_byte % _panel_width_bytes;
    uint8_t y_index = y / _row_pattern;
    uint8_t row_index = y % _row_pattern;

    uint32_t offset = y_index + _rows_per_pattern * x_index + (_panel_width_bytes * _rows_per_pattern) * panel_index;
    return _row_offset[row_index] - offset;
}

inline void PxMATRIX::fillMatrixBuffer(int16_t x, int16_t y, uint8_t r, bool selected_buffer) {
    uint8_t  nbit;
    uint32_t nbyte = mapBufferIndex(x, y, &nbit);
    if(nbyte == BUFFER_OUT_OF_BOUNDS)
        return;

#ifdef PxMATRIX_DATA_INVERT
    r = 255 - r;
#endif
#ifdef PxMATRIX_GAMMA_TABLE
    // Gamma-correction via lookup table
    r = PxMATRIX_GAMMA_TABLE[r];
#endif
#if PxMATRIX_COLOR_DEPTH != 8
    r = r >> (8 - PxMATRIX_COLOR_DEPTH);
#endif

    // Store pixel color value into bit planes
#ifndef PxMATRIX_DOUBLE_BUFFER
    uint8_t* pBuffer = PxMATRIX_buffer;
#else
    uint8_t* pBuffer = selected_buffer ? PxMATRIX_buffer2 : PxMATRIX_buffer;
#endif
    for(int i = 0; i < PxMATRIX_COLOR_DEPTH; i++) {
        if((r >> i) & 0x01)
            pBuffer[i * _buffer_size + nbyte] |= _BV(nbit);
        else
            pBuffer[i * _buffer_size + nbyte] &= ~_BV(nbit);
    }
}

inline uint8_t PxMATRIX::getPixel(int8_t x, int8_t y) {
#ifdef PxMATRIX_DOUBLE_BUFFER
    // Read from active buffer
    return getPixel(x, y, _active_buffer);
#else
    return getPixel(x, y, false);
#endif
}

inline uint8_t PxMATRIX::getPixel(int8_t x, int8_t y, bool selected_buffer) {
    uint8_t  nbit;
    uint32_t nbyte = mapBufferIndex(x, y, &nbit);
    if(nbyte == BUFFER_OUT_OF_BOUNDS)
        return 0;
    
    // Restore color value from bit planes
#ifndef PxMATRIX_DOUBLE_BUFFER
    uint8_t* pBuffer = PxMATRIX_buffer;
#else
    uint8_t* pBuffer = selected_buffer ? PxMATRIX_buffer2 : PxMATRIX_buffer;
#endif
    uint8_t r = 0;
    for(int i = 0; i < PxMATRIX_COLOR_DEPTH; i++) {
        if(pBuffer[i * _buffer_size + nbyte] & _BV(nbit))
            r |= _BV(i);
    }

    r = (r << (8 - PxMATRIX_COLOR_DEPTH));

    return r;
}

void PxMATRIX::spi_init() {
    SPI.begin();

#if defined(ESP32) || defined(ESP8266)
    SPI.setFrequency(PxMATRIX_SPI_FREQUENCY);
#endif

    SPI.setDataMode(SPI_MODE0);
    SPI.setBitOrder(MSBFIRST);
}

void PxMATRIX::begin(uint8_t row_pattern) {
    _row_pattern = row_pattern;
    _rows_per_pattern = _height / _row_pattern;
    _pattern_color_bytes = _rows_per_pattern * _width / 8;
    _send_buffer_size = _pattern_color_bytes * PxMATRIX_COLOR_COMP;

    spi_init();

    pinMode(_OE_PIN, OUTPUT);
    pinMode(_LATCH_PIN, OUTPUT);
    pinMode(_A_PIN, OUTPUT);
    pinMode(_B_PIN, OUTPUT);
    digitalWrite(_A_PIN, LOW);
    digitalWrite(_B_PIN, LOW);
    digitalWrite(_OE_PIN, HIGH ^ PxMATRIX_OE_INVERT);
    digitalWrite(_LATCH_PIN, LOW ^ PxMATRIX_LATCH_INVERT);
    if(_row_pattern >= 8) {
        pinMode(_C_PIN, OUTPUT);
        digitalWrite(_C_PIN, LOW);
    }
    if(_row_pattern >= 16) {
        pinMode(_D_PIN, OUTPUT);
        digitalWrite(_D_PIN, LOW);
    }
    if(_row_pattern >= 32) {
        pinMode(_E_PIN, OUTPUT);
        digitalWrite(_E_PIN, LOW);
    }

    // Precompute row offset values (the last byte of pattern plane)
    _row_offset = new uint32_t[_row_pattern];
    for(uint8_t row = 0; row < _row_pattern; row++) {
        _row_offset[row] = row * _send_buffer_size + (_send_buffer_size - 1);
    }
}

void PxMATRIX::set_mux(uint8_t value) {
    digitalWrite(_A_PIN, (value & 0x01) ? HIGH : LOW);
    if(_mux_delay_A) delayMicroseconds(_mux_delay_A);

    digitalWrite(_B_PIN, (value & 0x02) ? HIGH : LOW);
    if(_mux_delay_B) delayMicroseconds(_mux_delay_B);

    if(_row_pattern >= 8) {
        digitalWrite(_C_PIN, (value & 0x04) ? HIGH : LOW);
        if(_mux_delay_C) delayMicroseconds(_mux_delay_C);
    }
    if(_row_pattern >= 16) {
        digitalWrite(_D_PIN, (value & 0x08) ? HIGH : LOW);
        if(_mux_delay_D) delayMicroseconds(_mux_delay_D);
    }
    if(_row_pattern >= 32) {
        digitalWrite(_E_PIN, (value & 0x10) ? HIGH : LOW);
        if(_mux_delay_E) delayMicroseconds(_mux_delay_E);
    }
}

void PxMATRIX::latch(uint16_t show_time) {
    digitalWrite(_LATCH_PIN, HIGH ^ PxMATRIX_LATCH_INVERT);
    digitalWrite(_LATCH_PIN, LOW ^ PxMATRIX_LATCH_INVERT);
    if(show_time > 0) {
        digitalWrite(_OE_PIN, LOW ^ PxMATRIX_OE_INVERT);
        unsigned long start_time = micros();
        while((micros() - start_time) < show_time)
            asm volatile(" nop ");
        digitalWrite(_OE_PIN, HIGH ^ PxMATRIX_OE_INVERT);
    }
}

void PxMATRIX::display(uint16_t show_time) {
    if(show_time == 0) show_time = 1;
#if PxMATRIX_COLOR_DEPTH != 1
    // How long do we keep the pixels on
    uint16_t latch_time = ((show_time * (1 << _display_color) * _brightness) / 255 / 2);
#else
    uint16_t latch_time = (show_time * _brightness) / 255;
#endif

    unsigned long start_time = 0;
#ifdef ESP8266
    ESP.wdtFeed();
#endif

    uint8_t* PxMATRIX_bufferp = PxMATRIX_buffer;
#ifdef PxMATRIX_DOUBLE_BUFFER
    PxMATRIX_bufferp = _active_buffer ? PxMATRIX_buffer2 : PxMATRIX_buffer;
#endif

    for(uint8_t i = 0; i < _row_pattern; i++) {
        if(_fast_update && _brightness == 255) {
            // This will clock data into the display while the outputs are still
            // latched (LEDs on). We therefore utilize SPI transfer latency as LED
            // ON time and can reduce the waiting time (show_time). This is rather
            // timing sensitive and may lead to flicker however promises reduced
            // update times and increased brightness

            set_mux(i);
            digitalWrite(_LATCH_PIN, HIGH ^ PxMATRIX_LATCH_INVERT);
            digitalWrite(_LATCH_PIN, LOW ^ PxMATRIX_LATCH_INVERT);
            digitalWrite(_OE_PIN, LOW ^ PxMATRIX_OE_INVERT);
            start_time = micros();
            delayMicroseconds(1);
            if(i < _row_pattern - 1) {
                // This pre-buffers the data for the next row pattern of this _display_color
                SPI_BUFFER(&PxMATRIX_bufferp[_display_color * _buffer_size + (i + 1) * _send_buffer_size], _send_buffer_size);
            } else {
                // This pre-buffers the data for the first row pattern of the next _display_color
                SPI_BUFFER(&PxMATRIX_bufferp[((_display_color + 1) % PxMATRIX_COLOR_DEPTH) * _buffer_size], _send_buffer_size);
            }

            while((micros() - start_time) < latch_time)
                delayMicroseconds(1);

            digitalWrite(_OE_PIN, HIGH ^ PxMATRIX_OE_INVERT);
        } else {
            set_mux(i);
#ifdef __AVR__
            uint8_t this_byte;
            for(uint32_t byte_cnt = 0; byte_cnt < _send_buffer_size; byte_cnt++) {
                this_byte = PxMATRIX_bufferp[_display_color * _buffer_size + i * _send_buffer_size + byte_cnt];
                SPI_BYTE(this_byte);
            }
#else
            SPI_BUFFER(&PxMATRIX_bufferp[_display_color * _buffer_size + i * _send_buffer_size], _send_buffer_size);
#endif
            latch(latch_time);
        }
    }
    ++_display_color;
    if(_display_color >= PxMATRIX_COLOR_DEPTH) _display_color = 0;
}

void PxMATRIX::flushDisplay(void) {
#ifndef PxMATRIX_DATA_INVERT
    uint8_t b = 0x00;
#else
    uint8_t b = 0xFF;
#endif
    for(int i = 0; i < _send_buffer_size; ++i)
        SPI_BYTE(b);
}

void PxMATRIX::clearDisplay(void) {
#ifdef PxMATRIX_DOUBLE_BUFFER
    clearDisplay(!_active_buffer);
#else
    clearDisplay(false);
#endif
}

void PxMATRIX::clearDisplay(bool selected_buffer) {
#ifndef PxMATRIX_DATA_INVERT
    uint8_t val = 0x00;
#else
    uint8_t val = 0xFF;
#endif
#ifdef PxMATRIX_DOUBLE_BUFFER
    if(selected_buffer)
        memset(PxMATRIX_buffer2, val, PxMATRIX_COLOR_DEPTH * _buffer_size);
    else
        memset(PxMATRIX_buffer, val, PxMATRIX_COLOR_DEPTH * _buffer_size);
#else
    memset(PxMATRIX_buffer, val, PxMATRIX_COLOR_DEPTH * _buffer_size);
#endif
}

#endif /* _PxMATRIX_H */