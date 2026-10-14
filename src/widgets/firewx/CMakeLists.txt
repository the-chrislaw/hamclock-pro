hamclock_add_widget(firewx
    SOURCES firewx_model.cpp firewx_parse.cpp
    QML FirewxCard.qml
    TESTS tests/firewx_regression.cpp
    TEST_DATA_DIR tests/fixtures)
