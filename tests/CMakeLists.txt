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
