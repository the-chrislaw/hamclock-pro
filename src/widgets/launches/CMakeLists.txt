hamclock_add_widget(launches
    SOURCES launches_model.cpp launches_parse.cpp
    QML LaunchesCard.qml
    TESTS tests/launches_regression.cpp
    TEST_DATA_DIR tests/fixtures)
