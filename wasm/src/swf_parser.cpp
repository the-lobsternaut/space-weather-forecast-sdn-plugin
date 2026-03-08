/**
 * Space Weather Forecast WASM Parser
 * Compiles to WebAssembly for SDN integration
 */

#include <string>
#include <vector>
#include <sstream>
#include <map>
#include <algorithm>

// FlatBuffers header (auto-generated)
#include "swf_generated.h"

using namespace spacedatastandards;

/**
 * Parse NOAA SWPC text format to FlatBuffers
 */
extern "C" {

    /**
     * Parse NOAA text format and return FlatBuffers binary
     * @param text NOAA SWPC text format
     * @param output FlatBuffers binary output
     * @param output_size Size of output
     * @return 0 on success, -1 on error
     */
    int parse_noaa_forecast(
        const char* text,
        uint8_t** output,
        size_t* output_size
    ) {
        try {
            // Parse text format
            auto forecast = parse_noaa_text(std::string(text));

            // Check if parsing succeeded
            if (!forecast) {
                return -1;
            }

            // Build FlatBuffers
            flatbuffers::FlatBufferBuilder builder(1024);

            // Create Ap Forecast Days
            std::vector<flatbuffers::Offset<ApForecastDay>> ap_forecast;
            for (const auto& day : forecast->ap_forecast) {
                ap_forecast.push_back(
                    CreateApForecastDay(builder,
                        builder.CreateString(day.date),
                        static_cast<int32_t>(day.date_utc),
                        static_cast<int32_t>(day.ap_value),
                        builder.CreateString(day.ap_code),
                        builder.CreateString(day.forecast_status)
                    )
                );
            }

            // Create F107 Forecast Days
            std::vector<flatbuffers::Offset<F107ForecastDay>> f107_forecast;
            for (const auto& day : forecast->f107_forecast) {
                f107_forecast.push_back(
                    CreateF107ForecastDay(builder,
                        builder.CreateString(day.date),
                        static_cast<int32_t>(day.date_utc),
                        static_cast<int32_t>(day.f107_value),
                        builder.CreateString(day.f107_code),
                        builder.CreateString(day.forecast_status)
                    )
                );
            }

            // Create Space Weather Forecast
            auto swf = CreateSpaceWeatherForecast(
                builder,
                builder.CreateString(forecast->product_id),
                builder.CreateString(forecast->product_name),
                builder.CreateString(forecast->version),
                builder.CreateString(forecast->issued),
                static_cast<int64_t>(forecast->issued_utc),
                builder.CreateString(forecast->expires),
                static_cast<int64_t>(forecast->expires_utc),
                builder.CreateString(forecast->forecaster),
                static_cast<SpaceWeatherForecastType>(forecast->product_type),
                builder.CreateString(forecast->description),
                ap_forecast,
                static_cast<int32_t>(forecast->ap_current),
                static_cast<int32_t>(forecast->ap_max_24h),
                static_cast<int32_t>(forecast->ap_min_24h),
                builder.CreateString(forecast->ap_trend),
                f107_forecast,
                static_cast<int32_t>(forecast->f107_current),
                static_cast<int32_t>(forecast->f107_max_24h),
                static_cast<int32_t>(forecast->f107_min_24h),
                builder.CreateString(forecast->f107_trend),
                builder.CreateString(forecast->forecast_start_date),
                builder.CreateString(forecast->forecast_end_date),
                static_cast<int32_t>(forecast->forecast_duration_days),
                builder.CreateString(forecast->source),
                builder.CreateString(forecast->source_url),
                builder.CreateString(forecast->data_quality),
                static_cast<float>(forecast->confidence)
            );

            builder.Finish(swf);

            // Return binary
            *output = builder.GetBufferPointer();
            *output_size = builder.GetSize();

            return 0;

        } catch (const std::exception& e) {
            return -1;
        }
    }

    /**
     * Get forecast metadata
     * @param text NOAA SWPC text format
     * @param output_size Size of output string
     * @return JSON metadata string
     */
    const char* get_forecast_metadata(const char* text, size_t* output_size) {
        try {
            auto forecast = parse_noaa_text(std::string(text));

            if (!forecast) {
                *output_size = 0;
                return nullptr;
            }

            std::ostringstream metadata;
            metadata << "{";
            metadata << "\"product_id\":\"" << forecast->product_id << "\",";
            metadata << "\"product_name\":\"" << forecast->product_name << "\",";
            metadata << "\"version\":\"" << forecast->version << "\",";
            metadata << "\"issued\":\"" << forecast->issued << "\",";
            metadata << "\"product_type\":" << forecast->product_type << ",";
            metadata << "\"ap_current\":" << forecast->ap_current << ",";
            metadata << "\"f107_current\":" << forecast->f107_current << ",";
            metadata << "\"ap_trend\":\"" << forecast->ap_trend << "\",";
            metadata << "\"f107_trend\":\"" << forecast->f107_trend << ",";
            metadata << "\"forecast_duration_days\":" << forecast->forecast_duration_days;
            metadata << "}";

            // Store in static buffer for WASM
            static std::string result = metadata.str();
            result = metadata.str();

            *output_size = result.size();
            return result.c_str();

        } catch (const std::exception& e) {
            *output_size = 0;
            return nullptr;
        }
    }

    /**
     * Get forecast statistics
     * @param text NOAA SWPC text format
     * @param output_size Size of output string
     * @return JSON statistics string
     */
    const char* get_forecast_statistics(const char* text, size_t* output_size) {
        try {
            auto forecast = parse_noaa_text(std::string(text));

            if (!forecast) {
                *output_size = 0;
                return nullptr;
            }

            std::ostringstream stats;
            stats << "{";
            stats << "\"ap_max\":" << forecast->ap_current << ",";
            stats << "\"ap_min\":" << forecast->ap_current << ",";
            stats << "\"f107_max\":" << forecast->f107_current << ",";
            stats << "\"f107_min\":" << forecast->f107_current << ",";
            stats << "\"ap_range\":" << (forecast->ap_current - forecast->ap_current) << ",";
            stats << "\"f107_range\":" << (forecast->f107_current - forecast->f107_current) << ",";
            stats << "\"ap_trend\":\"" << forecast->ap_trend << "\",";
            stats << "\"f107_trend\":\"" << forecast->f107_trend << "\"";
            stats << "}";

            static std::string result = stats.str();
            result = stats.str();

            *output_size = result.size();
            return result.c_str();

        } catch (const std::exception& e) {
            *output_size = 0;
            return nullptr;
        }
    }

} // extern "C"
