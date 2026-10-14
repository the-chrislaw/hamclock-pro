# dx_spot_parse.cpp and telnet_client.cpp are compiled once, by the dxcluster widget.
hamclock_add_widget(hamalert
    SOURCES hamalert_model.cpp
    QML HamAlertCard.qml
    TESTS tests/hamalert_regression.cpp)
