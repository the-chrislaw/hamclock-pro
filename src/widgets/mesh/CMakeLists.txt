# telnet_client.cpp (TelnetSocket, the TCP factory, backoff) is compiled once, by the dxcluster widget.
hamclock_add_widget(mesh
    SOURCES mesh_parse.cpp mesh_overlay.cpp mesh_model.cpp
    QML MeshCard.qml
    TESTS tests/mesh_regression.cpp)
