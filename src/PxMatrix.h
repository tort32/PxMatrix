/*********************************************************************
This is a library for Chinese LED matrix displays

Originally written for RGB panels by Dominic Buchstaller.
Adapted for monochrome HUB12 1R panels by tort32@github.
BSD license, check LICENSE for more information
*********************************************************************/

#ifndef _PxMATRIX_H
#define _PxMATRIX_H

// Color depth (defines number of grayscale levels)
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

// Invert Output Enable signal
#ifndef PxMATRIX_OE_INVERT
#define PxMATRIX_OE_INVERT 0
#endif

// Invert Register Latch signal
#ifndef PxMATRIX_LATCH_INVERT
#define PxMATRIX_LATCH_INVERT 0
#endif

// Invert Register Data bits (for LEDs connected to power instead ground)
#ifdef PxMATRIX_DATA_INVERT
#if PxMATRIX_DATA_INVERT == 0
#undef PxMATRIX_DATA_INVERT
#endif
#endif

// Double buffer to avoid pixels flickering when clearing and drawing pixels
#ifdef PxMATRIX_DOUBLE_BUFFER
#if PxMATRIX_DOUBLE_BUFFER == 0
#undef PxMATRIX_DOUBLE_BUFFER
#endif
#endif

#include "Adafruit_GFX.h"

#ifdef PxMATRIX_GAMMA_PRESET
#include "PxMatrix_gamma.h"
#endif

class PxMATRIX : public Adafruit_GFX
{
public:
    enum Buffer_Type {ACTIVE, INACTIVE, FIRST, SECOND};

    // Create output display for LED matrix
    // Size of width and height = total number of LEDs in width and height for the whole matrix.
    //   Number of matrix pannels should be set with "setPanelsWidth" method.
    // LATCH = output pin for Register Latch signal (can be named as L, LAT, STB or SCLK)
    // OE = output pin for Output Enable (to light pixels of the current selected scanline)
    // A,B,C,D,E = ouput pins for scan pattern demultiplexing (to select current scanline)
    //   Scanline pattern (number of scans) should be passed as "begin" method argument.
    // NOTE: SPI pins MOSI and CLK are used to send data to shift registers (see board pinouts).
    inline PxMATRIX(uint16_t width, uint16_t height, uint8_t LATCH, uint8_t OE, uint8_t A, uint8_t B, uint8_t C = 0, uint8_t D = 0, uint8_t E = 0);

    // Prepare to render display
    // row_pattern = number of scan lines to display whole image (defined by hardware)
    //   Typical display scan rates: 1/4, 1/8, 1/16, 1/32
    inline void begin(uint8_t row_pattern = 4);

    // Clear display buffer
    inline void clearDisplay(Buffer_Type selected_buffer = Buffer_Type::INACTIVE);

    // Render buffer at display
    inline void display(uint16_t show_time = PxMATRIX_DEFAULT_SHOWTIME);

    // Draw pixel
    inline void drawPixel(int16_t x, int16_t y, uint16_t color = 0xFF) override;

    // Read pixel
    uint8_t getPixel(int8_t x, int8_t y, Buffer_Type selected_buffer = Buffer_Type::ACTIVE);

    // Flush the display registers (example at startup to purge previous data)
    // NOTE: It doesn't clear the buffer (use clearDisplay instead)
    inline void flushDisplay();

    // Rotate display
    inline void setRotate(bool rotate);

    // Flip display
    inline void setFlip(bool flip);

    // Helps to reduce display update latency on larger displays
    inline void setFastUpdate(bool fast_update);

    // When using double buffering, this swaps buffers makes new frame is ready to render
    inline void showBuffer();

    // When using double buffering, copy the display buffer to the drawing buffer (or reverse)
    inline void copyBuffer(bool reverse = false);

    // Set the time in microseconds that we pause after selecting each mux channel
    // (May help if some rows are missing / the mux chip is too slow)
    inline void setMuxDelay(uint8_t mux_delay_A, uint8_t mux_delay_B, uint8_t mux_delay_C = 0, uint8_t mux_delay_D = 0, uint8_t mux_delay_E = 0);

    // Set the number of panels that make up the display area width (default is 1)
    inline void setPanelsWidth(uint8_t panels);

    // Set the brightness of the panels (default is 255)
    inline void setBrightness(uint8_t brightness);

private:
    // Display buffer for the LED matrix
    // Array structure:
    // Whole buffer is splited into bit planes (color depth is used to display grayscale levels).
    // Each bit plane contain separate scan lines (by scan line pattern) so it can be send to display as continuous byte stream by SPI.
    // Each scanline buffer is splitted by number of color components (only 1 component is used for now).
    // Scanline buffer (for each color component) contains data for all matrix panels are sequenced in a chain.
    // Each panel data are mapped to its registers location at the panel (commonly it's a zigzag-chaining pattern).
    // Each register controls set of pixels (usally a byte in series, each bit for one LED per scan pattern).
    // For detais see comments in method mapBufferIndex for mapping of pixel location onto matrix data byte and bit.
    uint8_t* PxMATRIX_buffer;
#ifdef PxMATRIX_DOUBLE_BUFFER
    // Second display buffer (_active_buffer flag controls what buffer is active rendering)
    uint8_t* PxMATRIX_buffer2;
#endif

    // GPIO pins
    const uint8_t _LATCH_PIN;
    const uint8_t _OE_PIN;
    const uint8_t _A_PIN;
    const uint8_t _B_PIN;
    const uint8_t _C_PIN;
    const uint8_t _D_PIN;
    const uint8_t _E_PIN;

    // Number of chained panels
    uint8_t _panels_width;

    // Number of scan lines
    uint8_t _row_pattern;

    // Number of lines per a scan (number of shift registers by height at a single matrix)
    uint8_t _rows_per_pattern;
    // Number of panel bytes in width (number of shift registers by width at a single matrix)
    uint8_t _panel_width_bytes;

    // Panel Brightness
    uint8_t _brightness;

    // Counter for the current color bit plane to be render
    // Counts to PxMATRIX_COLOR_DEPTH (number of bits for color depth)
    uint8_t _display_color;

    // Holds some pre-computed values for faster pixel drawing
    uint16_t* _row_offset;

    // Total number of bytes that is pushed to the display at a time (single scan line)
    // = (HEIGHT / _row_pattern) * (WIDTH / 8) * PxMATRIX_COLOR_COMP
    uint16_t _send_buffer_size;
    // Total number of bytes for display to refresh (all scan lines)
    uint16_t _buffer_size;

    // This is for double buffering
    // Initially _active_buffer = false means that PxMATRIX_buffer is displayed and pixels are drawing into PxMATRIX_buffer2
    bool _active_buffer;

    // Display and color engine
    bool _rotate;
    bool _flip;
    bool _fast_update;

    // Delays for muxer channels
    uint8_t _mux_delay_A;
    uint8_t _mux_delay_B;
    uint8_t _mux_delay_C;
    uint8_t _mux_delay_D;
    uint8_t _mux_delay_E;

    static const uint16_t BUFFER_OUT_OF_BOUNDS = UINT16_MAX;

private:
    inline uint8_t* getBuffer(Buffer_Type selected_buffer);

    inline uint16_t mapBufferIndex(int16_t x, int16_t y, uint8_t* pBit);

    inline uint8_t mapColorLevel(uint8_t r);

    inline uint8_t unmapColorLevel(uint8_t level);

    inline void fillMatrixBuffer(int16_t x, int16_t y, uint8_t r, Buffer_Type selected_buffer);

    inline uint16_t getLatchTime(uint16_t show_time);

    // Light up LEDs and hold for show_time microseconds
    inline void latch(uint16_t show_time);

    // Set multiplexer scan line
    inline void set_mux(uint8_t value);

    inline void spi_init();
};

#include "PxMatrix_impl.h"

#endif /* _PxMATRIX_H */