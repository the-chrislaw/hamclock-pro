hamclock_add_widget(lookup
    SOURCES lookup_model.cpp lookup_parse.cpp
    QML LookupCard.qml
    TESTS tests/lookup_regression.cpp
    TEST_DATA_DIR tests/fixtures)
