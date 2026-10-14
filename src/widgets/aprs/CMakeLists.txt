# telnet_client.cpp (TelnetSocket, the TCP factory, backoff) is compiled once, by the dxcluster widget.
hamclock_add_widget(aprs
    SOURCES aprs_parse.cpp aprs_model.cpp
    QML AprsCard.qml
    TESTS tests/aprs_regression.cpp)
