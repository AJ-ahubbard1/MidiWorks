# MidiWorks

A wxWidgets-based Digital Audio Workstation (DAW) for MIDI composition and editing.

**Current Version:** 1.1 (In Progress) | [Roadmap to v2.0](GettingToVersion2.md)

## Overview

MidiWorks is a focused MIDI composition tool that makes creating music fast and intuitive. With a complete piano roll editor, multi-track recording, loop recording with overdub, and professional editing tools, MidiWorks provides everything you need to compose MIDI music.

### Key Features

- **Full Piano Roll Editing** - Add, move, resize, and delete notes with visual feedback
- **Multi-Selection** - Rectangle selection and Ctrl+A select all
- **Copy/Paste/Cut** - Complete clipboard operations for notes
- **Quantize** - Snap notes to grid with triplet support (Q key)
- **Undo/Redo System** - Complete command history with 50-level undo stack
- **Loop Recording** - Record loops with automatic overdub note merging
- **Metronome** - Downbeat-aware click track (channel 16)
- **15-Track Recording** - Multi-track MIDI with visual track colors
- **Channel Mixer** - Per-channel volume, mute, solo, record, and patch selection
- **Project Files** - Save/load projects in JSON format (.mwp)
- **Keyboard Shortcuts** - Comprehensive shortcuts for fast workflow
- **MIDI Import/Export** - Share compositions with other DAWs via standard .mid files
- **Track Organization** - Custom track naming, colors, and minimize/collapse
- **Time Signature Control** - Compose in 3/4, 6/8, 7/8, and other meters
- **Velocity Editing** - Edit note dynamics with visual velocity controls
- **Solo Visual Filtering** - Isolate tracks visually for precise editing

## System Requirements

- **OS:** Windows 10 or later (64-bit recommended)
- **Compiler:** Visual Studio 2022 with C++20 support
- **Dependencies:**
  - wxWidgets 3.x (set `WXWIN` environment variable)
  - Windows 10 SDK
  - MIDI device (input and/or output)

## Building from Source

### Prerequisites

1. Install [Visual Studio 2022](https://visualstudio.microsoft.com/) with C++ desktop development
2. Install or build [wxWidgets](https://www.wxwidgets.org/)
3. Set `WXWIN` environment variable to your wxWidgets directory

### Build Steps

```powershell
# Open solution in Visual Studio
# Press F5 for Debug build, or Ctrl+F5 for Release build

# Or build from command line:
msbuild MidiWorks.sln /p:Configuration=Release /p:Platform=x64
```

### Build Configurations

- **Debug|Win32** - 32-bit debug build
- **Release|Win32** - 32-bit release build
- **Debug|x64** - 64-bit debug build (recommended)
- **Release|x64** - 64-bit release build (recommended)

## Quick Start Guide

### Basic Workflow

1. **Setup MIDI Devices**
   - Go to View → MIDI Settings
   - Select your MIDI input device (keyboard/controller)
   - Select your MIDI output device (synthesizer/soundfont player)

2. **Configure a Channel**
   - In the Channel Mixer panel, click the patch dropdown
   - Select an instrument (e.g., "Acoustic Grand Piano")
   - Enable "Record" checkbox for the channel

3. **Record MIDI**
   - Press `R` or click Record button
   - Play notes on your MIDI keyboard
   - Press `Spacebar` to stop recording

4. **Edit in Piano Roll**
   - **Add notes**: Left-click on empty space
   - **Delete notes**: Middle-click or select and press Delete
   - **Move notes**: Left-click drag note
   - **Resize notes**: Drag right edge of note
   - **Select multiple**: Shift+Drag rectangle or Ctrl+A
   - **Copy/Paste**: Ctrl+C, Ctrl+V
   - **Quantize**: Select notes and press Q

5. **Loop Recording**
   - Enable Loop checkbox in Transport panel
   - Drag loop edges in the piano roll to set region
   - Press R to record - loop will repeat and merge notes

6. **Save Your Work**
   - Press Ctrl+S or File → Save
   - Projects save as .mwp files (JSON format)

### Keyboard Shortcuts

| Action | Shortcut |
|--------|----------|
| Play/Stop | Spacebar |
| Record | R |
| New Project | Ctrl+N |
| Open Project | Ctrl+O |
| Save Project | Ctrl+S |
| Save As | Ctrl+Shift+S |
| Undo | Ctrl+Z |
| Redo | Ctrl+Y |
| Copy Notes | Ctrl+C |
| Paste Notes | Ctrl+V |
| Cut Notes | Ctrl+X |
| Select All | Ctrl+A |
| Delete Notes | Delete or Middle-Click |
| Quantize | Q |
| Zoom In/Out | Mouse Wheel |
| Pan Horizontal | Shift+Mouse Wheel |
| Pan Any Direction | Right-Click Drag |

### Piano Roll Mouse Controls

- **Left-Click** on empty space: Add note
- **Left-Click Drag** on note: Move note
- **Left-Click Drag** on note edge: Resize note
- **Middle-Click** on note: Delete note
- **Shift+Left-Drag**: Rectangle selection
- **Right-Click Drag**: Pan canvas
- **Mouse Wheel**: Zoom horizontal
- **Shift+Mouse Wheel**: Pan horizontal

## Features

### Piano Roll Editor

The piano roll provides comprehensive note editing:

- **Visual Grid** - Configurable grid lines for beats and measures
- **Grid Snap** - Snap to whole, half, quarter, eighth, sixteenth notes, or triplets
- **Note Preview** - Hear notes before placing them
- **Auto-Scroll** - Canvas follows playhead during playback
- **Track Colors** - 15 distinct colors for visual track separation
- **Zoom & Pan** - Mouse wheel zoom, right-click pan

### Recording Features

- **15 Usable Tracks** - Channel 16 reserved for metronome
- **Loop Recording** - Set loop region and record repeating patterns
- **Overdub Merge** - Automatically merges overlapping notes during loop recording
- **Record-Armed Channels** - Only record-enabled channels receive MIDI input
- **Metronome** - Woodblock click on beats, louder on downbeat

### Mixing & Channels

- **16 MIDI Channels** - Full General MIDI support
- **128 Instruments** - General MIDI patch selection per channel
- **Volume Control** - 0-127 MIDI volume per channel
- **Mute/Solo** - Mute individual channels or solo to hear only selected channels
- **Record Enable** - Enable recording per channel
- **Visual Feedback** - Channel controls update in real-time

### Tempo & Timing

- **Variable Tempo** - 40-300 BPM range
- **High Resolution** - 960 PPQN (ticks per quarter note)
- **Time Display** - Both tick count and MM:SS:mmm format
- **Loop Region** - Visual loop markers with draggable edges

### Project Management

- **JSON Format** - Human-readable .mwp project files
- **Complete State** - Saves all tracks, channels, transport settings
- **Dirty Flag** - Asterisk in title bar shows unsaved changes
- **Unsaved Warning** - Prompts to save on New/Open/Exit

### Undo/Redo System

- **50-Level History** - Undo up to 50 operations
- **All Operations** - Add, delete, move, resize, quantize, paste
- **History Panel** - View command descriptions in real-time
- **Keyboard Shortcuts** - Ctrl+Z undo, Ctrl+Y redo

## Project File Format

MidiWorks uses a JSON-based .mwp (MidiWorks Project) format:

```json
{
  "version": "1.0",
  "appVersion": "1.0",
  "transport": {
    "tempo": 120.0,
    "timeSignature": [4, 4],
    "currentTick": 0
  },
  "channels": [ /* 16 channel settings */ ],
  "tracks": [ /* 15 track event data */ ]
}
```

Files are human-readable and can be edited in a text editor if needed.

## Architecture

MidiWorks follows a clean Model-View separation:

- **AppModel** - Central application state and business logic
- **Panels** - UI components (Transport, Mixer, Piano Roll, etc.)
- **Commands** - Command pattern for undo/redo
- **RtMidiWrapper** - MIDI device abstraction

See [CLAUDE.md](CLAUDE.md) for detailed architecture documentation.

## Development

See [DEVELOPMENT.md](DEVELOPMENT.md) for:
- Planned features roadmap
- Development principles
- Phase 1-4 feature breakdowns
- Design decision log

Archived development checklists from v1.0 development are in [docs/archive/v1-development/](docs/archive/v1-development/).

## Contributing

MidiWorks is a solo project, but ideas and feedback are welcome! Please:

1. Check existing issues before creating new ones
2. Provide clear reproduction steps for bugs
3. Suggest features with use case descriptions

## License

This project is currently private. License TBD.

## Credits

- **RtMidi** - Cross-platform MIDI I/O library
- **wxWidgets** - Cross-platform GUI framework
- **nlohmann/json** - Modern C++ JSON library

## Version History

### Version 1.1 (In Progress - January 2026)
**Production Ready** - Track organization, MIDI file exchange, and editing enhancements.

New features:
- MIDI file export (share with other DAWs)
- MIDI file import (bring in external compositions)
- Track naming (custom names for each channel)
- Track colors (visual organization with color picker)
- Track minimize (collapse tracks to focus on active ones)
- Clear track (delete all notes with undo support)
- Global time signature control (3/4, 6/8, 7/8, etc.)
- Velocity editing (single-note dynamics control)
- Solo visual filtering (isolate tracks for precise editing)

Remaining:
- Error handling and stability improvements

### Version 1.0 (December 2025)
**MVP Complete** - All core composition features implemented.

- Complete piano roll editing system
- Multi-selection and clipboard operations
- Quantize with triplet support
- Full undo/redo system
- Loop recording with overdub merging
- Project save/load
- Metronome with downbeat
- Comprehensive keyboard shortcuts
- 15-track recording and playback
- Professional channel mixer
- Tempo control (40-300 BPM)

---

**Made for musicians who want to compose MIDI quickly and intuitively.**
