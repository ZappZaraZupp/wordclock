#include <Adafruit_NeoPixel.h>
#include <Time.h>
#include <DCF77.h>

//#define DEBUG ja
#undef DEBUG

#define PIN_MLED  6
#define PIN_ZLED  7
#define PIN_LDR   A0  // Analog
#define PIN_DCF   2
#define DCF_INTERRUPT 0  // Interrupt number associated with PIN_DCF

#define M_WIDTH 11
#define M_HEIGHT 10
#define Z_LEDS 5

#define MINBRIGHT 100.0
#define MAXBRIGHT 255.0


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

//ZLED
#define T_M1  0b10000000
#define T_M2  0b11000000
#define T_M3  0b11010000
#define T_M4  0b11011000
#define T_ST  0b00100000

Adafruit_NeoPixel m_led = Adafruit_NeoPixel(M_WIDTH * M_HEIGHT, PIN_MLED, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel z_led = Adafruit_NeoPixel(Z_LEDS, PIN_ZLED, NEO_GRB + NEO_KHZ800);

uint16_t line[10] = { 0,0,0,0,0,0,0,0,0,0 };

uint8_t curstd=0, oldstd=0;
uint8_t curmin=0, oldmin=0;
uint8_t cursec=0, oldsec=0;
uint8_t curzled=0, oldzled=0;
uint16_t curbright=0, oldbright=0;
uint32_t curstatus_color=0, oldstatus_color=0;

time_t time;
DCF77 DCF = DCF77(PIN_DCF,DCF_INTERRUPT);

void setup() {
  
  #ifdef DEBUG
  Serial.begin(9600);  //Begin serial communcation
  #endif

  m_led.begin();
  m_led.show(); // Initialize all pixels to 'off'
  z_led.begin();
  z_led.show(); // Initialize all pixels to 'off'

  colorWipe(&m_led,m_led.Color(255, 255, 255), 10); // white
  colorWipe(&m_led,m_led.Color(0, 0, 0), 10); // off 
  colorWipe(&z_led,z_led.Color(255, 255, 255), 50); // white
  colorWipe(&z_led,m_led.Color(0, 0, 0), 50); // off 

  curstatus_color=z_led.Color(255,0,0); // nur status LED auf rot
  curzled = T_ST;
  setZLED();
  z_led.show();


  setTime(20,07,0,18,8,2016);
  //setTime(hr,min,sec,day,month,yr);
/*  DCF.Start();
  setSyncInterval(30);
  setSyncProvider(DCF.getTime);

  // wait until the time is set by the sync provider      
  while(timeStatus() == timeNotSet) {  
    if(digitalRead(PIN_DCF) == 1) {
      curstatus_color = z_led.Color(255,0,255);
    }
    else {
      curstatus_color = z_led.Color(255,0,0);
    }
    if(curstatus_color != oldstatus_color) {
      oldstatus_color=curstatus_color;
      setZLED();
      z_led.show();
    }
  } 
  oldstatus_color=curstatus_color=z_led.Color(0,255,0);
  setZLED();
  z_led.show();*/
}

void loop() {

  int16_t diff=0;
  //Test
  delay(100);

  curbright = (int)(MINBRIGHT+(MAXBRIGHT-MINBRIGHT)*analogRead(PIN_LDR)/1023.0);
  diff = oldbright - curbright;
  if(diff<0) { diff = diff * -1; }
#ifdef DEBUG
Serial.print(oldbright);
Serial.print(" : ");
Serial.print(curbright);
Serial.print(" : ");
Serial.println(diff);
#endif

  time=now();
  curmin=minute(time);
  curstd=hourFormat12(time);
  cursec=second(time);
/*
  if(timeStatus() == timeNotSet) {  
    if(digitalRead(PIN_DCF) == 1) {
      curstatus_color = z_led.Color(255,0,255);
    }
    else {
      curstatus_color = z_led.Color(255,0,0);
    }
  } 
  else {
    if(digitalRead(PIN_DCF) == 1) {
      curstatus_color = z_led.Color(0,255,255);
    }
    else {
      curstatus_color = z_led.Color(0,255,0);
    }
  } 
*/
  
  curzled = (curzled & T_M4) | ((cursec % 2) * T_ST);
  
  if(curstd != oldstd || curmin != oldmin) {  // eigentlich nur alle %5 minuten; nicht neuzeichnen bei leichten helligkeitsschwankungen
    setText();
    setMin();
    setMatrixLED();
    m_led.setBrightness(curbright);
    oldstd=curstd;
    oldmin=curmin;
    m_led.show();
  } else if (diff > 10) {
#ifdef DEBUG
Serial.print(oldbright);
Serial.print(" m ");
Serial.print(curbright);
Serial.print(" m ");
Serial.println(diff);
#endif
    m_led.setBrightness(curbright);
    m_led.show();
  }
  if(curzled != oldzled || curstatus_color != oldstatus_color) {
    setZLED();
    z_led.setBrightness(curbright);
    oldzled=curzled;
    oldbright=curbright;
    oldstatus_color=curstatus_color;
    z_led.show();
  } else if (diff > 10) {
#ifdef DEBUG
Serial.print(oldbright);
Serial.print(" z ");
Serial.print(curbright);
Serial.print(" z ");
Serial.println(diff);
#endif
    oldbright=curbright;
    z_led.setBrightness(curbright);
    z_led.show();
  }

}

// Get number of LED in matrix
uint16_t xy( uint8_t x, uint8_t y)
{
  uint16_t i;
  i = (y * M_WIDTH) + x;
  
  return i;
}

void setMatrixLED() {
  uint32_t c;
  uint16_t x;

  for(int i=0; i<=9; i++) { // zeile
    for(int j=0; j<=15; j++) { // spalte
      x=bitRead(line[i],15-j);
      c=ctimeOfDay(&m_led);
      //c=m_led.Color(255, 255, 255); // ersetzen mit funktion für farbmodus
      if(x == 1) {
        m_led.setPixelColor(xy(j,i),c);
      }
      else {
        m_led.setPixelColor(xy(j,i),0);
      }
    }
    line[i]=0; // reset der zeile
  }
}

void setText() {

  T_ES;
  T_IST0;

  if(curmin < 5) {
    if((int)random(2)==0) {
      // Es is xx Uhr
      setHourText(curstd == 1?100:curstd); // Ein Uhr
      T_UHR;
    }
    else {
      // Es ist um xx
      T_UM;
      setHourText(curstd);
    }
  }
  else if(curmin < 10) {
    //00:05	Es ist fünf[0] nach xx
    T_FUNF0;
    T_NACH3;
    setHourText(curstd);
  }
  else if(curmin < 15) {
    //00:10	Es ist zehn[1] nach xx
    T_ZEHN1;
    T_NACH3;
    setHourText(curstd);
  }
  else if(curmin < 20) {
    //00:15	Es ist viertel nach xx
    //		Es ist viertel xx+1
    if((int)random(2)==0) {
      T_VIERTEL;
      T_NACH3;
      setHourText(curstd);
    }
    else {
      T_VIERTEL;
      setHourText(curstd+1);
    }
  }
  else if(curmin < 25) {
    //00:20	Es ist zwanzig nach xx
    T_ZWANZIG;
    T_NACH3;
    setHourText(curstd);
  }
  else if(curmin < 30) {
    //00:25	Es ist fünf[0] vor halb xx+1
    T_FUNF0;
    T_VOR;
    T_HALB;
    setHourText(curstd+1);
  }
  else if(curmin < 35) {
    //00:30	Es ist halb xx+1
    T_HALB;
    setHourText(curstd+1);
  }
  else if(curmin < 40) {
    //00:35	Es ist fünf[0] nach halb xx+1
    T_FUNF0;
    T_NACH3;
    T_HALB;
    setHourText(curstd+1);
  }
  else if(curmin < 45) {
    //00:40	Es ist zwanzig vor xx+1
    T_ZWANZIG;
    T_VOR;
    setHourText(curstd+1);
  }
  else if(curmin < 50) {
    //00:45	Es ist viertel vor xx+1
    //		Es ist dreiviertel xx+1
    if((int)random(2)==0) {
      T_VIERTEL;
      T_VOR;
      setHourText(curstd+1);
    }
    else {
      T_DREIVIERTEL;
      setHourText(curstd+1);
    }
  }
  else if(curmin < 55) {
    //00:50	Es ist zehn[1] vor xx+1
    T_ZEHN1;
    T_VOR;
    setHourText(curstd+1);
  }
  else {
    //00:55	Es ist fünf[0] vor xx+1
    T_FUNF0;
    T_VOR;
    setHourText(curstd+1);
  }
}

void setHourText(uint8_t h) {
  switch(h) {
    case 0:
    case 12:
      T_ZWOLF;
      break;
    case 1:
      T_EINS;
      break;
    case 100:
      T_EIN;
      break;
    case 2:
      T_ZWEI;
      break;
    case 3:
      T_DREI6;
      break;
    case 4:
      T_VIER7;
      break;
    case 5:
      T_FUNF4;
      break;
    case 6:
      T_SECHS;
      break;
    case 7:
      T_SIEBEN;
      break;
    case 8:
      T_ACHT;
      break;
    case 9:
      T_NEUN;
      break;
    case 10:
      T_ZEHN9;
      break;
    case 11:
      T_ELF;
      break;
  }
}

void setZLED() {
  uint8_t x=0;
  uint32_t c=0;
  for(int i=0;i<5;i++) {
    x=bitRead(curzled,7-i);
    if(i==2) {
      c=curstatus_color;
    }
    else {
      c=ctimeOfDay(&z_led);
      //c=z_led.Color(255, 255, 255); // ersetzen mit funktion für farbmodus
    }
    if(x == 1) {
      z_led.setPixelColor(i,c);
    }
    else {
      z_led.setPixelColor(i,0);
    }
  }
}

void setMin() {
  curzled=curzled & T_ST; //status behalten, rest zurücksetzen
  switch(curmin % 5) {
    case 1:
      curzled |= T_M1;
      break;
    case 2:
      curzled |= T_M2;
      break;
    case 3:
      curzled |= T_M3;
      break;
    case 4:
      curzled |= T_M4;
      break;
  }
}

// Fill the dots one after the other with a color
void colorWipe(Adafruit_NeoPixel *strip,uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip->numPixels(); i++) {
    strip->setPixelColor(i, c);
    strip->show();
    delay(wait);
  }
}

// ein tag hat 288 5min intervalle
// 96 + 96 + 96
// 0,255,0 --> 0,0,255  schrittweite 2,65625
// 0,0,255 --> 255,0,0
// 255,0,0 --> 0,255,0
uint32_t ctimeOfDay(Adafruit_NeoPixel *strip) {

  int iOfDay=hour()*60+minute()/5;
  float s=2.65625;
  
  if(iOfDay < 96) {
    return strip->Color(255 - iOfDay * s, 0, iOfDay * s);
  }
  if(iOfDay < 2*96) {
    iOfDay -= 96;
    return strip->Color(0, iOfDay * s, 255 - iOfDay * s);
  }
  iOfDay -= 2*96;
  return strip->Color(iOfDay * s, 255 - iOfDay * s, 0);
}


