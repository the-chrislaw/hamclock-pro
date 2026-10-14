hamclock_add_widget(ota
    SOURCES ota_model.cpp ota_parse.cpp ota_overlay.cpp
    QML OtaCard.qml
    TESTS tests/ota_regression.cpp
    TEST_DATA_DIR tests/fixtures)
