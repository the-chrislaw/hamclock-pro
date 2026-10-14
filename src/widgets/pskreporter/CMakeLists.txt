hamclock_add_widget(pskreporter
    SOURCES psk_model.cpp psk_parse.cpp psk_overlay.cpp ../../domain/locator_input.cpp
    QML PskCard.qml
    TESTS tests/psk_regression.cpp
    TEST_DATA_DIR tests/fixtures)
