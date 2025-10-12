/*********************************************************************
This is a library for Chinese LED matrix displays

Originally written for RGB panels by Dominic Buchstaller.
Adapted for monochrome HUB12 1R panels by tort32@github.
BSD license, check LICENSE for more information
*********************************************************************/

#ifndef _PxMATRIX_IMPL_H
#define _PxMATRIX_IMPL_H

#ifndef _BV
#define _BV(x) (1 << (x))
#endif

#if defined(ESP8266) || defined(ESP32)
#define SPI_BUFFER(x, y) SPI.writeBytes(x, y)
#define SPI_BYTE(x)      SPI.write(x)
#endif

#ifdef __AVR__
#define SPI_BYTE(x)      SPI.transfer(x)
#define SPI_BUFFER(buf, size) { for(uint16_t cnt = 0; cnt < (size); ++cnt) SPI_BYTE(*((buf) + cnt)); }
#endif

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
#endif

#ifndef PxMATRIX_DATA_INVERT
#define PxMATRIX_DATA_CLEAR 0x00
#else
#define PxMATRIX_DATA_CLEAR 0xFF
#endif

inline void PxMATRIX::setMuxDelay(uint8_t mux_delay_A, uint8_t mux_delay_B, uint8_t mux_delay_C, uint8_t mux_delay_D, uint8_t mux_delay_E) {
    _mux_delay_A = mux_delay_A;
    _mux_delay_B = mux_delay_B;
    _mux_delay_C = mux_delay_C;
    _mux_delay_D = mux_delay_D;
    _mux_delay_E = mux_delay_E;
}

inline void PxMATRIX::setPanelsWidth(uint8_t panels) {
    setMatrixSize(panels, _LATCH_PINS.size);
}

inline void PxMATRIX::setMatrixSize(uint8_t width, uint8_t height, Chain_Mode mode) {
    _chaining = mode;
    _panels_width = width;
    _panels_height = height;
    if(mode == Chain_Mode::LINES) {
        _panel_width_bytes = (WIDTH / _panels_width) / 8;
    } else {
        // Zigzag configuration has all shift registers in a sequence
        _panel_width_bytes = _panels_height * (WIDTH / 8);
    }
    _panel_height = HEIGHT / _panels_height;
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

inline void PxMATRIX::init() {
    _row_pattern = 0;
    _panels_width = 1;
    _panels_height = _LATCH_PINS.size;
    _panel_width_bytes = WIDTH / 8;
    _panel_height = HEIGHT / _panels_height;
    _chaining = PxMATRIX::Chain_Mode::LINES;

    _active_buffer = false;
    _display_color = 0;
    _brightness = 255;
    _rotate = 0;
    _flip = 0;
    _fast_update = 0;
    _mux_delay_A = _mux_delay_B = _mux_delay_C = _mux_delay_D = _mux_delay_E = 0;

    _buffer_size = (uint16_t)(WIDTH * HEIGHT * PxMATRIX_COLOR_COMP / 8);
    PxMATRIX_buffer = new uint8_t[PxMATRIX_COLOR_DEPTH * _buffer_size];
#ifdef PxMATRIX_DOUBLE_BUFFER
    PxMATRIX_buffer2 = new uint8_t[PxMATRIX_COLOR_DEPTH * _buffer_size];
#endif
}

inline void PxMATRIX::drawPixel(int16_t x, int16_t y, uint16_t color) {
    uint8_t r = color & 0xFF;
    fillMatrixBuffer(x, y, r, PxMATRIX::Buffer_Type::INACTIVE);
}

inline void PxMATRIX::showBuffer() {
    _active_buffer = !_active_buffer;
}

inline uint8_t* PxMATRIX::getBuffer(PxMATRIX::Buffer_Type selected_buffer) {
#ifdef PxMATRIX_DOUBLE_BUFFER
    switch(selected_buffer) {
    case ACTIVE:
        // _active_buffer = true means that PxMATRIX_buffer2 is displayed
        return _active_buffer ?  PxMATRIX_buffer2 : PxMATRIX_buffer;
    case INACTIVE:
        return _active_buffer ?  PxMATRIX_buffer : PxMATRIX_buffer2;
    case FIRST:
        return PxMATRIX_buffer;
    case SECOND:
        return PxMATRIX_buffer2;
    }
#endif
    return PxMATRIX_buffer;
}

inline void PxMATRIX::copyBuffer(bool reverse) {
#ifdef PxMATRIX_DOUBLE_BUFFER
    // This copies the display buffer (active) to the drawing buffer (or reverse)
    // You may need this in case you rely on the framebuffer to always contain the last frame
    uint8_t* src = getBuffer(reverse ? PxMATRIX::Buffer_Type::INACTIVE : PxMATRIX::Buffer_Type::ACTIVE);
    uint8_t* dst = getBuffer(reverse ? PxMATRIX::Buffer_Type::ACTIVE : PxMATRIX::Buffer_Type::INACTIVE);
    memcpy(dst, src, PxMATRIX_COLOR_DEPTH * _buffer_size);
#endif /* PxMATRIX_DOUBLE_BUFFER */
}

inline uint16_t PxMATRIX::mapBufferIndex(int16_t x, int16_t y, uint8_t* pBit) {
    if(_rotate) {
        int16_t temp_x = x;
        x = y;
        y = (HEIGHT - 1) - temp_x;
    }
    // Panels are naturally flipped horizontally
    if(!_flip) {
        x = (WIDTH - 1) - x;
    }
    if(x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT)
        return BUFFER_OUT_OF_BOUNDS;

    // HUB12 (32x16) register chain (https://forum.arduino.cc/t/p10-led-matrix-panels-16x32/251251/13)
    // Data bytes order:
    //               3  7 11 15 <= data-in
    //               2  6 10 14
    //               1  5  9 13
    // next panel <= 0  4  8 12
    // Each register has bits in reverse order:
    //             7 6 5 4 3 2 1 0
    uint8_t nBit = x % 8;
    uint8_t h_index = y / _panel_height;
    uint8_t y_pos = y % _panel_height;

    if(_chaining != PxMATRIX::Chain_Mode::LINES) {
      if(_chaining == PxMATRIX::Chain_Mode::ZIGZAG_UP) {
         h_index = (_panels_height - 1) - h_index;
      }
      if(h_index % 2 == 1) {
        // each odd panel row is rotated
        y_pos = (_panel_height - 1) - y_pos;
        x = (WIDTH - 1) - x;
        nBit = 7 - nBit;
      }
      x += WIDTH * h_index;
    }
    uint8_t x_byte = x / 8;
    uint8_t panel_index = x_byte / _panel_width_bytes;
    uint8_t x_index = x_byte % _panel_width_bytes;
    uint8_t y_index = y_pos / _row_pattern;
    uint8_t row_index = y_pos % _row_pattern;
    *pBit = nBit;

    uint16_t offset = y_index + _rows_per_pattern * x_index;
    if(_chaining == PxMATRIX::Chain_Mode::LINES) {
      offset += (_panel_width_bytes * _rows_per_pattern) * panel_index;
      row_index += h_index * _row_pattern;
    } else {
      offset += _panel_width_bytes * panel_index;
    }
    return _row_offset[row_index] - offset;
}

inline uint8_t PxMATRIX::mapColorLevel(uint8_t r) {
#ifdef PxMATRIX_GAMMA_TABLE
    // Gamma-correction via lookup table
    r = PxMATRIX_GAMMA_TABLE[r];
#endif
#ifdef PxMATRIX_DATA_INVERT
    r = 255 - r;
#endif
    uint8_t level = r >> (8 - PxMATRIX_COLOR_DEPTH);
    return level;
}

inline uint8_t PxMATRIX::unmapColorLevel(uint8_t level) {
    uint8_t r = level << (8 - PxMATRIX_COLOR_DEPTH);
#ifdef PxMATRIX_DATA_INVERT
    r = 255 - r;
#endif
#ifdef PxMATRIX_GAMMA_TABLE
    // Unmap gamma-corrected value
    // TODO Fix gamma unmap for precision loss due to color depth truncation
    for(uint8_t i = 0; i < 255; ++i)
        if(r == PxMATRIX_GAMMA_TABLE[i])
            return i;
    return 255;
#endif
    return r;
}

inline void PxMATRIX::fillMatrixBuffer(int16_t x, int16_t y, uint8_t r, PxMATRIX::Buffer_Type selected_buffer) {
    uint8_t  nbit = 0;
    uint32_t nbyte = mapBufferIndex(x, y, &nbit);
    if(nbyte == BUFFER_OUT_OF_BOUNDS)
        return;

    uint8_t level = mapColorLevel(r);

    // Store pixel level bits separatelly into bit planes
    uint8_t* pBuffer = getBuffer(selected_buffer);
    for(uint8_t i = 0; i < PxMATRIX_COLOR_DEPTH; ++i) {
        if(level & _BV(i)) {
            pBuffer[i * _buffer_size + nbyte] |= _BV(nbit);
        } else {
            pBuffer[i * _buffer_size + nbyte] &= ~_BV(nbit);
        }
    }
}

inline uint8_t PxMATRIX::getPixel(int16_t x, int16_t y, PxMATRIX::Buffer_Type selected_buffer) {
    uint8_t  nbit = 0;
    uint32_t nbyte = mapBufferIndex(x, y, &nbit);
    if(nbyte == BUFFER_OUT_OF_BOUNDS)
        return 0;

    // Restore pixel level from bit planes
    uint8_t* pBuffer = getBuffer(selected_buffer);
    uint8_t  level = 0;
    for(uint8_t i = 0; i < PxMATRIX_COLOR_DEPTH; ++i) {
        if(pBuffer[i * _buffer_size + nbyte] & _BV(nbit))
            level |= _BV(i);
    }

    uint8_t r = unmapColorLevel(level);
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
    _rows_per_pattern = _panel_height / _row_pattern;
    uint8_t _pattern_color_bytes = WIDTH / 8;
    if(_chaining == PxMATRIX::Chain_Mode::LINES) {
        _pattern_color_bytes *= _rows_per_pattern;
    } else {
        _pattern_color_bytes *= HEIGHT / _row_pattern;
    }
    _send_buffer_size = _pattern_color_bytes * PxMATRIX_COLOR_COMP;

    spi_init();

    pinMode(_OE_PIN, OUTPUT);
    digitalWrite(_OE_PIN, HIGH ^ PxMATRIX_OE_INVERT);
    for(uint8_t i = 0; i < _LATCH_PINS.size; ++i) {
        uint8_t pin = _LATCH_PINS[i];
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW ^ PxMATRIX_LATCH_INVERT);
    }
    for(uint8_t i = 0; i < _MUX_PINS.size; ++i) {
        if(_row_pattern < _BV(i)) break;
        uint8_t pin = _MUX_PINS[i];
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
    }

    // Precompute row offset values (the last byte of pattern plane)
    _row_offset = new uint16_t[_row_pattern * _LATCH_PINS.size];
    for(uint8_t line = 0; line < _LATCH_PINS.size; ++line)
        for(uint8_t row = 0; row < _row_pattern; ++row) {
            _row_offset[_row_pattern * line + row] = _send_buffer_size * (_row_pattern * line + row) + (_send_buffer_size - 1);
        }
}

void PxMATRIX::set_mux(uint8_t value) {
    for(uint8_t i = 0; i < _MUX_PINS.size; ++i) {
        if(_row_pattern < _BV(i)) break;
        digitalWrite(_MUX_PINS[i], (value & _BV(i)) ? HIGH : LOW);
        if(i == 0 && _mux_delay_A) delayMicroseconds(_mux_delay_A);
        if(i == 1 && _mux_delay_B) delayMicroseconds(_mux_delay_B);
        if(i == 2 && _mux_delay_C) delayMicroseconds(_mux_delay_C);
        if(i == 3 && _mux_delay_D) delayMicroseconds(_mux_delay_D);
        if(i == 4 && _mux_delay_E) delayMicroseconds(_mux_delay_E);
    }
}

void PxMATRIX::latch(uint16_t show_time, uint8_t latch_index) {
    if(latch_index == LATCH_ALL) {
        for(uint8_t i = 0; i < _LATCH_PINS.size; ++i) {
            uint8_t pin = _LATCH_PINS[i];
            digitalWrite(pin, HIGH ^ PxMATRIX_LATCH_INVERT);
            digitalWrite(pin, LOW ^ PxMATRIX_LATCH_INVERT);
        }
    } else if(latch_index != LATCH_NONE) {
        digitalWrite(_LATCH_PINS[latch_index], HIGH ^ PxMATRIX_LATCH_INVERT);
        digitalWrite(_LATCH_PINS[latch_index], LOW ^ PxMATRIX_LATCH_INVERT);
    }
    if(show_time > 0) {
        digitalWrite(_OE_PIN, LOW ^ PxMATRIX_OE_INVERT);
        unsigned long start_time = micros();
        while((micros() - start_time) < show_time)
            asm volatile(" nop ");
        digitalWrite(_OE_PIN, HIGH ^ PxMATRIX_OE_INVERT);
    }
}

uint16_t PxMATRIX::getLatchTime(uint16_t show_time) {
#if PxMATRIX_COLOR_DEPTH == 1
    return (show_time * _brightness) / 255;
#else
    // Display bit planes in Bit Angle Modulation
    // Thus show_time is a total time to show all bit planes
#ifndef __AVR__
    return ((show_time * (1 << _display_color) * _brightness) / 255 / 2);
#else
    // AVR8 archtecture has 16-bit integer so overflow may occure
    if(show_time >= (65535U >> (PxMATRIX_COLOR_DEPTH - 1)))
        show_time = (65535U >> (PxMATRIX_COLOR_DEPTH - 1));
    uint16_t latch_time = show_time * (1 << _display_color);
    if(latch_time > 512) {
        return (latch_time / 255) * _brightness / 2;
    } else if(latch_time > 256) {
        return (latch_time / 2) * _brightness / 255;
    } else {
        return (latch_time * _brightness) / 255 / 2;
    }
#endif /* __AVR__ */
#endif /* PxMATRIX_COLOR_DEPTH */
}

void PxMATRIX::display(uint16_t show_time) {
    if(show_time == 0)
        show_time = 1;

    // How long do we keep the pixels on
    uint16_t latch_time = getLatchTime(show_time);

#ifdef ESP8266
    ESP.wdtFeed();
#endif

    unsigned long start_time = 0;
    uint8_t* pBuffer = getBuffer(PxMATRIX::Buffer_Type::ACTIVE);
    for(uint8_t row = 0; row < _row_pattern; ++row) {
        if(_fast_update && _brightness == 255 && _LATCH_PINS.size == 1) {
            // This will clock data into the display while the outputs are still
            // latched (LEDs on). We therefore utilize SPI transfer latency as LED
            // ON time and can reduce the waiting time (show_time). This is rather
            // timing sensitive and may lead to flicker however promises reduced
            // update times and increased brightness

            set_mux(row);
            digitalWrite(_LATCH_PINS[0], HIGH ^ PxMATRIX_LATCH_INVERT);
            digitalWrite(_LATCH_PINS[0], LOW ^ PxMATRIX_LATCH_INVERT);
            digitalWrite(_OE_PIN, LOW ^ PxMATRIX_OE_INVERT);
            start_time = micros();
            delayMicroseconds(1);
            if(row < _row_pattern - 1) {
                // This pre-buffers the data for the next row pattern of this _display_color
                SPI_BUFFER(&pBuffer[_display_color * _buffer_size + (row + 1) * _send_buffer_size], _send_buffer_size);
            } else {
                // This pre-buffers the data for the first row pattern of the next _display_color
                SPI_BUFFER(&pBuffer[((_display_color + 1) % PxMATRIX_COLOR_DEPTH) * _buffer_size], _send_buffer_size);
            }

            while((micros() - start_time) < latch_time)
                delayMicroseconds(1);

            digitalWrite(_OE_PIN, HIGH ^ PxMATRIX_OE_INVERT);
        } else {
            set_mux(row);
            for(uint8_t line = 0; line < _LATCH_PINS.size; ++line) {
                SPI_BUFFER(&pBuffer[_display_color * _buffer_size + (_row_pattern * line + row) * _send_buffer_size], _send_buffer_size);
                latch(0, line); // latch pulse
            }
            latch(latch_time, LATCH_NONE); // delay
        }
    }
    ++_display_color;
    if(_display_color >= PxMATRIX_COLOR_DEPTH)
        _display_color = 0;
}

void PxMATRIX::flushDisplay(void) {
    for(uint16_t i = 0; i < _send_buffer_size; ++i)
        SPI_BYTE(PxMATRIX_DATA_CLEAR);
    latch(0, LATCH_ALL);
}

void PxMATRIX::clearDisplay(PxMATRIX::Buffer_Type selected_buffer) {
    uint8_t* pBuffer = getBuffer(selected_buffer);
    memset(pBuffer, PxMATRIX_DATA_CLEAR, PxMATRIX_COLOR_DEPTH * _buffer_size);
}

#endif /* _PxMATRIX_IMPL_H */