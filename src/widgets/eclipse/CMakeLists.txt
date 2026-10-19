hamclock_add_widget(eclipse
    SOURCES eclipse_model.cpp ${CMAKE_SOURCE_DIR}/src/domain/astro_eclipse.cpp
            ${CMAKE_SOURCE_DIR}/src/domain/astro_eclipse_contacts.cpp
            ${CMAKE_SOURCE_DIR}/src/domain/astro_moon.cpp ${CMAKE_SOURCE_DIR}/src/domain/solar_position.cpp
            ${CMAKE_SOURCE_DIR}/src/domain/astro_elp.cpp
            ${CMAKE_SOURCE_DIR}/src/domain/astro_vsop.cpp
    QML EclipseCard.qml
    TESTS tests/eclipse_contacts_regression.cpp tests/eclipse_regression.cpp tests/elp_regression.cpp tests/vsop_regression.cpp)
