cmake_minimum_required(VERSION 3.22)
project(Dkash47GranularSynth VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Expect JUCE to be available locally. Provide -DJUCE_DIR=path to the JUCE folder (containing CMakeLists.txt)
if (NOT DEFINED JUCE_DIR)
    message(FATAL_ERROR "Please configure JUCE by passing -DJUCE_DIR=path/to/JUCE")
endif()

add_subdirectory(${JUCE_DIR} JUCE)

# Custom VST3 copy location on D drive
set(JUCE_VST3_COPY_DIR "D:/VST3")

juce_add_plugin(Dkash47GranularSynth
    VERSION ${PROJECT_VERSION}
    COMPANY_NAME "Dkash47"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Dkas
    PLUGIN_CODE DkGS
    FORMATS VST3 Standalone
    PRODUCT_NAME "Dkash47 Granular Synthesizer")

juce_generate_juce_header(Dkash47GranularSynth)

set(SOURCE_FILES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/GranularEngine.cpp
    Source/GranularEngine.h
    Source/PluginProcessor.h
    Source/PluginEditor.h
    Source/ParameterIDs.h
    Source/GlassmorphicLookAndFeel.h
)

# Resources (e.g., images) can be added here later

target_sources(Dkash47GranularSynth PRIVATE ${SOURCE_FILES})

# Useful JUCE/utility modules for synth/effects work
target_link_libraries(Dkash47GranularSynth
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_formats
        juce::juce_dsp
        juce::juce_gui_extra
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

# Disable splash and usage reporting
target_compile_definitions(Dkash47GranularSynth PUBLIC
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_USE_MP3AUDIOFORMAT=1
)

# Make Visual Studio start in the source dir when debugging
set_target_properties(Dkash47GranularSynth PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)