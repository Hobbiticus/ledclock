#include <Arduino.h>
#include <WiFi.h>
#include <ezTime.h>
#include "passwd.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

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

#define LATCH_PIN 25
#define ENABLE_PIN 33
#define DATA_PIN 27
#define CLOCK_PIN 26

#define LATCH_PIN2 18
#define ENABLE_PIN2 19
#define DATA_PIN2 16
#define CLOCK_PIN2 17

#define COLON_PIN 14

const static unsigned char STC = 1 << 5; //top center
const static unsigned char STL = 1 << 6; //top left
const static unsigned char STR = 1 << 4; //top right
const static unsigned char SMC = 1 << 2; //middle center
const static unsigned char SBL = 1 << 7; //bottom left
const static unsigned char SBR = 1 << 1; //bottom right
const static unsigned char SBC = 1 << 3; //bottom center
const static unsigned char DOT = 1 << 0; //decimal point

const static unsigned char STC2 = 1 << 1; //top center
const static unsigned char STL2 = 1 << 0; //top left
const static unsigned char STR2 = 1 << 2; //top right
const static unsigned char SMC2 = 1 << 3; //middle center
const static unsigned char SBL2 = 1 << 7; //bottom left
const static unsigned char SBR2 = 1 << 4; //bottom right
const static unsigned char SBC2 = 1 << 6; //bottom center
const static unsigned char DOT2 = 1 << 5; //decimal point

const static unsigned char NumberMap[10] =
{
STC | STL | STR | SBL | SBR | SBC,       //0
STR | SBR,                               //1
STC | STR | SMC | SBL | SBC,             //2
STC | STR | SMC | SBR | SBC,             //3
STL | STR | SMC | SBR,                   //4
STC | STL | SMC | SBR | SBC,             //5
STC | STL | SMC | SBL | SBR | SBC,       //6
STC | STR | SBR,                         //7
STC | STL | STR | SMC | SBL | SBR | SBC, //8
STC | STL | STR | SMC | SBR | SBC        //9
};

const static unsigned char NumberMap2[10] =
{
STC2 | STL2 | STR2 | SBL2 | SBR2 | SBC2,        //0
STR2 | SBR2,                                    //1
STC2 | STR2 | SMC2 | SBL2 | SBC2,               //2
STC2 | STR2 | SMC2 | SBR2 | SBC2,               //3
STL2 | STR2 | SMC2 | SBR2,                      //4
STC2 | STL2 | SMC2 | SBR2 | SBC2,               //5
STC2 | STL2 | SMC2 | SBL2 | SBR2 | SBC2,        //6
STC2 | STR2 | SBR2,                             //7
STC2 | STL2 | STR2 | SMC2 | SBL2 | SBR2 | SBC2, //8
STC2 | STL2 | STR2 | SMC2 | SBR2 | SBC2         //9
};

//PWM channels
const int ClockChannel = 0;
const int ColonChannel = 1;
const int TempChannel = 2;

unsigned char digit = 0;

#define USER_AGENT "{My_Clock, " MY_EMAIL "}"

String GetForecastURL()
{
  char url[256] = {0};
  snprintf(url, sizeof(url) - 1, "https://api.weather.gov/points/%f,%f", LATITUDE, LONGITUDE);

  HTTPClient http;
  http.begin(url);
  http.addHeader("User-Agent", USER_AGENT);
  http.addHeader("Accept", "application/geo+json");

  int httpResponseCode = http.GET();
  if (httpResponseCode > 0)
  {
    String payload = http.getString();
    Serial.println(payload);
    DynamicJsonDocument doc(8192);
    //JsonDocument doc(8192);
    DeserializationError error = deserializeJson(doc, payload);

    if (!error)
      return doc["properties"]["forecast"].as<String>();
    else
    {
      Serial.printf("Error deserializing points data: %s", error.c_str());
    }
  }
  return "";
}

void GetForecast(String URL, int& high, int& low)
{
  HTTPClient http;
  http.begin(URL);
  http.addHeader("User-Agent", USER_AGENT);
  http.addHeader("Accept", "application/geo+json");

  int httpResponseCode = http.GET();
  if (httpResponseCode > 0)
  {
    String payload = http.getString();
    Serial.println(payload);
    DynamicJsonDocument doc(8192);
    //JsonDocument doc(8192);
    DeserializationError error = deserializeJson(doc, payload);

    if (!error)
    {
      doc["properties"]["periods"][0]["temperature"].as<String>();
    }
  }
}

void setup()
{
  Serial.begin(115200);

  //NOTE: pwm pins do not need to be set to OUTPUT mode
  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);

  pinMode(DATA_PIN2, OUTPUT);
  pinMode(CLOCK_PIN2, OUTPUT);
  pinMode(LATCH_PIN2, OUTPUT);

  const int freq = 5000;
  const int resolution = 8; // 8-bit (0-255)

  ledcSetup(ClockChannel, freq, resolution);
  ledcAttachPin(ENABLE_PIN, ClockChannel);
  ledcSetup(ColonChannel, freq, resolution);
  ledcAttachPin(ENABLE_PIN2, ColonChannel);
  ledcSetup(TempChannel, freq, resolution);
  ledcAttachPin(COLON_PIN, TempChannel);

  ledcWrite(ClockChannel, 20);
  ledcWrite(ColonChannel, 20);

  digit = 0;

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

  Serial.println("Waiting for time sync...");
  int startTime = millis();
  waitForSync();
  Serial.printf("Sync took %d ms\n", (int)(millis() - startTime));
  myTZ.setLocation("America/New_York");

#endif
}

void safeShiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t val) {
  for (int i = 0; i < 8; i++)  {
    if (bitOrder == LSBFIRST) {
      digitalWrite(dataPin, !!(val & (1 << i)));
    } else {
      digitalWrite(dataPin, !!(val & (1 << (7 - i))));
    }
    
    digitalWrite(clockPin, HIGH);
    delayMicroseconds(20);  // Give the clock edge time to stabilize
    digitalWrite(clockPin, LOW);
    delayMicroseconds(20);
  }
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
  digitalWrite(LATCH_PIN, LOW);
  digitalWrite(CLOCK_PIN, LOW);
  safeShiftOut(DATA_PIN, CLOCK_PIN, LSBFIRST, NumberMap[second / 10] | dots);
  safeShiftOut(DATA_PIN, CLOCK_PIN, LSBFIRST, NumberMap[second % 10] | dots);
  digitalWrite(LATCH_PIN, HIGH);
  delay(250);
  return;

#else
  Serial.printf("Showing %hhu, %02hhx, %02hhx\n", digit, NumberMap[digit], NumberMap2[digit]);
  //digitalWrite(COLON_PIN, (digit % 2 == 0) ? HIGH : LOW);
  ledcWrite(TempChannel, digit % 2 == 0 ? 255 : 20);

  ledcWrite(ClockChannel, 187);

  digitalWrite(CLOCK_PIN, LOW);
  //delayMicroseconds(1);
  digitalWrite(LATCH_PIN, LOW);
  //delayMicroseconds(1);

  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, NumberMap[digit]);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, NumberMap[digit]);
  //safeShiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, NumberMap[digit] | dots);
  //safeShiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, NumberMap[digit] | dots);

  // Explicitly pull clock low after shifting finishes, then latch
  digitalWrite(CLOCK_PIN, LOW);
  //delayMicroseconds(1);
  digitalWrite(LATCH_PIN, HIGH);

  ledcWrite(ColonChannel, (digit % 2 == 0) ? 187 : 187);

  digitalWrite(CLOCK_PIN2, LOW);
  //delayMicroseconds(1);
  digitalWrite(LATCH_PIN2, LOW);
  //delayMicroseconds(1);

  shiftOut(DATA_PIN2, CLOCK_PIN2, MSBFIRST, NumberMap2[digit]);
  //safeShiftOut(DATA_PIN2, CLOCK_PIN2, MSBFIRST, NumberMap2[digit]);

  // Explicitly pull clock low after shifting finishes, then latch
  digitalWrite(CLOCK_PIN2, LOW);
  //delayMicroseconds(1);
  digitalWrite(LATCH_PIN2, HIGH);



  digit++;
  if (digit > 9)
    digit = 0;

  delay(1000);
#endif
}
