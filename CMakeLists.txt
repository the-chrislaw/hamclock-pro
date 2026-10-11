cmake_minimum_required(VERSION 3.16)
project(HamClockProStationEditor VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Widgets)

add_executable(hamclock-pro-editor MACOSX_BUNDLE
    src/qt/station_editor.cpp
    src/qt/station_editor_dialog.cpp
    src/qt/editor_appearance.cpp
    src/app/station_edit.cpp
    src/app/station_view.cpp
    src/domain/geometry.cpp
    src/platform/settings.cpp)
target_link_libraries(hamclock-pro-editor PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(hamclock-pro-editor PRIVATE -Wall -Wextra -Wpedantic)
endif()
set_target_properties(hamclock-pro-editor PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER org.hamclockpro.stationeditor
    MACOSX_BUNDLE_BUNDLE_NAME "HamClock Pro Station Editor")

add_library(hamclock-world-map STATIC
    src/qt/world_map_widget.cpp
    src/map/path.cpp
    src/map/projection.cpp
    src/domain/geometry.cpp)
target_link_libraries(hamclock-world-map PUBLIC Qt6::Core Qt6::Gui Qt6::Widgets)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(hamclock-world-map PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(qt-appearance-regression tests/qt_appearance_regression.cpp src/qt/editor_appearance.cpp)
    target_link_libraries(qt-appearance-regression PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(qt-appearance-regression PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME qt-appearance-regression COMMAND qt-appearance-regression)
    set_tests_properties(qt-appearance-regression PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()

if(BUILD_TESTING)
    add_executable(qt-map-regression tests/qt_map_regression.cpp)
    target_link_libraries(qt-map-regression PRIVATE hamclock-world-map)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(qt-map-regression PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME qt-map-regression COMMAND qt-map-regression)
    set_tests_properties(qt-map-regression PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()

add_executable(hamclock-pro-map-preview MACOSX_BUNDLE
    src/qt/map_preview.cpp
    src/qt/editor_appearance.cpp
    src/app/station_view.cpp
    src/platform/settings.cpp)
target_link_libraries(hamclock-pro-map-preview PRIVATE hamclock-world-map)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(hamclock-pro-map-preview PRIVATE -Wall -Wextra -Wpedantic)
endif()
set_target_properties(hamclock-pro-map-preview PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER org.hamclockpro.mappreview
    MACOSX_BUNDLE_BUNDLE_NAME "HamClock Pro Map Preview")

if(BUILD_TESTING)
    add_executable(qt-station-dialog-regression
        tests/qt_station_dialog_regression.cpp
        src/qt/station_editor_dialog.cpp
        src/qt/editor_appearance.cpp
        src/app/station_edit.cpp
        src/app/station_view.cpp
        src/platform/settings.cpp
        src/domain/geometry.cpp)
    target_link_libraries(qt-station-dialog-regression PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(qt-station-dialog-regression PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME qt-station-dialog-regression COMMAND qt-station-dialog-regression)
    add_test(NAME qt-station-dialog-mac-layout COMMAND qt-station-dialog-regression --mac-button-layout)
    set_tests_properties(qt-station-dialog-regression qt-station-dialog-mac-layout PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 15)
endif()

add_executable(hamclock-pro-dashboard MACOSX_BUNDLE
    src/qt/dashboard_main.cpp
    src/qt/station_dashboard.cpp
    src/qt/dashboard_appearance.cpp
    src/qt/station_editor_dialog.cpp
    src/qt/editor_appearance.cpp
    src/app/station_edit.cpp
    src/app/station_view.cpp
    src/platform/settings.cpp)
target_link_libraries(hamclock-pro-dashboard PRIVATE hamclock-world-map)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(hamclock-pro-dashboard PRIVATE -Wall -Wextra -Wpedantic)
endif()
set_target_properties(hamclock-pro-dashboard PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER org.hamclockpro.dashboard
    MACOSX_BUNDLE_BUNDLE_NAME "HamClock Pro Dashboard")
if(BUILD_TESTING)
    add_executable(qt-dashboard-regression
        tests/qt_dashboard_regression.cpp
        src/qt/station_dashboard.cpp
    src/qt/dashboard_appearance.cpp
        src/qt/station_editor_dialog.cpp
        src/qt/editor_appearance.cpp
        src/app/station_edit.cpp
        src/app/station_view.cpp
        src/platform/settings.cpp)
    target_link_libraries(qt-dashboard-regression PRIVATE hamclock-world-map)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(qt-dashboard-regression PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME qt-dashboard-regression COMMAND qt-dashboard-regression)
    set_tests_properties(qt-dashboard-regression PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 15)
endif()

if(BUILD_TESTING)
    add_executable(locator-input-regression tests/locator_input_regression.cpp src/domain/locator_input.cpp src/domain/geometry.cpp)
    add_executable(locator-boundary-regression tests/locator_boundary_regression.cpp src/domain/locator_input.cpp src/domain/geometry.cpp src/domain/tests.cpp)
    foreach(target locator-input-regression locator-boundary-regression)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
        endif()
        add_test(NAME ${target} COMMAND ${target})
        set_tests_properties(${target} PROPERTIES TIMEOUT 15)
    endforeach()
endif()

if(BUILD_TESTING)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_test(NAME land-import-regression COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/land_import_regression.py)
    set_tests_properties(land-import-regression PROPERTIES ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1" TIMEOUT 15)
endif()

if(BUILD_TESTING)
    add_executable(clock-state-regression tests/clock_state_regression.cpp src/domain/clock_state.cpp)
    add_executable(instrument-catalog-regression tests/instrument_catalog_regression.cpp src/app/instrument_catalog.cpp)
    add_executable(instrument-snapshot-regression tests/instrument_snapshot_regression.cpp src/app/instrument_snapshot.cpp src/app/instrument_catalog.cpp)
    add_executable(spot-store-regression tests/spot_store_regression.cpp src/spots/spot_event.cpp src/spots/spot_store.cpp src/domain/geometry.cpp)
    target_include_directories(spot-store-regression PRIVATE src)
    foreach(target clock-state-regression spot-store-regression instrument-catalog-regression instrument-snapshot-regression)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
        endif()
        add_test(NAME ${target} COMMAND ${target})
        set_tests_properties(${target} PROPERTIES TIMEOUT 15)
    endforeach()
endif()

if(BUILD_TESTING)
    add_executable(land-fill-regression tests/land_fill_regression.cpp src/qt/land_fill_path.cpp)
    target_link_libraries(land-fill-regression PRIVATE Qt6::Core Qt6::Gui)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(land-fill-regression PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME land-fill-regression COMMAND land-fill-regression)
    set_tests_properties(land-fill-regression PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 15)
endif()
