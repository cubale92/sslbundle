cmake_minimum_required(VERSION 3.22)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")
project(8ceeConsole4K VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use a local JUCE checkout with -DJUCE_PATH=/path/to/JUCE, otherwise it is downloaded.
set(JUCE_PATH "" CACHE PATH "Path to a local JUCE checkout (optional)")
if(JUCE_PATH)
    add_subdirectory(${JUCE_PATH} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

set(ECEE_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND ECEE_FORMATS AU)
endif()

# Set ON to copy the built plug-ins into the system plug-in folders.
option(ECEE_COPY_PLUGINS "Copy plug-ins to the system plug-in folders after building" OFF)

function(ecee_add_plugin target product code category)
    juce_add_plugin(${target}
        COMPANY_NAME "8cee"
        COMPANY_WEBSITE ""
        BUNDLE_ID "com.eightcee.${target}"
        PLUGIN_MANUFACTURER_CODE Ecee
        PLUGIN_CODE ${code}
        PRODUCT_NAME "${product}"
        FORMATS ${ECEE_FORMATS}
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD ${ECEE_COPY_PLUGINS}
        VST3_CATEGORIES Fx ${category}
        AU_MAIN_TYPE kAudioUnitType_Effect)

    target_sources(${target} PRIVATE
        plugins/${target}/PluginProcessor.cpp
        plugins/${target}/PluginEditor.cpp)

    target_include_directories(${target} PRIVATE shared plugins/${target})

    target_compile_definitions(${target} PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)

    target_link_libraries(${target}
        PRIVATE
            juce::juce_audio_utils
            juce::juce_dsp
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)
endfunction()

# 8cee Console 4K bundle
ecee_add_plugin(Channel4K "8cee Channel-4K" Ch4k EQ)
ecee_add_plugin(Bus4K     "8cee Bus-4K"     Bs4k Dynamics)
ecee_add_plugin(Listen4K  "8cee Listen-4K"  Ls4k Dynamics)

# Dev tools (UI snapshots + offline DSP checks). Not needed to build the plug-ins.
option(ECEE_BUILD_TOOLS "Build the snapshot/test tools" OFF)
if(ECEE_BUILD_TOOLS)
    add_subdirectory(tools)
endif()
