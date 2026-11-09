# GUI-independent QtTest suite, built against the production core sources.
add_executable(tst_core
    tst_core.cpp
    ${PROJECT_SOURCE_DIR}/src/interface_audio.cpp
    ${PROJECT_SOURCE_DIR}/src/interface_audio.h
    ${PROJECT_SOURCE_DIR}/src/midi_engine.cpp
    ${PROJECT_SOURCE_DIR}/src/midi_engine.h
    ${PROJECT_SOURCE_DIR}/src/midi_sounds_list.cpp
    ${PROJECT_SOURCE_DIR}/src/midi_sounds_list.h
    ${PROJECT_SOURCE_DIR}/src/preset_model.h
    ${PROJECT_SOURCE_DIR}/src/preset_serializer.cpp
    ${PROJECT_SOURCE_DIR}/src/preset_serializer.h
)

target_include_directories(tst_core PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(tst_core PRIVATE Qt6::Core Qt6::Test)

add_test(NAME core COMMAND tst_core)

# Exercise the Qt-default key tracker without physical input devices or a display.
add_executable(tst_keyboard_qt
    tst_keyboard_qt.cpp
    ${PROJECT_SOURCE_DIR}/src/input_keyboard_qt.cpp
    ${PROJECT_SOURCE_DIR}/src/input_keyboard_qt.h
)

target_include_directories(tst_keyboard_qt PRIVATE ${PROJECT_SOURCE_DIR}/src)
# input_keyboard_qt.h includes enums_structs.h, which declares Qt Widgets types.
target_link_libraries(tst_keyboard_qt PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test)

add_test(NAME keyboard_qt COMMAND tst_keyboard_qt)

# Exercise the PC-owned note mapping and responsive piano widget geometry.
add_executable(tst_keyboard_layout
    tst_keyboard_layout.cpp
    ${PROJECT_SOURCE_DIR}/src/keyboard_pc.cpp
    ${PROJECT_SOURCE_DIR}/src/keyboard_pc.h
    ${PROJECT_SOURCE_DIR}/src/keyboard_piano.cpp
    ${PROJECT_SOURCE_DIR}/src/keyboard_piano.h
    ${PROJECT_SOURCE_DIR}/src/button_piano.cpp
    ${PROJECT_SOURCE_DIR}/src/button_piano.h
)
target_include_directories(tst_keyboard_layout PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(tst_keyboard_layout PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test)
add_test(NAME keyboard_layout COMMAND tst_keyboard_layout)
set_tests_properties(keyboard_layout PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
