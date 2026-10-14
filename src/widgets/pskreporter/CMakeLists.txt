hamclock_add_widget(pskreporter
    SOURCES psk_model.cpp psk_parse.cpp
    QML PskCard.qml
    TESTS tests/psk_regression.cpp
    TEST_DATA_DIR tests/fixtures)
