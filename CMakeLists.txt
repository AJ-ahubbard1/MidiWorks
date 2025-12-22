cmake_minimum_required(VERSION 3.20)
project(MidiWorks VERSION 1.0 LANGUAGES CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find wxWidgets package (required)
find_package(wxWidgets REQUIRED COMPONENTS core base aui adv)
include(${wxWidgets_USE_FILE})

# Find ALSA library for MIDI support on Linux
if(UNIX AND NOT APPLE)
    find_package(ALSA REQUIRED)
endif()

# Source files
set(SOURCES
    src/App.cpp
    src/AppModel/AppModel.cpp
    src/AppModel/Clipboard/Clipboard.cpp
    src/AppModel/NoteEditor/NoteEditor.cpp
    src/AppModel/RecordingSession/RecordingSession.cpp
    src/AppModel/SoundBank/SoundBank.cpp
    src/AppModel/TrackSet/TrackSet.cpp
    src/Commands/NoteEditCommands.cpp
    src/MainFrame/MainFrame.cpp
    src/MainFrame/MainFrameEventHandlers.cpp
    src/Panels/MidiCanvas/MidiCanvas.cpp
    src/Panels/MidiCanvas/MidiCanvasEventHandlers.cpp
    src/RtMidiWrapper/RtMidi/RtMidi.cpp
)

# Header files (for IDE integration)
set(HEADERS
    src/AppModel/AppModel.h
    src/AppModel/Clipboard/Clipboard.h
    src/AppModel/NoteEditor/NoteEditor.h
    src/AppModel/RecordingSession/RecordingSession.h
    src/Commands/Command.h
    src/Commands/NoteEditCommands.h
    src/Commands/PasteCommand.h
    src/Commands/QuantizeCommand.h
    src/Commands/RecordCommand.h
    src/External/json.hpp
    src/MainFrame/PaneInfo.h
    src/AppModel/SoundBank/SoundBank.h
    src/AppModel/TrackSet/TrackSet.h
    src/AppModel/Transport.h
    src/MainFrame/MainFrame.h
    src/Panels/ChannelControls.h
    src/Panels/ChannelTrackRow.h
    src/Panels/Dummy.h
    src/Panels/Inspector.h
    src/Panels/Instrument.h
    src/Panels/Log.h
    src/Panels/MainSynth.h
    src/Panels/MidiCanvas/MidiCanvas.h
    src/Panels/MidiSettings.h
    src/Panels/Panels.h
    src/Panels/SoundBankPanel.h
    src/Panels/TransportPanel.h
    src/Panels/UndoHistoryPanel.h
    src/Panels/ShortcutsPanel.h
    src/RtMidiWrapper/MidiDevice/MidiError.h
    src/RtMidiWrapper/MidiDevice/MidiIn.h
    src/RtMidiWrapper/MidiDevice/MidiInCallback.h
    src/RtMidiWrapper/MidiDevice/MidiOut.h
    src/RtMidiWrapper/MidiMessage/MidiMessage.h
    src/RtMidiWrapper/MidiMessage/SoundMaps.h
    src/RtMidiWrapper/RtMidiWrapper.h
    src/RtMidiWrapper/RtMidi/RtMidi.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${wxWidgets_INCLUDE_DIRS}
)

# Platform-specific settings
if(UNIX AND NOT APPLE)
    # Linux-specific: Define ALSA API for RtMidi
    target_compile_definitions(${PROJECT_NAME} PRIVATE __LINUX_ALSA__)
    target_include_directories(${PROJECT_NAME} PRIVATE ${ALSA_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${ALSA_LIBRARIES} pthread)
elseif(WIN32)
    # Windows-specific: Define Windows MM API for RtMidi
    target_compile_definitions(${PROJECT_NAME} PRIVATE __WINDOWS_MM__)
    target_link_libraries(${PROJECT_NAME} PRIVATE winmm)
elseif(APPLE)
    # macOS-specific: Define CoreMIDI API for RtMidi
    target_compile_definitions(${PROJECT_NAME} PRIVATE __MACOSX_CORE__)
    find_library(COREMIDI_LIBRARY CoreMIDI)
    find_library(COREAUDIO_LIBRARY CoreAudio)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${COREMIDI_LIBRARY}
        ${COREAUDIO_LIBRARY}
        ${COREFOUNDATION_LIBRARY}
    )
endif()

# Link wxWidgets
target_link_libraries(${PROJECT_NAME} PRIVATE ${wxWidgets_LIBRARIES})

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Install target
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)
