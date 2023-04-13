#include <WiFiNINA.h>
#include <RTCZero.h>
#include <Wire.h>
#include "rgb_lcd.h"
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"

// lcd
rgb_lcd lcd;
const int colorR = 0;
const int colorG = 150;
const int colorB = 120;
int lcdPositionHorizontal = 0;

// weather data settings
#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10
String currentWeatherText = "";

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME680 bme; // I2C

// wifi settings
char ssid[] = "Galaxy XCover 5230F";  // wifi network
char pass[] = "baah5842";  // wifi password
int wifiConnectionTries = 0;
int wifiConnectionMaxTries = 4;

// time settings
const int GMT = 2; //time zone
const int myClock = 24;  // 4 clock
const int dateOrder = 1;  // 1 = MDY; 0 = DMY
// end settings

RTCZero rtc; // create instance of real time clock
int status = WL_IDLE_STATUS;
int myhours, mins, secs, myday, mymonth, myyear;
bool IsPM = false;

// lcd scrolling
int scrollCount = 0;
bool isScrollingLeft = true;

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

  if (wifiConnectionTries == wifiConnectionMaxTries) {
    lcd.print("Error");
    lcd.setCursor (0, 1);
    lcd.print("WiFi Connection");
    return;
  }

  rtc.begin();
  setRTC();  // get Epoch time from Internet Time Service
  fixTimeZone();

  if (!bme.begin()) {
    Serial.println(F("Could not find a valid BME680 sensor, check wiring!"));
    while (1);
  }

  // Set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms

  getWeatherData();
}

// LOOP
void loop() {
  if (wifiConnectionTries == wifiConnectionMaxTries) {
    return;
  }

  secs = rtc.getSeconds();
  if (secs == 0) {
    fixTimeZone(); // when secs is 0, update everything and correct for time zone
    // otherwise everything else stays the same.
  }

  if (secs % 20 == 0) {
    getWeatherData();
  }

  lcd.setCursor(0, 0);
  //lcd.print("test upper text");
  printDate();
  printTime();
  lcdPrintDateAndTime();

  //lcd.setCursor(lcdPositionHorizontal, 1);
  lcd.setCursor(0, 1);
  //lcd.print("test bottom text");
  lcd.print(currentWeatherText);

  Serial.println();
  while (secs == rtc.getSeconds())delay(10); // wait until seconds change
  if (mins == 59 && secs == 0) setRTC(); // get NTP time every hour at minute 59

  scroll();
}

void scroll() {
  if (scrollCount < 24 && isScrollingLeft) {
    lcd.scrollDisplayLeft();
    scrollCount++;
  }
  else {
    isScrollingLeft = false;
    lcd.scrollDisplayRight();
    scrollCount--;
  }

  if (scrollCount == 0) {
    isScrollingLeft = true;
  }
}

void lcdPrintDateAndTime() {
  String timeText = getLcdTimeText(myyear, myday, mymonth, myhours, mins, secs);
  lcd.print(timeText + "  " + timeText);
}

void printDate()
{
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
String getLcdTimeText(int year, int day, int month, int hour, int min, int second) {
  String dayString = getDoubleDigitString(day);
  String monthString = getDoubleDigitString(month);
  String yearString = getDoubleDigitString(year);
  String hourString = getDoubleDigitString(hour);
  String minString = getDoubleDigitString(min);
  String secString = getDoubleDigitString(second);

  return yearString + "/" + dayString + "/" + monthString + " " + hourString + ":" + minString + ":" + secString;
}

String getDoubleDigitString(int digit) {
  String digitString = String(digit);
  if (digit < 10) {
    digitString = "0" + digitString;
  }

  return digitString;
}

void getWeatherData() {
  // Tell BME680 to begin measurement.
  unsigned long endTime = bme.beginReading();
  if (endTime == 0) {
    Serial.println(F("Failed to begin reading :("));
    return;
  }
  Serial.print(F("Reading started at "));
  Serial.print(millis());
  Serial.print(F(" and will finish at "));
  Serial.println(endTime);

  Serial.println(F("You can do other work during BME680 measurement."));
  delay(50); // This represents parallel work.
  // There's no need to delay() until millis() >= endTime: bme.endReading()
  // takes care of that. It's okay for parallel work to take longer than
  // BME680's measurement time.

  // Obtain measurement results from BME680. Note that this operation isn't
  // instantaneous even if milli() >= endTime due to I2C/SPI latency.
  if (!bme.endReading()) {
    Serial.println(F("Failed to complete reading :("));
    return;
  }
  Serial.print(F("Reading completed at "));
  Serial.println(millis());

  Serial.print(F("Temperature = "));
  Serial.print(bme.temperature);
  Serial.println(F(" *C"));

  Serial.print(F("Pressure = "));
  Serial.print(bme.pressure / 100.0);
  Serial.println(F(" hPa"));

  Serial.print(F("Humidity = "));
  Serial.print(bme.humidity);
  Serial.println(F(" %"));

  Serial.print(F("Gas = "));
  Serial.print(bme.gas_resistance / 1000.0);
  Serial.println(F(" KOhms"));

  Serial.print(F("Approx. Altitude = "));
  Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
  Serial.println(F(" m"));

  currentWeatherText = getLcdWeatherText();

  Serial.println();
}

String getLcdWeatherText() {
  return "Temp.: " + String(bme.temperature) + "C Druck: " + String(bme.pressure / 100) + "hPa Feu.: " + String(bme.humidity) + "%";
}
