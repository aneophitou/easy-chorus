cmake_minimum_required(VERSION 3.22)

project(EasyChorus VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.6
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(EasyChorus
    COMPANY_NAME "Easy Plugins"
    BUNDLE_ID com.easyplugins.easychorus
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE EzPl
    PLUGIN_CODE EzCh
    FORMATS VST3 LV2 Standalone
    PRODUCT_NAME "Easy Chorus"
    LV2URI "https://easyplugins.audio/plugins/easy-chorus"
)

juce_generate_juce_header(EasyChorus)

target_sources(EasyChorus
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/dsp/ChorusEngine.cpp
        Source/dsp/DelayEngine.cpp
        Source/dsp/DemoToneGenerator.cpp
        Source/dsp/ShimmerEngine.cpp
)

target_include_directories(EasyChorus PRIVATE Source)


target_compile_definitions(EasyChorus
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(EasyChorus
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Copy finished plugin bundles into Dist/ for easy install
set(DIST_DIR "${CMAKE_SOURCE_DIR}/Dist")
add_custom_target(copy_dist ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_DIR}"
    COMMENT "Preparing Dist/ folder"
)

add_dependencies(copy_dist EasyChorus_VST3 EasyChorus_LV2)

add_custom_command(TARGET EasyChorus_VST3 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${DIST_DIR}/VST3"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_DIR}/VST3"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "$<TARGET_FILE_DIR:EasyChorus_VST3>/../.."
        "${DIST_DIR}/VST3/Easy Chorus.vst3"
    COMMENT "Copying VST3 into Dist/"
)

add_custom_command(TARGET EasyChorus_LV2 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${DIST_DIR}/LV2"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_DIR}/LV2"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "$<TARGET_FILE_DIR:EasyChorus_LV2>"
        "${DIST_DIR}/LV2/Easy Chorus.lv2"
    COMMENT "Copying LV2 into Dist/"
)

# Offline WAV renderer for Project sound samples
juce_add_console_app(EasyChorusRenderSamples
    PRODUCT_NAME "EasyChorusRenderSamples"
)

juce_generate_juce_header(EasyChorusRenderSamples)

target_sources(EasyChorusRenderSamples
    PRIVATE
        tools/RenderSamples.cpp
        Source/dsp/ChorusEngine.cpp
        Source/dsp/DelayEngine.cpp
        Source/dsp/ShimmerEngine.cpp
)

target_compile_definitions(EasyChorusRenderSamples
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(EasyChorusRenderSamples
    PRIVATE
        juce::juce_audio_formats
        juce::juce_audio_basics
        juce::juce_dsp
        juce::juce_events
        juce::juce_core
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_data_structures
    PUBLIC
        juce::juce_recommended_config_flags
)
