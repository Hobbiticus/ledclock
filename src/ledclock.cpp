#include <Arduino.h>
#include <WiFi.h>
#include <ezTime.h>
#include "passwd.h"
#include "Weather.h"
#include "TimePanel.h"
#include "TemperaturePanel.h"

#define DO_NETWORK


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





void UpdateForecast()
{
  Forecast forecast;
  Weather::WeatherError err = weather.GetForecast(forecast);
  if (err != Weather::SUCCESS)
  {
    Serial.printf("Failed to fetch forecast: %d\n", err);
    tempPanel.SetHighLow(-999, -999);
    return;
  }

  tempPanel.SetHighLow(forecast.m_High, forecast.m_Low);
}

bool ConnectStep()
{
  wl_status_t status = WiFi.status();
  if (status == WL_CONNECTED)
    return true;
  if (status == WL_CONNECT_FAILED || status == WL_CONNECTION_LOST)
  {
    Serial.printf("Huh? failed to connect? %d\n", status);
    WiFi.disconnect(true);
    WiFi.begin(MY_SSID, MY_WIFI_PASSWORD);
  }
  return false;
}

bool WantForecast = false;
int LastFetchForecastTime = 0;

void CheckOnForecast()
{
  if (!WantForecast)
    return;
  if (!weather.IsFetchComplete())
    return;
  WantForecast = false;

  Forecast forecast;
  Weather::WeatherError err = weather.GetForecast(forecast);
  if (err != Weather::SUCCESS)
  {
    Serial.printf("Failed to fetch forecast: %d\n", err);
  }
  Serial.printf("Fetching forecast returned high = %d, low = %d\n", forecast.m_High, forecast.m_Low);
  tempPanel.SetHighLow(forecast.m_High, forecast.m_Low);
}

void setup()
{
  Serial.begin(115200);

  timePanel.Init();
  tempPanel.Init();

#ifdef DO_NETWORK
  //connect to WiFi
  Serial.println("Connecting to WiFi...");
  WiFi.begin(MY_SSID, MY_WIFI_PASSWORD);
  Serial.println("Waiting for wifi to connect...");
  while (!ConnectStep())
  {
    timePanel.StepWifiAnimation();
    delay(100);
  }

  WantForecast = true;
  weather.BeginFetchForecast();
  LastFetchForecastTime = millis();
  //UpdateForecast();

  Serial.println("Waiting for time sync...");
  int startTime = millis();
  while (timeStatus() != timeSet)
  {
    CheckOnForecast();
    timePanel.StepTimeSyncAnimation();
    delay(100);
    events();
  }
  Serial.printf("Sync took %d ms\n", (int)(millis() - startTime));
  myTZ.setLocation("America/New_York");

#endif
}


void loop()
{
  if (!WantForecast && weather.GetLastError() != Weather::SUCCESS)
  {
    //if there was an error, then retry
    if (millis() - LastFetchForecastTime > 1000 * 60)
    {
      WantForecast = true;
      weather.BeginFetchForecast();
      LastFetchForecastTime = millis();
    }
  }
#ifdef DO_NETWORK
  events(); //for ezTime
  CheckOnForecast();

  time_t now = myTZ.now();
  uint8_t hour = myTZ.hour(now);
  uint8_t minute = myTZ.minute(now);
  //uint8_t second = myTZ.second(now);

  timePanel.SetTime(hour, minute);
  delay(100);

  return;

#else
`static unsigned char digit = 0;
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
