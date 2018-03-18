#include "NeoPixel.h"
#include <Arduino.h>
#include <cassert>

NeoPixel::NeoPixel(uint16_t numberOfLeds, uint8_t dataPin, uint8_t type, uint8_t *pixels, uint16_t pixelsLength)
    : m_begun(false), m_brightness(0), m_pixels(pixels), m_pixelsLength(pixelsLength), m_endTime(0) {
  updateType(type);
  updateLength(numberOfLeds);
  setPin(dataPin);
}

NeoPixel::~NeoPixel() {
  if (m_pin >= 0) {
    pinMode(m_pin, INPUT);
  }
}

void NeoPixel::begin() {
  if (m_pin >= 0) {
    pinMode(m_pin, OUTPUT);
    digitalWrite(m_pin, LOW);
  }
  m_begun = true;
}

void NeoPixel::updateLength(uint16_t n) {
  m_numBytes = n * ((m_wOffset == m_rOffset) ? 3 : 4);
  assert(m_pixelsLength >= m_numBytes);
  memset(m_pixels, 0, m_numBytes);
  m_numLEDs = n;
}

void NeoPixel::updateType(uint8_t t) {
  boolean oldThreeBytesPerPixel = (m_wOffset == m_rOffset); // false if RGBW

  m_wOffset = (t >> 6) & 0b11; // See notes in header file
  m_rOffset = (t >> 4) & 0b11; // regarding R/G/B/W offsets
  m_gOffset = (t >> 2) & 0b11;
  m_bOffset = t & 0b11;

  // If bytes-per-pixel has changed (and pixel data was previously
  // allocated), re-allocate to new size.  Will clear any data.
  boolean newThreeBytesPerPixel = (m_wOffset == m_rOffset);
  if (newThreeBytesPerPixel != oldThreeBytesPerPixel) {
    updateLength(m_numLEDs);
  }
}

void NeoPixel::show() {
  uint8_t *ptr = m_pixels;
  uint8_t *end = ptr + m_numBytes;
  uint8_t p = *ptr++;
  uint8_t bitMask = 0x80;

  GPIO_TypeDef *port = digitalPinToPort(m_pin);
  uint32_t setPin = digitalPinToBitMask(m_pin);
  uint32_t resetPin = setPin << 16U;

  noInterrupts();
  for (;;) {
    if (p & bitMask) { // ONE
      // High 800ns
      port->BSRR = setPin;
      asm("nop; nop; nop; nop; nop; nop; nop; nop;"
          "nop; nop; nop; nop; nop; nop; nop; nop;"
          "nop; nop; nop; nop; nop; nop; nop; nop;"
          "nop; nop; nop; nop; nop; nop; nop; nop;"
          "nop; nop; nop; nop; nop; nop; nop; nop;"
          "nop; nop; nop; nop; nop; nop; nop; nop;"
          "nop; nop; nop; nop; nop; nop; nop;");
      // Low 450ns
      port->BSRR = resetPin;
      asm("nop; nop; nop; nop; nop; nop; nop; nop;"
          "nop; nop; nop; nop; nop; nop; nop; nop;"
          "nop; nop; nop; nop; nop; nop; nop;");
    } else { // ZERO
      // High 400ns
      port->BSRR = setPin;
      asm("nop; nop; nop; nop; nop; nop; nop; nop;"
          "nop; nop; nop; nop; nop; nop; nop; nop;"
          "nop; nop; nop; nop; nop; nop; nop;");
      // Low 850ns
      port->BSRR = resetPin;
      asm("nop; nop; nop; nop; nop; nop; nop; nop;"
          "nop; nop; nop; nop; nop; nop; nop; nop;"
          "nop; nop; nop; nop; nop; nop; nop; nop;"
          "nop; nop; nop; nop; nop; nop; nop; nop;"
          "nop; nop; nop; nop; nop; nop; nop; nop;"
          "nop; nop; nop; nop; nop;");
    }
    if (bitMask >>= 1) {
      // Move on to the next pixel
      asm("nop;");
    } else {
      if (ptr >= end) {
        break;
      }
      p = *ptr++;
      bitMask = 0x80;
    }
  }
  interrupts();
}

void NeoPixel::setPin(uint8_t dataPin) {
  if (m_begun && (m_pin >= 0)) {
    pinMode(m_pin, INPUT);
  }
  m_pin = dataPin;
  if (m_begun) {
    pinMode(m_pin, OUTPUT);
    digitalWrite(m_pin, LOW);
  }
}

void NeoPixel::setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b) {
  if (n < m_numLEDs) {
    if (m_brightness) { // See notes in setBrightness()
      r = (r * m_brightness) >> 8;
      g = (g * m_brightness) >> 8;
      b = (b * m_brightness) >> 8;
    }
    uint8_t *p;
    if (m_wOffset == m_rOffset) { // Is an RGB-type strip
      p = &m_pixels[n * 3];    // 3 bytes per pixel
    } else {                 // Is a WRGB-type strip
      p = &m_pixels[n * 4];    // 4 bytes per pixel
      p[m_wOffset] = 0;        // But only R,G,B passed -- set W to 0
    }
    p[m_rOffset] = r;          // R,G,B always stored
    p[m_gOffset] = g;
    p[m_bOffset] = b;
  }
}

void NeoPixel::setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
  if (n < m_numLEDs) {
    if (m_brightness) { // See notes in setBrightness()
      r = (r * m_brightness) >> 8;
      g = (g * m_brightness) >> 8;
      b = (b * m_brightness) >> 8;
      w = (w * m_brightness) >> 8;
    }
    uint8_t *p;
    if (m_wOffset == m_rOffset) { // Is an RGB-type strip
      p = &m_pixels[n * 3];    // 3 bytes per pixel (ignore W)
    } else {                 // Is a WRGB-type strip
      p = &m_pixels[n * 4];    // 4 bytes per pixel
      p[m_wOffset] = w;        // Store W
    }
    p[m_rOffset] = r;          // Store R,G,B
    p[m_gOffset] = g;
    p[m_bOffset] = b;
  }
}

void NeoPixel::setPixelColor(uint16_t n, uint32_t c) {
  if (n < m_numLEDs) {
    uint8_t *p;
    uint8_t r = (uint8_t)(c >> 16);
    uint8_t g = (uint8_t)(c >> 8);
    uint8_t b = (uint8_t) c;
    if (m_brightness) { // See notes in setBrightness()
      r = (r * m_brightness) >> 8;
      g = (g * m_brightness) >> 8;
      b = (b * m_brightness) >> 8;
    }
    if (m_wOffset == m_rOffset) {
      p = &m_pixels[n * 3];
    } else {
      p = &m_pixels[n * 4];
      uint8_t w = (uint8_t)(c >> 24);
      p[m_wOffset] = m_brightness ? ((w * m_brightness) >> 8) : w;
    }
    p[m_rOffset] = r;
    p[m_gOffset] = g;
    p[m_bOffset] = b;
  }
}

// Convert separate R,G,B into packed 32-bit RGB color.
// Packed format is always RGB, regardless of LED strand color order.
/*static*/ uint32_t NeoPixel::Color(uint8_t r, uint8_t g, uint8_t b) {
  return ((uint32_t) r << 16) | ((uint32_t) g << 8) | b;
}

// Convert separate R,G,B,W into packed 32-bit WRGB color.
// Packed format is always WRGB, regardless of LED strand color order.
/*static*/ uint32_t NeoPixel::Color(uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
  return ((uint32_t) w << 24) | ((uint32_t) r << 16) | ((uint32_t) g << 8) | b;
}

uint16_t NeoPixel::numPixels() const {
  return m_numLEDs;
}

// Adjust output brightness; 0=darkest (off), 255=brightest.  This does
// NOT immediately affect what's currently displayed on the LEDs.  The
// next call to show() will refresh the LEDs at this level.  However,
// this process is potentially "lossy," especially when increasing
// brightness.  The tight timing in the WS2811/WS2812 code means there
// aren't enough free cycles to perform this scaling on the fly as data
// is issued.  So we make a pass through the existing color data in RAM
// and scale it (subsequent graphics commands also work at this
// brightness level).  If there's a significant step up in brightness,
// the limited number of steps (quantization) in the old data will be
// quite visible in the re-scaled version.  For a non-destructive
// change, you'll need to re-render the full strip data.  C'est la vie.
void NeoPixel::setBrightness(uint8_t brightness) {
// Stored brightness value is different than what's passed.
  // This simplifies the actual scaling math later, allowing a fast
  // 8x8-bit multiply and taking the MSB.  'brightness' is a uint8_t,
  // adding 1 here may (intentionally) roll over...so 0 = max brightness
  // (color values are interpreted literally; no scaling), 1 = min
  // brightness (off), 255 = just below max brightness.
  uint8_t newBrightness = brightness + 1;
  if (newBrightness != brightness) { // Compare against prior value
    // Brightness has changed -- re-scale existing data in RAM
    uint8_t c;
    uint8_t *ptr = m_pixels;
    uint8_t oldBrightness = brightness - 1; // De-wrap old brightness value
    uint16_t scale;
    if (oldBrightness == 0) {
      scale = 0;// Avoid /0
    } else if (brightness == 255) {
      scale = 65535 / oldBrightness;
    } else {
      scale = (((uint16_t) newBrightness << 8) - 1) / oldBrightness;
    }
    for (uint16_t i = 0; i < m_numBytes; i++) {
      c = *ptr;
      *ptr++ = (c * scale) >> 8;
    }
    brightness = newBrightness;
  }
}
