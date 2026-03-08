cmake_minimum_required(VERSION 3.10)
project(SpaceWeatherForecastParser)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Emscripten
find_program(EMSCRIPTEN emscripten)
if(NOT EMSCRIPTEN)
    message(FATAL_ERROR "Emscripten not found. Please install it: https://emscripten.org/")
endif()

# Set compiler to Emscripten
set(CMAKE_C_COMPILER ${EMSCRIPTEN})
set(CMAKE_CXX_COMPILER ${EMSCRIPTEN})

# Add FlatBuffers include path
# Assume FlatBuffers is available via spacedatastandards.org package
# or set a custom path
find_path(FLATBUFFERS_INCLUDE_DIR NAMES flatbuffers/flatbuffers.h)
if(NOT FLATBUFFERS_INCLUDE_DIR)
    message(WARNING "FlatBuffers headers not found. Please install spacedatastandards.org")
endif()

# Source files
set(SOURCES
    src/swf_parser.cpp
    src/parse.cpp
)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${FLATBUFFERS_INCLUDE_DIR})

# Create shared library (WASM module)
add_library(swfp_parser SHARED ${SOURCES})

# Emscripten specific settings
if(EMSCRIPTEN)
    set_target_properties(swfp_parser PROPERTIES
        SUFFIX ".js"
        LINK_FLAGS "-s WASM=1 -s MODULARIZE=1 -s EXPORT_NAME='SWFP_PARSER'"
    )

    # Set output directory
    set_target_properties(swfp_parser PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../dist
    )

    message(STATUS "Building WASM module for Space Weather Forecast Parser")
endif()

# Link FlatBuffers
target_link_libraries(swfp_parser flatbuffers)

# Export symbols for WASM
if(EMSCRIPTEN)
    target_compile_definitions(swfp_parser PRIVATE EMSCRIPTEN=1)
endif()

# Build instructions
message(STATUS "")
message(STATUS "Build instructions:")
message(STATUS "  cmake . -B build")
message(STATUS "  cmake --build build")
message(STATUS "")
message(STATUS "To build WASM:")
message(STATUS "  emcc src/swf_parser.cpp src/parse.cpp -o swfp_parser.js -s WASM=1 -s MODULARIZE=1")
message(STATUS "")
