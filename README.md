# NOAA SWPC 45-Day Forecast → SDN Plugin

C++ WASM parser for the NOAA Space Weather Prediction Center 45-Day Ap and F10.7cm Flux Forecast. Converts between NOAA text format and JSON, with output compatible with the [SPW schema](https://spacedatastandards.org/#/schema/SPW) from [spacedatastandards.org](https://spacedatastandards.org).

## What It Does

Parses the [45-Day Ap and F10.7cm Flux Forecast](https://www.spaceweather.gov/products/45-day-forecast) product from NOAA SWPC and converts it to SPW-compatible JSON for publishing on the Space Data Network.

## Architecture

```
src/cpp/
├── include/swpc/
│   └── forecast_parser.h      # Parser API
├── src/
│   └── forecast_parser.cpp    # Text/JSON parser + writer
├── tests/
│   └── test_roundtrip.cpp     # Round-trip tests
├── wasm_api.cpp               # Emscripten bindings
└── CMakeLists.txt             # Build config
wasm/node/
├── index.mjs                  # Node.js wrapper
├── swpc_forecast.js           # Emscripten glue (generated)
└── swpc_forecast.wasm         # WASM binary (generated)
```

## Building

Requires [Emscripten](https://emscripten.org/):

```bash
cd src/cpp
mkdir build && cd build
emcmake cmake ..
emmake make
```

### Native build (for testing):

```bash
cd src/cpp
mkdir build && cd build
cmake ..
make
./swpc_forecast_test
```

## Usage

### Node.js (WASM)

```javascript
import { init } from './wasm/node/index.mjs';

const sds = await init();

// Convert NOAA text format to JSON
const json = sds.convert(noaaTextInput, 'json');

// Convert JSON back to text
const text = sds.convert(jsonInput, 'text');

// Auto-detect format
sds.detectFormat(input);  // 'text' or 'json'

// Direct functions
sds.swpcTextToJson(textInput);
sds.swpcJsonToText(jsonInput);
sds.swpcTextRoundtrip(textInput);
sds.swpcJsonRoundtrip(jsonInput);
```

## SPW Schema Compatibility

Output JSON uses field names from the [SPW (Space Weather Data Record)](https://spacedatastandards.org/#/schema/SPW) schema:

| SPW Field | Description |
|-----------|-------------|
| `DATE` | ISO 8601 date |
| `AP_AVG` | Daily Ap index (predicted) |
| `F107_OBS` | F10.7cm solar radio flux (predicted) |
| `F107_DATA_TYPE` | `PRD` (45-day predicted) |

## Data Source

- **Product**: [45-Day Ap and F10.7cm Flux Forecast](https://www.spaceweather.gov/products/45-day-forecast)
- **Provider**: NOAA Space Weather Prediction Center
- **Update Frequency**: Daily at 0000 UTC

## License

Apache 2.0
