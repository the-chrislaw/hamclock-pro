# Satellite propagation domain code (vendored SGP4 + wrapper). The pane itself
# follows in a later slice; for now only the tested library code is built.
# Vendored reference code is compiled warning-free in its own library, unmodified.
add_library(hamclock-sgp4 STATIC ${CMAKE_SOURCE_DIR}/src/domain/astro_sgp4/SGP4.cpp)
target_compile_options(hamclock-sgp4 PRIVATE -w)
target_link_libraries(hamclock-widgets PUBLIC hamclock-sgp4)
target_sources(hamclock-widgets PRIVATE ${CMAKE_SOURCE_DIR}/src/domain/astro_satellite.cpp)
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
