////////////////////////////////////////////////////////////////////////////////////////////
/* minimal ESP8266 Class
 * with NTP call to set arduino time
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
 */

class ESP8266mini  {

public:

  /* Constructor
   * @param uart - softwareserial object
   * @param baud - baudrate
   */
  ESP8266mini(SoftwareSerial &uart, uint32_t baud);

  /* Send AT command, get return String
   * @param cmd - max length 250 chars, if empty only AT\r\n is sent, otherwise AT+cmd\r\n
   * @param recv - String return value
   * @retval - -1 error
   * @retval - number of sent chars to uart
   */
  int sendAT(char* cmd, String &recv);

  /* get time from ESP8266 and save it to system time
   * @retval - -1 error
   * @retval - 0 ok
   */
  int getTime(Timezone *tz);

private:
  // This is our SoftwareSerial object
  SoftwareSerial *esp_uart;
};

////////////////////////////////////////////////////////////////////////////////////////////
// Constructor
ESP8266mini::ESP8266mini(SoftwareSerial &uart, uint32_t baud): 
esp_uart(&uart)
{
  esp_uart->begin(baud);
  //serialFlush();
}

////////////////////////////////////////////////////////////////////////////////////////////
// Send AT command read RX
int ESP8266mini::sendAT(char* cmd, String &recv) {
  int ret=0;
  String buf;
  char c;
  uint32_t stime;
  uint32_t timeout=10000;
  char* match="\r\nOK";

  if(strlen(cmd) == 0) {
    buf = String("AT\r\n");
  }
  else {
    buf = String("AT+" + String(cmd)+ "\r\n");
  }

  DLOG(String("Sending: "+buf).c_str())

  while(esp_uart->available() > 0) {
    c = esp_uart->read();
  }
  
  recv = String("");

  ret = esp_uart->write(buf.c_str());
  if(ret != buf.length()) {
    return -1;
  }
  
  stime = millis();
  ret=-1;
  while (millis() - stime < timeout) {
    while(esp_uart->available() > 0) {
      c = esp_uart->read();
      if((c >= 0x20 && c<= 0x7e) || c==0x0a || c==0x0d) { // discard not printable
        recv += String(c);
        //buf += String(c);
      }
    }
    if(recv.indexOf(match) != -1) {
      ret = recv.length();
      DLOG(String("match found").c_str());
      break;
    }
  }
  DLOG(String("received:\r\n-----\r\n"+recv+"\r\n-----\r\n").c_str());
  return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////
// getTime from NTP and set system time NTP Setting of ESP is GMT0
int ESP8266mini::getTime(Timezone *tz) {
  long Y=0;
  int ts=0;
  tmElements_t tm;
  time_t dtime;
  String getTimeDummy;

  if(sendAT("CIPNTP?",getTimeDummy)<0) {
    return -1;
  }

  // AT+CIPNTP?
  // Time: 14:26:14 09/19/2016 GMT00
  // OK

  ts = getTimeDummy.indexOf("Time: "); // find where Time-String starts
  Y=getTimeDummy.substring(ts+21,ts+25).toInt();

  if( Y > 99)
    Y = Y - 1970;
  else
    Y += 30;
  tm.Year = Y;
  tm.Month = getTimeDummy.substring(ts+15,ts+17).toInt();
  tm.Day = getTimeDummy.substring(ts+18,ts+20).toInt();
  tm.Hour = getTimeDummy.substring(ts+6,ts+8).toInt();
  tm.Minute = getTimeDummy.substring(ts+9,ts+11).toInt();
  tm.Second = getTimeDummy.substring(ts+12,ts+14).toInt();

  dtime = makeTime(tm);

  setTime(tz->toLocal(dtime));

  time_t t = now(); // Store the current time in time 
  DLOG((String("localtime:\r\n")+String(year(t))+String("-")+String(month(t))+String("-")+String(day(t))+String(" ")+String(hour(t))+String(":")+String(minute(t))+String(":")+String(second(t))).c_str());
  return 0;
}

