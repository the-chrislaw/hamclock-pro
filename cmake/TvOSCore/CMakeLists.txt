# Standalone Qt-free build of the shared HamClock Pro core (area I).
# Lives outside the top-level CMakeLists so Linux/Pi/Qt builds are untouched.
# Usage: cmake -S cmake/TvOSCore -B build-core [-DCMAKE_TOOLCHAIN_FILE=cmake/TvOSToolchain.cmake]
cmake_minimum_required(VERSION 3.16)
project(HamClockCore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_filename_component(HCP_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(D ${HCP_ROOT}/src/domain)

# Only sources that include no Qt header. Ports of Qt-dependent parsers join
# this list once the owner approves the shim (issue #157).
add_library(hamclock-core STATIC
    ${D}/clock_state.cpp
    ${D}/stopwatch_state.cpp
    ${D}/countdown_state.cpp
    ${D}/solar_position.cpp
    ${D}/geometry.cpp
    ${D}/locator_input.cpp)
target_include_directories(hamclock-core PUBLIC ${HCP_ROOT}/src)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(hamclock-core PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Test executables only run on hosts; for the tvOS Simulator they are built
# (proving they link) and executed by the workflow with `xcrun simctl spawn`.
include(CTest)
set(_tests clock_state stopwatch_state countdown_state solar_position locator_input)
foreach(t ${_tests})
    add_executable(core-${t}-regression ${HCP_ROOT}/tests/${t}_regression.cpp)
    target_link_libraries(core-${t}-regression PRIVATE hamclock-core)
    add_test(NAME core-${t}-regression COMMAND core-${t}-regression)
endforeach()
