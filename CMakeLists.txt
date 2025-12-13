cmake_minimum_required(VERSION 3.22)

project(DeepMindSynth VERSION 0.0.1)

# --- JUCE Setup ---
# --- JUCE Setup (Local) ---
add_subdirectory("C:/JUCE" "${CMAKE_CURRENT_BINARY_DIR}/JUCE")

# --- Standard JUCE Options ---
# Enable VST3 and generic plugin client
juce_add_plugin(DeepMindSynth
    COMPANY_NAME "ABDMind"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE "Abdm"
    PLUGIN_CODE "Dmnd"
    FORMATS Standalone VST3
    PRODUCT_NAME "DeepMindSynth"
)

# Generate JuceHeader.h for compatibility with existing code
juce_generate_juce_header(DeepMindSynth)

# --- Optimization Flags (ARM/Raspberry Pi) ---
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
    message(STATUS "ARM64 Detected: Enabling aggressive optimizations for Raspberry Pi")
    target_compile_options(DeepMindSynth PRIVATE
        -O3
        -mcpu=native
        -mtune=native
        -funsafe-math-optimizations
    )
    # Enable NEON SIMD support in JUCE
    target_compile_definitions(DeepMindSynth PRIVATE JUCE_USE_SIMD=1)
else()
    message(STATUS "Standard x64 Build")
endif()

# --- Linux/Zynthian Specifics ---
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    # Link required Linux libraries
    target_link_libraries(DeepMindSynth PRIVATE
        alsa fontconfig freetype X11 Xext Xinerama Xrandr Xcursor Xcomposite Xrender
    )
    
    # Install LV2 to user directory for Zynthian/Carla detection
    install(TARGETS DeepMindSynth_LV2
        LIBRARY DESTINATION "$ENV{HOME}/.lv2"
        COMPONENT LV2
    )
endif()

# --- Source Files ---
# Recursively add all source files to the target
# Force re-glob: Added PresetManager, Midi, Osc, and FX Modules - Re-scan required
file(GLOB_RECURSE SourceFiles "Source/*.cpp" "Source/*.h" "Source/*.c" "Source/*.mm")

target_sources(DeepMindSynth PRIVATE
    ${SourceFiles}
)

# --- Include Directories ---
target_include_directories(DeepMindSynth PRIVATE
    Source
    Source/DSP
    Source/DSP/Oscillators
    Source/DSP/Filters
    Source/DSP/Modulation
    Source/DSP/Effects
    Source/GUI
    Source/Data
    Source/Voice
)

# --- JUCE Modules ---
target_link_libraries(DeepMindSynth PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
    juce::juce_core
    juce::juce_osc
)

# --- Definitions ---
target_compile_definitions(DeepMindSynth PRIVATE
    JUCE_VST3_CAN_REPLACE_VST2=0
)
