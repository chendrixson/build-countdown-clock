#include <Wire.h>
#include <Adafruit_SSD1306.h>

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
  char dateBuffer[16] = {0};
  sprintf(dateBuffer, "%d/%d/%d", m, d, y);
  display.println(dateBuffer);  

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
  char dtbBuffer[24] = {0};
  sprintf(dtbBuffer, "%d days until //BUILD", days);
  display.println(dtbBuffer);  
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

  char buffer[16] = {0};
  sprintf(buffer, "%2d:%02d:%02d", h, m, s);
  display.println(buffer);
  display.setTextSize(1);
  display.setCursor(108,24);
  display.println(ampm);
  display.display();
}


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

void setTime(int h, int m, int s)
{
  currentTimeHour = h;
  currentTimeMinute = m;
  currentTimeSecond = s;
}

void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, DISPLAY_ADDRESS);
  clearScreen();
  delay(2000);

  // print out the instructions for setting the date and time
  Serial.begin(115200);
  Serial.println("Use tHH:MM:SS followed by a newline to set the time\n");
  Serial.println("Use dMM/DD/YY followed by a newline to set the date\n");

  setTime(0,0,0);

  showTime(currentTimeHour, currentTimeMinute, currentTimeSecond);
  showDate(currentMonth, currentDay, currentYear);
}

void loop() {

  if(Serial.available() > 0)
  {
    // read to see if the character is a T or a d
    char c = Serial.read();
    if(c == 't')
    {
      char buffer[16] = {0};
      Serial.readBytesUntil('\n', buffer, 16);
      int h, m, s;
      if(sscanf(buffer, "%d:%d:%d", &h, &m, &s) == 3)
      {
        setTime(h, m, s);
        showTime(currentTimeHour, currentTimeMinute, currentTimeSecond);
      }
    }
    else if(c == 'd')
    {
      Serial.println("enter the date in the format MM/DD/YY and hit enter");
      char buffer[16] = {0};
      Serial.readBytesUntil('\n', buffer, 16);
      int m, d, y;
      if(sscanf(buffer, "%d/%d/%d", &m, &d, &y) == 3)
      {
        currentMonth = m;
        currentDay = d;
        currentYear = y;
        showDate(currentMonth, currentDay, currentYear);
      }
    }

  }

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

  // advance the time for each second 
  showTime(currentTimeHour, currentTimeMinute, currentTimeSecond);

  // if we've reached midnight, advance the date
  if(currentTimeHour == 0 && currentTimeMinute == 0 && currentTimeSecond == 0)
  {
    nextDay();
  }

  delay(MILLIS_PER_SECOND);
}

