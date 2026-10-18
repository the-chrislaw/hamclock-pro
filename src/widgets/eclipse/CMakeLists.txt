hamclock_add_widget(eclipse
    SOURCES eclipse_model.cpp ${CMAKE_SOURCE_DIR}/src/domain/astro_eclipse.cpp
            ${CMAKE_SOURCE_DIR}/src/domain/astro_moon.cpp ${CMAKE_SOURCE_DIR}/src/domain/solar_position.cpp
            ${CMAKE_SOURCE_DIR}/src/domain/astro_elp.cpp
    QML EclipseCard.qml
    TESTS tests/eclipse_regression.cpp tests/elp_regression.cpp)
