// This implementation is influenced by https://github.com/adafruit/Adafruit_NeoPixel

#ifndef NEOPIXEL_H
#define NEOPIXEL_H

#include <stdint.h>

// The order of primary colors in the NeoPixel data stream can vary
// among device types, manufacturers and even different revisions of
// the same item.  The third parameter to the Adafruit_NeoPixel
// constructor encodes the per-pixel byte offsets of the red, green
// and blue primaries (plus white, if present) in the data stream --
// the following #defines provide an easier-to-use named version for
// each permutation.  e.g. NEO_GRB indicates a NeoPixel-compatible
// device expecting three bytes per pixel, with the first byte
// containing the green value, second containing red and third
// containing blue.  The in-memory representation of a chain of
// NeoPixels is the same as the data-stream order; no re-ordering of
// bytes is required when issuing data to the chain.

// Bits 5,4 of this value are the offset (0-3) from the first byte of
// a pixel to the location of the red color byte.  Bits 3,2 are the
// green offset and 1,0 are the blue offset.  If it is an RGBW-type
// device (supporting a white primary in addition to R,G,B), bits 7,6
// are the offset to the white byte...otherwise, bits 7,6 are set to
// the same value as 5,4 (red) to indicate an RGB (not RGBW) device.
// i.e. binary representation:
// 0bWWRRGGBB for RGBW devices
// 0bRRRRGGBB for RGB

// RGB NeoPixel permutations; white and red offsets are always same
// Offset:              W          R          G          B
#define NEO_PIXEL_RGB  ((0 << 6) | (0 << 4) | (1 << 2) | (2))
#define NEO_PIXEL_RBG  ((0 << 6) | (0 << 4) | (2 << 2) | (1))
#define NEO_PIXEL_GRB  ((1 << 6) | (1 << 4) | (0 << 2) | (2))
#define NEO_PIXEL_GBR  ((2 << 6) | (2 << 4) | (0 << 2) | (1))
#define NEO_PIXEL_BRG  ((1 << 6) | (1 << 4) | (2 << 2) | (0))
#define NEO_PIXEL_BGR  ((2 << 6) | (2 << 4) | (1 << 2) | (0))

// RGBW NeoPixel permutations; all 4 offsets are distinct
// Offset:              W          R          G          B
#define NEO_PIXEL_WRGB ((0 << 6) | (1 << 4) | (2 << 2) | (3))
#define NEO_PIXEL_WRBG ((0 << 6) | (1 << 4) | (3 << 2) | (2))
#define NEO_PIXEL_WGRB ((0 << 6) | (2 << 4) | (1 << 2) | (3))
#define NEO_PIXEL_WGBR ((0 << 6) | (3 << 4) | (1 << 2) | (2))
#define NEO_PIXEL_WBRG ((0 << 6) | (2 << 4) | (3 << 2) | (1))
#define NEO_PIXEL_WBGR ((0 << 6) | (3 << 4) | (2 << 2) | (1))

#define NEO_PIXEL_RWGB ((1 << 6) | (0 << 4) | (2 << 2) | (3))
#define NEO_PIXEL_RWBG ((1 << 6) | (0 << 4) | (3 << 2) | (2))
#define NEO_PIXEL_RGWB ((2 << 6) | (0 << 4) | (1 << 2) | (3))
#define NEO_PIXEL_RGBW ((3 << 6) | (0 << 4) | (1 << 2) | (2))
#define NEO_PIXEL_RBWG ((2 << 6) | (0 << 4) | (3 << 2) | (1))
#define NEO_PIXEL_RBGW ((3 << 6) | (0 << 4) | (2 << 2) | (1))

#define NEO_PIXEL_GWRB ((1 << 6) | (2 << 4) | (0 << 2) | (3))
#define NEO_PIXEL_GWBR ((1 << 6) | (3 << 4) | (0 << 2) | (2))
#define NEO_PIXEL_GRWB ((2 << 6) | (1 << 4) | (0 << 2) | (3))
#define NEO_PIXEL_GRBW ((3 << 6) | (1 << 4) | (0 << 2) | (2))
#define NEO_PIXEL_GBWR ((2 << 6) | (3 << 4) | (0 << 2) | (1))
#define NEO_PIXEL_GBRW ((3 << 6) | (2 << 4) | (0 << 2) | (1))

#define NEO_PIXEL_BWRG ((1 << 6) | (2 << 4) | (3 << 2) | (0))
#define NEO_PIXEL_BWGR ((1 << 6) | (3 << 4) | (2 << 2) | (0))
#define NEO_PIXEL_BRWG ((2 << 6) | (1 << 4) | (3 << 2) | (0))
#define NEO_PIXEL_BRGW ((3 << 6) | (1 << 4) | (2 << 2) | (0))
#define NEO_PIXEL_BGWR ((2 << 6) | (3 << 4) | (1 << 2) | (0))
#define NEO_PIXEL_BGRW ((3 << 6) | (2 << 4) | (1 << 2) | (0))

#define NEO_PIXEL_KHZ800 0x0000 // 800 KHz datastream

#define NEO_PIXEL_PWM_BUFFER_NUMBER_OF_HALVES 2
#define NEO_PIXEL_PWM_BUFFER_BITS_PER_BYTE    8
#define NEO_PIXEL_PWM_BUFFER_PIXELS_PER_HALF  10
#define NEO_PIXEL_PWM_BUFFER_LEN (NEO_PIXEL_PWM_BUFFER_NUMBER_OF_HALVES * NEO_PIXEL_PWM_BUFFER_BITS_PER_BYTE * NEO_PIXEL_PWM_BUFFER_PIXELS_PER_HALF)

class NeoPixel {
public:
  NeoPixel(uint16_t pin, uint16_t numberOfLeds, uint8_t type, uint8_t *pixels, uint16_t pixelsLength);

  virtual ~NeoPixel();

  void setBrightness(uint8_t brightness);

  void begin();

  void show();

  void setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b);

  void setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b, uint8_t w);

  void setPixelColor(uint16_t n, uint32_t c);

  uint16_t numPixels() const;

  static uint32_t Color(uint8_t r, uint8_t g, uint8_t b);

  static uint32_t Color(uint8_t r, uint8_t g, uint8_t b, uint8_t w);

  void fillHalfOfPWMBuffer();

private:
  void configureDMA();

  void configureTIM();

  void updateLength(uint16_t n);

  void updateType(uint8_t t);

  void stopDMA();

  bool m_begun;         // true if begin() previously called
  uint16_t m_pin;
  uint16_t m_numLEDs;   // Number of RGB LEDs in strip
  uint16_t m_numBytes;  // Size of 'pixels' buffer below (3 or 4 bytes/pixel)
  uint8_t m_brightness;
  uint8_t *m_pixels;    // Holds LED color values (3 or 4 bytes each)
  uint8_t *m_pixelsEnd; // pointer to the end of the m_pixels array
  uint8_t *m_currentPixel;      // current pixel being read into to DMA
  uint32_t m_pwmBuffer[NEO_PIXEL_PWM_BUFFER_LEN]; // DMA PWM buffer
  uint32_t* m_pwmBufferEnd;     // end of m_pwmBuffer
  uint32_t* m_currentPwmBuffer; // current write location for next value
  uint32_t m_duty0;     // length of a 0 bit in PWM
  uint32_t m_duty1;     // length of a 1 bit in PWM
  uint8_t m_rOffset;    // Index of red byte within each 3- or 4-byte pixel
  uint8_t m_gOffset;    // Index of green byte
  uint8_t m_bOffset;    // Index of blue byte
  uint8_t m_wOffset;    // Index of white byte (same as rOffset if no white)
  uint32_t m_endTime;   // Latch timing reference
};

#endif // NEOPIXEL_H
