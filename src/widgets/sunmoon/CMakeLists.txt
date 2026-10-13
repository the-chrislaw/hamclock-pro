hamclock_add_widget(sunmoon
    SOURCES sun_moon_model.cpp ${CMAKE_SOURCE_DIR}/src/domain/astro_sun.cpp
            ${CMAKE_SOURCE_DIR}/src/domain/astro_moon.cpp
    QML SunMoonCard.qml
    TESTS tests/sun_regression.cpp tests/moon_regression.cpp)
