hamclock_add_widget(hamsat
    SOURCES hamsat_model.cpp hamsat_parse.cpp
    QML HamsatCard.qml
    TESTS tests/hamsat_regression.cpp
    TEST_DATA_DIR tests/fixtures)
