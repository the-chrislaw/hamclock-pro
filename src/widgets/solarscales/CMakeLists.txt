hamclock_add_widget(solarscales
    SOURCES solar_scales_model.cpp solar_scales_parse.cpp
    QML SolarScalesCard.qml
    TESTS tests/solar_scales_regression.cpp
    TEST_DATA_DIR tests/fixtures)
