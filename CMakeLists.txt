cmake_minimum_required(VERSION 3.22)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# These MUST come before project(): that is where CMake configures the Apple
# toolchain and reads them. Set afterwards, the deployment target is ignored
# and the architectures are unreliable (an arm64-only "universal" build).
# 10.13 rather than 11.0: 11.0 silently excludes Catalina.
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS deployment version")
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Target architectures")
endif()

project(StairwellToHeaven VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE, FxmeTools, clap-juce-extensions: sibling clones or the pinned commits.
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Dependencies.cmake)

juce_add_plugin(StairwellToHeaven
    COMPANY_NAME                "FX-Mechanics"
    COMPANY_WEBSITE             "www.fx-mechanics.com"
    PLUGIN_MANUFACTURER_CODE    FXME
    PLUGIN_CODE                 STWH
    FORMATS                     AU VST3 Standalone
    PRODUCT_NAME                "StairwellToHeaven"
    DESCRIPTION                 "FX-Mechanics algorithmic reverb"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES             Fx Reverb
    AU_MAIN_TYPE                kAudioUnitType_Effect
    BUNDLE_ID                   "com.fx-mechanics.StairwellToHeaven"
)

# CLAP, beside AU/VST3/Standalone. The ID is permanent once released.
fxmetools_add_clap(StairwellToHeaven
    ID       "com.fx-mechanics.StairwellToHeaven"
    FEATURES audio-effect reverb stereo)

file(GLOB_RECURSE STH_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h)
target_sources(StairwellToHeaven PRIVATE ${STH_SOURCES})
target_include_directories(StairwellToHeaven PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Dsp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Components)

# Images (logo, icon, splash) and factory presets (APVTS-state XML, root tag
# "Parameters"): PresetManager keeps the "*_xml" resources whose root tag
# matches and ignores the rest, so they share one binary-data target.
file(GLOB STH_ASSETS ${CMAKE_CURRENT_SOURCE_DIR}/Source/Assets/*.png
                     ${CMAKE_CURRENT_SOURCE_DIR}/Source/Assets/*.jpg
                     ${CMAKE_CURRENT_SOURCE_DIR}/Source/Assets/*.xml)
juce_add_binary_data(StairwellToHeavenBinaryData SOURCES ${STH_ASSETS})
set_target_properties(StairwellToHeavenBinaryData PROPERTIES INTERPROCEDURAL_OPTIMIZATION OFF)

juce_generate_juce_header(StairwellToHeaven)

target_compile_definitions(StairwellToHeaven
    PUBLIC
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(StairwellToHeaven
    PRIVATE
        StairwellToHeavenBinaryData
        FxmeTools
        FxmeCore
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Headless tests of the engine (Source/Dsp is JUCE-free): it links FxmeCore
# only, which is all it needs. Build with -DSTH_BUILD_TESTS=ON, run ctest.
option(STH_BUILD_TESTS "Build StairwellToHeaven's engine tests" OFF)
if(STH_BUILD_TESTS)
    enable_testing()
    add_executable(StairwellEngineTests Tests/EngineTests.cpp Source/Dsp/StairwellEngine.cpp)
    target_include_directories(StairwellEngineTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source/Dsp)
    target_link_libraries(StairwellEngineTests PRIVATE FxmeCore)
    add_test(NAME StairwellEngineTests COMMAND StairwellEngineTests)
endif()
