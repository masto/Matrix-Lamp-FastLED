#include <assert.h>
#include <FastLED.h>

#define DATA_PIN 6

#define WIDTH 32
#define HEIGHT 8
#define NUM_LEDS (WIDTH * HEIGHT)
#define SEQUENCE_TIME (60L * 1000L)

CRGB leds[NUM_LEDS];

// For efficient memory usage, these are globals shared among patterns.
int16_t xt = 0;  // Used for horizontal scrolling effects
uint16_t ht = 0; // Used for color cycling
uint16_t tt = 0; // Frame count to feed into the above

// Shared buffer for patterns that need scratch arrays
uint16_t data[NUM_LEDS];

// Global utilities
inline uint16_t mapY(uint8_t x, uint8_t y) {
  return x & 1 ? HEIGHT - 1 - y : y;
}

inline uint16_t mapXY(uint8_t x, uint8_t y) {
  return x * HEIGHT + mapY(x, y);
}

void toXY(uint16_t index, uint8_t &x, uint8_t &y) {
  x = index / HEIGHT;
  y = mapY(x, index % HEIGHT);
}

// PATTERN: Sinus
const uint16_t xScale = 65536 / WIDTH;
const uint16_t yScale = 16384 / HEIGHT;

unsigned long beforeSinus() {
  xt = sin16(tt * 40) * 4;
  ht += 15;

  return 0;
}

void renderSinus(uint16_t index, uint16_t x, int16_t y) {
  x *= xScale;
  y *= yScale;

  // Squeeze vertically just a little bit
  y = (y - 8192) * 1.4 + 9830;

  int16_t w = (int16_t)(sin16((x + xt) * 3) >> 2) + 8192;
  uint16_t v = constrain(16384 - (w > y ? w - y : y - w) * 2, 0, 16384);
  uint16_t h = constrain(16384 - (w > y ? w - y : y - w) / 2, 0, 16384) + ht;

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
    x = (x + 1) % WIDTH;
  }
  else if (r == 1) {
    x = x == 0 ? WIDTH - 1 : x - 1;
  }
  else if (r == 2) {
    y = constrain(y + 1, 0, HEIGHT - 1);
  }
  else if (r == 3) {
    y = constrain(y - 1, 0, HEIGHT - 1);
  }

  return mapXY(x, y);
}

uint16_t *wars = (uint16_t *)data;
static_assert(
  sizeof(typeof(*wars)) * NUM_WANDERERS <=
  sizeof(data), "out_of_memory");

void setupWander() {
  for (uint8_t i = 0; i < NUM_WANDERERS; i++) wars[i] = random(NUM_LEDS);
}

unsigned long beforeWander() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);

  uint8_t gHue = 0;
  uint8_t gHueDelta = 256 * 1 / NUM_WANDERERS;
  for (uint8_t i = 0; i < NUM_WANDERERS; i++) {
    wars[i] = wander(wars[i]);
    leds[wars[i]] = CHSV(gHue, 255, 255);
    gHue += gHueDelta;
  }

  return 25;
}

// PATTERN: SlideUp
unsigned long beforeSlideUp() {
  for (uint8_t x = 0; x < WIDTH; x++) {
    // slide up
    for (uint8_t y = 0; y < HEIGHT - 1; y++) {
      leds[mapXY(x, y)] = leds[mapXY(x, y + 1)];
    }
    // new ones enter at the bottom
    leds[mapXY(x, HEIGHT - 1)] = random(10) < 2 ? CHSV(random(256), 255, 255) : CHSV(0, 0, 0);
  }

  return 90;
}

// PATTERN: Bounce
int16_t cx = 0, cy = 0, cx2 = 0, cy2 = HEIGHT - 1;
int8_t dx = 1, dy = 1, dx2 = 1, dy2 = -1;
unsigned long beforeBounce() {
  if (++xt > 8) {
    xt = 0;

    leds[mapXY(cx, cy)] = CHSV(sin8(tt / 31), 255, 255);
    cx += dx;
    if (cx < 0) cx += WIDTH;
    if (cx >= WIDTH) cx -= WIDTH;
    if (random(100) < 2) dx = -dx;
    cy += dy;
    if (cy >= HEIGHT - 1 || cy <= 0) dy = -dy;

    leds[mapXY(cx2, cy2)] = CHSV(sin8(tt / 47) + 99, 255, 255);
    cx2 += dx2;
    if (cx2 < 0) cx2 += WIDTH;
    if (cx2 >= WIDTH) cx2 -= WIDTH;
    if (random(100) < 2) dx2 = -dx2;
    cy2 += dy2;
    if (cy2 >= HEIGHT - 1 || cy2 <= 0) dy2 = -dy2;
  }
  else {
    leds[mapXY(cx, cy)] = CRGB::Black;
    leds[mapXY(cx2, cy2)] = CRGB::Black;
  }

  return 0;
}

// PATTERN: Rain
int16_t *cols = (int16_t *)data;
uint8_t *speed = (uint8_t *)(cols + WIDTH);
uint8_t *color = (uint8_t *)(speed + WIDTH);
static_assert(
  sizeof(typeof(*cols)) * WIDTH +
  sizeof(typeof(*speed)) * WIDTH +
  sizeof(typeof(*color)) * WIDTH <=
  sizeof(data), "out_of_memory");

void setupRain() {
  for (uint8_t x = 0; x < WIDTH; x++) {
    cols[x] = -1;
  }
}

unsigned long beforeRain() {
  for (uint8_t x = 0; x < WIDTH; x++)
    if (cols[x] >= 0 && tt % speed[x] == 0)
      if (++cols[x] >= HEIGHT)
        cols[x] = -1;

  if (random(40) < 4) {
    uint8_t c = random(WIDTH);
    if (cols[c] == -1) {
      cols[c] = 0;
      speed[c] = 5 + random(7);
      color[c] = 130 + random(33);
    }
  }

  return 0;
}

void renderRain(uint16_t index, uint16_t x, int16_t y) {
  uint8_t h = color[x];
  uint8_t s = 255;
  uint8_t v = cols[x] == 0 ? 255 : 255 - 32 * (cols[x] - 1);
  leds[index] = CHSV(h, s, cols[x] == y ? v : 0);
}

// PATTERN: BlinkFade
uint8_t *bfVals = (uint8_t *)data;
uint8_t *bfHues = (uint8_t *)(bfVals + NUM_LEDS);
static_assert(
  sizeof(typeof(*bfVals)) * NUM_LEDS +
  sizeof(typeof(*bfHues)) * NUM_LEDS <=
  sizeof(data), "out_of_memory");

void setupBlinkFade() {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    bfVals[i] = 0;
  }
}

unsigned long beforeBlinkFade() {
  ht += 1;

  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    bfVals[i] = qsub8(bfVals[i], 1);
    if (bfVals[i] == 0) {
      bfVals[i] = random(256);
      bfHues[i] = ht / 2 + i / 5;
    }
  }

  return 0;
}

void renderBlinkFade(uint16_t index, uint16_t x, int16_t y) {
  uint8_t v = bfVals[index];
  leds[index] = CHSV(bfHues[index], 255, v);
}


// Pattern catalog
struct pattern {
  void (*setup)(void);
  unsigned long (*beforeRender)(void);
  void (*renderXY)(uint16_t index, uint16_t x, int16_t y);
};

pattern patterns[] = {
  { NULL,           beforeBounce,    NULL },
  { NULL,           beforeSinus,     renderSinus },
  { setupWander,    beforeWander,    NULL },
  { NULL,           beforeSlideUp,   NULL },
  { setupRain,      beforeRain,      renderRain },
  { setupBlinkFade, beforeBlinkFade, renderBlinkFade }
};

const size_t NUM_PATTERNS = sizeof patterns / sizeof *patterns;

// Driver program
size_t pi;
unsigned long startTime;
void setup() {
#if DEBUG_ENABLE
  Serial.begin(115200);
  Serial.print(F("NUM_PATTERNS = "));
  Serial.println(NUM_PATTERNS);
  Serial.flush();
#endif
  delay(1000);
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(30);
  startTime = millis();
  randomSeed(analogRead(0));
  pi = random(NUM_PATTERNS);
}

void loop() {
  if (tt == 0) {
    // Start new pattern
#if DEBUG_ENABLE
    Serial.print(F("Starting pattern "));
    Serial.println(pi);
    Serial.flush();
#endif
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    randomSeed(analogRead(0));
    if (*patterns[pi].setup) (*patterns[pi].setup)();
  }

  unsigned long delayms = 0;
  if (*patterns[pi].beforeRender) delayms = (*patterns[pi].beforeRender)();

  if (*patterns[pi].renderXY) {
    // call the renderXY function for each pixel
    uint16_t index = 0;
    for (uint8_t x = 0; x < WIDTH; x++) {
      for (uint8_t y = 0; y < HEIGHT; y++) {
        (*patterns[pi].renderXY)(index, x, mapY(x, y));
        index++;
      }
    }
  }

  FastLED.show();
  delay(delayms);
  tt += 1;

  unsigned long m = millis();
  if (m - startTime > SEQUENCE_TIME) {
#if DEBUG_ENABLE
    Serial.print(F("End pattern "));
    Serial.print(pi);
    Serial.print(F(": Time "));
    Serial.print(m);
    Serial.print(F(" - "));
    Serial.print(startTime);
    Serial.print(F(" ("));
    Serial.print(m - startTime);
    Serial.print(F(") > "));
    Serial.println(SEQUENCE_TIME);
    Serial.flush();
#endif
    if (++pi >= NUM_PATTERNS) pi = 0;
    startTime = m;
    tt = 0;
  }
}
