/**
 * SWPC Forecast WASM API
 *
 * Parses NOAA SWPC 45-day forecast text → FlatBuffers SPW aligned binary.
 * Also supports text ↔ JSON round-trips for convenience.
 *
 * SDN Plugin ABI: exports parse/convert via extern "C" + EMSCRIPTEN_KEEPALIVE.
 * Embind API: exports all functions for Node.js/browser usage.
 */

#include "swpc/forecast_parser.h"

// FlatBuffers SPW generated header (self-contained, no other schema deps)
#include "main_generated.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/bind.h>
using namespace emscripten;
#endif

#include <string>
#include <vector>
#include <cstring>
#include <stdexcept>

// ============================================================================
// Format Detection
// ============================================================================

static std::string detect_format(const std::string& input) {
    for (size_t i = 0; i < input.size(); i++) {
        char c = input[i];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') continue;
        if (c == '{' || c == '[') return "json";
        return "text";
    }
    return "text";
}

// ============================================================================
// Text ↔ JSON round-trips (convenience, no FlatBuffers)
// ============================================================================

static std::string swpc_text_roundtrip(const std::string& input) {
    auto forecast = swpc::parse_45day_text(input);
    return swpc::write_45day_text(forecast);
}

static std::string swpc_text_to_json(const std::string& input) {
    auto forecast = swpc::parse_45day_text(input);
    return swpc::write_45day_json(forecast);
}

static std::string swpc_json_to_text(const std::string& input) {
    auto forecast = swpc::parse_45day_json(input);
    return swpc::write_45day_text(forecast);
}

static std::string swpc_json_roundtrip(const std::string& input) {
    auto forecast = swpc::parse_45day_json(input);
    return swpc::write_45day_json(forecast);
}

static std::string convert_str(const std::string& input, const std::string& target_format) {
    std::string fmt = detect_format(input);
    if (fmt == "text" && target_format == "text") return swpc_text_roundtrip(input);
    if (fmt == "text" && target_format == "json") return swpc_text_to_json(input);
    if (fmt == "json" && target_format == "text") return swpc_json_to_text(input);
    if (fmt == "json" && target_format == "json") return swpc_json_roundtrip(input);
    throw std::runtime_error("Unsupported format: " + fmt + " -> " + target_format);
}

// ============================================================================
// FlatBuffers SPW Binary Output (Aligned Binary Format)
// ============================================================================

/**
 * Build an SPWCOLLECTION FlatBuffer from a parsed 45-day forecast.
 *
 * Each forecast day becomes one SPW record in the collection.
 * The Ap and F10.7 forecasts are merged by date: for each day,
 * AP_AVG gets the Ap value and F107_OBS/F107_ADJ get the F10.7 value.
 * F107_DATA_TYPE is set to PRD (45-day predicted).
 *
 * Returns the raw FlatBuffers buffer bytes (aligned, zero-copy ready).
 */
static std::string build_spw_collection(const swpc::Forecast45Day& forecast) {
    flatbuffers::FlatBufferBuilder fbb(4096);

    // Build SPW records — one per forecast day
    // Merge Ap and F10.7 by index (same dates in the NOAA product)
    size_t count = std::min(forecast.ap.size(), forecast.f107.size());
    std::vector<flatbuffers::Offset<SPW>> records;
    records.reserve(count);

    for (size_t i = 0; i < count; i++) {
        const auto& ap_day = forecast.ap[i];
        const auto& f107_day = forecast.f107[i];

        auto date_offset = fbb.CreateString(ap_day.date);

        auto spw = CreateSPW(
            fbb,
            date_offset,
            0,                                      // BSRN (not in 45-day forecast)
            0,                                      // ND
            0, 0, 0, 0, 0, 0, 0, 0,                // KP1-KP8 (not available)
            0,                                      // KP_SUM
            0, 0, 0, 0, 0, 0, 0, 0,                // AP1-AP8 (individual not available)
            ap_day.value,                            // AP_AVG
            0.0f,                                    // CP
            0,                                       // C9
            0,                                       // ISN
            static_cast<float>(f107_day.value),      // F107_OBS
            static_cast<float>(f107_day.value),      // F107_ADJ (same as OBS for forecast)
            F107DataType_PRD,                        // F107_DATA_TYPE = 45-day predicted
            0.0f,                                    // F107_OBS_CENTER81
            0.0f,                                    // F107_OBS_LAST81
            0.0f,                                    // F107_ADJ_CENTER81
            0.0f                                     // F107_ADJ_LAST81
        );
        records.push_back(spw);
    }

    // If there are more Ap days than F10.7 days, add Ap-only records
    for (size_t i = count; i < forecast.ap.size(); i++) {
        const auto& ap_day = forecast.ap[i];
        auto date_offset = fbb.CreateString(ap_day.date);
        auto spw = CreateSPW(fbb, date_offset,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,
            ap_day.value,
            0.0f, 0, 0,
            0.0f, 0.0f, F107DataType_PRD,
            0.0f, 0.0f, 0.0f, 0.0f);
        records.push_back(spw);
    }

    // If there are more F10.7 days than Ap days, add F10.7-only records
    for (size_t i = count; i < forecast.f107.size(); i++) {
        const auto& f107_day = forecast.f107[i];
        auto date_offset = fbb.CreateString(f107_day.date);
        auto spw = CreateSPW(fbb, date_offset,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,
            0,
            0.0f, 0, 0,
            static_cast<float>(f107_day.value),
            static_cast<float>(f107_day.value),
            F107DataType_PRD,
            0.0f, 0.0f, 0.0f, 0.0f);
        records.push_back(spw);
    }

    auto records_vec = fbb.CreateVector(records);
    auto collection = CreateSPWCOLLECTION(fbb, records_vec);

    // Finish with $SPW file identifier for SDN compatibility
    fbb.Finish(collection, "$SPW");

    // Return the aligned buffer as a binary string
    const uint8_t* buf = fbb.GetBufferPointer();
    size_t size = fbb.GetSize();
    return std::string(reinterpret_cast<const char*>(buf), size);
}

/**
 * Parse NOAA SWPC 45-day text → FlatBuffers SPWCOLLECTION binary.
 * Returns aligned binary bytes.
 */
static std::string swpc_text_to_flatbuffers(const std::string& input) {
    auto forecast = swpc::parse_45day_text(input);
    if (forecast.ap.empty() && forecast.f107.empty()) {
        throw std::runtime_error("No valid forecast data found in input");
    }
    return build_spw_collection(forecast);
}

/**
 * Parse NOAA SWPC JSON → FlatBuffers SPWCOLLECTION binary.
 * Returns aligned binary bytes.
 */
static std::string swpc_json_to_flatbuffers(const std::string& input) {
    auto forecast = swpc::parse_45day_json(input);
    if (forecast.ap.empty() && forecast.f107.empty()) {
        throw std::runtime_error("No valid forecast data found in input");
    }
    return build_spw_collection(forecast);
}

/**
 * Auto-detect format and convert to FlatBuffers SPWCOLLECTION binary.
 */
static std::string to_flatbuffers(const std::string& input) {
    std::string fmt = detect_format(input);
    if (fmt == "json") return swpc_json_to_flatbuffers(input);
    return swpc_text_to_flatbuffers(input);
}

// ============================================================================
// SDN Plugin ABI: extern "C" exports
// ============================================================================

#ifdef __EMSCRIPTEN__

extern "C" {

/**
 * Parse NOAA text/JSON → FlatBuffers SPW aligned binary.
 *
 * @param input      Pointer to input text (NOAA text or JSON)
 * @param input_len  Length of input
 * @param output     Pointer to output buffer (caller-allocated)
 * @param output_len Size of output buffer
 * @return           Bytes written on success, negative error code on failure
 *                   -1 = generic error, -2 = buffer too small, -3 = invalid input
 */
EMSCRIPTEN_KEEPALIVE
int32_t parse(const char* input, size_t input_len,
              uint8_t* output, size_t output_len) {
    try {
        std::string text(input, input_len);
        std::string fb = to_flatbuffers(text);

        if (fb.size() > output_len) return -2; // buffer too small

        std::memcpy(output, fb.data(), fb.size());
        return static_cast<int32_t>(fb.size());
    } catch (const std::exception&) {
        return -1;
    }
}

/**
 * Convert FlatBuffers SPW binary → NOAA text format.
 *
 * @param input      Pointer to FlatBuffers binary
 * @param input_len  Length of binary
 * @param output     Pointer to output text buffer (caller-allocated)
 * @param output_len Size of output buffer
 * @return           Bytes written on success, negative error code on failure
 */
EMSCRIPTEN_KEEPALIVE
int32_t convert(const uint8_t* input, size_t input_len,
                uint8_t* output, size_t output_len) {
    try {
        // Verify the FlatBuffer
        flatbuffers::Verifier verifier(input, input_len);
        auto* collection = flatbuffers::GetRoot<SPWCOLLECTION>(input);
        if (!collection || !collection->RECORDS()) return -3;

        // Build NOAA text output from SPW records
        swpc::Forecast45Day forecast;
        auto* records = collection->RECORDS();

        for (size_t i = 0; i < records->size(); i++) {
            auto* spw = records->Get(i);
            if (!spw || !spw->DATE()) continue;

            swpc::ForecastDay ap_day;
            ap_day.date = spw->DATE()->str();
            ap_day.value = spw->AP_AVG();
            forecast.ap.push_back(ap_day);

            swpc::ForecastDay f107_day;
            f107_day.date = spw->DATE()->str();
            f107_day.value = static_cast<int>(spw->F107_OBS());
            forecast.f107.push_back(f107_day);
        }

        std::string text = swpc::write_45day_text(forecast);

        if (text.size() > output_len) return -2;
        std::memcpy(output, text.data(), text.size());
        return static_cast<int32_t>(text.size());
    } catch (const std::exception&) {
        return -1;
    }
}

} // extern "C"

// ============================================================================
// Embind API (for Node.js / browser)
// ============================================================================

EMSCRIPTEN_BINDINGS(swpc_forecast) {
    // FlatBuffers binary output (aligned binary format)
    function("toFlatBuffers", &to_flatbuffers);
    function("textToFlatBuffers", &swpc_text_to_flatbuffers);
    function("jsonToFlatBuffers", &swpc_json_to_flatbuffers);

    // Text/JSON round-trips
    function("convert", &convert_str);
    function("detectFormat", &detect_format);
    function("swpcTextRoundtrip", &swpc_text_roundtrip);
    function("swpcTextToJson", &swpc_text_to_json);
    function("swpcJsonToText", &swpc_json_to_text);
    function("swpcJsonRoundtrip", &swpc_json_roundtrip);
}

#endif // __EMSCRIPTEN__
