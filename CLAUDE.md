# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

MidiWorks is a wxWidgets-based Digital Audio Workstation (DAW) written in C++20. It provides comprehensive MIDI composition tools including piano roll editing, multi-track recording/playback, loop recording with overdub, quantization, and a complete undo/redo system.

**Version:** 1.0 (MVP Complete - December 2025)

**Key Features:**
- Full piano roll editing with multi-selection, copy/paste, quantize
- 15-track MIDI recording with loop recording and overdub note merging
- Professional channel mixer with mute/solo/record controls
- Complete undo/redo system with command history
- Project save/load (.mwp JSON format)
- Metronome with downbeat detection
- Comprehensive keyboard shortcuts
- Dockable panel-based UI

## Build System

**Visual Studio Solution:** `MidiWorks.sln` / `MidiWorks.vcxproj`

### Prerequisites
- Visual Studio 2022 (Platform Toolset v143)
- wxWidgets library with `WXWIN` environment variable set
- Windows 10 SDK

### Build Commands
```powershell
# Build from Visual Studio IDE
# Open MidiWorks.sln and press F5 (Debug) or Ctrl+F5 (Release)

# Build from command line using MSBuild
msbuild MidiWorks.sln /p:Configuration=Debug /p:Platform=x64
msbuild MidiWorks.sln /p:Configuration=Release /p:Platform=x64
```

### Build Configurations
- **Debug|Win32** - 32-bit debug build
- **Release|Win32** - 32-bit release build
- **Debug|x64** - 64-bit debug build
- **Release|x64** - 64-bit release build

### Dependencies
- **wxWidgets** - GUI framework (must set `WXWIN` environment variable)
  - Win32: `$(WXWIN)\lib\vc_lib`
  - x64: `$(WXWIN)\lib\vc_x64_lib`
- **RtMidi** - Included in `src/RtMidiWrapper/RtMidi/`
- **winmm.lib** - Windows multimedia library (for MIDI)

## Architecture

### Core Design Principle
**Model-View separation:** AppModel contains all application data and business logic. Panels contain UI layout and event bindings. Event handlers should call AppModel functions to manipulate data, not manipulate data directly.

### Layer Structure

```
┌─────────────────────────────────────────┐
│  App (wxApp)                            │  Entry point
│  └─ MainFrame (wxFrame + wxAuiManager)  │  Window & layout orchestration
└─────────────────────────────────────────┘
            │
            ├─→ AppModel (Model)
            │   ├─ Transport - Playback state machine
            │   ├─ SoundBank - 16 MIDI channels
            │   ├─ TrackSet - Track storage & playback
            │   └─ MidiIn - MIDI input device
            │
            └─→ Dockable Panels (Views)
                ├─ TransportPanel - Play/record controls
                ├─ SoundBankPanel - Channel mixer (16 channels)
                ├─ MidiSettingsPanel - MIDI input selection
                ├─ MidiCanvasPanel - Piano roll visualization
                └─ LogPanel - MIDI event logging
```

### Key Components

#### AppModel (src/AppModel/)
Central application state manager. Updated every 10ms by timer.
- **AppModel.h** - Main orchestrator
  - Manages lifecycle: play, record, stop
  - Routes MIDI input to channels
  - Handles recording buffer and playback
- **Transport.h** - State machine for playback/recording
  - States: Stopped, Playing, Recording, Rewinding, FastForwarding, etc.
  - Tempo: 120 BPM default, 960 ticks per quarter note
- **SoundBank.h** - 16 MIDI channel management
  - Per-channel: program (patch), volume, mute, solo, record state
  - Solo logic: when active, only solo channels play
- **TrackSet.h** - Track storage (16 tracks, one per channel)
  - Efficient playback scheduling using iterators
  - Tracks are vectors of timed MIDI events

#### MainFrame (src/MainFrame/)
Application window and panel orchestration.
- **MainFrame.h/cpp** - wxFrame subclass
  - Uses wxAuiManager for dockable panels
  - `CreateDockablePanes()` - instantiates all panels
  - `mPanels` map: menuId → PanelInfo (panel state)
  - Timer-based update loop (10ms interval)
- **PaneInfo.h** - Panel layout metadata
  - Struct containing: name, wxWindow*, menuId, position, size, visibility

#### Panels (src/Panels/)
Dockable UI components. Mostly header-only with minimal state.
- **TransportPanel.h** - Playback controls and time display
- **SoundBankPanel.h** - Main mixer with 16 ChannelControlsPanel instances
- **ChannelControls.h** - Per-channel UI: patch selector, volume, mute, solo, record
- **MidiSettingsPanel.h** - MIDI input port selection
- **MidiCanvasPanel.h** - Piano roll visualization with zoom/pan
- **Log.h** - Real-time MIDI event logging

#### Commands (src/Commands/)
Command pattern implementation for undo/redo system.
- **Command.h** - Abstract base class with Execute/Undo/GetDescription
- **NoteEditCommands.h** - Note editing commands:
  - AddNoteCommand - Adds note-on and note-off events to track
  - DeleteNoteCommand - Removes note pair, stores for undo
  - MoveNoteCommand - Changes tick/pitch, maintains duration
  - ResizeNoteCommand - Changes duration by moving note-off
  - DeleteMultipleNotesCommand - Batch deletion with sorted index management
- **QuantizeCommand.h** - Quantizes all notes in track to grid
- **PasteCommand.h** - Pastes clipboard notes with relative timing

All commands executed through `AppModel::ExecuteCommand()` which manages undo/redo stacks.

#### RtMidiWrapper (src/RtMidiWrapper/)
Thin abstraction layer over RtMidi library.
- **MidiIn.h** - MIDI input interface (wraps RtMidiIn)
- **MidiOut.h** - MIDI output interface (wraps RtMidiOut)
- **MidiMessage.h** - MIDI message abstraction with factory methods
- **SoundMaps.h** - General MIDI instrument name lookup (128 patches)

### Update Loop (10ms Timer)
```
MainFrame::OnTimer()
  ├─→ AppModel::Update()
  │   ├─→ Handle Transport state machine
  │   ├─→ CheckMidiInQueue() - poll and route MIDI input
  │   └─→ TrackSet::PlayBack() - schedule track playback at current tick
  ├─→ TransportPanel::UpdateDisplay() - refresh tick/time
  ├─→ MidiCanvasPanel::Update() - redraw notes
  └─→ LogPanel::LogMidiEvent() - display incoming events
```

### Memory Management
- wxWidgets classes use parent-child ownership (automatic cleanup)
- `std::shared_ptr` for AppModel
- Raw pointers for UI components (wxWidgets manages lifetime)

## File Organization

```
src/
├── App.cpp                          # Entry point (wxApp)
├── MidiConstants.h                  # MIDI specification constants
├── MainFrame/
│   ├── MainFrame.{h,cpp}           # Main window & AUI manager
│   └── PaneInfo.h                  # Panel metadata
├── AppModel/                        # Business logic
│   ├── AppModel.{h,cpp}            # Central orchestrator
│   ├── Transport.h                 # Playback state machine
│   ├── SoundBank.h                 # 16 MIDI channels
│   └── TrackSet/
│       ├── TrackSet.{h,cpp}        # Track storage & search
│       └── Track.h                 # Track type definition
├── Commands/                        # Undo/Redo system
│   ├── Command.h                   # Base command class
│   ├── NoteEditCommands.h          # Add/Delete/Move/Resize
│   ├── DeleteMultipleNotesCommand.h # Batch deletion
│   ├── QuantizeCommand.h           # Grid quantization
│   └── PasteCommand.h              # Clipboard paste
├── Panels/                          # UI components
│   ├── Panels.h                    # Aggregation header
│   ├── TransportPanel.h
│   ├── SoundBankPanel.h
│   ├── ChannelControls.h
│   ├── MidiSettingsPanel.h
│   ├── MidiCanvas/
│   │   ├── MidiCanvas.{h,cpp}      # Piano roll editor
│   │   └── MidiCanvasConstants.h   # Canvas-specific constants
│   ├── UndoHistoryPanel.h
│   ├── ShortcutsPanel.h
│   └── Log.h
├── RtMidiWrapper/                   # MIDI abstraction
│   ├── RtMidiWrapper.h             # Main include
│   ├── RtMidi/
│   │   ├── RtMidi.h
│   │   └── RtMidi.cpp              # Third-party library
│   ├── MidiDevice/
│   │   ├── MidiIn.h
│   │   ├── MidiOut.h
│   │   ├── MidiError.h
│   │   └── MidiInCallback.h
│   └── MidiMessage/
│       ├── MidiMessage.h
│       └── SoundMaps.h
└── external/                        # Third-party headers
    └── json.hpp                     # nlohmann/json library
```

## Important Patterns

### Adding New Panels
1. Create panel header in `src/Panels/`
2. Inherit from `wxPanel` or `wxScrolledWindow`
3. Add to `src/Panels/Panels.h`
4. Register in `MainFrame::CreateDockablePanes()`:
   - Create `PanelInfo` struct
   - Add to `mPanels` map with unique menu ID
   - Call `CreatePaneInfo()` and add to AUI manager
5. Add to View menu in `MainFrame::CreateMenuBar()`

### MIDI Message Flow
```
MIDI Device → MidiIn → AppModel::CheckMidiInQueue()
  ├─→ [Recording] Add to recording buffer
  ├─→ [Playing] Route to appropriate channel → MidiOut
  └─→ [Stopped] Play immediately → MidiOut
```

### Transport State Management
Panels directly mutate `Transport.mState` enum. Transport state machine handled in `AppModel::Update()`.

States: `Stopped`, `ClickedPlay`, `Playing`, `Recording`, `ClickedRecord`, `StopPlaying`, `StopRecording`, `Rewinding`, `FastForwarding`

## MIDI Constants
- 16 MIDI channels (standard MIDI specification)
- 128 program numbers (General MIDI patches)
- 960 ticks per quarter note (PPQN)
- Default tempo: 120 BPM
- MIDI data values: 0-127

## Code Style Notes
- C++20 standard
- Most panels are header-only with inline implementations
- Include paths relative to `src/` directory
- Use `#include "AppModel/AppModel.h"` format
