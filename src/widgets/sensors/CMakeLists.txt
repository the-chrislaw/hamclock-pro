hamclock_add_widget(sensors
    SOURCES sensors_model.cpp sensors_read.cpp
    QML SensorsCard.qml
    TESTS tests/sensors_regression.cpp)
