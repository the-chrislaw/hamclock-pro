hamclock_add_widget(storms
    SOURCES storms_model.cpp storms_parse.cpp storms_overlay.cpp
    QML StormsCard.qml
    TESTS tests/storms_regression.cpp
    TEST_DATA_DIR tests/fixtures)
