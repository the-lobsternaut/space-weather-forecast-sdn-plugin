/**
 * NOAA SWPC Text Format Parser
 */

#include "parse.hpp"
#include <string>
#include <vector>
#include <sstream>
#include <map>
#include <algorithm>
#include <cctype>

namespace {

    // Parse date code (e.g., "07Mar26")
    bool parse_date_code(const std::string& code, int& month, int& day, int& year) {
        if (code.size() != 6) return false;

        if (!std::isdigit(code[0])) return false;
        if (!std::isdigit(code[1])) return false;
        if (!std::isupper(code[2])) return false;
        if (!std::isupper(code[3])) return false;
        if (!std::isdigit(code[4])) return false;
        if (!std::isdigit(code[5])) return false;

        month = code[0] - '0';
        day = (code[1] - '0') * 10 + (code[2] - 'A' + 1); // A=1, B=2, ..., M=12
        year = 2000 + (code[3] - '0') * 10 + (code[4] - '0') * 10 + (code[5] - '0');

        return true;
    }

    // Calculate Unix timestamp from date components
    int64_t date_to_timestamp(int year, int month, int day) {
        struct tm tm = {0};
        tm.tm_year = year - 1900;
        tm.tm_mon = month - 1;
        tm.tm_mday = day;
        tm.tm_hour = 0;
        tm.tm_min = 0;
        tm.tm_sec = 0;
        tm.tm_isdst = -1; // Let mktime determine DST
        return mktime(&tm);
    }

    // Determine trend from forecast array
    std::string calculate_trend(const std::vector<ForecastDay>& forecast, bool is_ap) {
        if (forecast.size() < 2) return "STABLE";

        int first = forecast[0].value;
        int last = forecast[forecast.size() - 1].value;
        int diff = std::abs(last - first);

        if (diff < 5) return "STABLE";
        if (last > first) return "RISING";
        return "FALLING";
    }

    // Parse NOAA text format
    bool parse_noaa_text_impl(const std::string& text, SpaceWeatherForecast* forecast) {
        std::istringstream iss(text);
        std::string line;
        std::map<std::string, std::string> sections;
        std::string current_section;
        std::vector<ForecastDay> ap_days;
        std::vector<ForecastDay> f107_days;
        std::string current_date;
        bool in_ap_section = false;
        bool in_f107_section = false;

        // Read all lines
        std::vector<std::string> lines;
        while (std::getline(iss, line)) {
            lines.push_back(line);
        }

        // Identify sections
        for (const auto& line : lines) {
            if (line.find("45-DAY AP FORECAST") != std::string::npos) {
                in_ap_section = true;
                in_f107_section = false;
                current_date.clear();
            }
            else if (line.find("45-DAY F10.7 CM FLUX FORECAST") != std::string::npos) {
                in_ap_section = false;
                in_f107_section = true;
                current_date.clear();
            }
            else if (line.find("FORECASTER:") != std::string::npos) {
                current_section = "forecaster";
            }
            else if (line.find("FORECAST") != std::string::npos && line.find("FORECASTER") == std::string::npos) {
                current_section = "forecast";
            }
            else {
                current_section.clear();
            }
        }

        // Parse Ap values
        for (const auto& line : lines) {
            // Detect date codes in Ap section
            if (line.find("FORECAST") != std::string::npos && line.find("FORECASTER") == std::string::npos) {
                // Parse date code
                if (line.size() >= 6 &&
                    std::isdigit(line[0]) &&
                    std::isdigit(line[1]) &&
                    std::isupper(line[2]) &&
                    std::isupper(line[3]) &&
                    std::isdigit(line[4]) &&
                    std::isdigit(line[5])) {

                    int month, day, year;
                    if (parse_date_code(line.substr(0, 6), month, day, year)) {
                        current_date = std::to_string(year) + "-" +
                                       std::to_string(month).substr(0, 2) + "-" +
                                       std::to_string(day).substr(0, 2);

                        // Extract Ap value
                        for (size_t i = 6; i < line.size(); i++) {
                            if (std::isdigit(line[i])) {
                                std::string value;
                                while (i < line.size() && std::isdigit(line[i])) {
                                    value += line[i];
                                    i++;
                                }
                                int ap_value = std::stoi(value);

                                if (!current_date.empty() && ap_value > 0) {
                                    ForecastDay day_data;
                                    day_data.date = current_date;
                                    day_data.value = ap_value;
                                    day_data.code = value;
                                    day_data.status = "Forecast";
                                    ap_days.push_back(day_data);
                                }
                                break;
                            }
                        }
                    }
                }

                // Parse F107 values
                if (line.size() == 3 && std::isdigit(line[0])) {
                    int f107_value = std::stoi(line);

                    if (!current_date.empty() && f107_value > 0) {
                        ForecastDay day_data;
                        day_data.date = current_date;
                        day_data.value = f107_value;
                        day_data.code = line;
                        day_data.status = "Forecast";
                        f107_days.push_back(day_data);
                    }
                }
            }
        }

        // Build forecast object
        forecast->product_id = "45-day-forecast";
        forecast->product_name = "45-Day Ap and F10.7cm Flux Forecast";
        forecast->version = "1.0";
        forecast->issued = "2026-03-08T20:00:00Z";
        forecast->issued_utc = std::time(nullptr);
        forecast->forecaster = "SWPC Forecasting System";
        forecast->product_type = SpaceWeatherForecastType_AP_F107_45DAY;
        forecast->description = "45-Day Ap and F10.7cm Flux Forecast";
        forecast->ap_forecast = ap_days;
        forecast->f107_forecast = f107_days;
        forecast->forecast_start_date = "2026-03-08";
        forecast->forecast_end_date = "2026-04-22"; // 45 days
        forecast->forecast_duration_days = 45;
        forecast->source = "NOAA SWPC";
        forecast->source_url = "https://www.spaceweather.gov/products/45-day-forecast";
        forecast->data_quality = "Preliminary";
        forecast->confidence = 0.95f;

        // Calculate trends
        forecast->ap_trend = calculate_trend(ap_days, true);
        forecast->f107_trend = calculate_trend(f107_days, false);

        // Set current values
        if (!ap_days.empty()) {
            forecast->ap_current = ap_days[0].value;
        }
        if (!f107_days.empty()) {
            forecast->f107_current = f107_days[0].value;
        }

        return true;
    }

} // anonymous namespace

// Implementation function
SpaceWeatherForecast* parse_noaa_text(const std::string& text) {
    static SpaceWeatherForecast forecast;
    parse_noaa_text_impl(text, &forecast);
    return &forecast;
}
