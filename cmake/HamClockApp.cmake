# HamClock Pro Qt Quick application and widget framework.
# Widgets live in src/widgets/<id>/ and call hamclock_add_widget() from their
# own CMakeLists.txt, so adding a widget never requires editing this file.

find_package(Qt6 6.4 REQUIRED COMPONENTS Network Quick QuickControls2)

# Core shared by all widgets: data plumbing, widget base/registry, settings.
add_library(hamclock-widget-core STATIC
    src/net/polled_source.cpp
    src/platform/apple_lifecycle.cpp
    src/platform/apple_safearea.cpp
    src/widgets/widget_model.cpp
    src/widgets/widget_registry.cpp
    src/ui/app_settings.cpp
    src/platform/settings.cpp)
target_link_libraries(hamclock-widget-core PUBLIC hamclock-map-core Qt6::Core Qt6::Gui Qt6::Network)
set_target_properties(hamclock-widget-core PROPERTIES AUTOMOC ON)
if(IOS)
    target_sources(hamclock-widget-core PRIVATE src/platform/apple_safearea_ios.mm)
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
    src/ui/qml/PaneSlot.qml src/ui/qml/PaneChooser.qml src/ui/qml/PaneOptions.qml src/ui/qml/MapMenu.qml src/ui/qml/MapTapMenu.qml src/ui/qml/TimeZonePicker.qml src/ui/qml/TimersPage.qml)
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
    add_executable(safe-area-regression tests/safe_area_regression.cpp)
    target_link_libraries(safe-area-regression PRIVATE hamclock-widget-core)
    set_target_properties(safe-area-regression PROPERTIES AUTOMOC ON)
    add_test(NAME safe-area-regression COMMAND safe-area-regression)
    set_tests_properties(safe-area-regression PROPERTIES TIMEOUT 30 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
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
    add_executable(alarms-regression tests/alarms_regression.cpp)
    target_include_directories(alarms-regression PRIVATE src)
    target_link_libraries(alarms-regression PRIVATE Qt6::Core)
    add_test(NAME alarms-regression COMMAND alarms-regression)
    set_tests_properties(alarms-regression PROPERTIES TIMEOUT 30)
    add_executable(blanking-regression tests/blanking_regression.cpp src/ui/blanking_model.cpp)
    target_include_directories(blanking-regression PRIVATE src)
    target_link_libraries(blanking-regression PRIVATE Qt6::Core)
    set_target_properties(blanking-regression PROPERTIES AUTOMOC ON)
    add_test(NAME blanking-regression COMMAND blanking-regression)
    set_tests_properties(blanking-regression PROPERTIES TIMEOUT 30)
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
    # Exit code 4 = a visible Text is truncated (issue #152); sizes cover small, primary and TV.
    foreach(size 800x480 1920x1080)
        add_test(NAME qml-app-truncation-${size} COMMAND hamclock-pro --offline --config-dir ${CMAKE_CURRENT_BINARY_DIR}/smoke-config-${size}
            --screenshot ${CMAKE_CURRENT_BINARY_DIR}/smoke-${size}.png --size ${size})
        set_tests_properties(qml-app-truncation-${size} PROPERTIES TIMEOUT 30
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen;XDG_CACHE_HOME=${CMAKE_CURRENT_BINARY_DIR}/smoke-cache")
    endforeach()
    set_tests_properties(qml-app-smoke PROPERTIES TIMEOUT 30
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;XDG_CACHE_HOME=${CMAKE_CURRENT_BINARY_DIR}/smoke-cache")
    # Widget panes only report truncation (widget layouts belong to other areas); the app smokes above enforce it.
    # Every registered widget renders in the first pane at the smallest size without QML warnings.
    foreach(id ${widget_ids})
        add_test(NAME qml-pane-${id} COMMAND hamclock-pro --offline --config-dir ${CMAKE_CURRENT_BINARY_DIR}/pane-config-${id}
            --pane 1=${id} --allow-truncation --screenshot ${CMAKE_CURRENT_BINARY_DIR}/pane-${id}.png --size 800x480)
        set_tests_properties(qml-pane-${id} PROPERTIES TIMEOUT 30
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen;XDG_CACHE_HOME=${CMAKE_CURRENT_BINARY_DIR}/pane-cache-${id}")
    endforeach()
endif()
