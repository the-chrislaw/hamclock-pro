hamclock_add_widget(marine
    SOURCES marine_model.cpp marine_parse.cpp
    QML MarineCard.qml
    TESTS tests/marine_regression.cpp
    TEST_DATA_DIR tests/fixtures)
