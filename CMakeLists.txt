cmake_minimum_required(VERSION 3.12)
project(UltralightJUCE VERSION 1)

set(CMAKE_CXX_STANDARD 20)

# Add JUCE as a subdirectory
add_subdirectory(JUCE)

add_subdirectory(Libs/ultralight-sdk)

# Add your project's source files
set(SOURCES
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        # Add more source files here
        )

juce_add_plugin(${PROJECT_NAME}
        # ICON_BIG ${CMAKE_CURRENT_SOURCE_DIR}/icon.png
        # ICON_SMALL ${CMAKE_CURRENT_SOURCE_DIR}/icon.png
        COMPANY_NAME ${TMPL_CREATOR}
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD TRUE
        PLUGIN_MANUFACTURER_CODE ${TMPL_MANUFACTURER_CODE}
        PLUGIN_CODE ${TMPL_PLUGIN_CODE}
        DESCRIPTION "${TMPL_DESCRIPTION}"
        VST3_CATEGORIES "Instrument Synth Stereo"
        AU_MAIN_TYPE "kAudioUnitType_MusicDevice"
        FORMATS VST3 AU Standalone
        BUNDLE_ID "${TMPL_BUNDLE_ID}"
        HARDENED_RUNTIME_ENABLED TRUE
        PRODUCT_NAME ${TMPL_SHORT_NAME})

juce_generate_juce_header(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
        PUBLIC
        Libs/ultralight-sdk/include
        )

target_sources(${PROJECT_NAME} PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/GUIMainComponent.cpp
        )

target_compile_definitions(${PROJECT_NAME}
        PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        )

# Set the paths where CMake should search for libraries
set(CMAKE_LIBRARY_PATH "${CMAKE_LIBRARY_PATH};Libs/ultralight-sdk/bin")
# Specify the names of the Ultralight libraries
set(ULTRALIGHT_LIBRARIES
        Ultralight
        UltralightCore
        WebCore
        AppCore)
# Find the Ultralight libraries
foreach(LIB ${ULTRALIGHT_LIBRARIES})
    find_library(FOUND_${LIB} ${LIB})
    if (NOT FOUND_${LIB})
        message(FATAL_ERROR "${LIB} was not found")
    else()
        list(APPEND ULTRALIGHT_LIBS ${FOUND_${LIB}})
    endif()
endforeach()

# Link JUCE libraries to your project
target_link_libraries(${PROJECT_NAME}
        PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_core
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_opengl

        ${ULTRALIGHT_LIBS}

        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
        )