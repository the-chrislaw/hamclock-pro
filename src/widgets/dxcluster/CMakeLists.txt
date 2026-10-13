hamclock_add_widget(dxcluster
    SOURCES dx_cluster_model.cpp dx_spot_parse.cpp ${CMAKE_SOURCE_DIR}/src/net/telnet_client.cpp
    QML DxClusterCard.qml
    TESTS tests/dx_cluster_regression.cpp
    TEST_DATA_DIR tests/fixtures)
