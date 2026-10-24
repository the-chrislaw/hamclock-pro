# Satellites: vendored SGP4 + wrapper (tested separately), and the pane.
# Vendored reference code is compiled warning-free in its own library, unmodified.
add_library(hamclock-sgp4 STATIC ${CMAKE_SOURCE_DIR}/src/domain/astro_sgp4/SGP4.cpp)
target_compile_options(hamclock-sgp4 PRIVATE -w)
target_link_libraries(hamclock-widgets PUBLIC hamclock-sgp4)
target_sources(hamclock-widgets PRIVATE ${CMAKE_SOURCE_DIR}/src/domain/astro_satellite.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/astro_finder.cpp
    ${CMAKE_SOURCE_DIR}/src/domain/astro_transmitters.cpp)
hamclock_add_widget(satellites
    SOURCES satellites_model.cpp
    QML SatellitesCard.qml
    TESTS tests/satellites_model_regression.cpp
    TEST_DATA_DIR tests/fixtures)
if(BUILD_TESTING)
    add_executable(widget-satellites-satellites_regression tests/satellites_regression.cpp)
    target_link_libraries(widget-satellites-satellites_regression PRIVATE hamclock-widgets)
    set_target_properties(widget-satellites-satellites_regression PROPERTIES AUTOMOC ON)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(widget-satellites-satellites_regression PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME widget-satellites-satellites_regression
        COMMAND widget-satellites-satellites_regression ${CMAKE_SOURCE_DIR}/tests/fixtures/sources/celestrak-amateur.tle)
    set_tests_properties(widget-satellites-satellites_regression PROPERTIES TIMEOUT 30)
endif()
if(BUILD_TESTING)
    add_executable(widget-satellites-transmitters_regression tests/transmitters_regression.cpp)
    target_link_libraries(widget-satellites-transmitters_regression PRIVATE hamclock-widgets)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(widget-satellites-transmitters_regression PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME widget-satellites-transmitters_regression
        COMMAND widget-satellites-transmitters_regression
            ${CMAKE_SOURCE_DIR}/tests/fixtures/sources/satnogs-transmitters.json
            ${CMAKE_SOURCE_DIR}/tests/fixtures/sources/celestrak-amateur.tle)
    set_tests_properties(widget-satellites-transmitters_regression PROPERTIES TIMEOUT 30)
endif()
if(BUILD_TESTING)
    add_executable(widget-satellites-finder_regression tests/finder_regression.cpp)
    target_link_libraries(widget-satellites-finder_regression PRIVATE hamclock-widgets)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(widget-satellites-finder_regression PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME widget-satellites-finder_regression
        COMMAND widget-satellites-finder_regression ${CMAKE_SOURCE_DIR}/tests/fixtures/sources/celestrak-amateur.tle)
    set_tests_properties(widget-satellites-finder_regression PROPERTIES TIMEOUT 30)
endif()
hamclock_add_widget(passes
    SOURCES passes_model.cpp
    QML PassesCard.qml
    TESTS tests/passes_regression.cpp
    TEST_DATA_DIR ../../../tests/fixtures/sources)
