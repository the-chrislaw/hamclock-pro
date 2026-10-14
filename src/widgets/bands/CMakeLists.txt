hamclock_add_widget(bands
    SOURCES bands_model.cpp band_outlook.cpp path_model.cpp ${CMAKE_SOURCE_DIR}/src/domain/solar_position.cpp
    QML BandsCard.qml
    TESTS tests/bands_regression.cpp tests/path_model_regression.cpp
    TEST_DATA_DIR tests/fixtures)
