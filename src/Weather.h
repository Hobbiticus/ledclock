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

    bool operator<(LatLon& rhs)
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

    WeatherError GetForecast(Forecast& forecast);

private:
    double m_Latitude;
    double m_Longitude;

    //populated by FetchGridpoints
    String m_Office;
    int m_GridX;
    int m_GridY;
    String m_ForecastURL;

    //current polygon served by the latest forecase
    std::set<LatLon> m_Polygon;

    WeatherError FetchGridpoints();
};
