cmake_minimum_required(VERSION 3.15)
project(swpc_forecast LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================================
# Paths to spacedatastandards.org monorepo dependencies
# ============================================================================

# FlatBuffers C++ headers
set(SDS_PROJECT_ROOT "${CMAKE_SOURCE_DIR}/../../../../software/space-data-network-project")

# Allow override via environment or CMake variable
if(DEFINED ENV{SDS_PROJECT_ROOT})
    set(SDS_PROJECT_ROOT "$ENV{SDS_PROJECT_ROOT}")
endif()
if(DEFINED SDS_ROOT)
    set(SDS_PROJECT_ROOT "${SDS_ROOT}")
endif()

set(FLATBUFFERS_INCLUDE "${SDS_PROJECT_ROOT}/packages/flatbuffers/include")
set(SPW_GENERATED "${SDS_PROJECT_ROOT}/packages/spacedatastandards.org/lib/cpp/SPW")

# ============================================================================
# Source files
# ============================================================================

set(SOURCES
    src/forecast_parser.cpp
    wasm_api.cpp
)

# Include directories
include_directories(include)
include_directories(${FLATBUFFERS_INCLUDE})
include_directories(${SPW_GENERATED})

# ============================================================================
# Emscripten WASM build
# ============================================================================

if(EMSCRIPTEN)
    add_executable(swpc_forecast ${SOURCES})
    set_target_properties(swpc_forecast PROPERTIES
        SUFFIX ".js"
        LINK_FLAGS "\
            -s WASM=1 \
            -s MODULARIZE=1 \
            -s EXPORT_ES6=1 \
            -s EXPORT_NAME='SWPC_WASM' \
            -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','UTF8ToString','stringToUTF8','lengthBytesUTF8'] \
            -s EXPORTED_FUNCTIONS=['_parse','_convert','_malloc','_free'] \
            -s ALLOW_MEMORY_GROWTH=1 \
            -s NO_EXIT_RUNTIME=1 \
            -s ENVIRONMENT='web,node' \
            -s DISABLE_EXCEPTION_CATCHING=0 \
            --bind \
            -O2 \
        "
    )
    # Output to wasm/node/
    set_target_properties(swpc_forecast PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/../../wasm/node"
    )

# ============================================================================
# Native build (for testing)
# ============================================================================

else()
    add_library(swpc_forecast_lib STATIC ${SOURCES})
    target_include_directories(swpc_forecast_lib PUBLIC include)

    # Test executable
    add_executable(swpc_forecast_test tests/test_roundtrip.cpp)
    target_link_libraries(swpc_forecast_test swpc_forecast_lib)
    target_include_directories(swpc_forecast_test PRIVATE include)
endif()
