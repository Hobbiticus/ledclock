#include <Arduino.h>
#include <WiFi.h>
#include <ezTime.h>
#include "passwd.h"
#include "Weather.h"
#include "TimePanel.h"
#include "TemperaturePanel.h"

//#define DO_NETWORK


//have to consult the points endpoint to get the gridpoints for the forecase
//https://api.weather.gov/points/12.3456,-98.7654
//this contains the actual forecast - LWX = office, 58,92 = griddpoint
//https://api.weather.gov/gridpoints/LWX/58,92/forecast

//let's query the points endpoint once, then cache the "geometry" from the forecast
//if the geometry every changes, then query the points endpoint again

//Applications may cache the grid for a location to improve latency and reduce the additional lookup
//request; however, it is important to note that while it generally does not occur often, the gridX
//and gridY values (and even the office) for a given coordinate may occasionally change. For this reason,
//it is necessary to check back to the /points endpoint periodically for the latest office/grid mapping.

//The /points endpoint also contains information about the issuing office, observation stations, and
//zones for a given point location.



Timezone myTZ;

#define TIME_LATCH_PIN 25
#define TIME_ENABLE_PIN 33
#define TIME_DATA_PIN 27
#define TIME_CLOCK_PIN 26
#define COLON_PIN 14

#define TEMP_LATCH_PIN 18
#define TEMP_ENABLE_PIN 19
#define TEMP_DATA_PIN 16
#define TEMP_CLOCK_PIN 17


//PWM channels
const int ClockChannel = 0;
const int ColonChannel = 1;
const int TempChannel = 2;

Weather weather(LATITUDE, LONGITUDE);
TimePanel timePanel(TIME_CLOCK_PIN, TIME_LATCH_PIN, TIME_DATA_PIN, TIME_ENABLE_PIN, ClockChannel, COLON_PIN, ColonChannel);
TemperaturePanel tempPanel(TEMP_CLOCK_PIN, TEMP_LATCH_PIN, TEMP_DATA_PIN, TEMP_ENABLE_PIN, TempChannel);

unsigned char digit = 0;

void UpdateForecast()
{
  Forecast forecast;
  Weather::WeatherError err = weather.GetForecast(forecast);
  if (err != Weather::SUCCESS)
  {
    Serial.printf("Failed to fetch forecast: %d\n", err);
    return;
  }


}

void setup()
{
  Serial.begin(115200);

  timePanel.Init();
  tempPanel.Init();

#ifdef DO_NETWORK
    //connect to WiFi
  while (true)
  {
    Serial.println("Connecting to WiFi...");
    WiFi.begin(MY_SSID, MY_WIFI_PASSWORD);
    Serial.println("Waiting for wifi to connect...");
    uint8_t result = WiFi.waitForConnectResult();
    Serial.printf("result = %hhu\n", result);
    if (result == WL_CONNECTED)
    {
      Serial.println("Connected to WiFi!");
      break;
    }
    WiFi.disconnect(true);
  }

  UpdateForecast();

  Serial.println("Waiting for time sync...");
  int startTime = millis();
  waitForSync();
  Serial.printf("Sync took %d ms\n", (int)(millis() - startTime));
  myTZ.setLocation("America/New_York");

#endif
}

void loop()
{
#ifdef DO_NETWORK
  events(); //for ezTime

  time_t now = myTZ.now();
  //uint8_t hour = myTZ.hour(now);
  //uint8_t minute = myTZ.minute(now);
  uint8_t second = myTZ.second(now);
  Serial.printf("%d\n", second);

  unsigned char dots = second % 2 == 0 ? DOT : 0;
  digitalWrite(TIME_LATCH_PIN, LOW);
  digitalWrite(TIME_CLOCK_PIN, LOW);
  shiftOut(TIME_DATA_PIN, TIME_CLOCK_PIN, LSBFIRST, NumberMap[second / 10] | dots);
  shiftOut(TIME_DATA_PIN, TIME_CLOCK_PIN, LSBFIRST, NumberMap[second % 10] | dots);
  digitalWrite(TIME_LATCH_PIN, HIGH);
  delay(250);
  return;

#else
  Serial.printf("Showing %hhu\n", digit);

  ledcWrite(ClockChannel, 127);

  timePanel.SetTime(digit * 10 + digit, digit * 10 + digit);
  tempPanel.SetHighLow(digit * 100 + digit * 10 + digit, digit * 100 + digit * 10 + digit);
  tempPanel.SetCurrent(digit * 100 + digit * 10 + digit);

  ledcWrite(ColonChannel, (digit % 2 == 0) ? 20 : 200);

  digit++;
  if (digit > 9)
    digit = 0;

  delay(1000);
#endif
}
