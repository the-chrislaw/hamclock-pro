hamclock_add_widget(solartrend
    SOURCES solar_trend_model.cpp solar_trend_parse.cpp
    QML SolarTrendCard.qml TrendLine.qml KpBars.qml
    TESTS tests/solar_trend_regression.cpp
    TEST_DATA_DIR tests/fixtures)
