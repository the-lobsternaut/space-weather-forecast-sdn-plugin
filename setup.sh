#!/bin/bash
# Setup script for NOAA SWPC Forecast SDN Plugin
# Installs emsdk and builds the WASM artifact

set -e

echo "=========================================="
echo "NOAA SWPC Forecast SDN Plugin - Setup"
echo "=========================================="
echo ""

# Check for Node.js
if ! command -v node &> /dev/null; then
    echo "Error: Node.js is required. Please install it first."
    exit 1
fi

echo "✓ Node.js found: $(node --version)"
echo ""

# Install emsdk as a git submodule (per SDN scaffold pattern)
EMSDK_DIR="${CMAKE_SOURCE_DIR}/../../../../emsdk"

echo "Installing emsdk as git submodule..."
git submodule add https://github.com/emscripten-core/emsdk.git "${EMSDK_DIR}"

echo ""
echo "Activating emsdk..."
cd "${EMSDK_DIR}"
./emsdk install latest
./emsdk activate latest

echo ""
echo "Source the environment:"
echo "  source ${EMSDK_DIR}/emsdk_env.sh"
echo ""
echo "Then run: cmake -B build -S . -DEMSCRIPTEN=1"
echo "And: cmake --build build"
