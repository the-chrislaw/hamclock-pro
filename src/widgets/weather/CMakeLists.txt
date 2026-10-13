hamclock_add_widget(weather
    SOURCES weather_model.cpp weather_parse.cpp
    QML WeatherCard.qml
    TESTS tests/weather_regression.cpp
    TEST_DATA_DIR tests/fixtures)
