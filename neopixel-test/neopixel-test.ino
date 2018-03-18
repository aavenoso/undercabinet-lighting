#include "NeoPixel.h"

#define LED_STRIP_NUM_LEDS   120
#define LED_STRIP_BRIGHTNESS 255

// see https://github.com/stm32duino/Arduino_Core_STM32/blob/b0869681bae922eee17f90ffd959eca88ff92bf6/variants/NUCLEO_F103RB/variant.cpp
#define LED_STRIP_PIN  D47  /* PA1 */
#define USER_LED       D13  /* PA5 */

uint8_t ledStripPixels[LED_STRIP_NUM_LEDS * 4];
NeoPixel ledStrip(
    LED_STRIP_NUM_LEDS,
    LED_STRIP_PIN,
    NEO_PIXEL_GRBW | NEO_PIXEL_KHZ800,
    ledStripPixels,
    sizeof(ledStripPixels)
);

void setup() {
  Serial.begin(9600);
  setupLedStrip();
  pinMode(USER_LED, OUTPUT);
  Serial.println("setup complete");
}

void setupLedStrip() {
  ledStrip.setBrightness(LED_STRIP_BRIGHTNESS);
  ledStrip.begin();
  ledStrip.show(); // Initialize all pixels to 'off'
}

void loop() {
  Serial.println("loop");
  colorWipe(ledStrip.Color(255, 0, 0), 50); // Red
  colorWipe(ledStrip.Color(0, 255, 0), 50); // Green
  colorWipe(ledStrip.Color(0, 0, 255), 50); // Blue
  colorWipe(ledStrip.Color(0, 0, 0, 255), 50); // White

  digitalWrite(USER_LED, LOW);
  delay(500);
  digitalWrite(USER_LED, HIGH);
  delay(500);
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for (uint16_t i = 0; i < ledStrip.numPixels(); i++) {
    ledStrip.setPixelColor(i, c);
    ledStrip.show();
    delay(wait);
  }
}
