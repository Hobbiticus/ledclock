#include "Weather.h"
#include <Arduino.h>
#include <WiFi.h>
#include "passwd.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define USER_AGENT "{My_Clock, " MY_EMAIL "}"

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
    Serial.println("GRIDPOINTS:");
    Serial.println(payload);
    DynamicJsonDocument doc(8192);
    // JsonDocument doc(8192);
    DeserializationError error = deserializeJson(doc, payload);
    if (error)
    {
        Serial.printf("Failed to parse gridpoints: %s\n", error.c_str());
        return FAILED_PARSE_GRIDPOINTS;
    }

    m_Office = doc["properties"]["gridId"].as<String>();
    m_GridX = doc["properties"]["gridX"].as<int>();
    m_GridY = doc["properties"]["gridY"].as<int>();
    m_ForecastURL = doc["properties"]["forecast"].as<String>();

    return SUCCESS;
}

Weather::WeatherError Weather::GetForecast(Forecast& forecast)
{
    if (m_ForecastURL.isEmpty())
    {
        Weather::WeatherError err = FetchGridpoints();
        if (err != SUCCESS)
            return err;
    }

    HTTPClient http;
    http.begin(m_ForecastURL);
    http.addHeader("User-Agent", USER_AGENT);
    http.addHeader("Accept", "application/geo+json");

    int httpResponseCode = http.GET();
    if (httpResponseCode <= 0)
    {
        Serial.printf("Failed to fetch forecast URL %s : %d\n", m_ForecastURL, httpResponseCode);
        return FAILED_FETCH_FORECAST;
    }

    String payload = http.getString();
    Serial.println(payload);
    DynamicJsonDocument doc(8192);
    // JsonDocument doc(8192);
    DeserializationError error = deserializeJson(doc, payload);
    if (error)
    {
        Serial.printf("Failed to parse forecast: %s\n", error.c_str());
        return FAILED_PARSE_FORECAST;
    }

    //make sure this is still where we want the forecast

    
    
    forecast.m_High = doc["properties"]["periods"][0]["temperature"].as<int>();
    forecast.m_Low = doc["properties"]["periods"][1]["temperature"].as<int>();

    return SUCCESS;
}