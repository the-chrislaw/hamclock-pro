hamclock_add_widget(dxcluster
    SOURCES dx_cluster_model.cpp dx_spot_parse.cpp udp_spot.cpp ${CMAKE_SOURCE_DIR}/src/net/telnet_client.cpp
    QML DxClusterCard.qml
    TESTS tests/dx_cluster_regression.cpp tests/udp_spot_regression.cpp ../watchlist/tests/watchlist_regression.cpp
    TEST_DATA_DIR tests/fixtures)
