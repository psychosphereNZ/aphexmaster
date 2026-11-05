cmake_minimum_required(VERSION 3.22)
project(ApexMaster VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(APEX_BUILD_TESTS "Build the offline test harness" OFF)

# JUCE: use ./JUCE if present, otherwise download JUCE 8.0.4
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

set(APEX_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND APEX_FORMATS AU)
endif()

juce_add_plugin(ApexMaster
    COMPANY_NAME "G Audio"
    PRODUCT_NAME "Apex Master"
    BUNDLE_ID "com.gaudio.apexmaster"
    PLUGIN_MANUFACTURER_CODE GAud
    PLUGIN_CODE ApxM
    FORMATS ${APEX_FORMATS}
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    VST3_CATEGORIES Fx Mastering Dynamics EQ
    AU_MAIN_TYPE kAudioUnitType_Effect
    COPY_PLUGIN_AFTER_BUILD FALSE)

set(APEX_SOURCES
    PluginProcessor.cpp
    PluginEditor.cpp)

target_sources(ApexMaster PRIVATE ${APEX_SOURCES})

target_compile_definitions(ApexMaster PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(ApexMaster
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if(APEX_BUILD_TESTS)
    juce_add_console_app(ApexMasterTest PRODUCT_NAME "ApexMasterTest")
    target_sources(ApexMasterTest PRIVATE TestMain.cpp ${APEX_SOURCES})
    target_compile_definitions(ApexMasterTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(ApexMasterTest PRIVATE
        juce::juce_audio_utils juce::juce_dsp
        juce::juce_recommended_config_flags)
endif()
