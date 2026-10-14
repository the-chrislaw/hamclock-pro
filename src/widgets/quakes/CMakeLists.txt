hamclock_add_widget(quakes
    SOURCES quakes_model.cpp quakes_parse.cpp quakes_overlay.cpp
    QML QuakesCard.qml
    TESTS tests/quakes_regression.cpp
    TEST_DATA_DIR tests/fixtures)
