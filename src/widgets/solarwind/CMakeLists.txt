hamclock_add_widget(solarwind
    SOURCES solar_wind_model.cpp solar_wind_parse.cpp
    QML SolarWindCard.qml Sparkline.qml
    TESTS tests/solar_wind_regression.cpp
    TEST_DATA_DIR tests/fixtures)
