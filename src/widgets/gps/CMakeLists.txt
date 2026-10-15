hamclock_add_widget(gps
    SOURCES gps_model.cpp gps_parse.cpp gps_nmea.cpp gps_serial.cpp
    QML GpsCard.qml
    TESTS tests/gps_regression.cpp)
