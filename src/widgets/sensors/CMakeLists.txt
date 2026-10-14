hamclock_add_widget(sensors
    SOURCES sensors_model.cpp sensors_read.cpp ../../platform/ntp_status.cpp ../../platform/gpio.cpp
    QML SensorsCard.qml
    TESTS tests/sensors_regression.cpp tests/gpio_regression.cpp)
