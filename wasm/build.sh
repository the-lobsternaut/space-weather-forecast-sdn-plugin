#!/bin/bash

# Space Weather Forecast WASM Builder

set -e

echo "Building Space Weather Forecast WASM Parser..."
echo ""

# Check for Emscripten
if ! command -v emcc &> /dev/null; then
    echo "Error: emcc not found. Please install Emscripten:"
    echo "  https://emscripten.org/docs/getting_started/downloads.html"
    exit 1
fi

# Compile to WASM
echo "Compiling C++ to WASM..."
emcc src/swf_parser.cpp src/parse.cpp \
    -o dist/swfp_parser.js \
    -s WASM=1 \
    -s MODULARIZE=1 \
    -s EXPORT_NAME='SWFP_PARSER' \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s USE_SDL=0 \
    --preload-file ../../swf-schema.fbs

echo ""
echo "Build complete!"
echo "Generated files:"
ls -lh dist/swfp_parser.js
