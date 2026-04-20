cmake_minimum_required(VERSION 3.22)

project(PatchSelectorPlugin VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

juce_add_plugin(PatchSelectorPlugin
    COMPANY_NAME "Matteo Bissoli"
    BUNDLE_ID com.matteobissoli.patchselector
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE OaiA
    PLUGIN_CODE Ptch
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "Patch Selector"
)

target_sources(PatchSelectorPlugin
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/model/PatchLibrary.h
        Source/repository/PatchLibraryLoader.cpp
        Source/repository/PatchLibraryLoader.h
        Source/midi/MidiDispatchEngine.cpp
        Source/midi/MidiDispatchEngine.h
)

target_compile_definitions(PatchSelectorPlugin
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(PatchSelectorPlugin
    PRIVATE
        juce::juce_audio_utils
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
        juce::juce_recommended_lto_flags
)
