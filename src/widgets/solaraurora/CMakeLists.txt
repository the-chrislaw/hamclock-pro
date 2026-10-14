hamclock_add_widget(solaraurora
    SOURCES solar_aurora_model.cpp solar_aurora_parse.cpp
    QML SolarAuroraCard.qml
    TESTS tests/solar_aurora_regression.cpp
    TEST_DATA_DIR tests/fixtures)
