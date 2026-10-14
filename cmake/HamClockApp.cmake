# HamClock Pro Qt Quick application and widget framework.
# Widgets live in src/widgets/<id>/ and call hamclock_add_widget() from their
# own CMakeLists.txt, so adding a widget never requires editing this file.

find_package(Qt6 6.4 REQUIRED COMPONENTS Network Quick QuickControls2)

# Core shared by all widgets: data plumbing, widget base/registry, settings.
add_library(hamclock-widget-core STATIC
    src/net/polled_source.cpp
    src/platform/apple_lifecycle.cpp
    src/widgets/widget_model.cpp
    src/widgets/widget_registry.cpp
    src/ui/app_settings.cpp
    src/platform/settings.cpp)
target_link_libraries(hamclock-widget-core PUBLIC hamclock-map-core Qt6::Core Qt6::Gui Qt6::Network)
set_target_properties(hamclock-widget-core PROPERTIES AUTOMOC ON)

# All widget models are compiled into this library by hamclock_add_widget().
add_library(hamclock-widgets STATIC)
target_link_libraries(hamclock-widgets PUBLIC hamclock-widget-core)
set_target_properties(hamclock-widgets PROPERTIES AUTOMOC ON)

add_executable(hamclock-pro MACOSX_BUNDLE
    src/ui/main.cpp
    src/ui/station_model.cpp
    src/ui/time_zone_search.cpp
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
    src/ui/qml/PaneSlot.qml src/ui/qml/PaneChooser.qml src/ui/qml/PaneOptions.qml src/ui/qml/MapMenu.qml src/ui/qml/MapTapMenu.qml src/ui/qml/TimeZonePicker.qml)
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
    add_executable(path-text-regression tests/path_text_regression.cpp)
    target_include_directories(path-text-regression PRIVATE src)
    target_link_libraries(path-text-regression PRIVATE Qt6::Core)
    add_test(NAME path-text-regression COMMAND path-text-regression)
    set_tests_properties(path-text-regression PROPERTIES TIMEOUT 30)
    add_executable(time-zone-search-regression tests/time_zone_search_regression.cpp src/ui/time_zone_search.cpp)
    target_include_directories(time-zone-search-regression PRIVATE src)
    target_link_libraries(time-zone-search-regression PRIVATE Qt6::Core)
    set_target_properties(time-zone-search-regression PROPERTIES AUTOMOC ON)
    add_test(NAME time-zone-search-regression COMMAND time-zone-search-regression)
    set_tests_properties(time-zone-search-regression PROPERTIES TIMEOUT 30 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
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
    # Every registered widget renders in the first pane at the smallest size without QML warnings.
    foreach(id ${widget_ids})
        add_test(NAME qml-pane-${id} COMMAND hamclock-pro --offline --config-dir ${CMAKE_CURRENT_BINARY_DIR}/pane-config-${id}
            --pane 1=${id} --screenshot ${CMAKE_CURRENT_BINARY_DIR}/pane-${id}.png --size 800x480)
        set_tests_properties(qml-pane-${id} PROPERTIES TIMEOUT 30
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen;XDG_CACHE_HOME=${CMAKE_CURRENT_BINARY_DIR}/pane-cache-${id}")
    endforeach()
endif()
