hamclock_add_widget(spaceweather
    SOURCES space_weather_model.cpp swpc_parse.cpp
    QML SpaceWeatherCard.qml
    TESTS tests/space_weather_regression.cpp
    TEST_DATA_DIR tests/fixtures)
