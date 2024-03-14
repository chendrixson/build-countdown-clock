#include <Wire.h>
#include <Adafruit_SSD1306.h>

#define USE_RTC

#ifdef USE_RTC
#include <RTClib.h>
RTC_DS3231 rtc;
#endif

Adafruit_SSD1306 display(128, 64, &Wire, -1);
const int DISPLAY_ADDRESS = 0x3c;

// from observation we see that millis() runs slow on this device, so we'll adjust for that
// in 10 minutes we lost 13 seconds.
const int MILLIS_PER_SECOND = 934;  

// current time in 24 hour format
int currentTimeHour = 0;
int currentTimeMinute = 0;
int currentTimeSecond = 0;

// and the current date
int currentMonth = 2;
int currentDay = 23;
int currentYear = 2024;

// create a constant for may 21st, 2024
const int targetMonth = 5;
const int targetDay = 21;

char smallBuffer[24] = {0};

int calculateDayOfYear(int month, int day) {
  int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  int dayOfYear = 0;

  // Calculate the number of days in the previous months
  for (int i = 0; i < month - 1; i++) {
    dayOfYear += daysInMonth[i];
  }

  // Add the current day
  dayOfYear += day;

  // Adjust for leap year if necessary
  if (month > 2 && (currentYear % 4 == 0 && currentYear % 100 != 0 || currentYear % 400 == 0)) {
    dayOfYear++;
  }

  return dayOfYear;
}

void clearScreen()
{
  display.clearDisplay();
  display.display();
}

void showDate(int m, int d, int y)
{
  display.setTextColor(WHITE,BLACK);
  display.setCursor(60,2);
  display.setTextSize(1);
  sprintf(smallBuffer, "%2d/%d/%d  ", m, d, y);
  display.println(smallBuffer);  

  // and then show how many days until showDaysTillBuild  
  int currentDayOfYear = calculateDayOfYear(currentMonth, currentDay);
  int targetDayOfYear = calculateDayOfYear(targetMonth, targetDay);
  int days = targetDayOfYear - currentDayOfYear;
  if(days < 0)
  {
    // look for next year!
    days = 365 + days;
  }

  display.setTextColor(WHITE,BLACK);
  display.setCursor(0,55);
  sprintf(smallBuffer, "%3d days till //Build", days);
  display.println(smallBuffer);  
  display.display();
}

void showTime(int h, int m, int s)
{
  display.setTextColor(WHITE,BLACK);
  display.setCursor(8,24);
  display.setTextSize(2);

  char* ampm = "AM";
  if(h >= 12)
  {
    h -= 12;
    ampm = "PM";
  }
  // correct for noon/midnight
  if(h == 0)
  {
    h = 12;
  }

  sprintf(smallBuffer, "%2d:%02d:%02d", h, m, s);
  display.println(smallBuffer);
  display.setTextSize(1);
  display.setCursor(108,24);
  display.println(ampm);
  display.display();
}

#ifndef USE_RTC
void nextDay()
{
  // function to advance the current date information, taking into account the days in the month,
  // annd leap years
  int daysInMonth = 31; // assume 31 days in the month
  if(currentMonth == 4 || currentMonth == 6 || currentMonth == 9 || currentMonth == 11)
  {
    daysInMonth = 30;
  }
  else if(currentMonth == 2)
  {
    daysInMonth = 28;
    if(currentYear % 4 == 0)
    {
      daysInMonth = 29;
    }
  }

  currentDay++;
  if(currentDay > daysInMonth)
  {
    currentDay = 1;
    currentMonth++;
    if(currentMonth > 12)
    {
      currentMonth = 1;
      currentYear++;
    }
  }

  showDate(currentMonth, currentDay, currentYear);
}
#endif

void setTime(int h, int m, int s)
{
  currentTimeHour = h;
  currentTimeMinute = m;
  currentTimeSecond = s;
}

void setDate(int m, int d, int y)
{
  currentMonth = m;
  currentDay = d;
  currentYear = y;
}

void outputDateAndTime(int h, int m, int s, int month, int day, int year)
{
  Serial.print(F("time: "));
  char outBuffer[16] = {0};
  sprintf(outBuffer, "%d:%d:%d", h, m, s);
  Serial.println(outBuffer);
  Serial.print(F("date: "));
  sprintf(outBuffer, "%d/%d/%d", month, day, year);
  Serial.println(outBuffer);
}

#ifdef USE_RTC
void getRTC(bool dumpInfo = false)
{
  DateTime now = rtc.now();

  if(dumpInfo)
  {
    Serial.println("Dumping RTC info");
    outputDateAndTime(now.hour(), now.minute(), now.second(), now.month(), now.day(), now.year());
  }

  setTime(now.hour(), now.minute(), now.second());
  setDate(now.month(), now.day(), now.year());
}

void adjustRTC(int h, int m, int s, int month, int day, int year)
{
  Serial.println(F("Adjusting RTC"));
  outputDateAndTime(h, m, s, month, day, year);
  rtc.adjust(DateTime(year, month, day, h, m, s));
}
#endif

void setup() {

  delay(2000);
  
  // print out the instructions for setting the date and time
  Serial.begin(115200);
  Serial.println(F("Use tHH:MM:SS followed by a newline to set the time"));
  Serial.println(F("Use dMM/DD/YY followed by a newline to set the date"));

  display.begin(SSD1306_SWITCHCAPVCC, DISPLAY_ADDRESS);
  clearScreen();
  delay(2000);

#ifdef USE_RTC
  Serial.println(F("Initializing RTC\n"));
  if (!rtc.begin()) {
    Serial.println(F("RTC failed to begin!"));
  }
  else
  {
    Serial.println(F("RTC is running"));
    DateTime now = rtc.now();
    getRTC(true);
    Serial.println(F("Got the RTC info"));
  }
#endif

  Serial.println(F("About to show time and date"));
  showTime(currentTimeHour, currentTimeMinute, currentTimeSecond);
  showDate(currentMonth, currentDay, currentYear);
  Serial.println(F("Finished showing time and date"));
  delay(2000);
}

char commandBuffer[16] = {0};

void loop() {

  if(Serial.available() > 0)
  {
    Serial.println(F("Got command data"));
    // read to see if the character is a t or a d
    char c = Serial.read();
    if(c == 't')
    {
      memset(commandBuffer, 0, 16);
      Serial.readBytes(commandBuffer, 15);

      int h, m, s;     
      Serial.println(F("Got command buffer:"));
      Serial.println(commandBuffer);

      if(sscanf(commandBuffer, "%d:%d:%d", &h, &m, &s) == 3)
      {
        Serial.println(F("Setting time"));        
        #ifdef USE_RTC
          adjustRTC(h, m, s, currentMonth, currentDay, currentYear);
        #else
          setTime(h, m, s);
          showTime(currentTimeHour, currentTimeMinute, currentTimeSecond);
        #endif
      }
      else
      {
        Serial.println(F("Failed to parse time"));
      }
    }
    else if(c == 'd')
    {      
      memset(commandBuffer, 0, 16);
      Serial.readBytes(commandBuffer, 15);

      int m, d, y;     
      Serial.println(F("Got command buffer:"));
      Serial.println(commandBuffer);

      if(sscanf(commandBuffer, "%d/%d/%d", &m, &d, &y) == 3)
      {
        Serial.println(F("Setting date"));
        #ifdef USE_RTC
          adjustRTC(currentTimeHour, currentTimeMinute, currentTimeSecond, m, d, y);
        #else
          setDate(m, d, y);
          showDate(currentMonth, currentDay, currentYear);
        #endif
      }
      else
      {
        Serial.println(F("Failed to parse date"));
      }      
    }
  }

#ifdef USE_RTC
  getRTC();
#else
  // advance current time by one second
  currentTimeSecond++;
  if(currentTimeSecond >= 60)
  {
    currentTimeSecond = 0;
    currentTimeMinute++;
    if(currentTimeMinute >= 60)
    {
      currentTimeMinute = 0;
      currentTimeHour++;
      if(currentTimeHour >= 24)
      {
        currentTimeHour = 0;
      }
    }
  }
#endif

  // advance the time for each second 
  showTime(currentTimeHour, currentTimeMinute, currentTimeSecond);

#ifdef USE_RTC
  showDate(currentMonth, currentDay, currentYear);
#else
  // if we've reached midnight, advance the date
  if(currentTimeHour == 0 && currentTimeMinute == 0 && currentTimeSecond == 0)
  {
    nextDay();
  }
#endif

  delay(MILLIS_PER_SECOND);
}

