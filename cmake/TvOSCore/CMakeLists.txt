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
    ${D}/astro_sun.cpp
    ${D}/astro_moon.cpp
    ${D}/astro_satellite.cpp
    ${D}/astro_sgp4/SGP4.cpp
    ${D}/geometry.cpp
    ${D}/locator_input.cpp
    ${HCP_ROOT}/apple/tvos/Core/json_mini.cpp
    ${HCP_ROOT}/apple/tvos/Core/swpc.cpp
    ${HCP_ROOT}/apple/tvos/Core/solarwind.cpp
    ${HCP_ROOT}/apple/tvos/Core/quakes.cpp
    ${HCP_ROOT}/apple/tvos/Core/bands.cpp
    ${HCP_ROOT}/apple/tvos/Core/weather.cpp
    ${HCP_ROOT}/apple/tvos/Core/hcpcore.cpp)
# Vendored Vallado SGP4: third-party code, not ours to reformat; silence its warnings only.
set_source_files_properties(${D}/astro_sgp4/SGP4.cpp PROPERTIES COMPILE_OPTIONS "-w")
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
    set_target_properties(core-${t}-regression PROPERTIES MACOSX_BUNDLE OFF)
    add_test(NAME core-${t}-regression COMMAND core-${t}-regression)
endforeach()

# Qt-free SWPC parser port; Qt parser fixtures are the oracle.
add_executable(core-swpc-regression ${HCP_ROOT}/apple/tvos/Core/tests/swpc_regression.cpp)
target_link_libraries(core-swpc-regression PRIVATE hamclock-core)
target_compile_definitions(core-swpc-regression PRIVATE
    HCP_FIXTURE_DIR="${HCP_ROOT}/src/widgets/spaceweather/tests/fixtures")
set_target_properties(core-swpc-regression PROPERTIES MACOSX_BUNDLE OFF)
add_test(NAME core-swpc-regression COMMAND core-swpc-regression)

# C ABI used by the Swift front end.
add_executable(core-hcpcore-regression ${HCP_ROOT}/apple/tvos/Core/tests/hcpcore_regression.cpp)
target_link_libraries(core-hcpcore-regression PRIVATE hamclock-core)
target_compile_definitions(core-hcpcore-regression PRIVATE
    HCP_FIXTURE_DIR="${HCP_ROOT}/src/widgets/spaceweather/tests/fixtures")
set_target_properties(core-hcpcore-regression PROPERTIES MACOSX_BUNDLE OFF)
add_test(NAME core-hcpcore-regression COMMAND core-hcpcore-regression)

# Qt-free solar wind parser; the Qt parser's fixtures are the oracle.
add_executable(core-solarwind-regression ${HCP_ROOT}/apple/tvos/Core/tests/solarwind_regression.cpp)
target_link_libraries(core-solarwind-regression PRIVATE hamclock-core)
target_compile_definitions(core-solarwind-regression PRIVATE
    HCP_FIXTURE_DIR="${HCP_ROOT}/src/widgets/solarwind/tests/fixtures")
set_target_properties(core-solarwind-regression PROPERTIES MACOSX_BUNDLE OFF)
add_test(NAME core-solarwind-regression COMMAND core-solarwind-regression)

# Qt-free Open-Meteo parser; the Qt parser's fixture is the oracle.
add_executable(core-weather-regression ${HCP_ROOT}/apple/tvos/Core/tests/weather_regression.cpp)
target_link_libraries(core-weather-regression PRIVATE hamclock-core)
target_compile_definitions(core-weather-regression PRIVATE
    HCP_FIXTURE_DIR="${HCP_ROOT}/src/widgets/weather/tests/fixtures")
set_target_properties(core-weather-regression PROPERTIES MACOSX_BUNDLE OFF)
add_test(NAME core-weather-regression COMMAND core-weather-regression)

# Qt-free USGS quakes parser; the Qt parser's fixture is the oracle.
add_executable(core-quakes-regression ${HCP_ROOT}/apple/tvos/Core/tests/quakes_regression.cpp)
target_link_libraries(core-quakes-regression PRIVATE hamclock-core)
target_compile_definitions(core-quakes-regression PRIVATE
    HCP_FIXTURE_DIR="${HCP_ROOT}/src/widgets/quakes/tests/fixtures")
set_target_properties(core-quakes-regression PROPERTIES MACOSX_BUNDLE OFF)
add_test(NAME core-quakes-regression COMMAND core-quakes-regression)
