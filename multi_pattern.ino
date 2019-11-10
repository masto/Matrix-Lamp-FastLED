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

// PATTERN: Sinus
const uint16_t xScale = 65536 / kMatrixWidth;
const uint16_t yScale = 16384 / kMatrixHeight;

void beforeSinus() {
  xt = sin16(tt * 40) * 4;
  ht += 15;
}

void renderSinus() {
  for (uint16_t index = 0; index < NUM_LEDS; index++) {
    uint16_t x = index / kMatrixHeight;
    int16_t y = index % kMatrixHeight;
    if (x & 1) y = kMatrixHeight - 1 - y;

    x *= xScale;
    y *= yScale;

    // Squeeze vertically just a little bit
    y = (y - 8192) * 1.4 + 9830;

    int16_t w = (int16_t)(sin16((x + xt) * 3) >> 2) + 8192;
    uint16_t v = constrain(16384 - (w > y ? w - y : y - w)*2, 0, 16384);
    uint16_t h = constrain(16384 - (w > y ? w - y : y - w)/2, 0, 16384) + ht;

    leds[index] = CHSV(h >> 6, 255, v >> 6);
  }
}

// Driver program
void setup() {
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(100);
}

void loop() {
  beforeSinus();
  renderSinus();
  FastLED.show();

  tt += 1;
}
