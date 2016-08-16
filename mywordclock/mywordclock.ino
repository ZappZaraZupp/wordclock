#include <Adafruit_NeoPixel.h>

#define PIN_MLED  6
#define PIN_ZLED  7

// texte
#define T_ES          line[0] |= 0b1100000000000000
#define T_IST0        line[0] |= 0b0001110000000000
#define T_FUNF0       line[0] |= 0b0000000111100000
#define T_ZEHN1       line[1] |= 0b1111000000000000
#define T_ZWANZIG     line[1] |= 0b0000111111100000
#define T_DREI2       line[2] |= 0b1111000000000000
#define T_DREIVIERTEL line[2] |= 0b1111111111100000
#define T_VIER2       line[2] |= 0b0000111100000000
#define T_VIERTEL     line[2] |= 0b0000111111100000
#define T_VOR         line[3] |= 0b1110000000000000
#define T_UM          line[3] |= 0b0000110000000000
#define T_NACH3       line[3] |= 0b0000000111100000
#define T_HALB        line[4] |= 0b1111000000000000
#define T_ELF         line[4] |= 0b0000011100000000
#define T_FUNF4       line[4] |= 0b0000000111100000
#define T_EIN         line[5] |= 0b1110000000000000
#define T_EINS        line[5] |= 0b1111000000000000
#define T_TAG         line[5] |= 0b0000111000000000
#define T_ZWEI        line[5] |= 0b0000000111100000
#define T_DREI6       line[6] |= 0b1111000000000000
#define T_IST6        line[6] |= 0b0001110000000000
#define T_ZWOLF       line[6] |= 0b0000001111100000
#define T_SIEBEN      line[7] |= 0b1111110000000000
#define T_VIER7       line[7] |= 0b0000000111100000
#define T_SECHS       line[8] |= 0b1111100000000000
#define T_NACH8       line[8] |= 0b0000001111100000
#define T_NACHT       line[8] |= 0b0000001111100000
#define T_ACHT        line[8] |= 0b0000000111100000
#define T_ZEHN9       line[9] |= 0b1111000000000000
#define T_NEUN        line[9] |= 0b0001111000000000
#define T_UHR         line[9] |= 0b0000000011100000

/*
kann man sicher noch mehr kombinationen machen (todo)

00:00	Es ist xx Uhr
	Es ist um xx
	** Es is ein Uhr
	** Es ist um eins
	** fünf[4]
	** zehn[9]
	** drei[6]
	** vier[7]
00:05	Es ist fünf[0] nach xx
00:10	Es ist zehn[1] nach xx
00:15	Es ist viertel nach xx
	Es ist viertel xx+1
00:20	Es ist zwanzig nach xx
00:25	Es ist fünf[0] vor halb xx+1
00:30	Es ist halb xx+1
00:35	Es ist fünf[0] nach halb xx+1
00:40	Es ist zwanzig vor xx+1
00:45	Es ist viertel vor xx+1
	Es ist dreiviertel xx+1
00:50	Es ist zehn[1] vor xx+1
00:55	Es ist fünf[0] vor xx+1
*/

#define M_WIDTH 11;
#define M_HEIGHT 10;
#define Z_LEDS 5;

Adafruit_NeoPixel m_led = Adafruit_NeoPixel(M_WIDTH * M_HEIGHT, PIN_MLED, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel z_led = Adafruit_NeoPixel(Z_LEDS, PIN_ZLED, NEO_GRB + NEO_KHZ800);

line[10] = { 0,0,0,0,0,0,0,0,0,0 };

void setup() {
  m_led.begin();
  m_led.show(); // Initialize all pixels to 'off'
  z_led.begin();
  z_led.show(); // Initialize all pixels to 'off'
}

// Get number of LED 
uint16_t xy( uint8_t x, uint8_t y)
{
  uint16_t i;

  //stay in range
  y %= M_HEIGHT;;
  x %= M_WIDTH;
  i = (y * M_WIDTH) + x;
  
  return i;
}

void setMatrix() {
  uint32_t c;
  uint16_t x;

  for(int i=0; i<=9; i++) { // zeile
    for(int j=0; j<=15; j++) { // spalte
      x=bitRead(line[i],15-j);
      c=m_led.Color(255, 255, 255); // ersetzen mit funktion für farbmodus
      if(x == 1) {
        m_led.setPixelColor(xy(i,j),c);
      }
      else {
        m_led.setPixelColor(xy(i,j),0);
      }
    }
    line[i]=0; // reset der zeile
  }
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

