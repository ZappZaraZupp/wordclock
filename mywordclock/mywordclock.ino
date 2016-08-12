#include <FastLED.h>

#define LED_MPIN  6
#define LED_ZPIN  7
#define COLOR_ORDER GRB
#define CHIPSET     WS2811
#define BRIGHTNESS 64

const uint8_t width = 11;
const uint8_t height = 10;
const uint8_t zleds = 5;

CRGB m-leds[width*height];
CRGB z-leds[zled];

// Get number of LED 
uint16_t XY( uint8_t x, uint8_t y)
{
  uint16_t i;

  //stay in range
  y %= height;
  x %= width;
  i = (y * width) + x;
  
  return i;
}


