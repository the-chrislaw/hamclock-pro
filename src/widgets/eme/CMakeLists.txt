# Vendored QR encoder: its own library, unmodified, warnings off for it only.
add_library(hamclock-qrcodegen STATIC ${CMAKE_SOURCE_DIR}/src/domain/astro_qr/qrcodegen.cpp)
target_compile_options(hamclock-qrcodegen PRIVATE -w)
target_link_libraries(hamclock-widgets PUBLIC hamclock-qrcodegen)
hamclock_add_widget(eme
    SOURCES eme_model.cpp ${CMAKE_SOURCE_DIR}/src/domain/astro_eme.cpp
            ${CMAKE_SOURCE_DIR}/src/domain/astro_moon.cpp
            ${CMAKE_SOURCE_DIR}/src/domain/astro_qr.cpp
    QML EmeCard.qml
    TESTS tests/eme_regression.cpp tests/qr_regression.cpp)
