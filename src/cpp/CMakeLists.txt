cmake_minimum_required(VERSION 3.15)
project(swpc_forecast LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================================
# Locate spacedatastandards.org dependencies
# ============================================================================

# SDS_ROOT can be passed via -DSDS_ROOT=... or environment
if(NOT DEFINED SDS_ROOT)
    if(DEFINED ENV{SDS_ROOT})
        set(SDS_ROOT "$ENV{SDS_ROOT}")
    else()
        # Search common locations
        foreach(_candidate
            "${CMAKE_SOURCE_DIR}/../../../../space-data-network-project"
            "$ENV{HOME}/software/space-data-network-project"
        )
            if(EXISTS "${_candidate}/packages/flatbuffers/include")
                get_filename_component(SDS_ROOT "${_candidate}" ABSOLUTE)
                break()
            endif()
        endforeach()
    endif()
endif()

if(SDS_ROOT)
    message(STATUS "SDS_ROOT: ${SDS_ROOT}")
    set(FLATBUFFERS_INCLUDE "${SDS_ROOT}/packages/flatbuffers/include")
    set(SPW_GENERATED "${SDS_ROOT}/packages/spacedatastandards.org/lib/cpp/SPW")
else()
    message(WARNING "SDS_ROOT not found. FlatBuffers headers may be missing. Pass -DSDS_ROOT=<path>")
    set(FLATBUFFERS_INCLUDE "")
    set(SPW_GENERATED "")
endif()

# ============================================================================
# Source files
# ============================================================================

set(SOURCES
    src/forecast_parser.cpp
    wasm_api.cpp
)

include_directories(include)
if(FLATBUFFERS_INCLUDE)
    include_directories(${FLATBUFFERS_INCLUDE})
endif()
if(SPW_GENERATED)
    include_directories(${SPW_GENERATED})
endif()

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
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/../../wasm/node"
    )

# ============================================================================
# Native build (for testing)
# ============================================================================

else()
    add_library(swpc_forecast_lib STATIC ${SOURCES})
    target_include_directories(swpc_forecast_lib PUBLIC include)

    add_executable(swpc_forecast_test tests/test_roundtrip.cpp)
    target_link_libraries(swpc_forecast_test swpc_forecast_lib)
    target_include_directories(swpc_forecast_test PRIVATE include)
endif()
