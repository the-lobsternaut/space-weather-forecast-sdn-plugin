#include "swpc/forecast_parser.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>
using namespace emscripten;
#endif

#include <string>
#include <stdexcept>

// Format detection for NOAA SWPC data
static std::string detect_format(const std::string& input) {
    for (size_t i = 0; i < input.size(); i++) {
        char c = input[i];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') continue;
        if (c == '{' || c == '[') return "json";
        return "text";
    }
    return "text";
}

// Text -> Text round-trip
static std::string swpc_text_roundtrip(const std::string& input) {
    auto forecast = swpc::parse_45day_text(input);
    return swpc::write_45day_text(forecast);
}

// Text -> JSON
static std::string swpc_text_to_json(const std::string& input) {
    auto forecast = swpc::parse_45day_text(input);
    return swpc::write_45day_json(forecast);
}

// JSON -> Text
static std::string swpc_json_to_text(const std::string& input) {
    auto forecast = swpc::parse_45day_json(input);
    return swpc::write_45day_text(forecast);
}

// JSON -> JSON round-trip
static std::string swpc_json_roundtrip(const std::string& input) {
    auto forecast = swpc::parse_45day_json(input);
    return swpc::write_45day_json(forecast);
}

// Generic convert: auto-detect and convert
static std::string convert(const std::string& input, const std::string& target_format) {
    std::string fmt = detect_format(input);

    if (fmt == "text" && target_format == "text") return swpc_text_roundtrip(input);
    if (fmt == "text" && target_format == "json") return swpc_text_to_json(input);
    if (fmt == "json" && target_format == "text") return swpc_json_to_text(input);
    if (fmt == "json" && target_format == "json") return swpc_json_roundtrip(input);

    throw std::runtime_error("Unsupported format: " + fmt + " -> " + target_format);
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(swpc_forecast) {
    function("convert", &convert);
    function("detectFormat", &detect_format);
    function("swpcTextRoundtrip", &swpc_text_roundtrip);
    function("swpcTextToJson", &swpc_text_to_json);
    function("swpcJsonToText", &swpc_json_to_text);
    function("swpcJsonRoundtrip", &swpc_json_roundtrip);
}
#endif
