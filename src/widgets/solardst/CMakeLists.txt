hamclock_add_widget(solardst
    SOURCES solar_dst_model.cpp solar_dst_parse.cpp
    QML SolarDstCard.qml DstLine.qml
    TESTS tests/solar_dst_regression.cpp
    TEST_DATA_DIR tests/fixtures)
