#include "Weather.h"
#include <Arduino.h>
#include <WiFi.h>
#include "passwd.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "weather.h"

#define USER_AGENT "{My_Clock, " MY_EMAIL "}"

bool FetchInProgress = false;
volatile bool FetchComplete = false;

Weather::Weather(double latitude, double longitude)
: m_Latitude(latitude), m_Longitude(longitude), m_GridX(0), m_GridY(0)
{

}

Weather::WeatherError Weather::FetchGridpoints()
{
    char url[256] = {0};
    snprintf(url, sizeof(url) - 1, "https://api.weather.gov/points/%f,%f", m_Latitude, m_Longitude);

    HTTPClient http;
    http.begin(url);
    http.addHeader("User-Agent", USER_AGENT);
    http.addHeader("Accept", "application/geo+json");

    int httpResponseCode = http.GET();
    if (httpResponseCode <= 0)
    {
        Serial.printf("Failed to query points URL %s : %d\n", url, httpResponseCode);
        return FAILED_FETCH_GRIDPOINTS;
    }

    String payload = http.getString();
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error)
    {
        Serial.printf("Failed to parse gridpoints: %s\n", error.c_str());
        Serial.println("==== Gridpoints ====");
        Serial.println(payload);
        Serial.println("==== END Gridpoints ====");
        return FAILED_PARSE_GRIDPOINTS;
    }

    m_Office = doc["properties"]["gridId"].as<String>();
    m_GridX = doc["properties"]["gridX"];
    m_GridY = doc["properties"]["gridY"];
    m_ForecastURL = doc["properties"]["forecast"].as<String>();

    return SUCCESS;
}

Weather::WeatherError Weather::FetchActualForecast(Forecast& forecast)
{
    HTTPClient http;
    http.begin(m_ForecastURL);
    http.addHeader("User-Agent", USER_AGENT);
    http.addHeader("Accept", "application/geo+json");

    int httpResponseCode = http.GET();
    if (httpResponseCode <= 0)
    {
        Serial.printf("Failed to fetch forecast URL %s : %d\n", m_ForecastURL.c_str(), httpResponseCode);
        return FAILED_FETCH_FORECAST;
    }

    String payload = http.getString();
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error)
    {
        Serial.printf("Failed to parse forecast: %s\n", error.c_str());
        Serial.println("==== Forecast JSON ====");
        Serial.println(payload);
        Serial.println("==== END Forecast JSON ====");
        return FAILED_PARSE_FORECAST;
    }
    Serial.println("Got forecast");

    //make sure this is still where we want the forecast
    JsonArray coords = doc["geometry"]["coordinates"];
    if (!m_Polygon.empty())
    {
        //is the polygon the same?
        for (int i = 0; i < coords.size(); i++)
        {
            LatLon pos(coords[i][0], coords[i][1]);
            if (m_Polygon.find(pos) == m_Polygon.end())
            {
                Serial.println("  !!!! !  ! ! !!  Geometry Changed  ! ! ! !!! !    ! ! ! !!");
                //not the same - fetch new gridpoints
                m_Polygon.clear();
                m_ForecastURL = "";
                return FAILED_GRIDPOINTS_CHANGED;
            }
        }
    }

    if (m_Polygon.empty())
    {
        for (int i = 0; i < coords.size(); i++)
        {
            LatLon pos(coords[i][0], coords[i][1]);
            m_Polygon.insert(pos);
        }
    }

    bool haveToday = false;
    JsonArray periods = doc["properties"]["periods"];
    for (int i = 0; i < periods.size(); i++)
    {
        String name = periods[i]["name"];
        Serial.printf("Period %d = \"%s\"\n", i, name.c_str());

        if (name == "This Afternoon" || name == "Today")
        {
            forecast.m_High = periods[i]["temperature"];
            forecast.m_RainChance = periods[i]["probabilityOfPrecipitation"]["value"];
            haveToday = true;
        }
        else if (name == "Tonight")
        {
            forecast.m_Low = periods[i]["temperature"];
            if (haveToday)
                break;
            //in the evening, the today/this-afternoon record goes away, so let's grab tomorrow's high
            //which should be the next record
            if (i + 1 < periods.size())
            {
                String nextname = periods[i+1]["name"];
                Serial.printf("Period %d = \"%s\"\n", i+1, nextname.c_str());
                forecast.m_High = periods[i+1]["temperature"];
                forecast.m_RainChance = periods[i+1]["probabilityOfPrecipitation"]["value"];
            }
            break;
        }
    }
    return SUCCESS;
}

Weather::WeatherError Weather::FetchForecast(Forecast& forecast)
{
    forecast.m_High = -999;
    forecast.m_Low = -999;
    forecast.m_RainChance = 0;

    while (m_ForecastURL.isEmpty())
    {
        Serial.println("Fetching gridpoints...");
        Weather::WeatherError err = FetchGridpoints();
        if (err != SUCCESS)
        {
            delay(5000);
            continue;
        }
        break;
    }

    while (true)
    {
        Serial.println("Fetching forecast...");
        Weather::WeatherError err = FetchActualForecast(forecast);
        if (err != SUCCESS)
        {
            delay(5000);
            continue;
        }

        Serial.printf("High = %d, low = %d, precip = %hhu\n", forecast.m_High, forecast.m_Low, forecast.m_RainChance);
        break;
    }

    return SUCCESS;
}

bool Weather::BeginFetchForecast()
{
    if (FetchInProgress)
        return true;
    //task, name, stack size, function, priority, handle, core
    //loop() happens on core 1, so pin this to core 0
    FetchComplete = false;
    //WARNING: need enough stack space to parse the json or core 0 will kill itself
    BaseType_t result = xTaskCreatePinnedToCore(sFetchTask, "FetchWeather", 8192, this, 1, NULL, 0);
    if (result != pdPASS)
        return false;
    FetchInProgress = true;
    return true;
}

bool Weather::IsFetchComplete()
{
    if (!FetchInProgress)
        return true;
    if (!FetchComplete)
        return false;
    FetchInProgress = false;
    return true;
}

void Weather::FetchTask()
{
    Serial.println("Entered fetch task");
    m_LastError = FetchForecast(m_CurrentForecast);
    Serial.println("Fetch task complete");
    FetchComplete = true;
    vTaskDelete(NULL);
}

const char* Weather::ErrorToString(WeatherError err) const
{
    switch (err)
    {
        case SUCCESS:
            return "Success";
        case FAILED_FETCH_GRIDPOINTS:
            return "Failed to fetch gridpoints";
        case FAILED_PARSE_GRIDPOINTS:
            return "Failed to parse gridpoints";
        case FAILED_FETCH_FORECAST:
            return "Failed to fetch forecast";
        case FAILED_PARSE_FORECAST:
            return "Failed to parse forecast";
        case FAILED_GRIDPOINTS_CHANGED:
            return "Gridpoints changed";
    }
    return "???";
}
