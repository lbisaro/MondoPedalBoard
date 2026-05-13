cmake_minimum_required(VERSION 3.22)

project(MondoHelixAnalyzer VERSION 1.0.0)

# C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE subdirectory
add_subdirectory(libs/JUCE)

# Create the JUCE audio plugin target
juce_add_plugin(MondoHelixAnalyzer
    # IS_SYNTH FALSE             
    # NEEDS_MIDI_INPUT FALSE     
    # NEEDS_MIDI_OUTPUT FALSE    
    # IS_MIDI_EFFECT FALSE       
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE "L6MD"
    PLUGIN_CODE "Mhza"
    FORMATS Standalone VST3
    PRODUCT_NAME "Mondo Helix Analyzer"
)

# Generar el JuceHeader.h
juce_generate_juce_header(MondoHelixAnalyzer)

# Target configuration flags
target_compile_definitions(MondoHelixAnalyzer PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

# Enable ASIO on Windows for the Standalone plugin
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_compile_definitions(MondoHelixAnalyzer PUBLIC JUCE_ASIO=1)
endif()

# CoreAudio is enabled by default on macOS, but explicit declaration doesn't hurt
if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_compile_definitions(MondoHelixAnalyzer PUBLIC JUCE_COREAUDIO=1)
endif()

# Source files
target_sources(MondoHelixAnalyzer PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
)

# Link to JUCE modules
target_link_libraries(MondoHelixAnalyzer PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_gui_extra
    juce::juce_gui_basics
    juce::juce_graphics
    juce::juce_events
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_plugin_client
    juce::juce_dsp
)
