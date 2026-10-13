hamclock_add_widget(ota
    SOURCES ota_model.cpp ota_parse.cpp
    QML OtaCard.qml
    TESTS tests/ota_regression.cpp
    TEST_DATA_DIR tests/fixtures)
