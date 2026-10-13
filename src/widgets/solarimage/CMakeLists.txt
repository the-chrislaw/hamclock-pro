hamclock_add_widget(solarimage
    SOURCES solar_image_model.cpp
    QML SolarImageCard.qml
    TESTS tests/solar_image_regression.cpp
    TEST_DATA_DIR tests/fixtures)
