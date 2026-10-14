hamclock_add_widget(solardrap
    SOURCES solar_drap_model.cpp solar_drap_parse.cpp
    QML SolarDrapCard.qml
    TESTS tests/solar_drap_regression.cpp
    TEST_DATA_DIR tests/fixtures)
