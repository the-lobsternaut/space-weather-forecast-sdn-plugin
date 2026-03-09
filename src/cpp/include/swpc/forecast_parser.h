#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace swpc {

/// A single day's forecast entry
struct ForecastDay {
    std::string date;       // ISO 8601 date (YYYY-MM-DD)
    int64_t date_utc;       // Unix timestamp
    int value;              // Forecast value (Ap or F10.7)
};

/// Parsed 45-day forecast product
struct Forecast45Day {
    std::string issued;          // ISO 8601 issued timestamp
    std::string forecaster;      // Forecaster name/system
    std::vector<ForecastDay> ap; // Daily Ap forecast
    std::vector<ForecastDay> f107; // Daily F10.7 forecast
};

/// Parse NOAA SWPC 45-day text format into structured data
Forecast45Day parse_45day_text(const std::string& input);

/// Write structured data back to NOAA text format
std::string write_45day_text(const Forecast45Day& forecast);

/// Convert parsed forecast to JSON string (SPW-compatible)
std::string write_45day_json(const Forecast45Day& forecast);

/// Parse the new NOAA SWPC JSON format
Forecast45Day parse_45day_json(const std::string& input);

} // namespace swpc
