hamclock_add_widget(adif
    SOURCES adif_model.cpp adif_parse.cpp adif_overlay.cpp
    QML AdifCard.qml
    TESTS tests/adif_regression.cpp)
