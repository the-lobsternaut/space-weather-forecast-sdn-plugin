#include "swpc/forecast_parser.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <ctime>
#include <map>
#include <stdexcept>

namespace swpc {

// Month abbreviation -> number
static const std::map<std::string, int> MONTH_MAP = {
    {"Jan", 1}, {"Feb", 2}, {"Mar", 3}, {"Apr", 4},
    {"May", 5}, {"Jun", 6}, {"Jul", 7}, {"Aug", 8},
    {"Sep", 9}, {"Oct", 10}, {"Nov", 11}, {"Dec", 12}
};

// Trim whitespace
static std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

// Parse date code like "07Mar26" -> (year=2026, month=3, day=7)
static bool parse_date_code(const std::string& code, int& year, int& month, int& day) {
    if (code.size() < 7) return false;

    // Format: DDMmmYY (e.g., "07Mar26")
    std::string dd = code.substr(0, 2);
    std::string mmm = code.substr(2, 3);
    std::string yy = code.substr(5, 2);

    auto it = MONTH_MAP.find(mmm);
    if (it == MONTH_MAP.end()) return false;

    day = std::stoi(dd);
    month = it->second;
    year = 2000 + std::stoi(yy);
    return true;
}

// Format date as ISO 8601
static std::string format_date(int year, int month, int day) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d", year, month, day);
    return std::string(buf);
}

// Calculate unix timestamp from date
static int64_t to_timestamp(int year, int month, int day) {
    struct tm t = {};
    t.tm_year = year - 1900;
    t.tm_mon = month - 1;
    t.tm_mday = day;
    return static_cast<int64_t>(timegm(&t));
}

Forecast45Day parse_45day_text(const std::string& input) {
    Forecast45Day result;
    std::istringstream iss(input);
    std::string line;

    enum Section { NONE, AP_SECTION, F107_SECTION } section = NONE;

    while (std::getline(iss, line)) {
        std::string trimmed = trim(line);
        if (trimmed.empty()) continue;

        // Skip comments
        if (trimmed[0] == '#') continue;

        // Detect issued date
        if (trimmed.find(":Issued:") != std::string::npos) {
            auto pos = trimmed.find(":Issued:");
            result.issued = trim(trimmed.substr(pos + 8));
            continue;
        }

        // Detect section headers
        if (trimmed.find("45-DAY AP FORECAST") != std::string::npos) {
            section = AP_SECTION;
            continue;
        }
        if (trimmed.find("45-DAY F10.7 CM FLUX FORECAST") != std::string::npos) {
            section = F107_SECTION;
            continue;
        }

        // Detect forecaster
        if (trimmed.find("FORECASTER:") != std::string::npos) {
            auto pos = trimmed.find("FORECASTER:");
            result.forecaster = trim(trimmed.substr(pos + 11));
            continue;
        }

        // Skip non-data lines
        if (trimmed.find("99999") != std::string::npos) continue;
        if (trimmed[0] == ':' || trimmed[0] == '-') continue;

        // Parse data lines: "DDMmmYY VVV DDMmmYY VVV ..."
        if (section == NONE) continue;

        std::istringstream linestream(trimmed);
        std::string token;
        std::vector<std::string> tokens;

        while (linestream >> token) {
            tokens.push_back(token);
        }

        // Tokens come in pairs: date_code value
        for (size_t i = 0; i + 1 < tokens.size(); i += 2) {
            int year, month, day;
            if (!parse_date_code(tokens[i], year, month, day)) continue;

            int value;
            try {
                value = std::stoi(tokens[i + 1]);
            } catch (...) {
                continue;
            }

            ForecastDay fd;
            fd.date = format_date(year, month, day);
            fd.date_utc = to_timestamp(year, month, day);
            fd.value = value;

            if (section == AP_SECTION) {
                result.ap.push_back(fd);
            } else if (section == F107_SECTION) {
                result.f107.push_back(fd);
            }
        }
    }

    return result;
}

std::string write_45day_text(const Forecast45Day& forecast) {
    std::ostringstream out;

    out << ":Product: 45 Day AP and F10.7cm Flux Forecast\n";
    out << ":Issued: " << forecast.issued << "\n";
    out << "# Prepared by Dept. of Commerce, NOAA, Space Weather Prediction Center.\n";
    out << "#\n";
    out << "45-DAY AP FORECAST\n";

    for (const auto& day : forecast.ap) {
        // Convert ISO date back to DDMmmYY
        int y = std::stoi(day.date.substr(0, 4));
        int m = std::stoi(day.date.substr(5, 2));
        int d = std::stoi(day.date.substr(8, 2));

        static const char* months[] = {
            "", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
        };

        char buf[16];
        snprintf(buf, sizeof(buf), "%02d%s%02d %03d",
                 d, months[m], y % 100, day.value);
        out << buf << "\n";
    }

    out << "\n45-DAY F10.7 CM FLUX FORECAST\n";

    for (const auto& day : forecast.f107) {
        int y = std::stoi(day.date.substr(0, 4));
        int m = std::stoi(day.date.substr(5, 2));
        int d = std::stoi(day.date.substr(8, 2));

        static const char* months[] = {
            "", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
        };

        char buf[16];
        snprintf(buf, sizeof(buf), "%02d%s%02d %03d",
                 d, months[m], y % 100, day.value);
        out << buf << "\n";
    }

    out << "FORECASTER: " << forecast.forecaster << "\n";
    out << "99999\n";

    return out.str();
}

std::string write_45day_json(const Forecast45Day& forecast) {
    std::ostringstream out;

    out << "{\n";
    out << "  \"product\": \"45-Day Ap and F10.7cm Flux Forecast\",\n";
    out << "  \"issued\": \"" << forecast.issued << "\",\n";
    out << "  \"forecaster\": \"" << forecast.forecaster << "\",\n";

    // Ap forecast
    out << "  \"ap_forecast\": [\n";
    for (size_t i = 0; i < forecast.ap.size(); i++) {
        const auto& d = forecast.ap[i];
        out << "    {\"DATE\": \"" << d.date << "\", \"AP_AVG\": " << d.value
            << ", \"F107_DATA_TYPE\": \"PRD\"}";
        if (i + 1 < forecast.ap.size()) out << ",";
        out << "\n";
    }
    out << "  ],\n";

    // F10.7 forecast
    out << "  \"f107_forecast\": [\n";
    for (size_t i = 0; i < forecast.f107.size(); i++) {
        const auto& d = forecast.f107[i];
        out << "    {\"DATE\": \"" << d.date << "\", \"F107_OBS\": " << d.value
            << ", \"F107_DATA_TYPE\": \"PRD\"}";
        if (i + 1 < forecast.f107.size()) out << ",";
        out << "\n";
    }
    out << "  ]\n";
    out << "}\n";

    return out.str();
}

Forecast45Day parse_45day_json(const std::string& input) {
    // Minimal JSON parser for the NOAA SWPC JSON format
    Forecast45Day result;

    // Extract issued
    auto pos = input.find("\"issued\"");
    if (pos != std::string::npos) {
        auto start = input.find("\"", pos + 8) + 1;
        auto end = input.find("\"", start);
        result.issued = input.substr(start, end - start);
    }

    // Extract forecaster
    pos = input.find("\"forecaster\"");
    if (pos != std::string::npos) {
        auto start = input.find("\"", pos + 12) + 1;
        auto end = input.find("\"", start);
        result.forecaster = input.substr(start, end - start);
    }

    // Parse ap_forecast array entries
    pos = input.find("\"ap_forecast\"");
    if (pos != std::string::npos) {
        size_t cursor = pos;
        while ((cursor = input.find("\"DATE\"", cursor)) != std::string::npos) {
            // Check if we're still in ap_forecast section
            auto section_end = input.find("\"f107_forecast\"", pos);
            if (section_end != std::string::npos && cursor > section_end) break;

            auto date_start = input.find("\"", cursor + 6) + 1;
            auto date_end = input.find("\"", date_start);
            std::string date = input.substr(date_start, date_end - date_start);

            auto ap_pos = input.find("\"AP_AVG\"", cursor);
            if (ap_pos != std::string::npos && ap_pos < cursor + 200) {
                auto val_start = input.find(":", ap_pos) + 1;
                while (val_start < input.size() && input[val_start] == ' ') val_start++;
                auto val_end = input.find_first_of(",}", val_start);
                int value = std::stoi(input.substr(val_start, val_end - val_start));

                ForecastDay fd;
                fd.date = date;
                fd.value = value;
                result.ap.push_back(fd);
            }
            cursor = date_end + 1;
        }
    }

    // Parse f107_forecast array entries
    pos = input.find("\"f107_forecast\"");
    if (pos != std::string::npos) {
        size_t cursor = pos;
        while ((cursor = input.find("\"DATE\"", cursor)) != std::string::npos) {
            auto date_start = input.find("\"", cursor + 6) + 1;
            auto date_end = input.find("\"", date_start);
            std::string date = input.substr(date_start, date_end - date_start);

            auto f107_pos = input.find("\"F107_OBS\"", cursor);
            if (f107_pos != std::string::npos && f107_pos < cursor + 200) {
                auto val_start = input.find(":", f107_pos) + 1;
                while (val_start < input.size() && input[val_start] == ' ') val_start++;
                auto val_end = input.find_first_of(",}", val_start);
                int value = std::stoi(input.substr(val_start, val_end - val_start));

                ForecastDay fd;
                fd.date = date;
                fd.value = value;
                result.f107.push_back(fd);
            }
            cursor = date_end + 1;
        }
    }

    return result;
}

} // namespace swpc
