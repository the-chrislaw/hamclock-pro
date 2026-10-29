# HamClock Pro Qt Quick application and widget framework.
# Widgets live in src/widgets/<id>/ and call hamclock_add_widget() from their
# own CMakeLists.txt, so adding a widget never requires editing this file.

find_package(Qt6 6.4 REQUIRED COMPONENTS Network Quick QuickControls2)

# Core shared by all widgets: data plumbing, widget base/registry, settings.
add_library(hamclock-widget-core STATIC
    src/net/polled_source.cpp
    src/platform/apple_lifecycle.cpp
    src/platform/apple_safearea.cpp
    src/platform/apple_orientation.cpp
    src/platform/apple_textentry.cpp
    src/platform/apple_attribution.cpp
    src/platform/apple_skypointer.cpp
    src/platform/apple_skyproject.cpp
    src/platform/apple_camerafov.cpp
    src/platform/apple_skyattitude.cpp
    src/platform/apple_skysensor.cpp
    src/platform/apple_skyfinder.cpp
    src/platform/apple_passreminder.cpp
    src/platform/apple_notifications.cpp
    src/widgets/widget_model.cpp
    src/widgets/widget_registry.cpp
    src/ui/app_settings.cpp
    src/ui/pane_grid.cpp
    src/platform/settings.cpp)
target_link_libraries(hamclock-widget-core PUBLIC hamclock-map-core hamclock-credential-store Qt6::Core Qt6::Gui Qt6::Network)
set_target_properties(hamclock-widget-core PROPERTIES AUTOMOC ON)
# "Find it in the sky" (#573) has no screen yet. Until it does, iOS builds leave
# out its sensor code, frameworks and permission prompts (see AppleBundle.cmake)
# so App Review doesn't ask about camera/location use it can't find.
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # System sleep/wake suspends and resumes polling and connections (AppLifecycle).
    target_sources(hamclock-widget-core PRIVATE src/platform/apple_lifecycle_mac.mm)
    find_library(HC_APPKIT AppKit REQUIRED)
    target_link_libraries(hamclock-widget-core PUBLIC ${HC_APPKIT})
endif()
option(HAMCLOCK_SKY_FINDER "Build the iOS sky finder sensors and permission prompts" OFF)
# Satellite pass alerts: UNUserNotificationCenter on iPhone, iPad and the Mac app.
if(APPLE)
    target_sources(hamclock-widget-core PRIVATE src/platform/apple_notifications.mm)
    set_source_files_properties(src/platform/apple_notifications.mm PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
    if(NOT IOS)
        find_library(HC_MAC_USERNOTIFICATIONS UserNotifications REQUIRED)
        target_link_libraries(hamclock-widget-core PUBLIC ${HC_MAC_USERNOTIFICATIONS})
    endif()
endif()
if(IOS)
    target_sources(hamclock-widget-core PRIVATE src/platform/apple_safearea_ios.mm src/platform/apple_textentry_ios.mm src/platform/apple_orientation_ios.mm src/platform/apple_haptics_ios.mm src/platform/apple_passreminder_ios.mm)
    find_library(HC_USERNOTIFICATIONS UserNotifications REQUIRED)
    find_library(HC_GAMECONTROLLER GameController REQUIRED)
    find_library(HC_UIKIT UIKit REQUIRED)
    target_link_libraries(hamclock-widget-core PUBLIC ${HC_USERNOTIFICATIONS} ${HC_GAMECONTROLLER} ${HC_UIKIT})
    if(HAMCLOCK_SKY_FINDER)
        target_sources(hamclock-widget-core PRIVATE src/platform/apple_skysensor_ios.mm src/platform/apple_camera_ios.mm)
        target_compile_definitions(hamclock-widget-core PRIVATE HAMCLOCK_SKY_FINDER)
        find_library(HC_COREMOTION CoreMotion REQUIRED)
        find_library(HC_CORELOCATION CoreLocation REQUIRED)
        find_library(HC_AVFOUNDATION AVFoundation REQUIRED)
        find_library(HC_COREMEDIA CoreMedia REQUIRED)
        target_link_libraries(hamclock-widget-core PUBLIC ${HC_COREMOTION} ${HC_CORELOCATION} ${HC_AVFOUNDATION} ${HC_COREMEDIA})
    endif()
endif()

# All widget models are compiled into this library by hamclock_add_widget().
add_library(hamclock-widgets STATIC)
target_link_libraries(hamclock-widgets PUBLIC hamclock-widget-core)
set_target_properties(hamclock-widgets PROPERTIES AUTOMOC ON)

add_executable(hamclock-pro MACOSX_BUNDLE
    src/ui/main.cpp
    src/ui/station_model.cpp
    src/ui/time_zone_search.cpp
    src/ui/location_tools.cpp
    src/ui/timers_model.cpp
    src/ui/alarms_model.cpp
    src/ui/blanking_model.cpp
    src/domain/locator_input.cpp
    src/ui/map_item.cpp
    src/app/station_view.cpp
    src/qt/de_time_zone.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/widgets_generated.cpp)
target_link_libraries(hamclock-pro PRIVATE hamclock-widgets Qt6::Quick Qt6::QuickControls2)
set_target_properties(hamclock-pro PROPERTIES AUTOMOC ON
    MACOSX_BUNDLE_GUI_IDENTIFIER org.hamclockpro.app
    MACOSX_BUNDLE_BUNDLE_NAME "HamClock Pro")
qt_add_resources(hamclock-pro "hamclock_ui" PREFIX "/ui" BASE src/ui/qml FILES
    src/ui/qml/Main.qml src/ui/qml/Theme.qml src/ui/qml/Card.qml src/ui/qml/Metric.qml
    src/ui/qml/StatusBadge.qml src/ui/qml/AppButton.qml src/ui/qml/FormField.qml
    src/ui/qml/StationCard.qml src/ui/qml/SettingsPage.qml
    src/ui/qml/PaneSlot.qml src/ui/qml/PaneChooser.qml src/ui/qml/PaneOptions.qml src/ui/qml/MapMenu.qml src/ui/qml/MapTapMenu.qml src/ui/qml/TimeZonePicker.qml src/ui/qml/TimersPage.qml src/ui/qml/BigClockPage.qml src/ui/qml/SatelliteAlertsCard.qml src/ui/qml/SatAlertBanner.qml)
foreach(target hamclock-widget-core hamclock-widgets hamclock-pro)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endforeach()

set_property(GLOBAL PROPERTY HAMCLOCK_WIDGET_IDS "")

# hamclock_add_widget(<id>
#     SOURCES <model .cpp files>          compiled into hamclock-widgets
#     QML <card .qml files>               served at qrc:/widgets/<id>/
#     TESTS <test .cpp files>             each becomes a CTest (offscreen Qt)
#     TEST_DATA_DIR <dir>)                passed to tests as argv[1] if given
# The widget must define `void hamclock::register_widget_<id>(WidgetRegistry&, const WidgetContext&)`.
function(hamclock_add_widget id)
    cmake_parse_arguments(W "" "TEST_DATA_DIR" "SOURCES;QML;TESTS" ${ARGN})
    if(NOT id MATCHES "^[a-z][a-z0-9_]*$")
        message(FATAL_ERROR "Widget id '${id}' must be lowercase [a-z0-9_]")
    endif()
    target_sources(hamclock-widgets PRIVATE ${W_SOURCES})
    if(W_QML)
        qt_add_resources(hamclock-pro "widget_${id}" PREFIX "/widgets/${id}" FILES ${W_QML})
    endif()
    set_property(GLOBAL APPEND PROPERTY HAMCLOCK_WIDGET_IDS ${id})
    if(BUILD_TESTING)
        foreach(test_source ${W_TESTS})
            get_filename_component(test_name ${test_source} NAME_WE)
            set(test_target "widget-${id}-${test_name}")
            add_executable(${test_target} ${test_source})
            target_link_libraries(${test_target} PRIVATE hamclock-widgets)
            set_target_properties(${test_target} PROPERTIES AUTOMOC ON)
            if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
                target_compile_options(${test_target} PRIVATE -Wall -Wextra -Wpedantic)
            endif()
            if(W_TEST_DATA_DIR)
                add_test(NAME ${test_target} COMMAND ${test_target} ${CMAKE_CURRENT_SOURCE_DIR}/${W_TEST_DATA_DIR})
            else()
                add_test(NAME ${test_target} COMMAND ${test_target})
            endif()
            set_tests_properties(${test_target} PROPERTIES TIMEOUT 30
                ENVIRONMENT "QT_QPA_PLATFORM=offscreen;XDG_CACHE_HOME=${CMAKE_CURRENT_BINARY_DIR}/test-cache-${test_name};HOME=${CMAKE_CURRENT_BINARY_DIR}/test-home-${test_name}")
        endforeach()
    endif()
endfunction()

file(GLOB widget_lists CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/*/CMakeLists.txt)
foreach(widget_list ${widget_lists})
    get_filename_component(widget_dir ${widget_list} DIRECTORY)
    add_subdirectory(${widget_dir})
endforeach()

# Generated registration: one call per widget, in directory order; the
# registry sorts by each widget's declared order.
get_property(widget_ids GLOBAL PROPERTY HAMCLOCK_WIDGET_IDS)
set(widget_declarations "")
set(widget_calls "")
foreach(id ${widget_ids})
    string(APPEND widget_declarations "void register_widget_${id}(WidgetRegistry&, const WidgetContext&);\n")
    string(APPEND widget_calls "    register_widget_${id}(registry, context);\n")
endforeach()
file(CONFIGURE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/widgets_generated.cpp CONTENT
"// Generated by cmake/HamClockApp.cmake from hamclock_add_widget() calls. Do not edit.
#include \"${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/widget_registry.hpp\"
namespace hamclock {
${widget_declarations}
void register_all_widgets(WidgetRegistry& registry, const WidgetContext& context) {
${widget_calls}}
} // namespace hamclock
")

# Framework tests (data plumbing, registry, settings) with a fake transport.
if(BUILD_TESTING)
    add_executable(widget-core-regression tests/widget_core_regression.cpp)
    target_link_libraries(widget-core-regression PRIVATE hamclock-widget-core)
    set_target_properties(widget-core-regression PROPERTIES AUTOMOC ON)
    add_test(NAME widget-core-regression COMMAND widget-core-regression)
    set_tests_properties(widget-core-regression PROPERTIES TIMEOUT 30
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;XDG_CACHE_HOME=${CMAKE_CURRENT_BINARY_DIR}/test-cache-core;HOME=${CMAKE_CURRENT_BINARY_DIR}/test-home-core")
    add_executable(app-lifecycle-regression tests/app_lifecycle_regression.cpp)
    target_link_libraries(app-lifecycle-regression PRIVATE hamclock-widget-core)
    set_target_properties(app-lifecycle-regression PROPERTIES AUTOMOC ON)
    add_test(NAME app-lifecycle-regression COMMAND app-lifecycle-regression)
    set_tests_properties(app-lifecycle-regression PROPERTIES TIMEOUT 30 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        add_executable(app-lifecycle-mac-regression tests/app_lifecycle_mac_regression.mm)
        target_link_libraries(app-lifecycle-mac-regression PRIVATE hamclock-widget-core)
        set_target_properties(app-lifecycle-mac-regression PROPERTIES MACOSX_BUNDLE OFF)
        add_test(NAME app-lifecycle-mac-regression COMMAND app-lifecycle-mac-regression)
        set_tests_properties(app-lifecycle-mac-regression PROPERTIES TIMEOUT 30)
    endif()
    add_executable(pane-grid-regression tests/pane_grid_regression.cpp)
    target_link_libraries(pane-grid-regression PRIVATE hamclock-widget-core)
    add_test(NAME pane-grid-regression COMMAND pane-grid-regression)
    add_executable(safe-area-regression tests/safe_area_regression.cpp)
    target_link_libraries(safe-area-regression PRIVATE hamclock-widget-core)
    set_target_properties(safe-area-regression PROPERTIES AUTOMOC ON)
    add_test(NAME safe-area-regression COMMAND safe-area-regression)
    set_tests_properties(safe-area-regression PROPERTIES TIMEOUT 30 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    add_executable(attribution-regression tests/attribution_regression.cpp)
    target_link_libraries(attribution-regression PRIVATE hamclock-widget-core)
    set_target_properties(attribution-regression PROPERTIES AUTOMOC ON)
    add_test(NAME attribution-regression COMMAND attribution-regression)
    set_tests_properties(attribution-regression PROPERTIES TIMEOUT 30 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    add_executable(text-entry-hint-regression tests/text_entry_hint_regression.cpp)
    target_link_libraries(text-entry-hint-regression PRIVATE hamclock-widget-core)
    set_target_properties(text-entry-hint-regression PROPERTIES AUTOMOC ON)
    add_test(NAME text-entry-hint-regression COMMAND text-entry-hint-regression)
    set_tests_properties(text-entry-hint-regression PROPERTIES TIMEOUT 30 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

    add_executable(sky-pointer-regression tests/sky_pointer_regression.cpp)
    target_link_libraries(sky-pointer-regression PRIVATE hamclock-widget-core)
    add_test(NAME sky-pointer-regression COMMAND sky-pointer-regression)
    set_tests_properties(sky-pointer-regression PROPERTIES TIMEOUT 30 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    add_executable(sky-project-regression tests/sky_project_regression.cpp)
    target_link_libraries(sky-project-regression PRIVATE hamclock-widget-core)
    add_test(NAME sky-project-regression COMMAND sky-project-regression)
    set_tests_properties(sky-project-regression PROPERTIES TIMEOUT 30 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    add_executable(camera-fov-regression tests/camera_fov_regression.cpp)
    target_link_libraries(camera-fov-regression PRIVATE hamclock-widget-core)
    add_test(NAME camera-fov-regression COMMAND camera-fov-regression)
    set_tests_properties(camera-fov-regression PROPERTIES TIMEOUT 30 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    add_executable(sky-attitude-regression tests/sky_attitude_regression.cpp)
    target_link_libraries(sky-attitude-regression PRIVATE hamclock-widget-core)
    add_test(NAME sky-attitude-regression COMMAND sky-attitude-regression)
    set_tests_properties(sky-attitude-regression PROPERTIES TIMEOUT 30 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    add_executable(sky-finder-regression tests/sky_finder_regression.cpp)
    target_link_libraries(sky-finder-regression PRIVATE hamclock-widget-core)
    add_executable(pass-reminder-regression tests/pass_reminder_regression.cpp)
    target_link_libraries(pass-reminder-regression PRIVATE hamclock-widget-core)
    add_test(NAME pass-reminder-regression COMMAND pass-reminder-regression)
    set_tests_properties(pass-reminder-regression PROPERTIES TIMEOUT 30 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    add_test(NAME sky-finder-regression COMMAND sky-finder-regression)
    set_tests_properties(sky-finder-regression PROPERTIES TIMEOUT 30 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    add_executable(path-text-regression tests/path_text_regression.cpp src/domain/geometry.cpp)
    target_include_directories(path-text-regression PRIVATE src)
    target_link_libraries(path-text-regression PRIVATE Qt6::Core)
    add_test(NAME path-text-regression COMMAND path-text-regression)
    set_tests_properties(path-text-regression PROPERTIES TIMEOUT 30)
    add_executable(sun-text-regression tests/sun_text_regression.cpp src/domain/astro_sun.cpp)
    target_include_directories(sun-text-regression PRIVATE src)
    target_link_libraries(sun-text-regression PRIVATE Qt6::Core)
    add_test(NAME sun-text-regression COMMAND sun-text-regression)
    set_tests_properties(sun-text-regression PROPERTIES TIMEOUT 30)
    add_executable(clock-text-regression tests/clock_text_regression.cpp)
    target_include_directories(clock-text-regression PRIVATE src)
    target_link_libraries(clock-text-regression PRIVATE Qt6::Core)
    add_test(NAME clock-text-regression COMMAND clock-text-regression)
    set_tests_properties(clock-text-regression PROPERTIES TIMEOUT 30)
    add_executable(aux-time-text-regression tests/aux_time_text_regression.cpp)
    target_include_directories(aux-time-text-regression PRIVATE src)
    target_link_libraries(aux-time-text-regression PRIVATE Qt6::Core)
    add_test(NAME aux-time-text-regression COMMAND aux-time-text-regression)
    set_tests_properties(aux-time-text-regression PROPERTIES TIMEOUT 30)
    add_executable(timers-regression tests/timers_regression.cpp)
    target_include_directories(timers-regression PRIVATE src)
    target_link_libraries(timers-regression PRIVATE Qt6::Core)
    add_test(NAME timers-regression COMMAND timers-regression)
    set_tests_properties(timers-regression PROPERTIES TIMEOUT 30)
    add_executable(alarms-regression tests/alarms_regression.cpp src/ui/alarms_model.cpp)
    target_include_directories(alarms-regression PRIVATE src)
    target_link_libraries(alarms-regression PRIVATE Qt6::Core)
    set_target_properties(alarms-regression PROPERTIES AUTOMOC ON)
    add_test(NAME alarms-regression COMMAND alarms-regression)
    set_tests_properties(alarms-regression PROPERTIES TIMEOUT 30)
    add_executable(blanking-regression tests/blanking_regression.cpp src/ui/blanking_model.cpp)
    target_include_directories(blanking-regression PRIVATE src)
    target_link_libraries(blanking-regression PRIVATE Qt6::Core)
    set_target_properties(blanking-regression PROPERTIES AUTOMOC ON)
    add_test(NAME blanking-regression COMMAND blanking-regression)
    set_tests_properties(blanking-regression PROPERTIES TIMEOUT 30)
    add_executable(config-store-regression tests/config_store_regression.cpp)
    target_include_directories(config-store-regression PRIVATE src)
    target_link_libraries(config-store-regression PRIVATE Qt6::Core)
    add_test(NAME config-store-regression COMMAND config-store-regression)
    set_tests_properties(config-store-regression PROPERTIES TIMEOUT 30)
    add_executable(banner-text-regression tests/banner_text_regression.cpp)
    target_include_directories(banner-text-regression PRIVATE src)
    target_link_libraries(banner-text-regression PRIVATE Qt6::Core)
    add_test(NAME banner-text-regression COMMAND banner-text-regression)
    set_tests_properties(banner-text-regression PROPERTIES TIMEOUT 30)
    add_executable(time-zone-search-regression tests/time_zone_search_regression.cpp src/ui/time_zone_search.cpp)
    target_include_directories(time-zone-search-regression PRIVATE src)
    target_link_libraries(time-zone-search-regression PRIVATE Qt6::Core)
    set_target_properties(time-zone-search-regression PROPERTIES AUTOMOC ON)
    add_test(NAME time-zone-search-regression COMMAND time-zone-search-regression)
    set_tests_properties(time-zone-search-regression PROPERTIES TIMEOUT 30 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    add_executable(location-tools-regression tests/location_tools_regression.cpp src/ui/location_tools.cpp
        src/domain/locator_input.cpp src/domain/geometry.cpp)
    target_include_directories(location-tools-regression PRIVATE src)
    target_link_libraries(location-tools-regression PRIVATE Qt6::Core Qt6::Network)
    set_target_properties(location-tools-regression PROPERTIES AUTOMOC ON)
    add_test(NAME location-tools-regression COMMAND location-tools-regression
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/sources/nominatim-search.json)
    set_tests_properties(location-tools-regression PROPERTIES TIMEOUT 30)
    # The real app loads its QML without errors and renders a frame offscreen.
    add_test(NAME qml-app-smoke COMMAND hamclock-pro --offline --config-dir ${CMAKE_CURRENT_BINARY_DIR}/smoke-config
        --screenshot ${CMAKE_CURRENT_BINARY_DIR}/smoke.png --size 1024x600)
    # Exit code 4 = a visible Text is elided or clipped by its card/window (issues #152, #532; objectName clipOk exempts); sizes cover small, primary,
    # TV, phones and the adaptive pane grid on desktops (MacBook window, 1440p two rows, Retina full resolution).
    foreach(size 800x480 1024x600 1920x1080 390x844 844x390 600x1024 820x1180 1180x820 1512x982 2560x1440 3024x1964)
        add_test(NAME qml-app-truncation-${size} COMMAND hamclock-pro --offline --config-dir ${CMAKE_CURRENT_BINARY_DIR}/smoke-config-${size}
            --screenshot ${CMAKE_CURRENT_BINARY_DIR}/smoke-${size}.png --size ${size})
        set_tests_properties(qml-app-truncation-${size} PROPERTIES TIMEOUT 30
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen;XDG_CACHE_HOME=${CMAKE_CURRENT_BINARY_DIR}/smoke-cache")
    endforeach()
    # A configured station fills every card (#587: at 800x480 the content item stayed 1024x600 and the grab was cropped).
    # 1920x1080 and 2560x1440 add the adaptive grid with a station (#638: all slots were 0x0; exit 4 reports an empty pane slot).
    foreach(size 800x480 1024x600 1920x1080 2560x1440)
        add_test(NAME qml-app-station-${size} COMMAND hamclock-pro --offline --config-dir ${CMAKE_CURRENT_BINARY_DIR}/smoke-config-station-${size}
            --station ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/station-configured.conf
            --screenshot ${CMAKE_CURRENT_BINARY_DIR}/smoke-station-${size}.png --size ${size})
        set_tests_properties(qml-app-station-${size} PROPERTIES TIMEOUT 30
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen;XDG_CACHE_HOME=${CMAKE_CURRENT_BINARY_DIR}/smoke-cache")
    endforeach()
    set_tests_properties(qml-app-smoke PROPERTIES TIMEOUT 30
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;XDG_CACHE_HOME=${CMAKE_CURRENT_BINARY_DIR}/smoke-cache")
    # A long reverse-geocoded place ("DX · Greater London, GB") must not be cut short on desktop windows.
    foreach(size 1512x982 2560x1440)
        add_test(NAME qml-app-weather-long-place-${size} COMMAND hamclock-pro --config-dir ${CMAKE_CURRENT_BINARY_DIR}/weather-place-config-${size}
            --station ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/station-configured.conf --pane 1=weather
            --fixtures ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/weather/tests/fixtures-long-place
            --screenshot ${CMAKE_CURRENT_BINARY_DIR}/weather-place-${size}.png --size ${size})
        set_tests_properties(qml-app-weather-long-place-${size} PROPERTIES TIMEOUT 30
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen;XDG_CACHE_HOME=${CMAKE_CURRENT_BINARY_DIR}/weather-place-cache-${size}")
    endforeach()
    # Widget panes only report truncation (widget layouts belong to other areas); the app smokes above enforce it.
    # Every registered widget renders in the first pane at the smallest size without QML warnings.
    foreach(id ${widget_ids})
        add_test(NAME qml-pane-${id} COMMAND hamclock-pro --offline --config-dir ${CMAKE_CURRENT_BINARY_DIR}/pane-config-${id}
            --pane 1=${id} --allow-truncation --screenshot ${CMAKE_CURRENT_BINARY_DIR}/pane-${id}.png --size 800x480)
        set_tests_properties(qml-pane-${id} PROPERTIES TIMEOUT 30
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen;XDG_CACHE_HOME=${CMAKE_CURRENT_BINARY_DIR}/pane-cache-${id}")
    endforeach()
endif()
