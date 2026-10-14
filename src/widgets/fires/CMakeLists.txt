hamclock_add_widget(fires
    SOURCES fires_model.cpp fires_parse.cpp fires_overlay.cpp
    QML FiresCard.qml
    TESTS tests/fires_regression.cpp
    TEST_DATA_DIR tests/fixtures)
