cmake_minimum_required(VERSION 3.16)
project(HamClockProStationEditor VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Widgets)

add_executable(hamclock-pro-editor MACOSX_BUNDLE
    src/qt/station_editor.cpp
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
