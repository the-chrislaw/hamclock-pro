hamclock_add_widget(beacons
    SOURCES beacons_model.cpp ${CMAKE_SOURCE_DIR}/src/domain/astro_beacons.cpp
    QML BeaconsCard.qml
    TESTS tests/beacons_regression.cpp)
