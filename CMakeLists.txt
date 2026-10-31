cmake_minimum_required(VERSION 3.20)
project(COOLTONE VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(COOLTONE_BUILD_TESTS  "Build the Core unit tests" ON)
option(COOLTONE_BUILD_PLUGIN "Build the JUCE VST3 / Standalone (NOT implemented yet - Phase 4)" OFF)

# ---- Core: pure C++17, no JUCE, fully unit-tested ----
add_library(cooltone_core STATIC
    Source/Core/ScaleEngine.cpp
    Source/Core/PitchDetection.cpp
    Source/Core/NoteSegmenter.cpp
    Source/Core/WavIO.cpp)
target_include_directories(cooltone_core PUBLIC Source/Core)
if(MSVC)
    target_compile_options(cooltone_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(cooltone_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(COOLTONE_BUILD_TESTS)
    enable_testing()
    add_executable(cooltone_tests Tests/test_main.cpp)
    target_link_libraries(cooltone_tests PRIVATE cooltone_core)
    add_test(NAME cooltone_core_tests COMMAND cooltone_tests)
endif()

if(COOLTONE_BUILD_PLUGIN)
    message(FATAL_ERROR "The JUCE plugin target is not implemented yet (planned for Phase 4). See README.md.")
endif()
