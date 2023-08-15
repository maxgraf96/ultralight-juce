cmake_minimum_required(VERSION 3.12)
project(UltralightJUCE VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 14)

# Add JUCE as a subdirectory
add_subdirectory(JUCE)
# Allow modal loops
add_definitions(-DJUCE_MODAL_LOOPS_PERMITTED=1)

# Add the Ultralight SDK as a subdirectory (by default, put it in the Libs/ultralight-sdk folder)
add_subdirectory(Libs/ultralight-sdk)

# Get a lock-free queue for the hot-reloading system
include(FetchContent)
FetchContent_Declare(
        readerwriterqueue
        GIT_REPOSITORY    https://github.com/cameron314/readerwriterqueue
        GIT_TAG           master
)
FetchContent_MakeAvailable(readerwriterqueue)

# Edit as needed
juce_add_plugin(${PROJECT_NAME}
        # ICON_BIG ${CMAKE_CURRENT_SOURCE_DIR}/icon.png
        # ICON_SMALL ${CMAKE_CURRENT_SOURCE_DIR}/icon.png
        COMPANY_NAME "UltralightJUCECompany" # Specify the name of the plugin's author
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE       # Should the plugin be installed to a default location after building?
        PLUGIN_MANUFACTURER_CODE Demo       # A four-character manufacturer id with at least one upper-case character
        PLUGIN_CODE Ulju                    # A unique four-character plugin id with exactly one upper-case character
        DESCRIPTION "My plugin description"
        VST3_CATEGORIES "Audio Effect Stereo"
        AU_MAIN_TYPE "kAudioUnitType_MusicDevice"
        FORMATS VST3 Standalone
        BUNDLE_ID "${TMPL_BUNDLE_ID}"
        HARDENED_RUNTIME_ENABLED TRUE
        PRODUCT_NAME "UltralightJUCE")     # The name of the final executable, which can differ from the target name

# Generate the JUCE header so we can use it in our source files
juce_generate_juce_header(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
        PUBLIC
        # Add the Ultralight SDK headers to the include path
        Libs/ultralight-sdk/include
        Source
        )

target_sources(${PROJECT_NAME} PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        
        Source/JSInteropBase.h
        Source/JSInteropExample.h
        Source/InspectorModalWindow.h
        Source/FileWatcher.hpp
        
        )

target_compile_definitions(${PROJECT_NAME}
        PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        DONT_SET_USING_JUCE_NAMESPACE=1
        )


# Set the paths where CMake should search for libraries
set(CMAKE_LIBRARY_PATH "${CMAKE_LIBRARY_PATH};${CMAKE_CURRENT_SOURCE_DIR}/Libs/ultralight-sdk/bin;${CMAKE_CURRENT_SOURCE_DIR}/Libs/ultralight-sdk/lib")

# Specify the names of the Ultralight libraries
set(ULTRALIGHT_LIBRARIES
        Ultralight
        UltralightCore
        WebCore
        AppCore
        )

if(WIN32)
    message("Operating system: Windows")

    # Background on how to link Ultralight on Windows:
    # Since the free Ultralight license tier only allows dynamic linking, we need to make the Ultralight DLLs available
    # to the application. On Windows, there is a special search order for DLLs, which is described here:
    # https://docs.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-search-order
    # The easiest way to make the Ultralight DLLs available to the application is to copy them to the same folder
    # as the executable (option 1)). This works for standalone builds. For audio plugins, however, the DLLs need to be copied
    # to any folder that is in the PATH environment variable. This is option 2) below.

    # Option 1)
    # Copy the Ultralight DLLs to the build output folder. Works for standalone builds, but not for audio plugins.
#        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#                COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/Libs/ultralight-sdk/bin" $<TARGET_FILE_DIR:${PROJECT_NAME}>/Standalone)

    # Option 2)
    # If your build target are audio plugins on Windows:
    # One option for dynamically linking the Ultralight libraries is to copy the Ultralight DLLs to
    # a subfolder of %AppData%\\<your plugin name> and add that one to the PATH environment variable
    # (%AppData% is the path to the AppData folder of the current user, usually C:\\Users\\<username>\\AppData\\Roaming)

    set(DESTINATION_FOLDER "$ENV{APPDATA}\\${PROJECT_NAME}")
    message("Destination folder for copying Ultralight DLLs: ${DESTINATION_FOLDER}")

    foreach(LIB ${ULTRALIGHT_LIBRARIES})
        # Specify the source DLL path
        set(SOURCE_DLL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Libs/ultralight-sdk/bin/${LIB}.dll")
        # Specify the destination folder in the AppData directory
        message("Source DLL path: ${SOURCE_DLL_PATH}")
        # Copy the DLL file to the destination folder
        file(COPY ${SOURCE_DLL_PATH} DESTINATION ${DESTINATION_FOLDER})
    endforeach()

    # Add the destination folder to the PATH environment variable
    execute_process(COMMAND powershell.exe -File "${CMAKE_CURRENT_SOURCE_DIR}/Libs/AddDLLFolderToPathWindows.ps1" "${DESTINATION_FOLDER}")

endif()

# Find our Ultralight libraries and add them to the "ULTRALIGHT_LIBS" list, which we will link below
foreach(LIB ${ULTRALIGHT_LIBRARIES})
    find_library(FOUND_${LIB} ${LIB})
    if (NOT FOUND_${LIB})
        message(FATAL_ERROR "${LIB} was not found")
    else()
        list(APPEND ULTRALIGHT_LIBS ${FOUND_${LIB}})
    endif()
endforeach()

# Link libraries
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

        # Link the Ultralight libraries to the project
        ${ULTRALIGHT_LIBS}

        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
        # Link the lock-free queue to the project
        readerwriterqueue
        )

# Use this command if you want to copy the built VST3 file to a specific directory after each build
# This makes it quicker to test your plugin in your DAW
# JUCE has an option to do this automatically, but it fails in some scenarios (see COPY_PLUGIN_AFTER_BUILD above)

# Set the source file and destination directory
#set(source_file "/Users/max/CLionProjects/ultralight-juce/cmake-build-debug/UltralightJUCE_artefacts/Debug/VST3/UltralightJUCE.vst3")
#set(destination_dir "/Library/Audio/Plug-Ins/VST3")
#add_custom_command(
#        TARGET UltralightJUCE  # Specify your target name
#        POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E copy "${source_file}" "${destination_dir}/UltralightJUCE.vst3"  # Copy the file, change the filename "UltralightJUCE.vst3" as needed
#        DEPENDS "${source_file}"   # Specify the dependency on the source file
#)