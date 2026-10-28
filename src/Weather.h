#pragma once
#include <set>
#include <Arduino.h>

struct LatLon
{
    LatLon(double lat, double lon)
    : m_Lat(lat), m_Lon(lon)
    {}
    
    double m_Lat;
    double m_Lon;

    bool operator<(const LatLon& rhs) const
    {
        if (m_Lat < rhs.m_Lat)
            return true;
        if (m_Lat > rhs.m_Lat)
            return false;
        return m_Lon < rhs.m_Lon;
    }
};

struct Forecast
{
    int m_High;
    int m_Low;
    unsigned char m_RainChance;
};


class Weather
{
public:
    Weather(double latitude, double longitude);

    enum WeatherError
    {
        SUCCESS = 0,
        FAILED_FETCH_GRIDPOINTS,
        FAILED_PARSE_GRIDPOINTS,
        FAILED_FETCH_FORECAST,
        FAILED_PARSE_FORECAST,
    };

    bool BeginFetchForecast();
    bool IsFetchComplete();

    WeatherError FetchForecast(Forecast& forecast);
    WeatherError GetForecast(Forecast& forecast) const
    {
        forecast = m_CurrentForecast;
        return m_LastError;
    }
    WeatherError GetLastError() const { return m_LastError; }

private:
    double m_Latitude;
    double m_Longitude;

    //populated by FetchGridpoints
    String m_Office;
    int m_GridX;
    int m_GridY;
    String m_ForecastURL;

    Forecast m_CurrentForecast;
    WeatherError m_LastError;

    //current polygon served by the latest forecase
    std::set<LatLon> m_Polygon;

    WeatherError FetchGridpoints();

    static void sFetchTask(void* arg)
    {
        Weather* obj = (Weather*)arg;
        obj->FetchTask();
    }
    void FetchTask();
};
