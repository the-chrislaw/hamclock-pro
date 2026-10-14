hamclock_add_widget(planets
    SOURCES planets_model.cpp ${CMAKE_SOURCE_DIR}/src/domain/astro_planets.cpp
    QML PlanetsCard.qml
    TESTS tests/planets_regression.cpp)
