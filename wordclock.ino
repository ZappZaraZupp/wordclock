/**
   Wordclock (German)
   Author: Christian Pohl
   https://github.com/ZappZaraZupp
   Version 0.1 2016-10-06

   Libraries used:
   ESP8266 Firmware:
   https://github.com/itead/ITEADLIB_Arduino_WeeESP8266
   NTP Firmware
   AT+CIPNTP=<offset from GMT>
   AT+CIPNTP? Current Time

   Timezone Library:
   https://github.com/JChristensen/Timezone

   Time Library:
   https://github.com/PaulStoffregen/Time

   LED:
   https://github.com/adafruit/Adafruit_NeoPixel

*/
//#define LOG(arg) Serial.print("[");Serial.print((const char*)__FUNCTION__);Serial.print("] ");Serial.print(arg);Serial.print("\r\n");
#define LOG(arg) ;
//#define DLOG(arg) Serial.print("VERBOSE [");Serial.print((const char*)__FUNCTION__);Serial.print("] ");Serial.print(arg);
#define DLOG(arg) ;

#define _SS_MAX_RX_BUFF 256 //increase buffer in softwareSerial
#include <SoftwareSerial.h>
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
#define PIN_MLED  7
#define PIN_ZLED  6
#define PIN_LDR   A0  // Analog
#define PIN_DSIP   8   // Displaymode
#define PIN_COL  9   // Colormode
#define PIN_MISC  10   // ???mode

// einige Konstanten
#define M_WIDTH 11
#define M_HEIGHT 10
#define Z_LEDS 5
#define MINBRIGHT 10.0
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
ESP8266mini wifi(esp8266, ESPBAUD);
// Timezone
TimeChangeRule t_dst = {
  "MESZ", Last, Sun, Mar, 2, +120
}; // Sommer +2h
TimeChangeRule t_st = {
  "MEZ", Last, Sun, Oct, 3, +60
}; // Winter +1h
Timezone t_tz(t_dst, t_st);

Adafruit_NeoPixel m_led = Adafruit_NeoPixel(M_WIDTH * M_HEIGHT, PIN_MLED, NEO_GRBW + NEO_KHZ800);
Adafruit_NeoPixel z_led = Adafruit_NeoPixel(Z_LEDS, PIN_ZLED, NEO_GRBW + NEO_KHZ800);

uint8_t colormode = 1;
uint8_t animode = 1;
time_t curtime;
time_t oldtime;
uint8_t f_timesync = 0;

uint8_t curbright = 0; // Brightness for LED
uint8_t curzled = 0; // Minutes in 5 min interval and status LED
uint32_t stcolor = 0; // color statusled

uint16_t matrix_line[10] = { // array for Matrix, bit=0: LED off, bit=1 LED on
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
uint16_t cur_matrix_line[10] = { // array for current Matrix, bit=0: LED off, bit=1 LED on
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

uint8_t doAni = 0; // do animation in next refresh

uint8_t lastState[3] = {0, 0, 0}; // for switches
unsigned long lastTime[3] = {0, 0, 0};
uint8_t curState[3] = {0, 0, 0};
unsigned long debounce = 50;


////////////////////////////////////////////////////////////////////////////////////////////
// Setup
void setup(void)
{
  Serial.begin(ESPBAUD);

  LOG("start");
  LOG(__DATE__);
  LOG(__TIME__);

  String sdummy = "";

  randomSeed(analogRead(PIN_LDR));

  LOG("Init LEDs");
  m_led.begin();
  m_led.setBrightness(255);
  m_led.show(); // Initialize all pixels to 'off'
  z_led.begin();
  z_led.setBrightness(255);
  z_led.show(); // Initialize all pixels to 'off'

  //Test
  colorWipe(&m_led, m_led.Color(255, 0, 0, 0), 1);
  colorWipe(&m_led, m_led.Color(0, 255, 0, 0), 1);
  colorWipe(&m_led, m_led.Color(0, 0, 255, 0), 1);
  colorWipe(&m_led, m_led.Color(0, 0, 0, 255), 1);
  colorWipe(&z_led, z_led.Color(255, 0, 0, 0), 50);
  colorWipe(&z_led, z_led.Color(0, 255, 0, 0), 50);
  colorWipe(&z_led, z_led.Color(0, 0, 255, 0), 50);
  colorWipe(&z_led, z_led.Color(0, 0, 0, 255), 50);
  colorWheel(&m_led, 1);
  colorWheel(&z_led, 50);

  curzled = T_ST;
  stcolor = z_led.Color(255, 0, 0, 0); //Status rot
  setZLED();
  z_led.show();

  setMLED();
  m_led.show();

  LOG("Init Wifi");
  esp8266.begin(ESPBAUD);

  if (wifi.sendAT("RST", sdummy) < 0) {
    LOG("RST Error");
    LOG(sdummy.c_str());
    m_led.setPixelColor(0, m_led.Color(255, 0, 0, 0));
  }
  else {
    m_led.setPixelColor(0, m_led.Color(0, 255, 0, 0));
    LOG("RST ok");
  }
  m_led.show();
  delay(5000); //RST needs some time

  if (wifi.sendAT("CWMODE=1", sdummy) < 0) {
    LOG("CWMODE=1 Error");
    LOG(sdummy.c_str());
    m_led.setPixelColor(1, m_led.Color(255, 0, 0, 0));
  }
  else {
    m_led.setPixelColor(1, m_led.Color(0, 255, 0, 0));
    LOG("CWMODE ok");
  }
  m_led.show();

  if (wifi.sendAT("CWJAP=\""SSID"\",\""PASSWORD"\"", sdummy) < 0) {
    LOG("CWJAP=... Error");
    LOG(sdummy.c_str());
    m_led.setPixelColor(2, m_led.Color(255, 0, 0, 0));
  }
  else {
    m_led.setPixelColor(2, m_led.Color(0, 255, 0, 0));
    LOG("CWJAP OK");
  }
  m_led.show();

  if (wifi.sendAT("CIPSTATUS", sdummy) < 0) {
    LOG("CIPSTATUS Error");
    LOG(sdummy.c_str());
    m_led.setPixelColor(3, m_led.Color(255, 0, 0, 0));
    m_led.show();
  }
  else {
    m_led.setPixelColor(3, m_led.Color(0, 255, 0, 0));
    m_led.show();
    if (sdummy.indexOf("STATUS:5") != -1) {
      LOG("Connected");
      m_led.setPixelColor(4, m_led.Color(0, 255, 0, 0));
      m_led.show();
    }
    else {
      LOG("NOT Connected");
      LOG(sdummy.c_str());
      m_led.setPixelColor(4, m_led.Color(255, 0, 0, 0));
      m_led.show();
    }
  }

  if (wifi.sendAT("CIPNTP=0", sdummy) < 0) { // CIPNTP=0 returns no standard value with tailing 'OK' sendAT returns -1 because of that.
    LOG(sdummy.c_str());
    m_led.setPixelColor(5, m_led.Color(255, 255, 0, 0));
    m_led.show();
  }

  if (wifi.getTime(&t_tz) < 0) {
    LOG("getTime Error");
  }
  else {
    LOG((String("time set: ") + String(now())).c_str());
    stcolor = z_led.Color(0, 0, 0, 255); //Status weiss
    setZLED();
    z_led.show();
  }

  curtime = now();
  setText();
}

////////////////////////////////////////////////////////////////////////////////////////////
// Loop
void loop(void)
{

  uint8_t diff = 0;
  uint8_t reading = 0;

  if (esp8266.available())
    Serial.write(esp8266.read());
  if (Serial.available())
    esp8266.write(Serial.read());

  // read buttons
  // only switch, if key is pressed for more than x millis
  // cycle color mode
  reading = digitalRead(PIN_COL);
  if (reading != lastState[0]) {
    lastTime[0] = millis();
  }
  if ((millis() - lastTime[0]) > debounce ) {
    if (reading != curState[0]) {
      curState[0] = reading;
      if (curState[0] == HIGH) {
        colormode = (colormode + 1) % 9;
      }
    }
  }
  lastState[0] = reading;

  /*  // cycle display mode
    reading = digitalRead(PIN_DISP);
    if (reading != lastState[1]) {
      lastTime[1] = millis();
    }
    if ((millis() - lastTime[1]) > debounce ) {
      if (reading != curState[1]) {
        curState[1] = reading;
        if (curState[1] == HIGH) {
          dispmode = (dispmode +1)%3;
        }
      }
    }
    lastState[1] = reading;
  */
  // cycle animation mode
  reading = digitalRead(PIN_MISC);
  if (reading != lastState[2]) {
    lastTime[2] = millis();
  }
  if ((millis() - lastTime[2]) > debounce ) {
    if (reading != curState[2]) {
      curState[2] = reading;
      if (curState[2] == HIGH) {
        animode = (animode + 1) % 3;
        switch (animode) {
          case 0: curzled = 0;
            break;
          case 1: curzled = 0b10000000;
            break;
          case 2: curzled = 0b01000000;
            break;
        }
      }
    }
  }
  lastState[2] = reading;


  // Read brightness
  curbright = (int)(MINBRIGHT + (MAXBRIGHT - MINBRIGHT) * analogRead(PIN_LDR) / 1023.0);

  curtime = now(); // store current time

  // every minute
  if (minute(curtime) != minute(oldtime)) {

    //stcolor=z_led.Color(255,255,0,0);
    if (wifi.getTime(&t_tz) < 0) { // call ESP8266 for time
      Serial.print("getTime Error\r\n");
      //stcolor=z_led.Color(0,0,0,255);
      f_timesync = 0;
    }
    else {
      LOG((String("time set: ") + String(now())).c_str());
      if (f_timesync != 1) {
        f_timesync = 1;
        curtime = now();
        setText();
      }
    }

    if (minute(curtime) % 5 == 0) { // 5 minutes intrerval --> new text
      setText();
      if (animode != 0) {
        doAni = 1;
      }
    }
  }

  // refresh every second
  // tbd: ---> auslagern in interrupt aber wie collidiert das mit animation? evtl nur die zled rausnehmen
  if (second(curtime) != second(oldtime)) {
    if (f_timesync == 1) {
      stcolor = colorwheel(&z_led, 60, second(curtime)); // cycle through colors once every minute
    }
    else {
      stcolor = z_led.Color(255, 255, 255, 255);
    }

    if (second(curtime) % 10 == 0) { //reset z led every 0 sec 
      curzled =  T_ST; // status LED, reset minute LED
      switch (minute(curtime) % 5) {
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

    oldtime = curtime;
    z_led.setBrightness(curbright);
    setZLED();
    z_led.show();
    m_led.setBrightness(curbright);
    if (doAni != 0) {
      doAni = 0;
      aniMLED();
    }
    setMLED();
    m_led.show();
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
// animate to new text
void aniMLED() {
  uint8_t  x = 0;
  uint8_t  new_m = 0;
  uint8_t  cur_m = 0;
  uint8_t  n = 0;
  uint8_t  ledx[110]; // max 110 LED to be changed
  uint8_t  ledy[110];

  // get all LED to be changed
  for (uint8_t i = 0; i <= 9; i++) { // row
    for (uint8_t j = 0; j <= 15; j++) { // column
      new_m = bitRead(matrix_line[i], 15 - j); // new
      cur_m = bitRead(cur_matrix_line[i], 15 - j); // current
      // LED must be switched on or off or stays on (then it will be switched on/off off/on)
      if ( new_m == 1 ) {
        LOG(((String("new  ") + String(j)) + (String(",") + String(i))).c_str());
        ledx[n] = j;
        ledy[n] = i;
        n += 1;
      }
      if ( cur_m == 1 ) {
        LOG(((String("cur  ") + String(j)) + (String(",") + String(i))).c_str());
        ledx[n] = j;
        ledy[n] = i;
        n += 1;
      }
      /*// only led that need to change - but that looks booooooring ;-)
           if ((new_m == 1) && (cur_m == 0) ) { // LED must be switched on
              LOG(((String("->ON  ") + String(j)) + (String(",") + String(i))).c_str());
              ledx[n] = j;
              ledy[n] = i;
              n += 1;
            }
            if ((new_m == 0) && (cur_m == 1)) { // LED must be switched off
              LOG(((String("->OFF ") + String(j)) + (String(",") + String(i))).c_str());
              ledx[n] = j;
              ledy[n] = i;
              n += 1;
            }*/
    }
  }
  LOG(((String("Total ") + String(n)) + String(" LED to change")).c_str());

  switch (animode) {
    case 1:
      // go random through all to be switched LED and switch it
      while (true) {
        x = random(n);
        LOG(n);
        if (bitRead(cur_matrix_line[ledy[x]], 15 - ledx[x]) == 0) {
          LOG(((String("OFF -> ON") + String(ledy[x])) + (String(",") + String(ledy[x]))).c_str());
          m_led.setPixelColor(xy(ledx[x], ledy[x]), mcolor(ledx[x], ledy[x]));
          bitSet(cur_matrix_line[ledy[x]], 15 - ledx[x]);
        }
        else {
          LOG(((String("ON -> OFF") + String(ledx[x])) + (String(",") + String(ledy[x]))).c_str());
          m_led.setPixelColor(xy(ledx[x], ledy[x]), 0);
          bitClear(cur_matrix_line[ledy[x]], 15 - ledx[x]);
        }
        // remove switched led from array
        // by copying last value to current
        // and decrease counter
        ledx[x] = ledx[n - 1];
        ledy[x] = ledy[n - 1];
        n -= 1;
        LOG("show");
        m_led.show();
        delay(10);
        if (n == 0) break;
      }
      break;
    case 2:
      // go left to right with white bar
      // every to change led will get lit up bright white in the 1st step
      // and switched to the desired state afterwards
      for (uint8_t j = 0; j < 11; j++) {  //column
        for (uint8_t i = 0; i < 10; i++) { // row
          new_m = bitRead(matrix_line[i], 15 - j); // new
          cur_m = bitRead(cur_matrix_line[i], 15 - j); // current
          if (new_m == 1 || cur_m == 1 ) {
            m_led.setPixelColor(xy(j, i), m_led.Color(255, 255, 255, 255));
          }
          else {
            m_led.setPixelColor(xy(j, i), mcolor(j, i));
          }
        }
        m_led.show();
        delay(30);
        // 2nd step show time
        for (uint8_t i = 0; i < 10; i++) { // row
          new_m = bitRead(matrix_line[i], 15 - j); // new
          cur_m = bitRead(cur_matrix_line[i], 15 - j); // current
          if (new_m == 1) {
            m_led.setPixelColor(xy(j, i), mcolor(j, i));
          }
          else {
            m_led.setPixelColor(xy(j, i), 0);
          }
        }
        m_led.show();
        delay(30);
      }
      break;
  }
}

/////////////////////////////////////
// set LED from array 'line[]'
void setMLED() {
  uint32_t c;
  uint16_t x;

  for (int i = 0; i <= 9; i++) { // row
    for (int j = 0; j <= 15; j++) { // column
      x = bitRead(matrix_line[i], 15 - j);
      c = mcolor(j, i);
      //c=setbrightness(mcolor(j,i));
      if (x == 1) {
        m_led.setPixelColor(xy(j, i), c);
      }
      else {
        m_led.setPixelColor(xy(j, i), 0);
      }
    }
    cur_matrix_line[i] = matrix_line[i];
  }
}

/////////////////////////////////////
// set text
// if more writings are possible, random
void setText() {
  //matrix-lines reset
  for (uint8_t i = 0; i < 10; i++) {
    matrix_line[i] = 0;
  }

  // mal mit und mal ohne 'es ist'
  if ((int)random(2) == 0) {
    T_ES;
    T_IST0;
  }

  if (minute(curtime) < 5) {
    //if((int)random(2)==0) {
    // Es is xx Uhr
    setHourText(hourFormat12(curtime) == 1 ? 100 : hourFormat12(curtime) ); // "Es ist Ein Uhr" (nicht: "Es ist Eins Uhr" ;-)
    T_UHR;
    //}
    //else {
    //  // Es ist um xx
    //  T_UM;
    //  setHourText(hourFormat12(curtime));
    //}
  }
  else if (minute(curtime) < 10) {
    //00:05	Es ist fünf[0] nach xx
    T_FUNF0;
    T_NACH3;
    setHourText(hourFormat12(curtime));
  }
  else if (minute(curtime) < 15) {
    //00:10	Es ist zehn[1] nach xx
    T_ZEHN1;
    T_NACH3;
    setHourText(hourFormat12(curtime));
  }
  else if (minute(curtime) < 20) {
    //00:15	Es ist viertel nach xx
    //		Es ist viertel xx+1
    //if((int)random(2)==0) {
    T_VIERTEL;
    T_NACH3;
    setHourText(hourFormat12(curtime));
    //}
    //else {
    //  T_VIERTEL;
    //  setHourText(hourFormat12(curtime)+1);
    //}
  }
  else if (minute(curtime) < 25) {
    //00:20	Es ist zwanzig nach xx
    T_ZWANZIG;
    T_NACH3;
    setHourText(hourFormat12(curtime));
  }
  else if (minute(curtime) < 30) {
    //00:25	Es ist fünf[0] vor halb xx+1
    T_FUNF0;
    T_VOR;
    T_HALB;
    setHourText(hourFormat12(curtime) + 1);
  }
  else if (minute(curtime) < 35) {
    //00:30	Es ist halb xx+1
    T_HALB;
    setHourText(hourFormat12(curtime) + 1);
  }
  else if (minute(curtime) < 40) {
    //00:35	Es ist fünf[0] nach halb xx+1
    T_FUNF0;
    T_NACH3;
    T_HALB;
    setHourText(hourFormat12(curtime) + 1);
  }
  else if (minute(curtime) < 45) {
    //00:40	Es ist zwanzig vor xx+1
    T_ZWANZIG;
    T_VOR;
    setHourText(hourFormat12(curtime) + 1);
  }
  else if (minute(curtime) < 50) {
    //00:45	Es ist viertel vor xx+1
    //		Es ist dreiviertel xx+1
    if ((int)random(2) == 0) {
      T_VIERTEL;
      T_VOR;
      setHourText(hourFormat12(curtime) + 1);
    }
    else {
      T_DREIVIERTEL;
      setHourText(hourFormat12(curtime) + 1);
    }
  }
  else if (minute(curtime) < 55) {
    //00:50	Es ist zehn[1] vor xx+1
    T_ZEHN1;
    T_VOR;
    setHourText(hourFormat12(curtime) + 1);
  }
  else {
    //00:55	Es ist fünf[0] vor xx+1
    T_FUNF0;
    T_VOR;
    setHourText(hourFormat12(curtime) + 1);
  }
}

/////////////////////////////////////
// hours text
void setHourText(uint8_t h) {
  if (h < 100) { // >=100 special cases
    h %= 12;
  }
  switch (h) {
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
  uint8_t x = 0;
  uint32_t c = 0;
  for (int i = 0; i < 5; i++) {
    x = bitRead(curzled, 7 - i);
    if (i == 2) {
      c = stcolor;
      //      c=setbrightness(stcolor);
    }
    else {
      c = zcolor(i > 2 ? i - 1 : i); // minutes are index 0,1,3,4
      //      c=setbrightness(zcolor(i>2?i-1:i)); // minutes are index 0,1,3,4
    }
    if (x == 1) {
      z_led.setPixelColor(i, c);
    }
    else {
      z_led.setPixelColor(i, 0);
    }
  }
}

/////////////////////////////////////
// colorfunctions
uint32_t mcolor(uint8_t x, uint8_t y) { // matrix
  int iOfDay = (int)(hour(curtime) * 60.0 + minute(curtime));
  switch (colormode) {
    case 0:
    default:
      return m_led.Color(0, 0, 0, 255);
    case 1:
      return colorwheel(&m_led, 110, (xy(x, y) + (int)(minute(curtime) / 60.0 * 110.0)) % 110); // rainbow over matrix, start cycle once every hour
    case 2:
      return colorwheel(&m_led, 1440, iOfDay); // go through cycle once every day whole matrix one color
    case 3: // red
      return m_led.Color(255, 0, 0, 0);
    case 4: // light red
      return m_led.Color(255, 0, 0, 255);
    case 5: // blue
      return m_led.Color(0, 255, 0, 0);
    case 6: // light blue
      return m_led.Color(0, 255, 0, 255);
    case 7: // green
      return m_led.Color(0, 0, 255, 0);
    case 8: // light green
      return m_led.Color(0, 0, 255, 255);
  }
}

uint32_t zcolor(uint8_t i) { // minutes
  int iOfDay = (int)(hour(curtime) * 60.0 + minute(curtime));
  switch (colormode) {
    case 0:
    default:
      return z_led.Color(0, 0, 0, 255);
    case 1:
      return colorwheel(&z_led, 110, ((int)(minute(curtime) / 60.0 * 110.0)) % 110); // same color as 1st led of matrix
    case 2:
      return colorwheel(&z_led, 1440, iOfDay);
    case 3: // red
      return m_led.Color(255, 0, 0, 0);
    case 4:
      return m_led.Color(255, 0, 0, 128);
    case 5: // green
      return m_led.Color(0, 255, 0, 0);
    case 6:
      return m_led.Color(0, 255, 0, 128);
    case 7: // blue
      return m_led.Color(0, 0, 255, 0);
    case 8:
      return m_led.Color(0, 0, 255, 128);
  }
}

// colorwheel RGB
// 255,0,0 --> 0,255,0
// 0,255,0 --> 0,0,255
// 0,0,255 --> 255,0,0
uint32_t colorwheel(Adafruit_NeoPixel *strip, uint16_t wheelsteps, uint16_t curstep) {

  float p = wheelsteps / 3.0;
  float s = 255.0 / p; // stepsize

  // 255,0,0 --> 0,255,0
  if (curstep < p) {
    return strip->Color(255 - curstep * s, curstep * s, 0, 0);
  }
  // 0,255,0 --> 0,0,255
  if (curstep < 2 * p) {
    curstep -= p;
    return strip->Color(0, 255 - curstep * s, curstep * s, 0);
  }
  // 0,0,255 --> 255,0,0
  curstep -= 2 * p;
  return strip->Color(curstep * s, 0, 255 - curstep * s, 0);
}

/////////////////////////////////////
// Test
// Fill the dots one after the other with a color
void colorWipe(Adafruit_NeoPixel *strip, uint32_t c, uint8_t wait) {
  for (uint16_t i = 0; i < strip->numPixels(); i++) {
    strip->setPixelColor(i, c);
    strip->show();
    delay(wait);
  }
}

void colorWheel(Adafruit_NeoPixel *strip, uint8_t wait) {
  for (uint16_t i = 0; i < strip->numPixels(); i++) {
    strip->setPixelColor(i, colorwheel(strip, strip->numPixels(), i));
    strip->show();
    delay(wait);
  }
}

