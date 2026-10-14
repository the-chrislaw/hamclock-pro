hamclock_add_widget(daylight
    SOURCES daylight_model.cpp ${CMAKE_SOURCE_DIR}/src/domain/astro_sun.cpp
    QML DaylightCard.qml
    TESTS tests/daylight_regression.cpp)
