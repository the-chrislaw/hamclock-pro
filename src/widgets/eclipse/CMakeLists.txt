hamclock_add_widget(eclipse
    SOURCES eclipse_model.cpp ${CMAKE_SOURCE_DIR}/src/domain/astro_eclipse.cpp
    QML EclipseCard.qml
    TESTS tests/eclipse_regression.cpp)
