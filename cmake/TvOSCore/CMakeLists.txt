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
    ${D}/locator_input.cpp
    ${HCP_ROOT}/apple/tvos/Core/json_mini.cpp
    ${HCP_ROOT}/apple/tvos/Core/swpc.cpp)
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
