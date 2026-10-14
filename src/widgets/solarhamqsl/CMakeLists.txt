hamclock_add_widget(solarhamqsl
    SOURCES solar_hamqsl_model.cpp solar_hamqsl_parse.cpp
    QML SolarHamqslCard.qml
    TESTS tests/solar_hamqsl_regression.cpp
    TEST_DATA_DIR tests/fixtures)
