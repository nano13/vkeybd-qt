# Application sources (mirrors the units formerly built by vkeybd-qt.pro).
add_executable(vkeybd-qt
    button_piano.cpp
    combo_keyboard_select.cpp
    config.cpp
    help_about_widget.cpp
    help_message.cpp
    input_keyboard_qt.cpp
    input_keyboard_raw.cpp
    input_keyboard_select.cpp
    input_keyboard_udp.cpp
    input_midi.cpp
    interface_alsa.cpp
    interface_audio.cpp
    interface_jack.cpp
    interface_notify.cpp
    interface_notify_dbus.cpp
    jack_save_load.cpp
    keyboard_notes.cpp
    keyboard_pc.cpp
    keyboard_piano.cpp
    main.cpp
    main_subtabs_switcher.cpp
    main_tabs.cpp
    main_tabs_switcher.cpp
    main_window.cpp
    menubar.cpp
    midi_channel_selector.cpp
    midi_channels_summary.cpp
    midi_engine.cpp
    midi_key_selector.cpp
    midi_key_shift_global.cpp
    midi_key_shift_widget.cpp
    midi_mastervolume.cpp
    midi_pitchwheel.cpp
    midi_sounds_list.cpp
    orgelwerk.cpp
    preset_serializer.cpp
    button_piano.h
    combo_keyboard_select.h
    config.h
    enums_structs.h
    help_about_widget.h
    help_message.h
    input_keyboard_qt.h
    input_keyboard_raw.h
    input_keyboard_select.h
    input_keyboard_udp.h
    input_midi.h
    interface_alsa.h
    interface_audio.h
    interface_jack.h
    interface_notify.h
    interface_notify_dbus.h
    jack_save_load.h
    keyboard_notes.h
    keyboard_pc.h
    keyboard_piano.h
    main_subtabs_switcher.h
    main_tabs.h
    main_tabs_switcher.h
    main_window.h
    menubar.h
    midi_channel_selector.h
    midi_channels_summary.h
    midi_engine.h
    midi_key_selector.h
    midi_key_shift_global.h
    midi_key_shift_widget.h
    midi_mastervolume.h
    midi_pitchwheel.h
    midi_sounds_list.h
    orgelwerk.h
    preset_model.h
    preset_serializer.h
    resources.qrc
)

# Leave the executable at build-debug/vkeybd-qt or build-release/vkeybd-qt.
set_target_properties(vkeybd-qt PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}"
)

target_compile_definitions(vkeybd-qt PRIVATE
    QT_DISABLE_DEPRECATED_BEFORE=0x060000
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(ALSA REQUIRED IMPORTED_TARGET alsa)
pkg_check_modules(JACK REQUIRED IMPORTED_TARGET jack)

target_link_libraries(vkeybd-qt PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Svg
    Qt6::SvgWidgets
    Qt6::Network
    Qt6::DBus
    PkgConfig::ALSA
    PkgConfig::JACK
)
