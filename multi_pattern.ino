#include <FastLED.h>

#define DATA_PIN 6

const uint8_t kMatrixWidth = 32;
const uint8_t kMatrixHeight = 8;
#define NUM_LEDS (kMatrixWidth * kMatrixHeight)

CRGB leds[NUM_LEDS];

// For efficient memory usage, these are globals shared among patterns.
int16_t xt = 0;  // Used for horizontal scrolling effects
uint16_t ht = 0; // Used for color cycling
uint16_t tt = 0; // Frame count to feed into the above

// Global utilities
inline uint16_t mapY(uint8_t x, uint8_t y) {
  return x & 1 ? kMatrixHeight - 1 - y : y;
}

inline uint16_t mapXY(uint8_t x, uint8_t y) {
  return x * kMatrixHeight + mapY(x, y);
}

void toXY(uint16_t index, uint8_t &x, uint8_t &y) {
  x = index / kMatrixHeight;
  y = mapY(x, index % kMatrixHeight);
}

// PATTERN: Sinus
const uint16_t xScale = 65536 / kMatrixWidth;
const uint16_t yScale = 16384 / kMatrixHeight;

void beforeSinus() {
  xt = sin16(tt * 40) * 4;
  ht += 15;
}

void renderSinus(uint16_t index, uint16_t x, int16_t y) {
  x *= xScale;
  y *= yScale;

  // Squeeze vertically just a little bit
  y = (y - 8192) * 1.4 + 9830;

  int16_t w = (int16_t)(sin16((x + xt) * 3) >> 2) + 8192;
  uint16_t v = constrain(16384 - (w > y ? w - y : y - w)*2, 0, 16384);
  uint16_t h = constrain(16384 - (w > y ? w - y : y - w)/2, 0, 16384) + ht;

  leds[index] = CHSV(h >> 6, 255, v >> 6);
}

// PATTERN: Wander
#define NUM_WANDERERS 50
const uint8_t kWanderSpeed = 100;

uint16_t wander(uint16_t p) {
  uint8_t x, y;
  toXY(p, x, y);

  uint8_t r = random(kWanderSpeed);
  if (r == 0) {
    // Wraps around a cylinder.
    x = (x + 1) % kMatrixWidth;
  }
  else if (r == 1) {
    x = x == 0 ? kMatrixWidth - 1 : x - 1;
  }
  else if (r == 2) {
    y = constrain(y + 1, 0, kMatrixHeight - 1);
  }
  else if (r == 3) {
    y = constrain(y - 1, 0, kMatrixHeight - 1);
  }

  return mapXY(x, y);
}

uint16_t wars[NUM_WANDERERS];

void setupWander() {
  randomSeed(analogRead(0));
  for (uint8_t i = 0; i < NUM_WANDERERS; i++) wars[i] = random(NUM_LEDS);
}

void beforeWander() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);

  uint8_t gHue = 0;
  uint8_t gHueDelta = 256 * 1 / NUM_WANDERERS;
  for (uint8_t i = 0; i < NUM_WANDERERS; i++) {
    wars[i] = wander(wars[i]);
    leds[wars[i]] = CHSV(gHue, 255, 255);
    gHue += gHueDelta;
  }

  delay(25);
}

void renderWander(uint16_t index, uint8_t x, uint8_t y) {
}

// PATTERN: SlideUp
void setupSlideUp() {
  randomSeed(analogRead(0));
}

void beforeSlideUp() {
  for (uint8_t x = 0; x < kMatrixWidth; x++) {
    // slide up
    for (uint8_t y = 0; y < kMatrixHeight - 1; y++) {
      leds[mapXY(x, y)] = leds[mapXY(x, y + 1)];
    }
    // new ones enter at the bottom
    leds[mapXY(x, kMatrixHeight - 1)] = random(10) < 2 ? CHSV(random(256), 255, 255) : CHSV(0, 0, 0);
  }
}

void renderSlideUp() {
  delay(90);
}

// Driver program
void setup() {
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(30);

  //setupWander();
}

void loop() {
  beforeSinus();

  // call the renderXY function for each pixel
  uint16_t index = 0;
  for (uint8_t x = 0; x < kMatrixWidth; x++) {
    for (uint8_t y = 0; y < kMatrixHeight; y++) {
      renderSinus(index, x, mapY(x, y));
      index++;
    }
  }

  FastLED.show();
  tt += 1;
}
