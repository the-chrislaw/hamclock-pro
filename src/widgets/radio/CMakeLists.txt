hamclock_add_widget(radio
    SOURCES radio_model.cpp radio_parse.cpp flrig_parse.cpp flrig_client.cpp
    QML RadioCard.qml
    TESTS tests/radio_regression.cpp)
