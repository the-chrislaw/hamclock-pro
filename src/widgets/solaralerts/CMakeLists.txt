hamclock_add_widget(solaralerts
    SOURCES solar_alerts_model.cpp solar_alerts_parse.cpp
    QML SolarAlertsCard.qml
    TESTS tests/solar_alerts_regression.cpp
    TEST_DATA_DIR tests/fixtures)
