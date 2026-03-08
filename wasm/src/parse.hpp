/**
 * NOAA SWPC Text Format Parser Header
 */

#pragma once

#include <string>
#include <vector>

// Forward declaration
struct SpaceWeatherForecast {
    std::string product_id;
    std::string product_name;
    std::string version;
    std::string issued;
    int64_t issued_utc;
    std::string expires;
    int64_t expires_utc;
    std::string forecaster;
    int product_type;
    std::string description;
    std::vector<struct {
        std::string date;
        int64_t date_utc;
        int value;
        std::string code;
        std::string status;
    }}> ap_forecast;
    std::vector<struct {
        std::string date;
        int64_t date_utc;
        int value;
        std::string code;
        std::string status;
    }}> f107_forecast;
    std::string ap_trend;
    std::string f107_trend;
    std::string forecast_start_date;
    std::string forecast_end_date;
    int forecast_duration_days;
    std::string source;
    std::string source_url;
    std::string data_quality;
    float confidence;
};

// Enum for forecast types
enum SpaceWeatherForecastType {
    AP_F107_45DAY = 0,
    AP_F107_27DAY = 1,
    AP_3DAY = 2,
    F107_3DAY = 3,
    GEOMAGNETIC = 4,
    SOLAR_FLUX = 5,
    AURORA = 6,
    OTHER = 7
};

/**
 * Parse NOAA text format
 * @param text NOAA SWPC text format
 * @return Pointer to SpaceWeatherForecast structure
 */
SpaceWeatherForecast* parse_noaa_text(const std::string& text);
