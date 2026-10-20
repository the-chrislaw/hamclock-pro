hamclock_add_widget(balloons
    SOURCES balloons_model.cpp balloons_parse.cpp
    QML BalloonsCard.qml
    TESTS tests/balloons_regression.cpp
    TEST_DATA_DIR tests/fixtures)
