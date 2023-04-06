#include <WiFiNINA.h>
#include <RTCZero.h>
#include <Wire.h>
#include "rgb_lcd.h"

// lcd
rgb_lcd lcd;
const int colorR = 0;
const int colorG = 150;
const int colorB = 120;

// settings
char ssid[] = "Galaxy XCover 5230F";  // wifi network
char pass[] = "baah5842";  // wifi password
const int GMT = 2; //time zone
const int myClock = 24;  // 4 clock
const int dateOrder = 1;  // 1 = MDY; 0 = DMY
// end settings

RTCZero rtc; // create instance of real time clock
int status = WL_IDLE_STATUS;
int myhours, mins, secs, myday, mymonth, myyear;
bool IsPM = false;

int wifiConnectionTries = 0;
int wifiConnectionMaxTries = 4;

void setup() {
  Serial.begin(115200);
  while ( status != WL_CONNECTED && wifiConnectionTries < wifiConnectionMaxTries) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);

    lcd.begin(16, 2);
    lcd.setRGB(colorR, colorG, colorB);

    wifiConnectionTries++;
  }
  delay(6000);

  lcd.clear();
  lcd.setCursor (0, 0);

  if (wifiConnectionTries == wifiConnectionMaxTries) {
    lcd.print("Error");
    lcd.setCursor (0, 1);
    lcd.print("WiFi Connection");
    return;
  }

  rtc.begin();
  setRTC();  // get Epoch time from Internet Time Service
  fixTimeZone();
}

void loop() {
  if (wifiConnectionTries == wifiConnectionMaxTries) {
    return;
  }

  secs = rtc.getSeconds();
  if (secs == 0) fixTimeZone(); // when secs is 0, update everything and correct for time zone
  // otherwise everything else stays the same.
  printDate();
  printTime();
  Serial.println();
  while (secs == rtc.getSeconds())delay(10); // wait until seconds change
  if (mins == 59 && secs == 0) setRTC(); // get NTP time every hour at minute 59
}

void printDate()
{
  //String lcdDate = String(myday) + "/" + String(mymonth) + "/" + String(myyear);
  //String lcdTime = String(myhours) + ":" + String(mins) + ":" + String(secs);
  lcd.clear();
  //lcd.print(lcdDate + " " + lcdTime);
  lcd.print(getLcdTimeText(myday, mymonth, myhours, mins, secs));
  if (dateOrder == 0) {
    Serial.print(myday);
    Serial.print("/");
  }
  Serial.print(mymonth);
  Serial.print("/");
  if (dateOrder == 1) {
    Serial.print(myday);
    Serial.print("/");
  }
  Serial.print("20");
  Serial.print(myyear);
  Serial.print(" ");
}

void printTime()
{
  print2digits(myhours);
  Serial.print(":");
  print2digits(mins);
  Serial.print(":");
  print2digits(secs);
  if (myClock == 12) {
    if (IsPM) Serial.print("  PM");
    else Serial.print("  AM");
  }
  Serial.println();
}

void print2digits(int number) {
  if (number < 10) {
    Serial.print("0");
  }
  Serial.print(number);
}

void setRTC() { // get the time from Internet Time Service
  unsigned long epoch;
  int numberOfTries = 0, maxTries = 6;
  do {
    epoch = WiFi.getTime(); // The RTC is set to GMT or 0 Time Zone and stays at GMT.
    numberOfTries++;
  }
  while ((epoch == 0) && (numberOfTries < maxTries));

  if (numberOfTries == maxTries) {
    Serial.print("Internet Time Service not avaialble");
    lcd.print("Internet Time Service");
    lcd.setCursor(0, 1);
    lcd.print("not available");
    while (1);  // hang
  }
  else {
    Serial.print("Epoch Time = ");
    Serial.println(epoch);
    rtc.setEpoch(epoch);
    Serial.println();
  }
}

/* There is more to adjusting for time zone that just changing the hour.
   Sometimes it changes the day, which sometimes chnages the month, which
   requires knowing how many days are in each month, which is different
   in leap years, and on Near Year's Eve, it can even change the year! */
void fixTimeZone() {
  int daysMon[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (myyear % 4 == 0) daysMon[2] = 29; // fix for leap year
  myhours = rtc.getHours();
  mins = rtc.getMinutes();
  myday = rtc.getDay();
  mymonth = rtc.getMonth();
  myyear = rtc.getYear();
  myhours +=  GMT; // initial time zone change is here
  if (myhours < 0) {  // if hours rolls negative
    myhours += 24; // keep in range of 0-23
    myday--;  // fix the day
    if (myday < 1) {  // fix the month if necessary
      mymonth--;
      if (mymonth == 0) mymonth = 12;
      myday = daysMon[mymonth];
      if (mymonth == 12) myyear--; // fix the year if necessary
    }
  }
  if (myhours > 23) {  // if hours rolls over 23
    myhours -= 24; // keep in range of 0-23
    myday++; // fix the day
    if (myday > daysMon[mymonth]) {  // fix the month if necessary
      mymonth++;
      if (mymonth > 12) mymonth = 1;
      myday = 1;
      if (mymonth == 1)myyear++; // fix the year if necessary
    }
  }
  if (myClock == 12) {  // this is for 12 hour clock
    IsPM = false;
    if (myhours > 11)IsPM = true;
    myhours = myhours % 12; // convert to 12 hour clock
    if (myhours == 0) myhours = 12;  // show noon or midnight as 12
  }
}

// for now without year, because display is not big enough
String getLcdTimeText(int day, int month, int hour, int min, int second) {
  String dayString = getDoubleDigitString(day);
  String monthString = getDoubleDigitString(month);
  //String yearString = getDoubleDigitString(year);
  String hourString = getDoubleDigitString(hour);
  String minString = getDoubleDigitString(min);
  String secString = getDoubleDigitString(second);

  return dayString + "/" + monthString + " " + hourString + ":" + minString + ":" + secString;
}

String getDoubleDigitString(int digit) {
  String digitString = String(digit);
  if (digit < 10) {
    digitString = "0" + digitString;
  }

  return digitString;
}
