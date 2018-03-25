#include "NeoPixel.h"

#define LED_STRIP_NUM_LEDS 240

#define MAX_BRIGHTNESS 255
#define BUTTON_OFF_MAX_BRIGHTNESS 200
#define MAX_HUE 255
#define MIN_LED_DELAY 10

#define MOTION_DELAY 10000

#define MOTION_INTERRUPT_PIN 2
#define LED_STRIP_PIN D47 /* PA1 */
#define BUTTON_LED_RED_PIN 9
#define BUTTON_LED_GREEN_PIN 10
#define BUTTON_LED_BLUE_PIN 11
#define MAIN_BUTTON_INTERRUPT_PIN 3
#define SECONDARY_BUTTON_INTERRUPT_PIN 4

// change the motion sensor to poll instead of interrupt
// use the two interrupt pins for the buttons

uint8_t ledStripPixels[LED_STRIP_NUM_LEDS * 4];
NeoPixel strip(
    LED_STRIP_PIN,
    LED_STRIP_NUM_LEDS,
    NEO_PIXEL_GRBW | NEO_PIXEL_KHZ800,
    ledStripPixels,
    sizeof(ledStripPixels)
);

byte neopix_gamma[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };

#define STATE_OFF 0
#define STATE_TURNING_ON 1
#define STATE_ON 2
#define STATE_TURNING_OFF 3
#define STATE_MOTION_OFF 4
#define STATE_MOTION_TURNING_ON 5
#define STATE_MOTION_TURNING_OFF 6
#define STATE_MOTION_ON 7
#define STATE_COLOR 8
#define STATE_DISCO 9

#define BTN2_STATE_BRIGHTNESS 0
#define BTN2_STATE_COLOR 1
#define BTN2_STATE_SPEED 2
#define BTN2_STATE_PROGRAM 3

#define PROGRAM_CYLON 0
#define PROGRAM_FIREWORKS 1
#define PROGRAM_COUNT 2

volatile unsigned long motionDetectedAt;
volatile unsigned int state;
volatile unsigned int brightness = 255;
volatile float delayMultiplier = 1.0;
volatile unsigned int hueOffset;
volatile unsigned int btn2State; 
volatile unsigned int discoProgram;
volatile unsigned int counter;

void setup() {
  Serial.begin(9600);
  setupNeoPixels();
  setupMainButton();
  setupSecondaryButton();
  setupMotionSensors();
  state = STATE_DISCO;
  discoProgram = PROGRAM_CYLON; 
}

void setupMotionSensors() {
  attachInterrupt(digitalPinToInterrupt(MOTION_INTERRUPT_PIN), motionDetected, RISING);
}

void setupNeoPixels() {
  strip.setBrightness(MAX_BRIGHTNESS);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}

void setupMainButton() {
  pinMode(BUTTON_LED_RED_PIN, OUTPUT);
  pinMode(BUTTON_LED_GREEN_PIN, OUTPUT);
  pinMode(BUTTON_LED_BLUE_PIN, OUTPUT);
  analogWrite(BUTTON_LED_RED_PIN, 255);
  analogWrite(BUTTON_LED_GREEN_PIN, 255);
  analogWrite(BUTTON_LED_BLUE_PIN, 255);
  attachInterrupt(digitalPinToInterrupt(MAIN_BUTTON_INTERRUPT_PIN), mainButtonChanged, CHANGE);
}

void setupSecondaryButton() {
  attachInterrupt(digitalPinToInterrupt(SECONDARY_BUTTON_INTERRUPT_PIN), secondaryButtonPressed, RISING);
  // attach Interrupts for turning the digital encoder up and down
}

void loop() {
  switch(state) {
    case STATE_OFF: stateOff(); break;
    case STATE_TURNING_ON: stateTurningOn(); break;
    case STATE_ON: stateOn(); break;
    case STATE_TURNING_OFF: stateTurningOff(); break;
    case STATE_MOTION_OFF: stateMotionOff(); break;
    case STATE_MOTION_TURNING_ON: stateMotionTurningOn(); break;
    case STATE_MOTION_TURNING_OFF: stateMotionTurningOff(); break;
    case STATE_MOTION_ON: stateMotionOn(); break;
    case STATE_COLOR: stateColor(); break;
    case STATE_DISCO: stateDisco(); break;
  }
}

// do nothing to the neopixels, make the button breath in red
void stateOff() {
  unsigned int buttonOffBrightness = counter;
  if (counter > BUTTON_OFF_MAX_BRIGHTNESS) {
    buttonOffBrightness = (BUTTON_OFF_MAX_BRIGHTNESS * 2) - counter;
    if (counter > (BUTTON_OFF_MAX_BRIGHTNESS * 2)) {
      counter = -1;
    }
  }
  analogWrite(BUTTON_LED_RED_PIN, neopix_gamma[buttonOffBrightness]);
  delay(MIN_LED_DELAY);
  counter++;
}

// fade up to white and then set state to ON
void stateTurningOn() {
  fadeUp(STATE_TURNING_OFF);
  // only do it this way until the button is hooked up
  //fadeUp(STATE_ON);
}

void stateOn() {
  // do nothing
}

// fade down to black and then set state to OFF
void stateTurningOff() {
//  fadeDown(STATE_OFF);
// only do it this way until the button is hooked up
  fadeDown(STATE_TURNING_ON);
}

void stateMotionOff() {
  // do nothing
}

void stateMotionTurningOn() {
 fadeUp(STATE_MOTION_ON); 
}

void stateMotionTurningOff() {
  fadeDown(STATE_MOTION_OFF);
}

void stateMotionOn() {
  if (millis() - motionDetectedAt > MOTION_DELAY) {
    state = STATE_MOTION_TURNING_OFF;
  }
  delay(MIN_LED_DELAY);
}

void stateColor() {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, adjustBrightnessOfColor(colorWheel(hueOffset)) );
  }
  delay(MIN_LED_DELAY);
}

void stateDisco() {
  switch(discoProgram) {
    case PROGRAM_CYLON: programCylon(); break;
    case PROGRAM_FIREWORKS: programFireworks(); break;
  }
}

void programCylon() {
  unsigned int cylonLength = 10;
  unsigned int startingLed = counter;
  unsigned int maxStartingLed = strip.numPixels() - cylonLength;
  if (startingLed > maxStartingLed) {
    startingLed = (maxStartingLed * 2) - counter;
  }
  uint32_t black = strip.Color(0, 0, 0);
//  uint32_t color = adjustBrightnessOfColor(colorWheel(hueOffset));
  uint32_t color = adjustBrightnessOfColor(colorWheel(startingLed));
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    if (i >= startingLed && i < startingLed + cylonLength) {
      strip.setPixelColor(i, color);
    } else {
      strip.setPixelColor(i, black);
    }
    
  }
  strip.show();
  delay(MIN_LED_DELAY * delayMultiplier);
  if (counter < (maxStartingLed * 2)) {
    counter++;
  } else {
    counter = 0;
  }
}

void programFireworks() {

}

void mainButtonChanged() {
  counter = 0;
  btn2State = BTN2_STATE_BRIGHTNESS;
  // change state
  // how do I tell if it was pressed or released
}

void secondaryButtonPressed() {
  if (state == STATE_COLOR && btn2State == BTN2_STATE_BRIGHTNESS) {
    btn2State = BTN2_STATE_COLOR;
  } else if (state == STATE_DISCO && btn2State == BTN2_STATE_BRIGHTNESS) {
    btn2State = BTN2_STATE_COLOR;
  } else if (state == STATE_DISCO && btn2State == BTN2_STATE_COLOR) {
    btn2State = BTN2_STATE_SPEED;
  } else if (state == STATE_DISCO && btn2State == BTN2_STATE_SPEED) {
    btn2State = BTN2_STATE_PROGRAM;
  } else {
    btn2State = BTN2_STATE_BRIGHTNESS;
  }
}

void dialTurnedUp() {
  switch(btn2State) {
    case BTN2_STATE_BRIGHTNESS:
      brightness++;
      if (brightness > MAX_BRIGHTNESS) {
        brightness = MAX_BRIGHTNESS;
      }
      break;
    case BTN2_STATE_COLOR:
      hueOffset++;
      if (hueOffset > MAX_HUE) {
        hueOffset = 0;
      }
      break;
    case BTN2_STATE_SPEED:
      delayMultiplier += 0.1;
      if (delayMultiplier > 4.0) {
        delayMultiplier = 4.0;
      }
      break;
    case BTN2_STATE_PROGRAM:
      discoProgram++;
      if (discoProgram >= PROGRAM_COUNT) {
        discoProgram = 0;
      }
  }
}

void motionDetected() {
  Serial.print("Motion Detected");
  if (state == STATE_MOTION_OFF || state == STATE_MOTION_TURNING_OFF) {
    state = STATE_MOTION_TURNING_ON;
  }
  motionDetectedAt = millis();
}

void fadeUp(unsigned int finalState) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(0,0,0, neopix_gamma[counter] ) );
  }
  strip.show();
  delay(MIN_LED_DELAY);
  if (counter < 255) {
    counter++;
  } else {
    state = finalState;
    counter = 0;
  }
}

void fadeDown(unsigned int finalState) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(0,0,0, neopix_gamma[255-counter] ) );
  }
  strip.show();
  delay(MIN_LED_DELAY);
  if (counter < 255) {
    counter++;
  } else {
    state = finalState;
    counter = 0;
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t colorWheel(byte wheelPos) {
  wheelPos = wheelPos % 255;
  wheelPos = 255 - wheelPos;
  if(wheelPos < 85) {
    return strip.Color(255 - wheelPos * 3, 0, wheelPos * 3,0);
  }
  if(wheelPos < 170) {
    wheelPos -= 85;
    return strip.Color(0, wheelPos * 3, 255 - wheelPos * 3,0);
  }
  wheelPos -= 170;
  return strip.Color(wheelPos * 3, 255 - wheelPos * 3, 0,0);
}

uint32_t adjustBrightnessOfColor(uint32_t color) {
  // how to I adjust the brightness of the color ?
  return color;
}

uint8_t red(uint32_t c) {
  return (c >> 16);
}
uint8_t green(uint32_t c) {
  return (c >> 8);
}
uint8_t blue(uint32_t c) {
  return (c);
}


