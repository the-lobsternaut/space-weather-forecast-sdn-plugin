# Space Weather Forecast WASM Parser

WebAssembly-based parser for NOAA SWPC 45-Day Ap and F10.7cm Flux Forecast data.

## Overview

This WASM module parses NOAA space weather forecast text format and converts it to FlatBuffers for Space Data Network integration.

## Features

- **WASM-based**: Compiles to WebAssembly for browser and Node.js compatibility
- **Noah Text Parsing**: Parses NOAA SWPC text format to structured data
- **FlatBuffers Serialization**: Converts to FlatBuffers binary format
- **Cross-platform**: Works in browser and Node.js environments
- **Space Data Network Ready**: Compatible with SDN for decentralized publishing

## Installation

```bash
npm install spacedatastandards.org
npm install -g emscripten
```

## Building

```bash
# Navigate to wasm directory
cd wasm

# Build WASM module
bash build.sh
```

This will generate:
- `dist/swfp_parser.js` - WASM module and JavaScript wrapper
- `dist/swfp_parser.wasm` - Raw WebAssembly binary

## Usage

### Node.js

```javascript
const { SpaceWeatherForecastWASM } = require('./wasm/node');

const parser = new SpaceWeatherForecastWASM();

(async () => {
    await parser.init();

    // Parse NOAA forecast
    const forecast = await parser.parseNOAAForecast(noaaText);

    // Get metadata
    const metadata = await parser.getMetadata(noaaText);

    // Get statistics
    const stats = await parser.getStatistics(noaaText);
})();
```

### Browser

```html
<script type="module">
    import { SpaceWeatherForecastWASM } from './wasm/node/index.js';

    const parser = new SpaceWeatherForecastWASM();
    await parser.init();

    const forecast = await parser.parseNOAAForecast(noaaText);
    console.log('Forecast:', forecast);
</script>
```

## Data Flow

1. **Parse NOAA Text**: C++ parses NOAA SWPC text format
2. **Build FlatBuffers**: C++ constructs FlatBuffers structure
3. **Export to WASM**: Compiles to WebAssembly module
4. **Import in JS**: JavaScript loads and uses the WASM module
5. **Serialize**: Convert to FlatBuffers binary for SDN
6. **Publish**: Upload to Space Data Network

## Schema

Uses FlatBuffers schema `swf-schema.fbs` for Space Weather Forecast data.

## C++ Structure

```
wasm/
├── src/
│   ├── swf_parser.cpp    // WASM export functions
│   ├── parse.cpp          // NOAA text parser
│   └── parse.hpp          // Parser header
├── include/               // FlatBuffers headers (auto-generated)
├── build.sh               # Build script
├── CMakeLists.txt         # CMake build configuration
└── node/
    └── index.js           # Node.js wrapper
```

## Building Requirements

- **Emscripten**: Compile C++ to WASM
- **FlatBuffers**: Schema definitions
- **Node.js**: Runtime (for testing)

## License

Apache 2.0
