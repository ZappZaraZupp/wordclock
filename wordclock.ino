/**
 * Wordclock (German)
 * Author: Christian Pohl
 * https://github.com/ZappZaraZupp
 * Version 0.1 2016-10-06
 *
 * Libraries used:
 * ESP8266 Firmware:
 * https://github.com/itead/ITEADLIB_Arduino_WeeESP8266
 * NTP Firmware
 * AT+CIPNTP=<offset from GMT>
 * AT+CIPNTP? Current Time
 *
 * Timezone Library:
 * https://github.com/JChristensen/Timezone
 * 
 * Time Library:
 * https://github.com/PaulStoffregen/Time
 *
 * LED:
 * https://github.com/adafruit/Adafruit_NeoPixel
 *
 */

#define LOG(arg) Serial.print("[");Serial.print((const char*)__FUNCTION__);Serial.print("] ");Serial.print(arg);Serial.print("\r\n");
#define DEBUGLOG(arg) Serial.print("DEBUG [");Serial.print((const char*)__FUNCTION__);Serial.print("] ");Serial.print(arg);Serial.print("\r\n");

#include <SoftwareSerial.h>
// !! Changed: Buffer to 256 !!
#include <Time.h>
#include <TimeLib.h>
#include <Timezone.h>
#include <Adafruit_NeoPixel.h>
#include "ESP8266mini.h"
// WLAN Credentials
// #define SSID        "your SSID"
// #define PASSWORD    "your Password"
#include "WLAN_cred.h"

// ESP8266 Connection settings
#define RXPIN       11
#define TXPIN       12
#define ESPBAUD     19200

// PINs
#define PIN_MLED  6
#define PIN_ZLED  7 
#define PIN_LDR   A0  // Analog
#define PIN_DSIP  8   // Displaymode
#define PIN_COLM  9   // Colormode

// einige Konstanten
#define M_WIDTH 11
#define M_HEIGHT 10
#define Z_LEDS 5
#define MINBRIGHT 50.0
#define MAXBRIGHT 255.0

//ZLED
#define T_M1  0b10000000
#define T_M2  0b11000000
#define T_M3  0b11010000
#define T_M4  0b11011000
#define T_ST  0b00100000

// texte
#define T_ES          matrix_line[0] |= 0b1100000000000000
#define T_IST0        matrix_line[0] |= 0b0001110000000000
#define T_FUNF0       matrix_line[0] |= 0b0000000111100000
#define T_ZEHN1       matrix_line[1] |= 0b1111000000000000
#define T_ZWANZIG     matrix_line[1] |= 0b0000111111100000
#define T_DREI2       matrix_line[2] |= 0b1111000000000000
#define T_DREIVIERTEL matrix_line[2] |= 0b1111111111100000
#define T_VIER2       matrix_line[2] |= 0b0000111100000000
#define T_VIERTEL     matrix_line[2] |= 0b0000111111100000
#define T_VOR         matrix_line[3] |= 0b1110000000000000
#define T_UM          matrix_line[3] |= 0b0000110000000000
#define T_NACH3       matrix_line[3] |= 0b0000000111100000
#define T_HALB        matrix_line[4] |= 0b1111000000000000
#define T_ELF         matrix_line[4] |= 0b0000011100000000
#define T_FUNF4       matrix_line[4] |= 0b0000000111100000
#define T_EIN         matrix_line[5] |= 0b1110000000000000
#define T_EINS        matrix_line[5] |= 0b1111000000000000
#define T_TAG         matrix_line[5] |= 0b0000111000000000
#define T_ZWEI        matrix_line[5] |= 0b0000000111100000
#define T_DREI6       matrix_line[6] |= 0b1111000000000000
#define T_IST6        matrix_line[6] |= 0b0001110000000000
#define T_ZWOLF       matrix_line[6] |= 0b0000001111100000
#define T_SIEBEN      matrix_line[7] |= 0b1111110000000000
#define T_VIER7       matrix_line[7] |= 0b0000000111100000
#define T_SECHS       matrix_line[8] |= 0b1111100000000000
#define T_NACH8       matrix_line[8] |= 0b0000001111100000
#define T_NACHT       matrix_line[8] |= 0b0000001111100000
#define T_ACHT        matrix_line[8] |= 0b0000000111100000
#define T_ZEHN9       matrix_line[9] |= 0b1111000000000000
#define T_NEUN        matrix_line[9] |= 0b0001111000000000
#define T_UHR         matrix_line[9] |= 0b0000000011100000

////////////////////////////////////////////////////////////////////////////////////////////
// Globals
// the ESP8266
SoftwareSerial esp8266(RXPIN, TXPIN);
ESP8266mini wifi(esp8266,ESPBAUD);
// Timezone
TimeChangeRule t_dst = {
  "MESZ", Last, Sun, Mar, 2, +120}; // Sommer +2h
TimeChangeRule t_st = {
  "MEZ", Last, Sun, Oct, 3, +60}; // Winter +1h
Timezone t_tz(t_dst,t_st);

Adafruit_NeoPixel m_led = Adafruit_NeoPixel(M_WIDTH * M_HEIGHT, PIN_MLED, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel z_led = Adafruit_NeoPixel(Z_LEDS, PIN_ZLED, NEO_GRB + NEO_KHZ800);

uint8_t colormode=1;
time_t curtime;
time_t oldtime;

uint8_t curbright=0; // Brightness for LED
uint8_t curzled=0;   // Minutes in 5 min interval and status LED
uint32_t stcolor=0;  // color statusled

uint16_t matrix_line[10] = { // array for Matrix, bit=0: LED off, bit=1 LED on
  0,0,0,0,0,0,0,0,0,0 };

uint8_t f_colm=0;  // flag, if key was pressed in last loop
uint8_t f_disp=0;  // flag, if key was pressed in last loop

////////////////////////////////////////////////////////////////////////////////////////////
// Setup
void setup(void)
{
  Serial.begin(ESPBAUD);

  LOG("start");

  String sdummy="";

  randomSeed(analogRead(PIN_LDR));

  LOG("Init LEDs");
  m_led.begin();
  m_led.show(); // Initialize all pixels to 'off'
  z_led.begin();
  z_led.show(); // Initialize all pixels to 'off'

  //Test
  colorWipe(&m_led,m_led.Color(255, 255, 255), 10); // white
  colorWheel(&m_led, 10); 
  colorWipe(&z_led,z_led.Color(255, 255, 255), 50); // white
  colorWheel(&z_led, 50); 

  curzled = T_ST;
  stcolor = z_led.Color(255,0,0); //Status rot
  setZLED();
  z_led.show();

  setMLED();
  m_led.show();

  LOG("Init Wifi");
  esp8266.begin(ESPBAUD);

  if(wifi.sendAT("RST",sdummy)<0) {
    LOG("RST Error\r\n");
    LOG(sdummy.c_str());
    m_led.setPixelColor(0,m_led.Color(255,0,0));
  }
  else {
    m_led.setPixelColor(0,m_led.Color(0,255,0));
    LOG("RST ok");
  }
  m_led.show();
  delay(5000); //RST needs some time

  if(wifi.sendAT("CWMODE=1",sdummy)<0) {
    LOG("CWMODE=1 Error\r\n");
    LOG(sdummy.c_str());
    m_led.setPixelColor(1,m_led.Color(255,0,0));
  }
  else {
    m_led.setPixelColor(1,m_led.Color(0,255,0));
    LOG("CWMODE ok");
  }
  m_led.show();

  if(wifi.sendAT("CWJAP=\""SSID"\",\""PASSWORD"\"",sdummy)<0) {
    LOG("CWJAP=... Error\r\n");
    LOG(sdummy.c_str());
    m_led.setPixelColor(2,m_led.Color(255,0,0));
  }
  else {
    m_led.setPixelColor(2,m_led.Color(0,255,0));
    LOG("CWJAP OK");
  }
  m_led.show();

  if(wifi.sendAT("CIPSTATUS",sdummy)<0) {
    LOG("CIPSTATUS Error\r\n");
    LOG(sdummy.c_str());
    m_led.setPixelColor(3,m_led.Color(255,0,0));
    m_led.show();
  }
  else {
    m_led.setPixelColor(3,m_led.Color(0,255,0));
    m_led.show();
    if(sdummy.indexOf("STATUS:5") != -1) {
      LOG("Connected\r\n");
      m_led.setPixelColor(4,m_led.Color(0,255,0));
      m_led.show();
    }
    else {
      LOG("NOT Connected\r\n");
      LOG(sdummy.c_str());
      m_led.setPixelColor(4,m_led.Color(255,0,0));
      m_led.show();
    }
  }

  if(wifi.sendAT("CIPNTP=0",sdummy)<0) {  // CIPNTP=0 returns no standard value with tailing 'OK' sendAT returns -1 because of that.
    LOG(sdummy.c_str());
    m_led.setPixelColor(5,m_led.Color(255,255,0));
    m_led.show();
  }

  if(wifi.getTime(&t_tz)<0) {
    LOG("getTime Error\r\n");
  }
  else {
    LOG((String("time set: ")+String(now())).c_str());
    stcolor = z_led.Color(255,255,255); //Status weiss
    setZLED();
    z_led.show();
  }

  curtime=now();
  setText();
}

////////////////////////////////////////////////////////////////////////////////////////////
// Loop
void loop(void)
{

  uint8_t diff=0;

  if (esp8266.available())
    Serial.write(esp8266.read());
  if (Serial.available())
    esp8266.write(Serial.read());


  // color mode
  // only switch, if key is pressed for more than 10 loops
  if(digitalRead(PIN_COLM) == 1) {
    if(f_colm == 10) {
      colormode=(colormode+1)%3;  // currently 3 color modes
      LOG(String(("Switch color mode to ")+String(colormode)).c_str());
    }
    if(f_colm <= 10) { 
      f_colm += 1;
    }
  }
  else {
    if(f_colm > 0) {
      f_colm -= 1;
    }
  }

  // Read brightness
  curbright = (int)(MINBRIGHT+(MAXBRIGHT-MINBRIGHT)*analogRead(PIN_LDR)/1023.0);

  curtime=now(); // store current time

  // Minute has changed
  if(minute(curtime) != minute(oldtime)) {
    curzled=curzled & T_ST; // keep status LED, reset minute LED
    switch(minute(curtime) % 5) {
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
    stcolor=z_led.Color(255,255,0);
    if(wifi.getTime(&t_tz)<0) {  // call ESP8266 for time
      Serial.print("getTime Error\r\n");
      stcolor=z_led.Color(255,0,0);
    }
    else {
      LOG((String("time set: ")+String(now())).c_str());
      stcolor=z_led.Color(255,255,255);
    }

    if(minute(curtime) %5 == 0) {  // 5 minutes intrerval --> new text
      setText();
    }
  }

  // refresh every second
  if(second(curtime) != second(oldtime)) {
    // blink statusLED
    curzled = (curzled & T_M4) | ((second(curtime) % 2) * T_ST);  // keep minutes, set statusLED

    oldtime=curtime;
    setMLED();
    m_led.setBrightness(curbright);
    m_led.show();
    setZLED();
    z_led.setBrightness(curbright);
    z_led.show();

  }
}


/////////////////////////////////////
// get LED number from x,y
uint16_t xy( uint8_t x, uint8_t y)
{
  uint16_t i;
  i = (y * M_WIDTH) + x;

  return i;
}

/////////////////////////////////////
// set LED from array 'line[]'
void setMLED() {
  uint32_t c;
  uint16_t x;

  for(int i=0; i<=9; i++) { // row
    for(int j=0; j<=15; j++) { // column
      x=bitRead(matrix_line[i],15-j);
      c=mcolor(j,i);
      if(x == 1) {
        m_led.setPixelColor(xy(j,i),c);
      }
      else {
        m_led.setPixelColor(xy(j,i),0);
      }
    }
  }
}

/////////////////////////////////////
// set text
// if more writings are possible, random
void setText() {
  //matrix-lines reset
  for(uint8_t i=0; i<10; i++) {
    matrix_line[i]=0;
  }

  T_ES;
  T_IST0;


  if(minute(curtime) < 5) {
    if((int)random(2)==0) {
      // Es is xx Uhr
      setHourText(hourFormat12(curtime) == 1 ? 100 : hourFormat12(curtime) ); // "Es ist Ein Uhr" (nicht: "Es ist Eins Uhr" ;-)
      T_UHR;
    }
    else {
      // Es ist um xx
      T_UM;
      setHourText(hourFormat12(curtime));
    }
  }
  else if(minute(curtime) < 10) {
    //00:05	Es ist fünf[0] nach xx
    T_FUNF0;
    T_NACH3;
    setHourText(hourFormat12(curtime));
  }
  else if(minute(curtime) < 15) {
    //00:10	Es ist zehn[1] nach xx
    T_ZEHN1;
    T_NACH3;
    setHourText(hourFormat12(curtime));
  }
  else if(minute(curtime) < 20) {
    //00:15	Es ist viertel nach xx
    //		Es ist viertel xx+1
    if((int)random(2)==0) {
      T_VIERTEL;
      T_NACH3;
      setHourText(hourFormat12(curtime));
    }
    else {
      T_VIERTEL;
      setHourText(hourFormat12(curtime)+1);
    }
  }
  else if(minute(curtime) < 25) {
    //00:20	Es ist zwanzig nach xx
    T_ZWANZIG;
    T_NACH3;
    setHourText(hourFormat12(curtime));
  }
  else if(minute(curtime) < 30) {
    //00:25	Es ist fünf[0] vor halb xx+1
    T_FUNF0;
    T_VOR;
    T_HALB;
    setHourText(hourFormat12(curtime)+1);
  }
  else if(minute(curtime) < 35) {
    //00:30	Es ist halb xx+1
    T_HALB;
    setHourText(hourFormat12(curtime)+1);
  }
  else if(minute(curtime) < 40) {
    //00:35	Es ist fünf[0] nach halb xx+1
    T_FUNF0;
    T_NACH3;
    T_HALB;
    setHourText(hourFormat12(curtime)+1);
  }
  else if(minute(curtime) < 45) {
    //00:40	Es ist zwanzig vor xx+1
    T_ZWANZIG;
    T_VOR;
    setHourText(hourFormat12(curtime)+1);
  }
  else if(minute(curtime) < 50) {
    //00:45	Es ist viertel vor xx+1
    //		Es ist dreiviertel xx+1
    if((int)random(2)==0) {
      T_VIERTEL;
      T_VOR;
      setHourText(hourFormat12(curtime)+1);
    }
    else {
      T_DREIVIERTEL;
      setHourText(hourFormat12(curtime)+1);
    }
  }
  else if(minute(curtime) < 55) {
    //00:50	Es ist zehn[1] vor xx+1
    T_ZEHN1;
    T_VOR;
    setHourText(hourFormat12(curtime)+1);
  }
  else {
    //00:55	Es ist fünf[0] vor xx+1
    T_FUNF0;
    T_VOR;
    setHourText(hourFormat12(curtime)+1);
  }
}

/////////////////////////////////////
// hours text
void setHourText(uint8_t h) {
  if(h<100) { // >=100 special cases
    h %= 12;
  }
  switch(h) {
  case 0:
  case 12:
    T_ZWOLF;
    break;
  case 1:
    T_EINS;
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
    // specail cases
  case 100:
    T_EIN;
    break;
  }
}



/////////////////////////////////////
// set zLED 
void setZLED() {
  uint8_t x=0;
  uint32_t c=0;
  for(int i=0;i<5;i++) {
    x=bitRead(curzled,7-i);
    if(i==2) {
      c=stcolor;
    }
    else {
      c=zcolor(i>2?i-1:i); // minutes are index 0,1,3,4
    }
    if(x == 1) {
      z_led.setPixelColor(i,c);
    }
    else {
      z_led.setPixelColor(i,0);
    }
  }
}

/////////////////////////////////////
// colorfunctions
uint32_t mcolor(uint8_t x, uint8_t y) { // matrix
  switch(colormode) {
  case 0:
  default:
    return m_led.Color(255,255,255);
  case 1:
    return colorwheel(&m_led,110,(xy(x,y)+(int)(minute(curtime)/60.0*110.0))%110); // go through cycle ones every hour 
  case 2:
    int iOfDay=(int)(hour(curtime)*60.0+minute(curtime));
    return colorwheel(&m_led,1440,iOfDay); // go through cycle onece every day
  }
}

uint32_t zcolor(uint8_t i) { // minutes
  switch(colormode) {
  case 0:
  default:
    return z_led.Color(255,255,255);
  case 1:
    return colorwheel(&z_led,110,((int)(minute(curtime)/60.0*110.0))%110); // same color as 1st led of matrix 
  case 2:
    int iOfDay=(int)(hour(curtime)*60.0+minute(curtime));
    return colorwheel(&z_led,1440,iOfDay);
  }
}

// colorwheel
// 255,0,0 --> 0,255,0
// 0,255,0 --> 0,0,255
// 0,0,255 --> 255,0,0
uint32_t colorwheel(Adafruit_NeoPixel *strip, uint8_t wheelsteps, uint8_t curstep) {

  float p=wheelsteps/3.0;
  float s=255.0/p; // stepsize

  // 255,0,0 --> 0,255,0
  if(curstep < p) {
    return strip->Color(255-curstep*s, curstep*s, 0);
  }
  // 0,255,0 --> 0,0,255
  if(curstep < 2*p) {
    curstep -= p;
    return strip->Color(0, 255-curstep*s, curstep*s);
  }
  // 0,0,255 --> 255,0,0
  curstep -= 2*p;
  return strip->Color(curstep*s,0, 255-curstep*s);
}

/////////////////////////////////////
// Test
// Fill the dots one after the other with a color
void colorWipe(Adafruit_NeoPixel *strip,uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip->numPixels(); i++) {
    strip->setPixelColor(i, c);
    strip->show();
    delay(wait);
  }
}

void colorWheel(Adafruit_NeoPixel *strip, uint8_t wait) {
  for(uint16_t i=0; i<strip->numPixels(); i++) {
    strip->setPixelColor(i, colorwheel(strip,strip->numPixels(),i));
    strip->show();
    delay(wait);
  }
}



