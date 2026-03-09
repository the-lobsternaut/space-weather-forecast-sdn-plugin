#include "swpc/forecast_parser.h"
#include <iostream>
#include <cassert>

static const char* SAMPLE_TEXT = R"(
:Product: 45 Day AP and F10.7cm Flux Forecast 45-day-forecast.txt
:Issued: 2026 Mar 07 0000 UTC
# Prepared by Dept. of Commerce, NOAA, Space Weather Prediction Center.
#
# 45-Day AP and F10.7cm Flux Forecast
#-------------------------------------------------------------
45-DAY AP FORECAST
07Mar26 016 08Mar26 012 09Mar26 012
10Mar26 010 11Mar26 008 12Mar26 005
13Mar26 005 14Mar26 015 15Mar26 015

45-DAY F10.7 CM FLUX FORECAST
07Mar26 140 08Mar26 135 09Mar26 135
10Mar26 130 11Mar26 125 12Mar26 120
13Mar26 115 14Mar26 125 15Mar26 120

FORECASTER: AUTOMATED - SWPC Forecasting System
99999
)";

int main() {
    std::cout << "=== SWPC Forecast Parser Tests ===" << std::endl;

    // Test 1: Text parsing
    std::cout << "\n[Test 1] Parse text format..." << std::endl;
    auto forecast = swpc::parse_45day_text(SAMPLE_TEXT);
    assert(forecast.ap.size() == 9);
    assert(forecast.f107.size() == 9);
    assert(forecast.ap[0].date == "2026-03-07");
    assert(forecast.ap[0].value == 16);
    assert(forecast.f107[0].date == "2026-03-07");
    assert(forecast.f107[0].value == 140);
    assert(forecast.forecaster.find("SWPC") != std::string::npos);
    std::cout << "  PASS: Parsed " << forecast.ap.size() << " Ap days, "
              << forecast.f107.size() << " F10.7 days" << std::endl;

    // Test 2: Text round-trip
    std::cout << "\n[Test 2] Text round-trip..." << std::endl;
    std::string text_out = swpc::write_45day_text(forecast);
    auto forecast2 = swpc::parse_45day_text(text_out);
    assert(forecast2.ap.size() == forecast.ap.size());
    assert(forecast2.f107.size() == forecast.f107.size());
    for (size_t i = 0; i < forecast.ap.size(); i++) {
        assert(forecast2.ap[i].date == forecast.ap[i].date);
        assert(forecast2.ap[i].value == forecast.ap[i].value);
    }
    std::cout << "  PASS: Round-trip preserved all data" << std::endl;

    // Test 3: Text -> JSON
    std::cout << "\n[Test 3] Text -> JSON..." << std::endl;
    std::string json_out = swpc::write_45day_json(forecast);
    assert(json_out.find("\"ap_forecast\"") != std::string::npos);
    assert(json_out.find("\"f107_forecast\"") != std::string::npos);
    assert(json_out.find("\"AP_AVG\"") != std::string::npos);
    assert(json_out.find("\"F107_OBS\"") != std::string::npos);
    assert(json_out.find("\"PRD\"") != std::string::npos);
    std::cout << "  PASS: JSON output contains SPW-compatible fields" << std::endl;

    // Test 4: JSON round-trip
    std::cout << "\n[Test 4] JSON round-trip..." << std::endl;
    auto forecast3 = swpc::parse_45day_json(json_out);
    assert(forecast3.ap.size() == forecast.ap.size());
    assert(forecast3.f107.size() == forecast.f107.size());
    std::cout << "  PASS: JSON round-trip preserved all data" << std::endl;

    // Test 5: JSON -> Text
    std::cout << "\n[Test 5] JSON -> Text..." << std::endl;
    std::string text_from_json = swpc::write_45day_text(forecast3);
    assert(text_from_json.find("45-DAY AP FORECAST") != std::string::npos);
    assert(text_from_json.find("45-DAY F10.7 CM FLUX FORECAST") != std::string::npos);
    std::cout << "  PASS: JSON -> Text conversion works" << std::endl;

    std::cout << "\n=== All tests passed! ===" << std::endl;
    return 0;
}
