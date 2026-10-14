hamclock_add_widget(gps
    SOURCES gps_model.cpp gps_parse.cpp
    QML GpsCard.qml
    TESTS tests/gps_regression.cpp)
