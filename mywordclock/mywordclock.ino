#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PIN_MLED  6
#define PIN_ZLED  7

const uint8_t width = 11;
const uint8_t height = 10;
const uint8_t zleds = 5;

Adafruit_NeoPixel m_led = Adafruit_NeoPixel(width*height, PIN_MLED, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel z_led = Adafruit_NeoPixel(zleds, PIN_ZLED, NEO_GRB + NEO_KHZ800);

void setup() {
  m_led.begin();
  m_led.show(); // Initialize all pixels to 'off'
  z_led.begin();
  z_led.show(); // Initialize all pixels to 'off'
}

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

void loop() {
  colorWipe(&m_led,m_led.Color(255, 0, 0), 5); // Red
  colorWipe(&m_led,m_led.Color(0, 255, 0), 5); // Green
  colorWipe(&m_led,m_led.Color(0, 0, 255), 5); // Blue
  colorWipe(&m_led,m_led.Color(255, 255, 255), 5); // white
  colorWipe(&z_led,z_led.Color(255, 0, 0), 50); // Red
  colorWipe(&z_led,z_led.Color(0, 255, 0), 50); // Green
  colorWipe(&z_led,z_led.Color(0, 0, 255), 50); // Blue
  colorWipe(&z_led,z_led.Color(255, 255, 255), 50); // white
}

// Fill the dots one after the other with a color
void colorWipe(Adafruit_NeoPixel *strip,uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip->numPixels(); i++) {
    strip->setPixelColor(i, c);
    strip->show();
    delay(wait);
  }
}


