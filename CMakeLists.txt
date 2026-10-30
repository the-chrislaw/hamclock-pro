cmake_minimum_required(VERSION 3.16)
project(HamClockPro VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Widgets)

# QtGui-only map core (renderer, viewport, projection, land geometry): no
# QtWidgets, so the Qt Quick app and the iPad build depend on this alone.
add_library(hamclock-map-core STATIC
    src/qt/land_fill_path.cpp
    src/map/path.cpp
    src/map/overlay.cpp
    src/map/view_options.cpp
    src/map/styles.cpp
    src/map/projection.cpp
    src/map/viewport.cpp
    src/map/map_renderer.cpp
    src/domain/geometry.cpp
    src/domain/solar_position.cpp
    src/domain/astro_moon.cpp)
target_link_libraries(hamclock-map-core PUBLIC Qt6::Core Qt6::Gui)
# Legacy QWidget map used by the old dashboard/preview tools only.
add_library(hamclock-world-map STATIC src/qt/world_map_widget.cpp)
target_link_libraries(hamclock-world-map PUBLIC hamclock-map-core Qt6::Widgets)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(hamclock-map-core PRIVATE -Wall -Wextra -Wpedantic)
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

if(BUILD_TESTING)
    add_executable(qt-map-terminator-regression tests/qt_map_terminator_regression.cpp)
    target_link_libraries(qt-map-terminator-regression PRIVATE hamclock-world-map)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(qt-map-terminator-regression PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME qt-map-terminator-regression COMMAND qt-map-terminator-regression)
    set_tests_properties(qt-map-terminator-regression PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 15)
endif()

if(BUILD_TESTING)
    add_executable(qt-land-widget-regression tests/qt_land_widget_regression.cpp)
    target_link_libraries(qt-land-widget-regression PRIVATE hamclock-world-map)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(qt-land-widget-regression PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME qt-land-widget-regression COMMAND qt-land-widget-regression)
    set_tests_properties(qt-land-widget-regression PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 15)
endif()

if(BUILD_TESTING)
    add_executable(qt-map-viewport-regression tests/qt_map_viewport_regression.cpp)
    target_link_libraries(qt-map-viewport-regression PRIVATE hamclock-world-map)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(qt-map-viewport-regression PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME qt-map-viewport-regression COMMAND qt-map-viewport-regression)
    set_tests_properties(qt-map-viewport-regression PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)
endif()

if(BUILD_TESTING)
    add_executable(qt-map-overlay-regression tests/qt_map_overlay_regression.cpp)
    target_link_libraries(qt-map-overlay-regression PRIVATE hamclock-map-core)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(qt-map-overlay-regression PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME qt-map-overlay-regression COMMAND qt-map-overlay-regression)
    set_tests_properties(qt-map-overlay-regression PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)
    add_executable(qt-map-view-options-regression tests/qt_map_view_options_regression.cpp)
    target_link_libraries(qt-map-view-options-regression PRIVATE hamclock-map-core)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(qt-map-view-options-regression PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME qt-map-view-options-regression COMMAND qt-map-view-options-regression)
    set_tests_properties(qt-map-view-options-regression PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)
    add_executable(qt-map-styles-regression tests/qt_map_styles_regression.cpp)
    target_link_libraries(qt-map-styles-regression PRIVATE hamclock-map-core)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(qt-map-styles-regression PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME qt-map-styles-regression COMMAND qt-map-styles-regression)
    set_tests_properties(qt-map-styles-regression PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)
endif()

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
    add_test(NAME pane-categories-regression COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/pane_categories_regression.py)
    add_test(NAME pane-keys-regression COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/pane_keys_regression.py)
    add_test(NAME settings-zone-regression COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/settings_zone_regression.py)
    add_test(NAME desktop-shortcuts-regression COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/desktop_shortcuts_regression.py)
    set_tests_properties(pane-categories-regression PROPERTIES ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1" TIMEOUT 15)
    add_test(NAME kiosk-unit-regression COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/kiosk_unit_regression.py)
    set_tests_properties(kiosk-unit-regression PROPERTIES ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1" TIMEOUT 15)
    add_test(NAME apple-bundle-metadata-regression COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/apple_bundle_metadata_regression.py)
    set_tests_properties(apple-bundle-metadata-regression PROPERTIES ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1" TIMEOUT 15)
    add_test(NAME overlay-layout-regression COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/overlay_layout_regression.py)
    set_tests_properties(overlay-layout-regression PROPERTIES ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1" TIMEOUT 15)
    add_test(NAME map-menu-position-regression COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/map_menu_position_regression.py)
    set_tests_properties(map-menu-position-regression PROPERTIES ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1" TIMEOUT 15)
    add_test(NAME empty-dx-regression COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/empty_dx_regression.py)
endif()

if(BUILD_TESTING)
    add_executable(clock-state-regression tests/clock_state_regression.cpp src/domain/clock_state.cpp)
    add_executable(solar-position-regression tests/solar_position_regression.cpp src/domain/solar_position.cpp)
    add_executable(instrument-catalog-regression tests/instrument_catalog_regression.cpp src/app/instrument_catalog.cpp)
    add_executable(instrument-snapshot-regression tests/instrument_snapshot_regression.cpp src/app/instrument_snapshot.cpp src/app/instrument_catalog.cpp)
    add_executable(spot-store-regression tests/spot_store_regression.cpp src/spots/spot_event.cpp src/spots/spot_store.cpp src/domain/geometry.cpp)
    target_include_directories(spot-store-regression PRIVATE src)
    foreach(target clock-state-regression solar-position-regression spot-store-regression instrument-catalog-regression instrument-snapshot-regression)
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

# Offline provider syntax decoding only; no transport or dashboard feed wiring.
add_library(hamclock-noaa-kp STATIC src/providers/noaa_kp_parser.cpp)
target_link_libraries(hamclock-noaa-kp PUBLIC Qt6::Core)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(hamclock-noaa-kp PRIVATE -Wall -Wextra -Wpedantic)
endif()
if(BUILD_TESTING)
    add_executable(noaa-kp-parser-regression tests/noaa_kp_parser_regression.cpp)
    target_link_libraries(noaa-kp-parser-regression PRIVATE hamclock-noaa-kp)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(noaa-kp-parser-regression PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME noaa-kp-parser-regression COMMAND noaa-kp-parser-regression)
    set_tests_properties(noaa-kp-parser-regression PROPERTIES TIMEOUT 15)
endif()

if(BUILD_TESTING)
    add_executable(stopwatch-state-regression
        tests/stopwatch_state_regression.cpp src/domain/stopwatch_state.cpp)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(stopwatch-state-regression PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME stopwatch-state-regression COMMAND stopwatch-state-regression)
    set_tests_properties(stopwatch-state-regression PROPERTIES TIMEOUT 15)
endif()

if(BUILD_TESTING)
    add_executable(stopwatch-presentation-regression
        tests/stopwatch_presentation_regression.cpp
        src/app/stopwatch_presentation.cpp src/domain/stopwatch_state.cpp)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(stopwatch-presentation-regression PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME stopwatch-presentation-regression COMMAND stopwatch-presentation-regression)
    set_tests_properties(stopwatch-presentation-regression PROPERTIES TIMEOUT 15)
endif()

add_library(hamclock-dashboard-appearance STATIC src/qt/dashboard_appearance.cpp)
target_link_libraries(hamclock-dashboard-appearance PUBLIC Qt6::Core Qt6::Gui Qt6::Widgets)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(hamclock-dashboard-appearance PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_library(hamclock-stopwatch-dialog STATIC
    src/qt/stopwatch_dialog.cpp
    src/app/stopwatch_presentation.cpp src/domain/stopwatch_state.cpp)
target_link_libraries(hamclock-stopwatch-dialog PUBLIC hamclock-dashboard-appearance)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(hamclock-stopwatch-dialog PRIVATE -Wall -Wextra -Wpedantic)
endif()
if(BUILD_TESTING)
    add_executable(qt-stopwatch-dialog-regression tests/qt_stopwatch_dialog_regression.cpp)
    target_link_libraries(qt-stopwatch-dialog-regression PRIVATE hamclock-stopwatch-dialog)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(qt-stopwatch-dialog-regression PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME qt-stopwatch-dialog-regression COMMAND qt-stopwatch-dialog-regression)
    set_tests_properties(qt-stopwatch-dialog-regression PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 15)
endif()

# Pure injected countdown model only; no UI, scheduler or hardware activation.
if(BUILD_TESTING)
    add_executable(countdown-state-regression
        tests/countdown_state_regression.cpp src/domain/countdown_state.cpp)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(countdown-state-regression PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME countdown-state-regression COMMAND countdown-state-regression)
    set_tests_properties(countdown-state-regression PROPERTIES TIMEOUT 15)
endif()

if(BUILD_TESTING)
    add_executable(countdown-presentation-regression
        tests/countdown_presentation_regression.cpp
        src/app/countdown_presentation.cpp src/domain/countdown_state.cpp)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(countdown-presentation-regression PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME countdown-presentation-regression COMMAND countdown-presentation-regression)
    set_tests_properties(countdown-presentation-regression PROPERTIES TIMEOUT 15)
endif()

if(BUILD_TESTING)
    add_executable(countdown-service-regression
        tests/countdown_service_regression.cpp src/app/countdown_service.cpp
        src/app/countdown_presentation.cpp src/domain/countdown_state.cpp)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(countdown-service-regression PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME countdown-service-regression COMMAND countdown-service-regression)
    set_tests_properties(countdown-service-regression PROPERTIES TIMEOUT 15)
endif()

add_library(hamclock-countdown-dialog STATIC
    src/qt/countdown_dialog.cpp src/app/countdown_service.cpp
    src/app/countdown_presentation.cpp src/domain/countdown_state.cpp)
target_link_libraries(hamclock-countdown-dialog PUBLIC hamclock-dashboard-appearance)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(hamclock-countdown-dialog PRIVATE -Wall -Wextra -Wpedantic)
endif()
if(BUILD_TESTING)
    add_executable(qt-countdown-dialog-regression tests/qt_countdown_dialog_regression.cpp)
    target_link_libraries(qt-countdown-dialog-regression PRIVATE hamclock-countdown-dialog)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(qt-countdown-dialog-regression PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME qt-countdown-dialog-regression COMMAND qt-countdown-dialog-regression)
    set_tests_properties(qt-countdown-dialog-regression PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 15)
endif()

if(BUILD_TESTING)
    add_executable(big-clock-preferences-regression
        tests/big_clock_preferences_regression.cpp src/app/big_clock_preferences.cpp)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(big-clock-preferences-regression PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME big-clock-preferences-regression COMMAND big-clock-preferences-regression)
    set_tests_properties(big-clock-preferences-regression PROPERTIES TIMEOUT 15)
endif()

if(BUILD_TESTING)
    add_executable(big-clock-presentation-regression
        tests/big_clock_presentation_regression.cpp src/app/big_clock_presentation.cpp
        src/app/big_clock_preferences.cpp)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(big-clock-presentation-regression PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME big-clock-presentation-regression COMMAND big-clock-presentation-regression)
    set_tests_properties(big-clock-presentation-regression PROPERTIES TIMEOUT 15)
endif()

if(BUILD_TESTING)
    add_executable(qt-big-clock-civil-adapter-regression
        tests/qt_big_clock_civil_adapter_regression.cpp src/qt/big_clock_civil_adapter.cpp
        src/app/big_clock_presentation.cpp src/app/big_clock_preferences.cpp)
    target_link_libraries(qt-big-clock-civil-adapter-regression PRIVATE Qt6::Core)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(qt-big-clock-civil-adapter-regression PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME qt-big-clock-civil-adapter-regression COMMAND qt-big-clock-civil-adapter-regression)
    set_tests_properties(qt-big-clock-civil-adapter-regression PROPERTIES ENVIRONMENT "TZ=UTC" TIMEOUT 15)
endif()

add_library(hamclock-big-clock-dialog STATIC
    src/qt/big_clock_dialog.cpp src/qt/big_clock_civil_adapter.cpp
    src/app/big_clock_presentation.cpp src/app/big_clock_preferences.cpp)
target_link_libraries(hamclock-big-clock-dialog PUBLIC hamclock-dashboard-appearance)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(hamclock-big-clock-dialog PRIVATE -Wall -Wextra -Wpedantic)
endif()
if(BUILD_TESTING)
    add_executable(qt-big-clock-dialog-regression tests/qt_big_clock_dialog_regression.cpp)
    target_link_libraries(qt-big-clock-dialog-regression PRIVATE hamclock-big-clock-dialog)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(qt-big-clock-dialog-regression PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME qt-big-clock-dialog-regression COMMAND qt-big-clock-dialog-regression)
    set_tests_properties(qt-big-clock-dialog-regression PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 15)
endif()

if(BUILD_TESTING)
    add_executable(de-clock-configuration-regression
        tests/de_clock_configuration_regression.cpp src/app/de_clock_configuration.cpp
        src/domain/geometry.cpp)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(de-clock-configuration-regression PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME de-clock-configuration-regression COMMAND de-clock-configuration-regression)
    set_tests_properties(de-clock-configuration-regression PROPERTIES TIMEOUT 15)
endif()

if(BUILD_TESTING)
    add_executable(qt-de-clock-manual-civil-adapter-regression
        tests/qt_de_clock_manual_civil_adapter_regression.cpp
        src/qt/de_clock_manual_civil_adapter.cpp src/qt/big_clock_civil_adapter.cpp
        src/app/de_clock_configuration.cpp src/domain/geometry.cpp
        src/app/big_clock_presentation.cpp src/app/big_clock_preferences.cpp)
    target_link_libraries(qt-de-clock-manual-civil-adapter-regression PRIVATE Qt6::Core)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(qt-de-clock-manual-civil-adapter-regression PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME qt-de-clock-manual-civil-adapter-regression COMMAND qt-de-clock-manual-civil-adapter-regression)
    set_tests_properties(qt-de-clock-manual-civil-adapter-regression PROPERTIES ENVIRONMENT "TZ=UTC" TIMEOUT 15)
endif()

if(BUILD_TESTING)
    add_executable(qt-de-clock-display-snapshot-regression
        tests/qt_de_clock_display_snapshot_regression.cpp src/qt/de_clock_display_snapshot.cpp
        src/qt/de_clock_manual_civil_adapter.cpp src/qt/de_clock_zone_civil_adapter.cpp src/qt/de_time_zone.cpp src/qt/big_clock_civil_adapter.cpp
        src/app/de_clock_configuration.cpp src/domain/geometry.cpp
        src/app/big_clock_presentation.cpp src/app/big_clock_preferences.cpp)
    target_link_libraries(qt-de-clock-display-snapshot-regression PRIVATE Qt6::Core)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(qt-de-clock-display-snapshot-regression PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME qt-de-clock-display-snapshot-regression COMMAND qt-de-clock-display-snapshot-regression)
    set_tests_properties(qt-de-clock-display-snapshot-regression PROPERTIES ENVIRONMENT "TZ=UTC" TIMEOUT 15)
endif()

if(BUILD_TESTING)
    add_executable(qt-de-clock-settings-dialog-regression
        tests/qt_de_clock_settings_dialog_regression.cpp src/qt/de_clock_settings_dialog.cpp
        src/app/de_clock_configuration.cpp src/domain/geometry.cpp src/qt/de_time_zone.cpp)
    target_link_libraries(qt-de-clock-settings-dialog-regression PRIVATE hamclock-dashboard-appearance)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(qt-de-clock-settings-dialog-regression PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME qt-de-clock-settings-dialog-regression COMMAND qt-de-clock-settings-dialog-regression)
    set_tests_properties(qt-de-clock-settings-dialog-regression PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 15)
endif()

# Offline command-line tools (replaces the retired SDL2 executable).
add_executable(hamclock-pro-cli
    src/cli_main.cpp src/app/commands.cpp
    src/app/station_view.cpp src/app/station_view_tests.cpp
    src/app/pane_controls.cpp src/app/pane_controls_tests.cpp
    src/app/station_edit.cpp src/app/station_edit_tests.cpp
    src/platform/settings.cpp src/platform/settings_tests.cpp
    src/map/projection.cpp src/map/projection_tests.cpp src/map/path.cpp src/map/path_tests.cpp
    src/domain/geometry.cpp src/domain/tests.cpp)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(hamclock-pro-cli PRIVATE -Wall -Wextra -Wpedantic)
endif()
if(BUILD_TESTING)
    add_test(NAME cli-self-test COMMAND hamclock-pro-cli --self-test)
    add_test(NAME cli-regression COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/cli_regression.py $<TARGET_FILE:hamclock-pro-cli>)
    add_test(NAME map-import-regression COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/map_import_regression.py)
    set_tests_properties(cli-self-test cli-regression map-import-regression PROPERTIES TIMEOUT 60 ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1")
endif()

include(cmake/HamClockApp.cmake)
include(cmake/AppleBundle.cmake)

# Installation and packaging (used by .github/workflows/release.yml).
include(GNUInstallDirs)
install(TARGETS hamclock-pro hamclock-pro-cli
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION .)
install(FILES LICENSE README.md DESTINATION ${CMAKE_INSTALL_DOCDIR})
set(CPACK_PACKAGE_NAME hamclock-pro)
set(CPACK_PACKAGE_VENDOR "HamClock Pro contributors")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Amateur radio world-time and station dashboard")
set(CPACK_PACKAGE_CONTACT "https://github.com/the-chrislaw/hamclock-pro")
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_SECTION hamradio)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
# Qt platform plugins are loaded at runtime, so shlibdeps cannot see them.
# Raspberry Pi OS desktops are Wayland; without this the app cannot start.
set(CPACK_DEBIAN_PACKAGE_DEPENDS "qt6-wayland, qml6-module-qtquick, qml6-module-qtquick-controls, qml6-module-qtquick-layouts, qml6-module-qtquick-window, qml6-module-qtquick-templates, qml6-module-qtqml-workerscript")
include(CPack)

if(BUILD_TESTING)
    add_executable(qt-de-time-zone-regression
        tests/qt_de_time_zone_regression.cpp src/qt/de_time_zone.cpp)
    target_link_libraries(qt-de-time-zone-regression PRIVATE Qt6::Core)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(qt-de-time-zone-regression PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME qt-de-time-zone-regression COMMAND qt-de-time-zone-regression)
    set_tests_properties(qt-de-time-zone-regression PROPERTIES TIMEOUT 15)
endif()

if(BUILD_TESTING)
    add_executable(qt-de-clock-zone-civil-adapter-regression
        tests/qt_de_clock_zone_civil_adapter_regression.cpp
        src/qt/de_clock_zone_civil_adapter.cpp src/qt/de_time_zone.cpp src/qt/big_clock_civil_adapter.cpp
        src/domain/geometry.cpp src/app/big_clock_presentation.cpp src/app/big_clock_preferences.cpp)
    target_link_libraries(qt-de-clock-zone-civil-adapter-regression PRIVATE Qt6::Core)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(qt-de-clock-zone-civil-adapter-regression PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME qt-de-clock-zone-civil-adapter-regression COMMAND qt-de-clock-zone-civil-adapter-regression)
    set_tests_properties(qt-de-clock-zone-civil-adapter-regression PROPERTIES ENVIRONMENT "TZ=UTC" TIMEOUT 15)
endif()

add_library(hamclock-credential-store STATIC src/platform/credential_store.cpp)
target_link_libraries(hamclock-credential-store PUBLIC Qt6::Core)
if(APPLE)
    enable_language(OBJCXX)
    set(CMAKE_OBJCXX_STANDARD 17)
    target_sources(hamclock-credential-store PRIVATE src/platform/credential_store_keychain.mm)
    target_link_libraries(hamclock-credential-store PUBLIC "-framework Security" "-framework Foundation")
endif()
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(hamclock-credential-store PRIVATE -Wall -Wextra -Wpedantic)
endif()
if(BUILD_TESTING)
    add_executable(credential-store-regression tests/credential_store_regression.cpp)
    target_link_libraries(credential-store-regression PRIVATE hamclock-credential-store)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(credential-store-regression PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME credential-store-regression COMMAND credential-store-regression)
    set_tests_properties(credential-store-regression PROPERTIES TIMEOUT 30)
endif()
