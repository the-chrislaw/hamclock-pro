hamclock_add_widget(eme
    SOURCES eme_model.cpp ${CMAKE_SOURCE_DIR}/src/domain/astro_eme.cpp
            ${CMAKE_SOURCE_DIR}/src/domain/astro_moon.cpp
    QML EmeCard.qml
    TESTS tests/eme_regression.cpp)
