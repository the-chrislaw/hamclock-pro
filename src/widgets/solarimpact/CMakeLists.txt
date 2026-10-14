hamclock_add_widget(solarimpact
    SOURCES solar_impact_model.cpp
    QML SolarImpactCard.qml
    TESTS tests/solar_impact_regression.cpp
    TEST_DATA_DIR tests/fixtures)
