hamclock_add_widget(bandsmatrix
    SOURCES bands_matrix_model.cpp ../bands/path_model.cpp ${CMAKE_SOURCE_DIR}/src/domain/solar_position.cpp
    QML BandsMatrixCard.qml
    TESTS tests/bands_matrix_regression.cpp
    TEST_DATA_DIR tests/fixtures)
