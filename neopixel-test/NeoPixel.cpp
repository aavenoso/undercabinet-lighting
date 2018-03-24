#include "NeoPixel.h"
#include <Arduino.h>
#include <cassert>

static NeoPixel *neoPixel;

NeoPixel::NeoPixel(uint16_t pin, uint16_t numberOfLeds, uint8_t type, uint8_t *pixels, uint16_t pixelsLength)
    : m_begun(false), m_pin(pin), m_brightness(0), m_pixels(pixels), m_pixelsEnd(pixels + pixelsLength), m_endTime(0) {
  updateType(type);
  updateLength(numberOfLeds);
}

NeoPixel::~NeoPixel() {
}

void NeoPixel::begin() {
  configureDMA();
  configureTIM();
  m_begun = true;
}

void NeoPixel::configureDMA() {
  NVIC_SetPriority(DMA1_Channel7_IRQn, 0);
  NVIC_EnableIRQ(DMA1_Channel7_IRQn);

  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);

  LL_DMA_ConfigTransfer(
      DMA1,
      LL_DMA_CHANNEL_7,
      LL_DMA_DIRECTION_MEMORY_TO_PERIPH
      | LL_DMA_PRIORITY_HIGH
      | LL_DMA_MODE_CIRCULAR
      | LL_DMA_PERIPH_NOINCREMENT
      | LL_DMA_MEMORY_INCREMENT
      | LL_DMA_PDATAALIGN_WORD
      | LL_DMA_MDATAALIGN_WORD
  );

  LL_DMA_EnableIT_HT(DMA1, LL_DMA_CHANNEL_7);
  LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_7);
}

void NeoPixel::configureTIM() {
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOA);
  LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_1, LL_GPIO_MODE_ALTERNATE);
  LL_GPIO_SetPinPull(GPIOA, LL_GPIO_PIN_1, LL_GPIO_PULL_DOWN);
  LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_1, LL_GPIO_SPEED_FREQ_HIGH);
  NVIC_SetPriority(TIM2_IRQn, 0);
  NVIC_EnableIRQ(TIM2_IRQn);

  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM2);

  LL_TIM_SetPrescaler(TIM2, 0);

  LL_TIM_SetRepetitionCounter(TIM2, 4 - 1);

  LL_TIM_OC_SetMode(TIM2, LL_TIM_CHANNEL_CH2, LL_TIM_OCMODE_PWM1);

  LL_TIM_OC_ConfigOutput(TIM2, LL_TIM_CHANNEL_CH2, LL_TIM_OCPOLARITY_HIGH | LL_TIM_OCIDLESTATE_HIGH);

  LL_TIM_OC_SetCompareCH2(TIM2, 0);

  LL_TIM_OC_EnablePreload(TIM2, LL_TIM_CHANNEL_CH2);

  LL_TIM_EnableDMAReq_UPDATE(TIM2);

  LL_TIM_EnableDMAReq_CC2(TIM2);
}

void NeoPixel::show() {
  stopDMA();

  uint32_t autoReload = (1200 * (SystemCoreClock / 100000)) / 10000;
  LL_TIM_SetAutoReload(TIM2, autoReload);
  m_duty0 = autoReload * 300 / 1200;
  m_duty1 = autoReload * 600 / 1200;

  neoPixel = this;
  m_currentPixel = m_pixels;
  m_currentPwmBuffer = m_pwmBuffer;
  m_pwmBufferEnd = m_pwmBuffer + NEO_PIXEL_PWM_BUFFER_LEN;
  memset(m_pwmBuffer, 0, NEO_PIXEL_PWM_BUFFER_LEN * sizeof(uint32_t));

  LL_DMA_ConfigAddresses(
      DMA1,
      LL_DMA_CHANNEL_7,
      (uint32_t) m_pwmBuffer,
      (uint32_t) & TIM2->CCR2,
      LL_DMA_GetDataTransferDirection(DMA1, LL_DMA_CHANNEL_7)
  );
  LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_7, NEO_PIXEL_PWM_BUFFER_LEN);

  LL_TIM_CC_EnableChannel(TIM2, LL_TIM_CHANNEL_CH2);
  LL_TIM_EnableAllOutputs(TIM2);
  LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_7);
  LL_TIM_EnableCounter(TIM2);
  LL_TIM_GenerateEvent_UPDATE(TIM2);
}

void NeoPixel::stopDMA() {
  LL_TIM_DisableAllOutputs(TIM2);
  LL_TIM_CC_DisableChannel(TIM2, LL_TIM_CHANNEL_CH2);
  LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_7);
  LL_TIM_DisableCounter(TIM2);
}

void NeoPixel::updateLength(uint16_t n) {
  m_numBytes = n * ((m_wOffset == m_rOffset) ? 3 : 4);
  assert((m_pixelsEnd - m_pixels) >= m_numBytes);
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

void NeoPixel::setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b) {
  if (n < m_numLEDs) {
    if (m_brightness) { // See notes in setBrightness()
      r = (r * m_brightness) >> 8;
      g = (g * m_brightness) >> 8;
      b = (b * m_brightness) >> 8;
    }
    uint8_t *p;
    if (m_wOffset == m_rOffset) { // Is an RGB-type strip
      p = &m_pixels[n * 3];       // 3 bytes per pixel
    } else {                      // Is a WRGB-type strip
      p = &m_pixels[n * 4];       // 4 bytes per pixel
      p[m_wOffset] = 0;           // But only R,G,B passed -- set W to 0
    }
    p[m_rOffset] = r;             // R,G,B always stored
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

void NeoPixel::fillHalfOfPWMBuffer() {
  uint8_t bitMask = 0x80;
  uint8_t p = m_currentPixel < m_pixelsEnd ? *m_currentPixel : 0;

  for (int i = 0; i < NEO_PIXEL_PWM_BUFFER_LEN / 2; i++) {
    if (m_currentPixel >= m_pixelsEnd) {
      *m_currentPwmBuffer = 0;
    } else if (p & bitMask) {
      *m_currentPwmBuffer = m_duty1;
    } else {
      *m_currentPwmBuffer = m_duty0;
    }
    bitMask = bitMask >> 1;
    if (bitMask == 0x00) {
      if (m_currentPixel < m_pixelsEnd - 1) {
        m_currentPixel++;
        p = *m_currentPixel;
      } else {
        p = 0;
      }
      bitMask = 0x80;
    }
    m_currentPwmBuffer++;
    if (m_currentPwmBuffer >= m_pwmBufferEnd) {
      m_currentPwmBuffer = m_pwmBuffer;
    }
  }
}

extern "C" {
void DMA1_Channel7_IRQHandler() {
  if (LL_DMA_IsActiveFlag_TC7(DMA1) == 1) {
    LL_DMA_ClearFlag_TC7(DMA1);
    neoPixel->fillHalfOfPWMBuffer();
  }
  if (LL_DMA_IsActiveFlag_HT7(DMA1) == 1) {
    LL_DMA_ClearFlag_HT7(DMA1);
    neoPixel->fillHalfOfPWMBuffer();
  }
}
}
