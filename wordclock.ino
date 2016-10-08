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
#include <SoftwareSerial.h>
// !! Changed: Buffer to 256 !!
#include <Time.h>
#include <TimeLib.h>
#include <Timezone.h>
#include <Adafruit_NeoPixel.h>

#define DEBUG ja 
#define DEBUGLOG(arg) Serial.print("*** [");Serial.print((const char*)__FUNCTION__);Serial.print("] ");Serial.print(arg);Serial.print(" ***\r\n");

// WLAN Credentials
// #define SSID        "your SSID"
// #define PASSWORD    "your Password"
#include "WLAN_cred.h";
// ESP8266 Connection settings
#define RXPIN       11
#define TXPIN       12
#define ESPBAUD     19200

// PINs
#define PIN_MLED  6
#define PIN_ZLED  7 
#define PIN_LDR   A0  // Analog
#define PIN_COLM  8   // Colormode
#define PIN_DSIP  9   // Displaymode

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
// minimal ESP8266 Class
// with NTP call to set arduino time
class ESP8266mini  {
public:

  // Constructor
  // @param uart - softwareserial object
  // @param baud - baudrate
  ESP8266mini(SoftwareSerial &uart, uint32_t baud);

  // Send AT command, get return String
  // @param cmd - max length 250 chars, if empty only AT\r\n is sent, otherwise AT+cmd\r\n
  // @param recv - String return value
  // @retval - -1 error
  // @retval - number of sent chars to uart
  int sendAT(char* cmd, String &recv);

  // get time from ESP8266 and save it to system time
  // @retval - -1 error
  // @retval - 0 ok
  int getTime(Timezone *tz);

private:
  // This is our SoftwareSerial object
  SoftwareSerial *esp_uart;

  // Clear RX buffer
  void serialFlush(void);

  // Read data from uart if available till timeout(ms) or match
  String serialRead(char * match, uint32_t timeout=5000);
};

// Constructor
ESP8266mini::ESP8266mini(SoftwareSerial &uart, uint32_t baud): 
esp_uart(&uart)
{
  esp_uart->begin(baud);
  serialFlush();
}

// Send AT command read RX
int ESP8266mini::sendAT(char* cmd, String &recv) {
#ifdef DEBUG
  DEBUGLOG("start")
#endif
    int ret=0;
  char buf[256];

  if(strlen(cmd)+6 > 256) {  // AT+cmd\r\n\0
    return -1;
  }

  if(strlen(cmd) == 0) {
    sprintf(buf,"AT\r\n");
  }
  else {
    sprintf(buf,"AT+%s\r\n",cmd);
  }

  serialFlush();

#ifdef DEBUG
  DEBUGLOG("Sending:");
  DEBUGLOG(buf);
#endif
  ret = esp_uart->write(buf);
  if(ret != strlen(buf)) {
    return -1;
  }

  recv = serialRead("\r\nOK",5000);

#ifdef DEBUG
  DEBUGLOG("end")
#endif
    return ret;
}

// Clear RX buffer
void ESP8266mini::serialFlush(void) {
#ifdef DEBUG
  DEBUGLOG("start")
#endif
    uint8_t t;
  while(esp_uart->available() > 0) {
    t = esp_uart->read();
  }
#ifdef DEBUG
  DEBUGLOG("end")
#endif
  }

  // Read data from uart if available till timeout or match
  String ESP8266mini::serialRead(char * match, uint32_t timeout) {
#ifdef DEBUG
    DEBUGLOG("start")
#endif
      String ret="";
    char c;
    uint32_t stime = millis();
    while (millis() - stime < timeout) {
      while(esp_uart->available() > 0) {
        c = esp_uart->read();
        if(c != 0) { //discard end of String
          ret += c;
        }
      }
      if(ret.indexOf(match) != -1) {
        break;
      }
    }
#ifdef DEBUG
    DEBUGLOG("end")
#endif
      return ret;
  }

// getTime from NTP and set system time NTP Setting of ESP is GMT0
int ESP8266mini::getTime(Timezone *tz) {
#ifdef DEBUG
  DEBUGLOG("start")
#endif

    String dummy="";
  long Y=0;
  int ts=0;
  tmElements_t tm;
  time_t dtime;

  if(sendAT("CIPNTP?",dummy)<0) {
    return -1;
  }

  // AT+CIPNTP?
  // Time: 14:26:14 09/19/2016 GMT00
  // OK

  ts = dummy.indexOf("Time: "); // find where Time-String starts
  Y=dummy.substring(ts+21,ts+25).toInt();

  if( Y > 99)
    Y = Y - 1970;
  else
    Y += 30;
  tm.Year = Y;
  tm.Month = dummy.substring(ts+15,ts+17).toInt();
  tm.Day = dummy.substring(ts+18,ts+20).toInt();
  tm.Hour = dummy.substring(ts+6,ts+8).toInt();
  tm.Minute = dummy.substring(ts+9,ts+11).toInt();
  tm.Second = dummy.substring(ts+12,ts+14).toInt();

  dtime = makeTime(tm);

#ifdef DEBUG
  DEBUGLOG("From ESP8266");
  DEBUGLOG(dummy.substring(ts+6,ts+8).toInt());
  DEBUGLOG(dummy.substring(ts+9,ts+11).toInt());
  DEBUGLOG(dummy.substring(ts+12,ts+14).toInt());
  DEBUGLOG(dummy.substring(ts+15,ts+17).toInt());
  DEBUGLOG(dummy.substring(ts+18,ts+20).toInt());
  DEBUGLOG(Y);
#endif

  setTime(tz->toLocal(dtime));

#ifdef DEBUG
  time_t t = now(); // Store the current time in time 
  DEBUGLOG("localtime");
  DEBUGLOG(hour(t));          // Returns the hour for the given
  DEBUGLOG(minute(t));        // Returns the minute for the given
  DEBUGLOG(second(t));        // Returns the second for the given
  DEBUGLOG(day(t));           // The day for the given time t
  DEBUGLOG(weekday(t));       // Day of the week for the given Sun=1, Mon=2...
  DEBUGLOG(month(t));         // The month for the given time t
  DEBUGLOG(year(t));          // The year for the given time t
#endif


#ifdef DEBUG
  DEBUGLOG("end")
#endif
    return 0;
}


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

uint8_t prevkey=0;  // flag, if key was pressed in last loop

////////////////////////////////////////////////////////////////////////////////////////////
// Setup
void setup(void)
{
  Serial.begin(ESPBAUD);
#ifdef DEBUG
  DEBUGLOG("start")
#endif

    randomSeed(analogRead(PIN_LDR));

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


  String sdummy="";

  esp8266.begin(ESPBAUD);

  if(wifi.sendAT("RST",sdummy)<0) {
    Serial.print("Error\r\n");
  }
  delay(3000); // RST needs some time

#ifdef DEBUG
  if(wifi.sendAT("GMR",sdummy)<0) {
    Serial.print("Error\r\n");
  }
  else {
    Serial.print(sdummy.c_str());
  }
#endif
  if(wifi.sendAT("CWMODE=1",sdummy)<0) {
    Serial.print("Error\r\n");
  }
  else {
    Serial.print(sdummy.c_str());
  }

  if(wifi.sendAT("CWJAP=\""SSID"\",\""PASSWORD"\"",sdummy)<0) {
    Serial.print("Error\r\n");
  }
  else {
    Serial.print(sdummy.c_str());
  }

#ifdef DEBUG
  if(wifi.sendAT("CIPSTA?",sdummy)<0) {
    Serial.print("Error\r\n");
  }
  else {
    Serial.print(sdummy.c_str());
  }

  if(wifi.sendAT("CIPSTATUS",sdummy)<0) {
    Serial.print("Error\r\n");
  }
  else {
    Serial.print(sdummy.c_str());
  }
#endif

  if(wifi.sendAT("CIPNTP=0",sdummy)<0) {
    Serial.print("Error\r\n");
  }
  else {
    Serial.print(sdummy.c_str());
  }

#ifdef DEBUG
  if(wifi.sendAT("CIPNTP?",sdummy)<0) {
    Serial.print("Error\r\n");
  }
  else {
    Serial.print(sdummy.c_str());
  }
#endif

  if(wifi.getTime(&t_tz)<0) {
    Serial.print("Error\r\n");
  }
  else {
    Serial.print("time set\r\n");
    stcolor=z_led.Color(255,255,255);
  }

  curtime=now();
  setText();

#ifdef DEBUG
  DEBUGLOG("end")
#endif
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

/*
    // color mode
    if(digitalRead(PIN_COLM)==1 && prevkey == 0 ) {
      if(prevkey == 0) {  
        prevkey = 1;
        colormode=(colormode+1)%3;  // currently 3 color modes
      }
    } else {
      prevkey = 0;
    }
*/
/*
    // display mode
    if(digitalRead(PIN_DISP)==1 && prevkey == 0 ) {
      if(prevkey == 0) {  
        prevkey = 1;
        displaymode=(displaymode+1)%3;
      }
    } else {
      prevkey = 0;
    }
*/    
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
        Serial.print("Error\r\n");
        stcolor=z_led.Color(255,0,0);
      }
      else {
        Serial.print("time set\r\n");
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
#ifdef DEBUG
  DEBUGLOG("start");  
#endif
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
#ifdef DEBUG
  DEBUGLOG("end");  
#endif
}

/////////////////////////////////////
// set text
// if more writings are possible, random
void setText() {
#ifdef DEBUG
  DEBUGLOG("start");  
#endif

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
#ifdef DEBUG
  DEBUGLOG("end");  
#endif
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
    return colorwheel(&z_led,4,(i+(int)(minute(curtime)/60.0*4.0))%4); 
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

