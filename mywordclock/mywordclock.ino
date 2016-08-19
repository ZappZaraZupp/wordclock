//
// mywordclock
// Autor C. Pohl
// http://github.com/zappzarazupp
//
// Mein version einer Wortuhr
// Baut auf WS2811B LED
// 

#include <Adafruit_NeoPixel.h>
#include <Time.h>
#include <DCF77.h>

//#define DEBUG ja
#undef DEBUG

// PINs
#define PIN_MLED  6
#define PIN_ZLED  7
#define PIN_LDR   A0  // Analog
#define PIN_DCF   2
#define DCF_INTERRUPT 0  // Interrupt number associated with PIN_DCF
#define PIN_TASTCOLM  3  // farbmodus 
#define PIN_TASTDISP  4  // anzeigemodus

// einige Konstanten
#define M_WIDTH 11
#define M_HEIGHT 10
#define Z_LEDS 5
#define TPOLL=10;
#define MINBRIGHT 50.0
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

// Globale Variablen
Adafruit_NeoPixel m_led = Adafruit_NeoPixel(M_WIDTH * M_HEIGHT, PIN_MLED, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel z_led = Adafruit_NeoPixel(Z_LEDS, PIN_ZLED, NEO_GRB + NEO_KHZ800);

uint16_t line[10] = { 0,0,0,0,0,0,0,0,0,0 };

uint8_t curstd=0, oldstd=0;
uint8_t curmin=0, oldmin=0;
uint8_t cursec=0, oldsec=0;
uint8_t curzled=0, oldzled=0;
uint16_t curbright=0, oldbright=0;
uint32_t curstatus_color=0, oldstatus_color=0;
uint8_t colormode=0;
uint8_t dispmode=0;
uint8_t pollcolm=0;
uint8_t tastcolm=0;
uint8_t polldisp=0;
uint8_t tastdisp=0;

time_t time;
DCF77 DCF = DCF77(PIN_DCF,DCF_INTERRUPT);

/////////////////////////////////////
// SETUP
void setup() {
  
#ifdef DEBUG
Serial.begin(9600);  //Begin serial communcation
#endif

  pinMode(PIN_TASTCOLM, INPUT);
  pinMode(PIN_TASTDISP, INPUT);
 
  randomeSeed(analogRead(PIN_LDR));

  m_led.begin();
  m_led.show(); // Initialize all pixels to 'off'
  z_led.begin();
  z_led.show(); // Initialize all pixels to 'off'

  //Test
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
  DCF.Start();
  setSyncInterval(30);
  setSyncProvider(DCF.getTime);
}

/////////////////////////////////////
// LOOP
void loop() {

#ifdef DEBUG
delay(100);
#endif

  int16_t diff=0;
 
  // meine version, das prellen in sw abzufangen #define TPOLL zum optimieren
  if(digitalRead(PIN_TASTCOLM == HIGH) {
    pollcolm+=1;
    if(pollcolm >= TPOLL && tastcolm==0) {
      pollcolm=TPOLL;
      tastcolm=1;
      //tastenaktion drück   
      colmode+=1;
      colmode%=2; // gibt momentan nur 2 colmode
      oldbright=0; // erzwinge neuzeichnen
    }
  }
  else {
    pollcolm-=1;
    if(pollcolm <= 0 && tastcolm==1) {
      pollcolm=0;
      tastcolm=0;
      //tastenaktion loslass 
    }
  }

//  if(digitalRead(PIN_TASTDISP == HIGH) {
//    polldisp+=1;
//    if(polldisp >= TPOLL && tastdisp==0) {
//      polldisp=TPOLL;
//      tastdisp=1;
//      //tastenaktion drück   
//    }
//  }
//  else {
//    polldisp-=1;
//    if(polldisp <= 0 && tastdisp==1) {
//      polldisp=0;
//      tastdisp=0;
//      //tastenaktion loslass 
//    }
//  }
//

  // Helligkeit einlesen
  curbright = (int)(MINBRIGHT+(MAXBRIGHT-MINBRIGHT)*analogRead(PIN_LDR)/1023.0);
  diff = oldbright - curbright;
  if(diff<0) { diff = -diff;  }  //abs macht probleme

  // Zeit lesen
  time=now();
  curmin=minute(time);
  curstd=hourFormat12(time);
  cursec=second(time);

//  if(timeStatus() == timeNotSet) {  
//    if(digitalRead(PIN_DCF) == 1) {
//      curstatus_color = z_led.Color(255,0,255);
//    }
//    else {
//      curstatus_color = z_led.Color(255,0,0);
//    }
//  } 
//  else {
//    if(digitalRead(PIN_DCF) == 1) {
//      curstatus_color = z_led.Color(0,255,255);
//    }
//    else {
//      curstatus_color = z_led.Color(0,255,0);
//    }
//  } 
//
  
  // sekundenblinken statusled
  // anpassen, wenn DCF Blinken dazukommt
  curzled = (curzled & T_M4) | ((cursec % 2) * T_ST);
  
  // Alle 5 minuten Matrix neu schreiben
  if(curmin != oldmin && curmin%5 == 0) {
    setText();
    setMin();
    setMatrixLED();
    m_led.setBrightness(curbright);
    oldstd=curstd;
    oldmin=curmin;
    m_led.show();
  }
  else if (diff > 10) { // bei größeren helligkeitsschwakungen neu zeichnen mit neuer helligkeit
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

  // wenn sich an den zled was geändert hat
  if(curzled != oldzled || curstatus_color != oldstatus_color) {
    setZLED();
    z_led.setBrightness(curbright);
    oldzled=curzled;
    oldbright=curbright;
    oldstatus_color=curstatus_color;
    z_led.show();
  }
  else if (diff > 10) { // bei größeren helligkeitsschwakungen neu zeichnen mit neuer helligkeit
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

/////////////////////////////////////
// LED nummer aus matrixkoordinaten
uint16_t xy( uint8_t x, uint8_t y)
{
  uint16_t i;
  i = (y * M_WIDTH) + x;
  
  return i;
}

/////////////////////////////////////
// Matrix LED setzen aus array 'line[]'
void setMatrixLED() {
  uint32_t c;
  uint16_t x;

  for(int i=0; i<=9; i++) { // zeile
    for(int j=0; j<=15; j++) { // spalte
      x=bitRead(line[i],15-j);
      c=mcolor(j,i);
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

/////////////////////////////////////
// Texte aus aktueller zeit setzen
// bei mehr schreibweisen entscheidet der zufall
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

/////////////////////////////////////
// Stundentext
// sonderfall: es ist ein uhr
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

/////////////////////////////////////
// zLED aus curzled setzen
// sonderbehandlung status LED
void setZLED() {
  uint8_t x=0;
  uint32_t c=0;
  for(int i=0;i<5;i++) {
    x=bitRead(curzled,7-i);
    if(i==2) {
      c=curstatus_color;
    }
    else {
      c=zcolor(i>2?i-=1:i); // zminuten sind index 0,1,3,4 aber für farbe muss es 0,1,2,3 sein
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
// zwischenminuten setzen
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

/////////////////////////////////////
// farbfunktion(en)
uint32_t mcolor(uint8_t x, uint8_t y) { // farbfunktion für Matrix
  switch(colormode) {
    case 0:
    default:
      return colorwheel(m_led,110,xy(x,y)); 
    case 1:
      int iOfDay=(int)((curstd*60.0+curmin)/5.0); // aktuelle anzahl der 5 min intervalle
      return colorwheel(m_led,288,iOfDay); // ein tag hat 288 5min intervalle
  }
}

uint32_t zcolor(uint8_t i) { // farbfunktion für minuten
  switch(colormode) {
    case 0:
    default:
      return colorwheel(z_led,4,i); 
    case 1:
      int iOfDay=(int)((curstd*60.0+curmin)/5.0); // aktuelle anzahl der 5 min intervalle
      return colorwheel(z_led,288,iOfDay); // ein tag hat 288 5min intervalle
  }
}

// farbrad
// 255,0,0 --> 0,255,0
// 0,255,0 --> 0,0,255
// 0,0,255 --> 255,0,0
uint32_t colorwheel(Adafruit_NeoPixel *strip, uint8_t wheelsteps, uint8_t curstep) {

  float p=wheelsteps/3.0;
  float s=255.0/p); // schrittweite
  
  // 255,0,0 --> 0,255,0
  if(curstp < p) {
    return strip->Color(255-curstp*s, curstp*s, 0);
  }
  // 0,255,0 --> 0,0,255
  if(curstp < 2*p) {
    curstp -= p;
    return strip->Color(0, 255-curstp*s, curstp*s);
  }
  // 0,0,255 --> 255,0,0
    curstp -= 2*p;
    return strip->Color(curstp*s,0, 255-curstp*s);
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


