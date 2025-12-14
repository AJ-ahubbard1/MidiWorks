# MidiWorks Architecture and Function Reference

**Version:** 1.0
**Last Updated:** 2025-12-08

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Design Patterns](#design-patterns)
3. [Data Flow](#data-flow)
4. [Function Reference by Module](#function-reference-by-module)
   - [Application Entry Point](#1-application-entry-point)
   - [AppModel (Business Logic)](#2-appmodel-business-logic)
   - [MainFrame (Window Management)](#3-mainframe-window-management)
   - [Panels (User Interface)](#4-panels-user-interface)
   - [Commands (Undo/Redo System)](#5-commands-undoredo-system)
   - [RtMidiWrapper (MIDI Abstraction)](#6-rtmidiwrapper-midi-abstraction)
5. [Statistics](#statistics)

---

## Architecture Overview

MidiWorks follows a strict **Model-View** separation pattern where:

- **AppModel** contains all application data and business logic
- **Panels** contain UI layout and event bindings
- **Event handlers** call AppModel functions to manipulate data, never manipulating data directly

### Layer Hierarchy

```
┌─────────────────────────────────────────┐
│  App (wxApp)                            │  ← Entry point
│  └─ MainFrame (wxFrame + wxAuiManager)  │  ← Window & layout orchestration
└─────────────────────────────────────────┘
            │
            ├─→ AppModel (Model)           ← Business logic layer
            │   ├─ Transport              ← Playback state machine
            │   ├─ SoundBank              ← 16 MIDI channels
            │   ├─ TrackSet               ← Track storage & playback
            │   └─ MidiIn                 ← MIDI input device
            │
            └─→ Dockable Panels (Views)   ← UI layer
                ├─ TransportPanel          ← Play/record controls
                ├─ SoundBankPanel          ← Channel mixer (16 channels)
                ├─ MidiSettingsPanel       ← MIDI input selection
                ├─ MidiCanvasPanel         ← Piano roll visualization
                ├─ LogPanel                ← MIDI event logging
                ├─ UndoHistoryPanel        ← Undo/redo stack display
                └─ ShortcutsPanel          ← Keyboard reference
```

### Update Loop (10ms Timer)

The application runs on a 10ms timer that drives all updates:

```
MainFrame::OnTimer() (10ms interval)
  │
  ├─→ AppModel::Update()
  │   ├─→ Handle Transport state machine
  │   ├─→ CheckMidiInQueue() - Poll and route MIDI input
  │   └─→ TrackSet::PlayBack() - Schedule track playback at current tick
  │
  ├─→ TransportPanel::UpdateDisplay() - Refresh tick/time
  ├─→ MidiCanvasPanel::Update() - Redraw notes and auto-scroll
  └─→ LogPanel::LogMidiEvent() - Display incoming events
```

---

## Design Patterns

### 1. Model-View Separation
- **AppModel** holds all state (tracks, channels, transport, MIDI devices)
- **Panels** display state and send commands to AppModel
- **No direct state manipulation** in UI code

### 2. Command Pattern (Undo/Redo)
- All editing operations inherit from abstract `Command` class
- Commands implement `Execute()`, `Undo()`, and `GetDescription()`
- AppModel maintains undo/redo stacks (50 actions max)
- Commands include: AddNote, DeleteNote, MoveNote, ResizeNote, Record, Quantize, Paste, DeleteMultiple

### 3. State Machine (Transport)
- Transport uses enum-based state machine
- States: `Stopped`, `Playing`, `Recording`, `Rewinding`, `FastForwarding`, etc.
- Transition states: `ClickedPlay`, `ClickedRecord`, `StopPlaying`, `StopRecording`
- State transitions handled in `AppModel::Update()`

### 4. Dockable Panel System
- wxAuiManager handles panel docking and layout
- `PanelInfo` struct describes panel metadata
- Panels can be floated, docked, hidden, and resized
- View menu dynamically generated from panel registry

### 5. Callback Pattern
- MIDI logging callback: `AppModel::SetLogCallback()`
- Dirty state callback: `AppModel::SetDirtyStateCallback()`
- Allows decoupling of model from specific UI components

### 6. Efficient Playback with Iterators
- TrackSet maintains per-track playback iterators
- `FindStart()` initializes iterators at playback position
- `PlayBack()` scans forward from iterator position only
- Avoids O(n) search on every timer tick

---

## Data Flow

### MIDI Input Flow
```
MIDI Device → MidiIn::checkForMessage()
           → AppModel::CheckMidiInQueue()
           ├─→ [Stopped] Play immediately → MidiOut
           ├─→ [Playing] Route to channels → MidiOut
           └─→ [Recording] Add to recording buffer + route to MidiOut
```

### Recording Flow
```
User clicks Record → Transport.mState = ClickedRecord
                  → AppModel::Update() detects state
                  → Clear recording buffer
                  → Set state = Recording
                  → CheckMidiInQueue() adds events to buffer
                  → User clicks Stop
                  → Create RecordCommand
                  → Execute command (merges buffer into tracks)
                  → Add to undo stack
```

### Playback Flow
```
User clicks Play → Transport.mState = ClickedPlay
                → AppModel::Update() detects state
                → TrackSet::FindStart(currentTick)
                → Set state = Playing
                → AppModel::Update() calls TrackSet::PlayBack()
                → Returns events at current tick
                → PlayMessages() sends to MidiOut (respecting mute/solo)
```

### Editing Flow (Piano Roll)
```
User adds note → MidiCanvasPanel::OnLeftDown()
               → Create note-on/note-off events
               → Create AddNoteCommand
               → AppModel::ExecuteCommand(cmd)
               → Command::Execute() adds to track
               → AppModel::MarkDirty()
               → Track sorted for playback
```

---

## Function Reference by Module

## 1. Application Entry Point

### **src/App.cpp**

Entry point for the wxWidgets application.

#### **class App : public wxApp**

**Purpose:** Initialize and launch the application.

##### Member Functions:

###### `bool OnInit()`
- **Description:** Called on application startup
- **Actions:**
  - Creates MainFrame window
  - Calls `SetScreenSizeAndPosition()`
  - Shows the window
- **Returns:** `true` on success

###### `void SetScreenSizeAndPosition()`
- **Description:** Sizes and centers the main window
- **Actions:**
  - Sets window size to 2/3 of screen dimensions
  - Centers window on screen
- **Parameters:** None
- **Returns:** void

##### Member Variables:

- `MainFrame* mMainFrame` - Pointer to main application window

---

## 2. AppModel (Business Logic)

### **src/AppModel/AppModel.h / AppModel.cpp**

Central orchestrator for all application state and business logic.

#### **class AppModel**

**Purpose:** Manages Transport, SoundBank, TrackSet, MIDI I/O, undo/redo, and project state.

##### Public Member Functions:

###### `AppModel()`
- **Description:** Constructor - initializes MIDI input and metronome
- **Actions:**
  - Opens first available MIDI input port
  - Calls `InitializeMetronome()`
  - Initializes member variables

###### `void InitializeMetronome()`
- **Description:** Configures channel 16 for metronome playback
- **Actions:**
  - Sets channel 15 (zero-indexed) to Woodblock (patch 115)
  - Sets volume to 100

###### `void Update()`
- **Description:** Main update loop called every 10ms
- **Actions:**
  - Calculates delta time since last update
  - Handles Transport state machine transitions
  - Calls `CheckMidiInQueue()`
  - Updates Transport playback time
  - Handles metronome beats
  - Processes playback and loop recording
  - Manages state transitions (play, record, stop, rewind, FF)
- **Called by:** `MainFrame::OnTimer()`

###### `void CheckMidiInQueue()`
- **Description:** Polls MIDI input and routes messages to channels
- **Actions:**
  - Checks for incoming MIDI messages
  - Filters non-musical messages
  - Routes to appropriate channel based on record-enable state
  - Adds to recording buffer if recording
  - Plays messages immediately if stopped or playing
  - Calls logging callback

###### `SoundBank& GetSoundBank()`
- **Description:** Access to MIDI channel manager
- **Returns:** Reference to SoundBank

###### `Transport& GetTransport()`
- **Description:** Access to playback state machine
- **Returns:** Reference to Transport

###### `TrackSet& GetTrackSet()`
- **Description:** Access to track storage
- **Returns:** Reference to TrackSet

###### `Track& GetRecordingBuffer()`
- **Description:** Access to temporary recording buffer
- **Returns:** Reference to recording buffer Track

###### `Track& GetTrack(ubyte c)`
- **Description:** Get specific track by channel number
- **Parameters:**
  - `c` - Channel number (0-14)
- **Returns:** Reference to Track

###### `std::vector<std::string> GetMidiInputPortNames() const`
- **Description:** Get list of available MIDI input port names
- **Returns:** Vector of port name strings

###### `void SetMidiInputPort(int portIndex)`
- **Description:** Change active MIDI input port
- **Parameters:**
  - `portIndex` - Port index to switch to

###### `int GetCurrentMidiInputPort() const`
- **Description:** Get currently active MIDI input port index
- **Returns:** Current port index

###### `void SetLogCallback(MidiLogCallback callback)`
- **Description:** Register callback for MIDI event logging
- **Parameters:**
  - `callback` - Function to call when MIDI events arrive
- **Usage:** `LogPanel` registers to display events

###### `void SetDirtyStateCallback(DirtyStateCallback callback)`
- **Description:** Register callback for project dirty state changes
- **Parameters:**
  - `callback` - Function to call when dirty state changes
- **Usage:** `MainFrame` registers to update title bar

###### `bool IsMetronomeEnabled() const`
- **Description:** Check if metronome is enabled
- **Returns:** True if metronome is on

###### `void SetMetronomeEnabled(bool enabled)`
- **Description:** Enable/disable metronome
- **Parameters:**
  - `enabled` - True to enable, false to disable

###### `bool SaveProject(const std::string& filepath)`
- **Description:** Save current project to JSON file
- **Parameters:**
  - `filepath` - File path to save to
- **Actions:**
  - Serializes Transport settings, SoundBank, TrackSet to JSON
  - Marks project as clean
  - Updates current project path
- **Returns:** True on success, false on failure

###### `bool LoadProject(const std::string& filepath)`
- **Description:** Load project from JSON file
- **Parameters:**
  - `filepath` - File path to load from
- **Actions:**
  - Stops playback
  - Parses JSON and reconstructs Transport, SoundBank, TrackSet
  - Marks project as clean
  - Updates current project path
  - Clears undo history
- **Returns:** True on success, false on failure

###### `void ClearProject()`
- **Description:** Reset to empty project state
- **Actions:**
  - Stops playback
  - Resets Transport to defaults
  - Clears all tracks
  - Resets all channels to default patches
  - Marks project as clean
  - Clears project path
  - Clears undo history

###### `bool IsProjectDirty() const`
- **Description:** Check if project has unsaved changes
- **Returns:** True if modified since last save

###### `void MarkDirty()`
- **Description:** Mark project as modified
- **Actions:**
  - Sets dirty flag
  - Calls dirty state callback

###### `void MarkClean()`
- **Description:** Mark project as saved
- **Actions:**
  - Clears dirty flag
  - Calls dirty state callback

###### `const std::string& GetCurrentProjectPath() const`
- **Description:** Get path of currently loaded/saved project
- **Returns:** File path string (empty if untitled)

###### `void ExecuteCommand(std::unique_ptr<Command> cmd)`
- **Description:** Execute command and add to undo stack
- **Parameters:**
  - `cmd` - Unique pointer to command object
- **Actions:**
  - Calls `cmd->Execute()`
  - Adds to undo stack
  - Clears redo stack
  - Limits undo stack to MAX_UNDO_STACK_SIZE (50)
  - Marks project as dirty

###### `void Undo()`
- **Description:** Undo last command
- **Actions:**
  - Pops command from undo stack
  - Calls `cmd->Undo()`
  - Pushes to redo stack
  - Marks project as dirty

###### `void Redo()`
- **Description:** Redo last undone command
- **Actions:**
  - Pops command from redo stack
  - Calls `cmd->Execute()`
  - Pushes to undo stack
  - Marks project as dirty

###### `bool CanUndo() const`
- **Description:** Check if undo is available
- **Returns:** True if undo stack is not empty

###### `bool CanRedo() const`
- **Description:** Check if redo is available
- **Returns:** True if redo stack is not empty

###### `const std::vector<std::unique_ptr<Command>>& GetUndoStack() const`
- **Description:** Access undo stack for display
- **Returns:** Reference to undo stack vector

###### `const std::vector<std::unique_ptr<Command>>& GetRedoStack() const`
- **Description:** Access redo stack for display
- **Returns:** Reference to redo stack vector

###### `void ClearUndoHistory()`
- **Description:** Clear undo and redo stacks
- **Usage:** Called when loading new project

###### `void CopyToClipboard(const std::vector<ClipboardNote>& notes)`
- **Description:** Store notes in clipboard
- **Parameters:**
  - `notes` - Vector of clipboard note data

###### `const std::vector<ClipboardNote>& GetClipboard() const`
- **Description:** Access clipboard contents
- **Returns:** Reference to clipboard vector

###### `bool HasClipboardData() const`
- **Description:** Check if clipboard has notes
- **Returns:** True if clipboard is not empty

###### `void ClearClipboard()`
- **Description:** Empty the clipboard
- **Actions:** Clears clipboard vector

##### Private Member Functions:

###### `uint64_t GetDeltaTimeMs()`
- **Description:** Calculate time since last update
- **Returns:** Delta time in milliseconds
- **Usage:** Called in `Update()` to advance playback

###### `bool IsMusicalMessage(const MidiMessage& msg)`
- **Description:** Filter out non-musical MIDI messages
- **Parameters:**
  - `msg` - MIDI message to check
- **Returns:** True if message is note-on, note-off, or CC
- **Usage:** Prevents clock and system messages from being recorded

###### `void PlayMessages(std::vector<MidiMessage> msgs)`
- **Description:** Send MIDI messages respecting channel mute/solo state
- **Parameters:**
  - `msgs` - Vector of MIDI messages to play
- **Actions:**
  - Checks if channel is muted or un-solo'd
  - Sends messages to MIDI output if not filtered

###### `void PlayMetronomeClick(bool isDownbeat)`
- **Description:** Play metronome click sound
- **Parameters:**
  - `isDownbeat` - True for first beat of measure (higher pitch)
- **Actions:**
  - Plays high note (C6) for downbeat
  - Plays low note (C5) for other beats
  - Uses channel 15 (metronome channel)

###### `void SilenceAllChannels()`
- **Description:** Send all-notes-off to all channels
- **Actions:**
  - Sends MIDI CC 123 (All Notes Off) to channels 0-14
- **Usage:** Called when stopping playback

###### `void MergeOverlappingNotes(Track& buffer)`
- **Description:** Handle consecutive note-ons during loop recording
- **Parameters:**
  - `buffer` - Recording buffer track
- **Actions:**
  - Detects when same note is held across loop boundary
  - Extends note duration instead of creating overlapping notes
  - Maintains `mActiveNotes` vector to track held notes

##### Member Variables:

- `std::chrono::steady_clock::time_point mLastTick` - Last update timestamp
- `SoundBank mSoundBank` - 15 MIDI channel manager
- `Transport mTransport` - Playback state machine
- `TrackSet mTrackSet` - 15 track storage
- `Track mRecordingBuffer` - Temporary recording buffer
- `int mRecordingBufferIterator` - Playback iterator for loop recording
- `std::vector<ActiveNote> mActiveNotes` - Held notes during loop recording
- `std::shared_ptr<MidiIn> mMidiIn` - MIDI input device
- `MidiLogCallback mMidiLogCallback` - Event logging callback
- `DirtyStateCallback mDirtyStateCallback` - Dirty state callback
- `bool mMetronomeEnabled` - Metronome on/off
- `bool mIsDirty` - Has unsaved changes
- `std::string mCurrentProjectPath` - Current file path
- `std::vector<std::unique_ptr<Command>> mUndoStack` - Undo command stack
- `std::vector<std::unique_ptr<Command>> mRedoStack` - Redo command stack
- `std::vector<ClipboardNote> mClipboard` - Copy/paste buffer

##### Nested Types:

###### `struct ClipboardNote`
- **Purpose:** Copy/paste data structure
- **Members:**
  - `uint64_t relativeStartTick` - Relative to first note in selection
  - `uint64_t duration` - Note length in ticks
  - `uint8_t pitch` - MIDI note number (0-127)
  - `uint8_t velocity` - Note velocity (0-127)
  - `int trackIndex` - Source track (0-14)

###### `struct ActiveNote`
- **Purpose:** Track held notes during loop recording
- **Members:**
  - `ubyte pitch` - Note number
  - `ubyte channel` - MIDI channel
  - `uint64_t startTick` - When note started

##### Type Aliases:

- `using MidiLogCallback = std::function<void(const TimedMidiEvent&)>`
- `using DirtyStateCallback = std::function<void(bool isDirty)>`

##### Constants:

- `static const size_t MAX_UNDO_STACK_SIZE = 50`

---

### **src/AppModel/Transport.h**

Playback/recording state machine and timing engine.

#### **class Transport**

**Purpose:** Manage playback state, timing, tempo, and transport controls.

##### Public Member Functions:

###### `Transport()`
- **Description:** Default constructor
- **Actions:**
  - Initializes state to `Stopped`
  - Sets tempo to 120 BPM
  - Sets time signature to 4/4
  - Sets loop range to 0-15360 (4 bars at 960 PPQN)

###### `uint64_t StartPlayBack()`
- **Description:** Mark playback start position
- **Returns:** Current tick position where playback started
- **Usage:** Called when transitioning to Playing state

###### `void UpdatePlayBack(uint64_t deltaMs)`
- **Description:** Advance playback time
- **Parameters:**
  - `deltaMs` - Milliseconds since last update
- **Actions:**
  - Converts delta time to ticks based on tempo
  - Advances `mCurrentTick`
  - Handles loop wraparound if loop is enabled

###### `uint64_t GetCurrentTick() const`
- **Description:** Get current playback position
- **Returns:** Current tick (0-based)

###### `uint64_t GetStartPlayBackTick() const`
- **Description:** Get tick where playback started
- **Returns:** Start playback tick

###### `void ShiftCurrentTime()`
- **Description:** Accelerated rewind/fast-forward
- **Actions:**
  - Moves playhead forward/backward based on state
  - Accelerates over time (`mShiftSpeed *= mShiftAccel`)
  - Clamps to valid range [0, last event]

###### `void ShiftToTick(uint64_t newTick)`
- **Description:** Jump to specific tick position
- **Parameters:**
  - `newTick` - Target tick position
- **Actions:**
  - Sets `mCurrentTick` directly
  - Converts to time in milliseconds

###### `void Stop()`
- **Description:** Reset state machine to Stopped
- **Actions:**
  - Sets state to `Stopped`
  - Resets shift speed

###### `void Reset()`
- **Description:** Jump playhead to tick 0
- **Actions:**
  - Sets `mCurrentTick = 0`
  - Sets `mCurrentTimeMs = 0`

###### `wxString GetFormattedTime() const`
- **Description:** Get current time as MM:SS:mmm string
- **Returns:** Formatted time string (e.g., "01:30:500")

###### `wxString GetFormattedTime(uint64_t timeMs) const`
- **Description:** Format arbitrary time value
- **Parameters:**
  - `timeMs` - Time in milliseconds
- **Returns:** Formatted time string

###### `BeatInfo CheckForBeat(uint64_t lastTick, uint64_t currentTick) const`
- **Description:** Detect if a beat occurred between two ticks
- **Parameters:**
  - `lastTick` - Previous tick position
  - `currentTick` - Current tick position
- **Returns:** BeatInfo struct with `beatOccurred` and `isDownbeat` flags
- **Usage:** Used for metronome timing

##### Public Member Variables:

- `State mState` - Current transport state
- `double mTempo` - Tempo in BPM (default 120.0)
- `int mTimeSignatureNumerator` - Beats per measure (default 4)
- `int mTimeSignatureDenominator` - Note value (default 4)
- `bool mLoopEnabled` - Loop playback enabled
- `uint64_t mLoopStartTick` - Loop region start (default 0)
- `uint64_t mLoopEndTick` - Loop region end (default 15360 = 4 bars)

##### Private Member Variables:

- `uint64_t mCurrentTimeMs` - Current time in milliseconds
- `uint64_t mStartPlayBackTick` - Tick when playback started
- `uint64_t mCurrentTick` - Current playback tick
- `int mTicksPerQuarter` - PPQN (960)
- `const double DEFAULT_SHIFT_SPEED` - Rewind/FF speed (5.0)
- `double mShiftSpeed` - Current shift speed
- `double mShiftAccel` - Acceleration factor (1.01)

##### Nested Types:

###### `enum State`
- **Purpose:** Transport state machine states
- **Values:**
  - `Stopped` - Not playing or recording
  - `StopRecording` - Transition state after clicking stop while recording
  - `StopPlaying` - Transition state after clicking stop while playing
  - `Playing` - Playback active
  - `ClickedPlay` - Transition state after clicking play
  - `Recording` - Recording active
  - `ClickedRecord` - Transition state after clicking record
  - `Rewinding` - Fast rewind active
  - `FastForwarding` - Fast forward active

###### `struct BeatInfo`
- **Purpose:** Beat detection result
- **Members:**
  - `bool beatOccurred` - True if beat boundary crossed
  - `bool isDownbeat` - True if first beat of measure

##### Constants:

- `static constexpr unsigned char METRONOME_CHANNEL = 15` (channel 16, zero-indexed)

---

### **src/AppModel/SoundBank.h**

MIDI channel manager for 15 user channels.

#### **struct MidiChannel**

**Purpose:** Store per-channel settings.

##### Members:

- `ubyte channelNumber` - MIDI channel (0-14)
- `ubyte programNumber` - Instrument patch (0-127)
- `ubyte volume` - Volume level (0-127, default 100)
- `bool mute` - Channel muted
- `bool solo` - Channel solo'd
- `bool record` - Record-enabled

---

#### **class SoundBank**

**Purpose:** Manage 15 MIDI channels and apply settings to MIDI output.

##### Public Member Functions:

###### `SoundBank()`
- **Description:** Constructor - initialize channels with default patches
- **Actions:**
  - Sets each channel to sequential GM patches (Piano, Bright Piano, etc.)
  - Sets volume to 100
  - Channel 0 defaults to record-enabled

###### `void SetMidiOutDevice(std::shared_ptr<MidiOut> device)`
- **Description:** Assign MIDI output device
- **Parameters:**
  - `device` - Shared pointer to MidiOut

###### `std::shared_ptr<MidiOut> GetMidiOutDevice() const`
- **Description:** Get MIDI output device
- **Returns:** Shared pointer to MidiOut

###### `void ApplyChannelSettings()`
- **Description:** Send program change and volume CC messages for all channels
- **Actions:**
  - Sends MIDI program change for each channel
  - Sends MIDI CC 7 (volume) for each channel
- **Usage:** Called after loading project to sync MIDI device

###### `MidiChannel& GetChannel(ubyte c)`
- **Description:** Access specific channel
- **Parameters:**
  - `c` - Channel number (0-14)
- **Returns:** Reference to MidiChannel

###### `std::span<MidiChannel> GetAllChannels()`
- **Description:** Access all channels
- **Returns:** Span view of channel array

###### `bool SolosFound() const`
- **Description:** Check if any channel is solo'd
- **Returns:** True if at least one channel has solo=true
- **Usage:** Used to filter playback when solos are active

##### Private Member Variables:

- `std::shared_ptr<MidiOut> mMidiOut` - MIDI output device
- `MidiChannel mChannels[15]` - Array of 15 channels (channel 16 reserved for metronome)

---

### **src/AppModel/TrackSet.h**

Track storage and efficient playback scheduling.

#### Type Aliases:

- `using Track = std::vector<TimedMidiEvent>` - Single track
- `using TrackBank = std::array<Track, 15>` - 15 tracks (one per channel)

---

#### **struct TimedMidiEvent**

**Purpose:** MIDI event with timestamp.

##### Members:

- `MidiMessage mm` - MIDI message data
- `uint64_t tick` - Tick position

---

#### **class TrackSet**

**Purpose:** Store and play back MIDI tracks with efficient iterator-based scheduling.

##### Public Member Functions:

###### `Track& GetTrack(ubyte channelNumber)`
- **Description:** Access track by channel number
- **Parameters:**
  - `channelNumber` - Channel (0-14)
- **Returns:** Reference to Track

###### `std::vector<MidiMessage> PlayBack(uint64_t currentTick)`
- **Description:** Return all MIDI events at current tick
- **Parameters:**
  - `currentTick` - Current playback position
- **Actions:**
  - For each track, scans forward from iterator position
  - Collects all events at currentTick
  - Advances iterator past returned events
- **Returns:** Vector of MIDI messages to play
- **Called by:** `AppModel::Update()`

###### `void FindStart(uint64_t startTick)`
- **Description:** Initialize playback iterators
- **Parameters:**
  - `startTick` - Position to start playback from
- **Actions:**
  - For each track, finds first event at or after startTick
  - Sets iterator to that position (or -1 if past end)
- **Usage:** Called when starting playback or seeking

###### `void FinalizeRecording(Track& recordingBuffer)`
- **Description:** Merge recording buffer into tracks
- **Parameters:**
  - `recordingBuffer` - Temporary recording buffer
- **Actions:**
  - Distributes events to appropriate tracks by channel
  - Sorts each modified track by tick
  - Clears recording buffer
- **Called by:** `AppModel::Update()` when stopping recording

##### Private Member Functions:

###### `void Sort()`
- **Description:** Sort all tracks by tick
- **Actions:**
  - Sorts each track using `std::sort` with tick comparator

##### Private Member Variables:

- `TrackBank mTracks` - Array of 15 tracks
- `int iterators[15]` - Per-track playback iterators (index into track, -1 = done)

---

## 3. MainFrame (Window Management)

### **src/MainFrame/MainFrame.h / MainFrame.cpp**

Main application window with dockable panel system.

#### **class MainFrame : public wxFrame**

**Purpose:** Application window, menu system, and panel orchestration.

##### Public Member Functions:

###### `MainFrame()`
- **Description:** Constructor - create main window and UI
- **Actions:**
  - Sets up wxAuiManager for dockable panels
  - Creates AppModel
  - Calls `CreateDockablePanes()`
  - Calls `CreateMenuBar()`
  - Binds accelerator keys (Ctrl+S, Ctrl+Z, Spacebar, etc.)
  - Starts 10ms update timer
  - Registers callbacks with AppModel

##### Private Member Functions:

###### `void CreateDockablePanes()`
- **Description:** Instantiate and register all panels
- **Actions:**
  - Creates TransportPanel, SoundBankPanel, MidiSettingsPanel, etc.
  - Calls `RegisterPanel()` for each
  - Commits layout with `mAuiManager.Update()`

###### `void RegisterPanel(const PanelInfo& info)`
- **Description:** Add panel to AUI manager and registry
- **Parameters:**
  - `info` - PanelInfo struct describing panel
- **Actions:**
  - Converts to `wxAuiPaneInfo`
  - Adds to `mAuiManager`
  - Stores in `mPanels` map

###### `std::unordered_map<int, PanelInfo>& GetAllPanels()`
- **Description:** Access panel registry
- **Returns:** Reference to panels map

###### `void SetPanelVisibility(int id, bool vis)`
- **Description:** Show/hide panel
- **Parameters:**
  - `id` - Menu ID
  - `vis` - True to show, false to hide
- **Actions:**
  - Updates AUI manager
  - Syncs menu checkmark

###### `void CreateMenuBar()`
- **Description:** Build File/Edit/View menus
- **Actions:**
  - Creates File menu (New, Open, Save, Save As, Exit)
  - Creates Edit menu (Undo, Redo, Quantize)
  - Creates View menu (dynamically from panel registry)
- **Accelerators:**
  - Ctrl+N: New
  - Ctrl+O: Open
  - Ctrl+S: Save
  - Ctrl+Shift+S: Save As
  - Ctrl+Z: Undo
  - Ctrl+Y: Redo
  - Q: Quantize
  - Space: Toggle Play
  - R: Start Record

###### `void CreateSizer()`
- **Description:** Apply vertical sizer to frame
- **Actions:**
  - Creates wxBoxSizer for layout

###### `void SyncMenuChecks()`
- **Description:** Sync view menu checkmarks with panel visibility
- **Actions:**
  - Iterates over panels, updates checkmarks

###### `void OnTogglePane(wxCommandEvent& event)`
- **Description:** Handle view menu item click
- **Parameters:**
  - `event` - Menu event with ID
- **Actions:**
  - Toggles panel visibility
  - Updates AUI and syncs menu

###### `void OnPaneClosed(wxAuiManagerEvent& event)`
- **Description:** Handle panel close button
- **Parameters:**
  - `event` - AUI manager event
- **Actions:**
  - Updates panel registry visibility
  - Syncs menu checkmark

###### `void OnTimer(wxTimerEvent&)`
- **Description:** Main 10ms update loop
- **Actions:**
  - Calls `AppModel::Update()`
  - Updates TransportPanel display
  - Updates MidiCanvasPanel
  - Updates UndoHistoryPanel

###### `void OnAuiRender(wxAuiManagerEvent& event)`
- **Description:** Debug handler for AUI render events
- **Usage:** Unused, for debugging panel sizes

###### `void OnUndo(wxCommandEvent& event)`
- **Description:** Ctrl+Z handler
- **Actions:**
  - Calls `AppModel::Undo()`
  - Refreshes canvas and log

###### `void OnRedo(wxCommandEvent& event)`
- **Description:** Ctrl+Y handler
- **Actions:**
  - Calls `AppModel::Redo()`
  - Refreshes canvas and log

###### `void OnQuantize(wxCommandEvent& event)`
- **Description:** Q key handler
- **Actions:**
  - Gets grid size from MidiCanvasPanel
  - Creates QuantizeCommand for each track
  - Executes commands
  - Refreshes UI

###### `void OnNew(wxCommandEvent& event)`
- **Description:** Ctrl+N handler
- **Actions:**
  - Prompts to save if dirty
  - Calls `AppModel::ClearProject()`
  - Updates UI and title

###### `void OnOpen(wxCommandEvent& event)`
- **Description:** Ctrl+O handler
- **Actions:**
  - Shows file open dialog
  - Calls `AppModel::LoadProject()`
  - Syncs UI controls with loaded data
  - Updates title

###### `void OnSave(wxCommandEvent& event)`
- **Description:** Ctrl+S handler
- **Actions:**
  - If untitled, shows save dialog
  - Calls `AppModel::SaveProject()`
  - Updates title

###### `void OnSaveAs(wxCommandEvent& event)`
- **Description:** Ctrl+Shift+S handler
- **Actions:**
  - Shows save dialog
  - Calls `AppModel::SaveProject()`
  - Updates title

###### `void OnExit(wxCommandEvent& event)`
- **Description:** Exit menu/Alt+F4 handler
- **Actions:**
  - Prompts to save if dirty
  - Closes window

###### `void OnTogglePlay(wxCommandEvent& event)`
- **Description:** Spacebar handler
- **Actions:**
  - If playing/recording, stops
  - If stopped, starts playback

###### `void OnStartRecord(wxCommandEvent& event)`
- **Description:** R key handler
- **Actions:**
  - Sets transport state to ClickedRecord

###### `void UpdateTitle()`
- **Description:** Update title bar with filename and dirty state
- **Actions:**
  - Shows filename or "Untitled"
  - Adds asterisk if dirty
  - Prepends "MidiWorks - "

###### `uint64_t GetDeltaTimeMs()`
- **Description:** Calculate time since last timer tick
- **Returns:** Delta time in milliseconds (unused, legacy)

##### Private Member Variables:

- `std::shared_ptr<AppModel> mAppModel` - Application model
- `wxAuiManager mAuiManager` - Dockable panel manager
- `wxTimer mTimer` - 10ms update timer
- `std::unordered_map<int, PanelInfo> mPanels` - Panel registry
- `MidiSettingsPanel* mMidiSettingsPanel` - Pointer to MIDI settings panel
- `SoundBankPanel* mSoundBankPanel` - Pointer to mixer panel
- `TransportPanel* mTransportPanel` - Pointer to transport panel
- `MidiCanvasPanel* mMidiCanvasPanel` - Pointer to piano roll panel
- `LogPanel* mLogPanel` - Pointer to log panel
- `UndoHistoryPanel* mUndoHistoryPanel` - Pointer to undo history panel
- `ShortcutsPanel* mShortcutsPanel` - Pointer to shortcuts panel

---

### **src/MainFrame/PaneInfo.h**

Panel layout metadata and configuration.

#### **enum PanePosition**

**Purpose:** Panel dock position options.

##### Values:

- `Left` - Dock on left side
- `Right` - Dock on right side
- `Bottom` - Dock on bottom
- `Center` - Dock in center (fills remaining space)
- `Top` - Dock on top
- `Float` - Floating window

---

#### **struct PanelInfo**

**Purpose:** Panel configuration metadata.

##### Members:

- `wxString name` - Display name shown in title bar and menu
- `wxWindow* window` - Pointer to panel widget
- `int menuId` - Unique menu item ID
- `PanePosition defaultPosition` - Default dock position
- `wxSize minSize` - Minimum size (default -1, -1 = auto)
- `wxSize bestSize` - Preferred size (default -1, -1 = auto)
- `bool hasCaption` - Show title bar (default true)
- `bool hasCloseButton` - Show close button (default true)
- `bool isVisible` - Currently visible (default true)

---

#### Standalone Functions:

###### `wxAuiPaneInfo CreatePaneInfo(const PanelInfo& info)`
- **Description:** Convert PanelInfo to wxAuiPaneInfo
- **Parameters:**
  - `info` - PanelInfo struct
- **Returns:** Configured wxAuiPaneInfo
- **Actions:**
  - Maps PanePosition to wxAuiPaneInfo dock methods
  - Sets caption, close button, sizes, visibility

---

## 4. Panels (User Interface)

### **src/Panels/TransportPanel.h**

Playback/recording controls and time display.

#### **class TransportPanel : public wxPanel**

**Purpose:** Transport controls (play, stop, record, rewind, etc.) and time/tempo display.

##### Public Member Functions:

###### `TransportPanel(wxWindow* parent, std::shared_ptr<AppModel> model, const wxColour& bgColor, const wxString& label)`
- **Description:** Constructor
- **Parameters:**
  - `parent` - Parent window
  - `model` - AppModel reference
  - `bgColor` - Background color
  - `label` - Panel label
- **Actions:**
  - Calls `CreateControls()`
  - Calls `SetupSizers()`
  - Calls `BindEventHandlers()`

###### `void UpdateDisplay()`
- **Description:** Refresh tick and time displays
- **Actions:**
  - Updates tick display (e.g., "12480")
  - Updates time display (e.g., "01:30:250")
- **Called by:** `MainFrame::OnTimer()` every 10ms

###### `void UpdateTempoDisplay()`
- **Description:** Sync tempo control with loaded project
- **Actions:**
  - Sets `mTempoControl` value from Transport
- **Usage:** Called after loading project

##### Private Member Functions:

###### `void CreateControls()`
- **Description:** Create all UI widgets
- **Actions:**
  - Creates tick and time displays
  - Creates Reset, Rewind, Stop, Play, Record, FF buttons
  - Creates tempo spin control (40-300 BPM)
  - Creates metronome and loop checkboxes

###### `void SetupSizers()`
- **Description:** Layout controls horizontally
- **Actions:**
  - Uses wxBoxSizer with horizontal orientation
  - Adds spacing between button groups

###### `void BindEventHandlers()`
- **Description:** Bind button/control events
- **Actions:**
  - Binds button clicks, mouse events, spin events

###### `void OnStop(wxCommandEvent&)`
- **Description:** Stop button handler
- **Actions:**
  - Sets transport state to `StopPlaying` or `StopRecording`

###### `void OnPlay(wxCommandEvent&)`
- **Description:** Play button handler
- **Actions:**
  - Sets transport state to `ClickedPlay`

###### `void OnReset(wxCommandEvent&)`
- **Description:** Reset button handler
- **Actions:**
  - Calls `Transport::Reset()` (jump to tick 0)

###### `void OnRecord(wxCommandEvent&)`
- **Description:** Record button handler
- **Actions:**
  - Sets transport state to `ClickedRecord`

###### `void OnRewindDown(wxMouseEvent& event)`
- **Description:** Rewind button mouse-down handler
- **Actions:**
  - Saves current state
  - Sets transport state to `Rewinding`

###### `void OnFastForwardDown(wxMouseEvent& event)`
- **Description:** Fast-forward button mouse-down handler
- **Actions:**
  - Saves current state
  - Sets transport state to `FastForwarding`

###### `void StopTransport(wxMouseEvent& event)`
- **Description:** Rewind/FF button mouse-up handler
- **Actions:**
  - Restores previous transport state

###### `void OnTempoChange(wxSpinDoubleEvent& event)`
- **Description:** Tempo control change handler
- **Actions:**
  - Updates `Transport::mTempo`

###### `void OnMetronomeToggle(wxCommandEvent& event)`
- **Description:** Metronome checkbox handler
- **Actions:**
  - Calls `AppModel::SetMetronomeEnabled()`

###### `void OnLoopToggle(wxCommandEvent& event)`
- **Description:** Loop checkbox handler
- **Actions:**
  - Toggles `Transport::mLoopEnabled`

##### Private Member Variables:

- `std::shared_ptr<AppModel> mModel` - AppModel reference
- `Transport& mTransport` - Transport reference
- `wxStaticText* mTickDisplay` - Tick counter display
- `wxStaticText* mTimeDisplay` - Time display (MM:SS:mmm)
- `wxButton* mResetButton` - Reset button (|<<)
- `wxButton* mRewindButton` - Rewind button (<<)
- `wxButton* mStopButton` - Stop button
- `wxButton* mPlayButton` - Play button
- `wxButton* mRecordButton` - Record button
- `wxButton* mFastForwardButton` - Fast-forward button (>>)
- `wxStaticText* mTempoLabel` - "BPM:" label
- `wxSpinCtrlDouble* mTempoControl` - Tempo spinner (40-300)
- `wxCheckBox* mMetronomeCheckBox` - "Click" checkbox
- `wxCheckBox* mLoopCheckBox` - Loop enable checkbox
- `Transport::State mPreviousState` - For rewind/FF state restoration

---

### **src/Panels/SoundBankPanel.h**

Container for 15 channel control panels.

#### **class SoundBankPanel : public wxScrolledWindow**

**Purpose:** MIDI output selector and 15-channel mixer.

##### Public Member Functions:

###### `SoundBankPanel(wxWindow* parent, SoundBank& soundBank, wxWindowID id = wxID_ANY)`
- **Description:** Constructor
- **Parameters:**
  - `parent` - Parent window
  - `soundBank` - SoundBank reference
  - `id` - Widget ID
- **Actions:**
  - Calls `CreateControls()`
  - Calls `SetupSizers()`
  - Calls `BindEventHandlers()`
  - Enables scrolling

###### `void UpdateFromModel()`
- **Description:** Sync all channel controls with loaded project
- **Actions:**
  - Calls `UpdateFromModel()` on each ChannelControlsPanel
- **Usage:** Called after loading project

##### Private Member Functions:

###### `void CreateControls()`
- **Description:** Create MIDI out selector and 15 channel panels
- **Actions:**
  - Creates wxChoice for MIDI output port
  - Creates 15 ChannelControlsPanel instances

###### `void SetupSizers()`
- **Description:** Vertical layout
- **Actions:**
  - Uses wxBoxSizer with vertical orientation
  - Adds MIDI out selector at top
  - Adds channel panels below

###### `void BindEventHandlers()`
- **Description:** Bind MIDI out choice event

###### `void OnMidiOutChoice(wxCommandEvent& event)`
- **Description:** MIDI output port change handler
- **Actions:**
  - Gets selected port index
  - Creates new MidiOut device
  - Calls `SoundBank::SetMidiOutDevice()`
  - Calls `SoundBank::ApplyChannelSettings()` to sync

##### Private Member Variables:

- `SoundBank& mSoundBank` - SoundBank reference
- `wxStaticText* mMidiOutLabel` - "MIDI Out:" label
- `wxChoice* mMidiOutChoice` - MIDI output port selector
- `std::array<ChannelControlsPanel*, 15> mChannelControls` - Array of channel panels

---

### **src/Panels/ChannelControls.h**

Per-channel controls (program, volume, mute, solo, record).

#### **class ChannelControlsPanel : public wxPanel**

**Purpose:** Single channel strip with patch selector, volume slider, and mute/solo/record buttons.

##### Public Member Functions:

###### `ChannelControlsPanel(wxWindow* parent, MidiChannel& channel, std::shared_ptr<MidiOut> midiOut)`
- **Description:** Constructor
- **Parameters:**
  - `parent` - Parent window
  - `channel` - MidiChannel reference
  - `midiOut` - MIDI output device
- **Actions:**
  - Calls `CreateControls()`
  - Calls `BindEvents()`
  - Calls `SetupSizers()`

###### `void UpdateFromModel()`
- **Description:** Sync UI with channel data (for loading)
- **Actions:**
  - Sets patch choice selection
  - Sets volume slider position
  - Sets mute/solo/record checkbox states

##### Private Member Functions:

###### `void CreateControls()`
- **Description:** Create all channel controls
- **Actions:**
  - Creates "Channel N" label
  - Creates wxChoice with 128 GM patches
  - Creates volume slider (0-127)
  - Creates mute, solo, record checkboxes
  - Creates separator line

###### `void BindEvents()`
- **Description:** Bind event handlers

###### `void SetupSizers()`
- **Description:** Vertical layout
- **Actions:**
  - Uses wxBoxSizer with vertical orientation
  - Stacks label, patch, volume, checkboxes, separator

###### `void OnPatchChanged(wxCommandEvent& event)`
- **Description:** Patch selector change handler
- **Actions:**
  - Updates `mChannel.programNumber`
  - Calls `SendPatch()`

###### `void OnVolumeChanged(wxCommandEvent& event)`
- **Description:** Volume slider change handler
- **Actions:**
  - Updates `mChannel.volume`
  - Calls `SendVolume()`

###### `void OnMuteToggled(wxCommandEvent& event)`
- **Description:** Mute checkbox handler
- **Actions:**
  - Toggles `mChannel.mute`

###### `void OnSoloToggled(wxCommandEvent& event)`
- **Description:** Solo checkbox handler
- **Actions:**
  - Toggles `mChannel.solo`

###### `void OnRecordToggled(wxCommandEvent& event)`
- **Description:** Record checkbox handler
- **Actions:**
  - Toggles `mChannel.record`

###### `void SendPatch()`
- **Description:** Send MIDI program change
- **Actions:**
  - Creates program change message
  - Sends to MIDI output

###### `void SendVolume()`
- **Description:** Send MIDI volume control change
- **Actions:**
  - Creates CC 7 (volume) message
  - Sends to MIDI output

##### Private Member Variables:

- `MidiChannel& mChannel` - Channel data reference
- `std::shared_ptr<MidiOut> mMidiOut` - MIDI output device
- `wxStaticLine* mStaticLine` - Visual separator
- `wxStaticText* mLabel` - "Channel N" label
- `wxChoice* mPatchChoice` - 128 GM patches
- `wxSlider* mVolumeSlider` - 0-127 range
- `wxCheckBox* mMuteCheck` - Mute button
- `wxCheckBox* mSoloCheck` - Solo button
- `wxCheckBox* mRecordCheck` - Record-enable button

---

### **src/Panels/MidiSettings.h**

MIDI input port selection panel.

#### **class MidiSettingsPanel : public wxPanel**

**Purpose:** Select MIDI input port.

##### Public Member Functions:

###### `MidiSettingsPanel(wxWindow* parent, std::shared_ptr<AppModel> appModel, const wxColour& bgColor, const wxString& label)`
- **Description:** Constructor
- **Parameters:**
  - `parent` - Parent window
  - `appModel` - AppModel reference
  - `bgColor` - Background color
  - `label` - Panel label
- **Actions:**
  - Calls `CreateControls()`
  - Calls `SetupSizers()`
  - Calls `BindEventHandlers()`

##### Private Member Functions:

###### `void CreateControls()`
- **Description:** Create port radio box
- **Actions:**
  - Gets available MIDI input ports from AppModel
  - Creates wxRadioBox with port names
  - Selects current port

###### `void SetupSizers()`
- **Description:** Layout
- **Actions:**
  - Uses wxBoxSizer

###### `void BindEventHandlers()`
- **Description:** Bind radio box event

###### `void OnInPortClicked(wxCommandEvent& evt)`
- **Description:** Port selection handler
- **Actions:**
  - Gets selected port index
  - Calls `AppModel::SetMidiInputPort()`

##### Private Member Variables:

- `std::shared_ptr<AppModel> mAppModel` - AppModel reference
- `wxRadioBox* mInPortList` - MIDI input port selector

---

### **src/Panels/MidiCanvas.h / MidiCanvas.cpp**

Piano roll editor with note editing, zoom, pan, and selection.

#### **class MidiCanvasPanel : public wxPanel**

**Purpose:** Piano roll editor for adding, moving, resizing, and deleting notes.

##### Public Member Functions:

###### `MidiCanvasPanel(wxWindow* parent, std::shared_ptr<AppModel> appModel, const wxString& label)`
- **Description:** Constructor
- **Parameters:**
  - `parent` - Parent window
  - `appModel` - AppModel reference
  - `label` - Panel label
- **Actions:**
  - Creates debug display, grid snap checkbox, duration dropdown, custom ticks input
  - Sets initial zoom levels
  - Binds paint, mouse, keyboard events
  - Sets scroll rate

###### `void Update()`
- **Description:** Auto-scroll during playback and refresh display
- **Actions:**
  - If playing/recording, scrolls to keep playhead visible
  - Calls `Refresh()` to trigger repaint
- **Called by:** `MainFrame::OnTimer()`

###### `uint64_t GetGridSize() const`
- **Description:** Get selected duration for quantize
- **Returns:** Grid size in ticks (e.g., 240 = 16th note)
- **Usage:** Called by quantize command

###### `NoteInfo FindNoteAtPosition(int screenX, int screenY)`
- **Description:** Hit detection for note at screen coordinates
- **Parameters:**
  - `screenX`, `screenY` - Screen coordinates
- **Returns:** NoteInfo struct (valid=false if no note found)
- **Actions:**
  - Converts screen to tick/pitch
  - Searches all tracks for note at that position

###### `bool IsOnResizeEdge(int screenX, const NoteInfo& note)`
- **Description:** Check if cursor is near note edge (for resizing)
- **Parameters:**
  - `screenX` - Mouse X position
  - `note` - Note to check
- **Returns:** True if within 5 pixels of note end

###### `std::vector<NoteInfo> FindNotesInRectangle(wxPoint start, wxPoint end)`
- **Description:** Rectangle selection hit detection
- **Parameters:**
  - `start`, `end` - Rectangle corners
- **Returns:** Vector of selected notes
- **Actions:**
  - Converts screen rectangle to tick/pitch ranges
  - Searches all tracks for overlapping notes

###### `void ClearSelection()`
- **Description:** Clear multi-selection
- **Actions:**
  - Clears `mSelectedNotes` vector

###### `bool IsNoteSelected(const NoteInfo& note) const`
- **Description:** Check if note is in selection
- **Parameters:**
  - `note` - Note to check
- **Returns:** True if note is selected

###### `bool IsNearLoopStart(int screenX)`
- **Description:** Loop edge detection for dragging
- **Parameters:**
  - `screenX` - Mouse X position
- **Returns:** True if within 5 pixels of loop start marker

###### `bool IsNearLoopEnd(int screenX)`
- **Description:** Loop edge detection for dragging
- **Parameters:**
  - `screenX` - Mouse X position
- **Returns:** True if within 5 pixels of loop end marker

###### `void ClampOffset()`
- **Description:** Constrain pan within bounds
- **Actions:**
  - Prevents panning beyond track boundaries

##### Private Member Functions:

###### `void Draw(wxPaintEvent&)`
- **Description:** Render grid, notes, playhead, selection
- **Actions:**
  - Creates wxGraphicsContext for antialiased rendering
  - Draws pitch grid (white/black keys)
  - Draws beat/measure grid
  - Draws loop region highlight
  - Draws all notes from all tracks (colored by track)
  - Draws recording buffer notes (semi-transparent)
  - Draws playhead line
  - Draws selection rectangle
  - Draws hovered note highlight

###### `void DrawGrid(wxGraphicsContext* gc)`
- **Description:** Draw beat/measure/pitch grid
- **Actions:**
  - Draws vertical lines at beat intervals
  - Darker lines at measure boundaries
  - Draws horizontal lines at pitch boundaries
  - Colors white/black key rows differently

###### `int Flip(int y)`
- **Description:** Flip Y coordinate (screen to MIDI pitch)
- **Parameters:**
  - `y` - Screen Y coordinate
- **Returns:** Flipped Y coordinate
- **Reason:** MIDI pitch 0 is at bottom, screen 0 is at top

###### `uint64_t ScreenXToTick(int screenX)`
- **Description:** Screen to tick conversion
- **Parameters:**
  - `screenX` - Screen X coordinate
- **Returns:** Tick position

###### `uint8_t ScreenYToPitch(int screenY)`
- **Description:** Screen to MIDI pitch conversion
- **Parameters:**
  - `screenY` - Screen Y coordinate
- **Returns:** MIDI pitch (0-127)

###### `int TickToScreenX(uint64_t tick)`
- **Description:** Tick to screen conversion
- **Parameters:**
  - `tick` - Tick position
- **Returns:** Screen X coordinate

###### `int PitchToScreenY(uint8_t pitch)`
- **Description:** MIDI pitch to screen conversion
- **Parameters:**
  - `pitch` - MIDI pitch
- **Returns:** Screen Y coordinate

###### `void PlayPreviewNote(uint8_t pitch)`
- **Description:** Audio preview on record channels
- **Parameters:**
  - `pitch` - MIDI note to preview
- **Actions:**
  - Sends note-on to all record-enabled channels
  - Stores preview state

###### `void StopPreviewNote()`
- **Description:** Stop audio preview
- **Actions:**
  - Sends note-off to all preview channels
  - Clears preview state

###### `uint64_t GetSelectedDuration() const`
- **Description:** Get duration from dropdown
- **Returns:** Duration in ticks

###### `uint64_t ApplyGridSnap(uint64_t tick) const`
- **Description:** Snap tick to grid
- **Parameters:**
  - `tick` - Unsnapped tick
- **Returns:** Snapped tick (if grid snap enabled)

###### Event Handlers:

###### `void OnMouseWheel(wxMouseEvent& event)`
- **Description:** Zoom (horizontal/vertical)
- **Actions:**
  - Shift+Wheel: Horizontal zoom (adjust `mTicksPerPixel`)
  - Ctrl+Wheel: Vertical zoom (adjust `mNoteHeight`)
  - Plain wheel: Vertical scroll

###### `void OnLeftDown(wxMouseEvent& event)`
- **Description:** Add/select/move/resize notes or drag loop edges
- **Actions:**
  - Check if near loop start/end → start dragging loop edge
  - Hit test for note
    - If on resize edge → start resizing
    - If on note body → start moving (or add to selection if Shift)
  - If no note → start adding new note (or rectangle selection if Shift)
  - Stores original note data for undo

###### `void OnLeftUp(wxMouseEvent& event)`
- **Description:** Finalize edit with undo command
- **Actions:**
  - If adding note → create AddNoteCommand
  - If moving note → create MoveNoteCommand
  - If resizing note → create ResizeNoteCommand
  - If dragging loop edge → update Transport loop bounds
  - Executes command via AppModel
  - Resets mouse mode to Idle

###### `void OnMiddleDown(wxMouseEvent& event)`
- **Description:** Delete note or move playhead
- **Actions:**
  - Shift+Middle: Move playhead to clicked position
  - Middle on note: Create DeleteNoteCommand

###### `void OnRightDown(wxMouseEvent& event)`
- **Description:** Start panning
- **Actions:**
  - Sets `mIsDragging = true`
  - Stores mouse position

###### `void OnRightUp(wxMouseEvent& event)`
- **Description:** Stop panning
- **Actions:**
  - Sets `mIsDragging = false`

###### `void OnMouseMove(wxMouseEvent& event)`
- **Description:** Handle drag operations
- **Actions:**
  - Update debug display with tick/pitch
  - If panning → adjust `mOriginOffset`
  - If adding note → update note duration preview
  - If moving note → update note position preview
  - If resizing note → update note duration preview
  - If dragging loop edge → update loop marker preview
  - If rectangle selecting → update selection rectangle
  - Hover detection for note highlighting
  - Audio preview when hovering (if not playing/recording)

###### `void OnSize(wxSizeEvent& event)`
- **Description:** Recalculate zoom limits
- **Actions:**
  - Updates `mMinNoteHeight` and `mMaxNoteHeight` based on panel size

###### `void OnMouseLeave(wxMouseEvent& event)`
- **Description:** Stop preview notes
- **Actions:**
  - Calls `StopPreviewNote()`

###### `void OnKeyDown(wxKeyEvent& event)`
- **Description:** Keyboard shortcuts
- **Actions:**
  - Delete: Delete selected notes (create DeleteMultipleNotesCommand)
  - Escape: Clear selection
  - Ctrl+A: Select all notes
  - Ctrl+C: Copy selected notes to clipboard
  - Ctrl+V: Paste clipboard notes at playhead (create PasteCommand)
  - Ctrl+X: Cut selected notes (copy + delete)

##### Private Member Variables:

- `std::shared_ptr<AppModel> mAppModel` - AppModel reference
- `Transport& mTransport` - Transport reference
- `TrackSet& mTrackSet` - TrackSet reference
- `Track& mRecordingBuffer` - Recording buffer reference
- `wxStaticText* mDebugMessage` - Mouse position display
- `wxCheckBox* mGridSnapCheckbox` - Grid snap toggle
- `wxChoice* mDurationChoice` - Note duration selector (whole, half, quarter, etc.)
- `wxSpinCtrl* mCustomTicksCtrl` - Custom duration input
- `int mNoteHeight` - Pixels per MIDI note (vertical zoom, default 10)
- `int mMinNoteHeight`, `mMaxNoteHeight` - Zoom limits (1-50)
- `int mTicksPerPixel` - Horizontal zoom (default 10)
- `bool mIsDragging` - Right-click panning active
- `wxPoint mLastMouse` - Last mouse position (for panning)
- `wxPoint mOriginOffset` - Pan offset
- `int mCurrentEditTrack` - Selected track (0-14)
- `MouseMode mMouseMode` - Current mouse operation
- `size_t mSelectedNoteOnIndex`, `mSelectedNoteOffIndex` - Selected note indices
- `bool mIsPreviewingNote` - Audio preview active
- `uint8_t mPreviewPitch` - Preview note pitch
- `uint64_t mPreviewStartTick` - Preview start time
- `std::vector<uint8_t> mPreviewChannels` - Channels playing preview
- `NoteInfo mHoveredNote` - Hovered note (for highlighting)
- `NoteInfo mSelectedNote` - Primary selected note
- `std::vector<NoteInfo> mSelectedNotes` - Multi-selection
- `bool mIsSelecting` - Rectangle selection active
- `wxPoint mSelectionStart`, `mSelectionEnd` - Selection rectangle
- `uint64_t mOriginalStartTick`, `mOriginalEndTick` - For undo
- `uint8_t mOriginalPitch` - For undo
- `wxPoint mDragStartPos` - Drag starting position

##### Nested Types:

###### `enum MouseMode`
- **Purpose:** Current mouse operation
- **Values:**
  - `Idle` - No operation
  - `Adding` - Adding new note
  - `MovingNote` - Moving existing note
  - `ResizingNote` - Resizing note duration
  - `DraggingLoopStart` - Dragging loop start marker
  - `DraggingLoopEnd` - Dragging loop end marker

###### `struct NoteInfo`
- **Purpose:** Note identification data
- **Members:**
  - `int trackIndex` - Track number (0-14)
  - `size_t noteOnIndex` - Index of note-on event in track
  - `size_t noteOffIndex` - Index of note-off event in track
  - `uint64_t startTick` - Note start position
  - `uint64_t endTick` - Note end position
  - `uint8_t pitch` - MIDI note number
  - `bool valid` - True if note found
- **Methods:**
  - `bool operator==(const NoteInfo& other) const` - Equality comparison

---

### **src/Panels/Log.h**

Real-time MIDI event logging panel.

#### **class LogPanel : public wxPanel**

**Purpose:** Display MIDI events as they occur.

##### Public Member Functions:

###### `LogPanel(wxWindow* parent)`
- **Description:** Constructor
- **Parameters:**
  - `parent` - Parent window
- **Actions:**
  - Creates read-only multiline wxTextCtrl
  - Sets monospace font

###### `void LogMidiEvent(const TimedMidiEvent& msg)`
- **Description:** Append MIDI event to log
- **Parameters:**
  - `msg` - Timed MIDI event
- **Actions:**
  - Formats event as "Tick: 12345, [NoteOn/NoteOff], Ch: 1, Pitch: 60"
  - Appends to text control
  - Auto-scrolls to bottom

###### `void RefreshFromTrack(const std::vector<TimedMidiEvent>& track)`
- **Description:** Rebuild log from track
- **Parameters:**
  - `track` - Track to display
- **Actions:**
  - Clears log
  - Iterates over track events
  - Calls `LogMidiEvent()` for each
- **Usage:** Called after undo/redo to refresh log

##### Private Member Variables:

- `wxTextCtrl* mTextCtrl` - Read-only multiline text control

---

### **src/Panels/UndoHistoryPanel.h**

Display undo/redo command stacks.

#### **class UndoHistoryPanel : public wxPanel**

**Purpose:** Show undo and redo history for user visibility.

##### Public Member Functions:

###### `UndoHistoryPanel(wxWindow* parent, std::shared_ptr<AppModel> appModel)`
- **Description:** Constructor
- **Parameters:**
  - `parent` - Parent window
  - `appModel` - AppModel reference
- **Actions:**
  - Creates two wxListBox widgets (undo and redo)
  - Creates labels

###### `void UpdateDisplay()`
- **Description:** Refresh command history display
- **Actions:**
  - Clears both list boxes
  - Gets undo and redo stacks from AppModel
  - Populates list boxes with command descriptions
- **Called by:** `MainFrame::OnTimer()` every 10ms

##### Private Member Variables:

- `std::shared_ptr<AppModel> mAppModel` - AppModel reference
- `wxListBox* mUndoList` - Undo stack display
- `wxListBox* mRedoList` - Redo stack display

---

### **src/Panels/ShortcutsPanel.h**

Keyboard/mouse reference guide.

#### **class ShortcutsPanel : public wxScrolledWindow**

**Purpose:** Display comprehensive keyboard and mouse shortcuts.

##### Public Member Functions:

###### `ShortcutsPanel(wxWindow* parent, const wxColour& bgColor, const wxString& label)`
- **Description:** Constructor
- **Parameters:**
  - `parent` - Parent window
  - `bgColor` - Background color
  - `label` - Panel label
- **Actions:**
  - Calls `CreateControls()`
  - Calls `SetupSizers()`
  - Enables scrolling

##### Private Member Functions:

###### `void CreateControls()`
- **Description:** Build comprehensive shortcut list
- **Actions:**
  - Adds sections: Transport, File, Edit, Piano Roll, View
  - Calls helper methods to populate shortcuts

###### `void SetupSizers()`
- **Description:** Layout
- **Actions:**
  - Uses wxBoxSizer

###### `void AddSection(const wxString& sectionName)`
- **Description:** Add section header
- **Parameters:**
  - `sectionName` - Section name (e.g., "Transport Controls")
- **Actions:**
  - Creates bold static text
  - Adds to sizer

###### `void AddShortcut(const wxString& key, const wxString& description)`
- **Description:** Add shortcut entry
- **Parameters:**
  - `key` - Key combination (e.g., "Ctrl+S")
  - `description` - Action description (e.g., "Save project")
- **Actions:**
  - Creates horizontal sizer with key and description
  - Adds to main sizer

###### `void AddInfo(const wxString& info)`
- **Description:** Add italic info text
- **Parameters:**
  - `info` - Information text
- **Actions:**
  - Creates italic static text
  - Adds to sizer

###### `void AddTip(const wxString& tip)`
- **Description:** Add bulleted tip
- **Parameters:**
  - `tip` - Tip text
- **Actions:**
  - Creates indented static text with bullet
  - Adds to sizer

##### Private Member Variables:

- `wxBoxSizer* mMainSizer` - Main layout sizer

---

### **src/Panels/Dummy.h**

Simple placeholder panel for testing.

#### **class DummyPanel : public wxPanel**

**Purpose:** Placeholder panel with label.

##### Public Member Functions:

###### `DummyPanel(wxWindow* parent, const wxColour& bgColor, const wxString& label)`
- **Description:** Constructor
- **Parameters:**
  - `parent` - Parent window
  - `bgColor` - Background color
  - `label` - Label text
- **Actions:**
  - Creates static text with label
  - Sets background color

---

## 5. Commands (Undo/Redo System)

### **src/Commands/Command.h**

Abstract base class for Command pattern.

#### **class Command** (abstract)

**Purpose:** Define interface for undoable operations.

##### Pure Virtual Functions:

###### `virtual void Execute() = 0`
- **Description:** Perform the action
- **Called by:** `AppModel::ExecuteCommand()` or `AppModel::Redo()`

###### `virtual void Undo() = 0`
- **Description:** Reverse the action
- **Called by:** `AppModel::Undo()`

###### `virtual std::string GetDescription() const = 0`
- **Description:** Human-readable description for UI
- **Returns:** Description string (e.g., "Add Note C4")
- **Usage:** Displayed in UndoHistoryPanel

##### Virtual Destructor:

###### `virtual ~Command() = default`
- **Description:** Virtual destructor for polymorphism

---

### **src/Commands/NoteEditCommands.h / NoteEditCommands.cpp**

Commands for adding, deleting, moving, and resizing individual notes.

#### **class AddNoteCommand : public Command**

**Purpose:** Add a note to a track.

##### Public Member Functions:

###### `AddNoteCommand(Track& track, TimedMidiEvent noteOn, TimedMidiEvent noteOff)`
- **Description:** Constructor
- **Parameters:**
  - `track` - Target track
  - `noteOn` - Note-on event
  - `noteOff` - Note-off event

###### `void Execute() override`
- **Description:** Add note and sort track
- **Actions:**
  - Pushes note-on and note-off to track
  - Sorts track by tick
  - Calls `FindNoteIndices()` to locate note

###### `void Undo() override`
- **Description:** Remove added note
- **Actions:**
  - Erases note-off event (higher index first)
  - Erases note-on event

###### `std::string GetDescription() const override`
- **Description:** Get command description
- **Returns:** "Add Note [pitch name]" (e.g., "Add Note C4")

##### Private Member Functions:

###### `void FindNoteIndices()`
- **Description:** Locate note after sorting
- **Actions:**
  - Searches track for matching note-on and note-off events
  - Updates `mNoteOnIndex` and `mNoteOffIndex`

##### Private Member Variables:

- `Track& mTrack` - Target track reference
- `TimedMidiEvent mNoteOn`, `mNoteOff` - Note events
- `size_t mNoteOnIndex`, `mNoteOffIndex` - Indices in track

---

#### **class DeleteNoteCommand : public Command**

**Purpose:** Delete a note from a track.

##### Public Member Functions:

###### `DeleteNoteCommand(Track& track, size_t noteOnIndex, size_t noteOffIndex)`
- **Description:** Constructor
- **Parameters:**
  - `track` - Target track
  - `noteOnIndex` - Index of note-on event
  - `noteOffIndex` - Index of note-off event

###### `void Execute() override`
- **Description:** Remove note
- **Actions:**
  - Stores note events for undo
  - Erases note-off event (higher index first)
  - Erases note-on event

###### `void Undo() override`
- **Description:** Re-add note and sort
- **Actions:**
  - Pushes note-on and note-off back to track
  - Sorts track by tick

###### `std::string GetDescription() const override`
- **Description:** Get command description
- **Returns:** "Delete Note [pitch name]" (e.g., "Delete Note C4")

##### Private Member Variables:

- `Track& mTrack` - Target track reference
- `size_t mNoteOnIndex`, `mNoteOffIndex` - Original indices
- `TimedMidiEvent mNoteOn`, `mNoteOff` - Stored for undo

---

#### **class MoveNoteCommand : public Command**

**Purpose:** Move a note to a different tick/pitch.

##### Public Member Functions:

###### `MoveNoteCommand(Track& track, size_t noteOnIndex, size_t noteOffIndex, uint64_t newTick, uint8_t newPitch)`
- **Description:** Constructor
- **Parameters:**
  - `track` - Target track
  - `noteOnIndex` - Index of note-on event
  - `noteOffIndex` - Index of note-off event
  - `newTick` - New start tick
  - `newPitch` - New MIDI pitch

###### `void Execute() override`
- **Description:** Change note position and sort
- **Actions:**
  - Calculates note duration
  - Updates note-on tick and pitch
  - Updates note-off tick (preserves duration)
  - Sorts track by tick
  - Calls `FindNoteIndex()` to re-locate note

###### `void Undo() override`
- **Description:** Restore original position
- **Actions:**
  - Restores original tick and pitch
  - Recalculates note-off tick
  - Sorts track
  - Re-locates note

###### `std::string GetDescription() const override`
- **Description:** Get command description
- **Returns:** "Move Note [pitch name]" (e.g., "Move Note C4")

##### Private Member Functions:

###### `size_t FindNoteIndex(uint64_t tick, uint8_t pitch, MidiEvent eventType)`
- **Description:** Re-locate note after sorting
- **Parameters:**
  - `tick` - Tick position
  - `pitch` - MIDI pitch
  - `eventType` - NOTE_ON or NOTE_OFF
- **Returns:** Index in track

##### Private Member Variables:

- `Track& mTrack` - Target track reference
- `size_t mNoteOnIndex`, `mNoteOffIndex` - Current indices
- `uint64_t mOldTick`, `mNewTick` - Old and new tick positions
- `uint8_t mOldPitch`, `mNewPitch` - Old and new pitches
- `uint64_t mNoteDuration` - Note length in ticks

---

#### **class ResizeNoteCommand : public Command**

**Purpose:** Change note duration.

##### Public Member Functions:

###### `ResizeNoteCommand(Track& track, size_t noteOnIndex, size_t noteOffIndex, uint64_t newDuration)`
- **Description:** Constructor
- **Parameters:**
  - `track` - Target track
  - `noteOnIndex` - Index of note-on event
  - `noteOffIndex` - Index of note-off event
  - `newDuration` - New duration in ticks

###### `void Execute() override`
- **Description:** Change note duration and sort
- **Actions:**
  - Calculates old duration
  - Stores note-on tick and pitch
  - Updates note-off tick (note-on + new duration)
  - Sorts track
  - Re-locates note

###### `void Undo() override`
- **Description:** Restore original duration
- **Actions:**
  - Restores note-off tick (note-on + old duration)
  - Sorts track
  - Re-locates note

###### `std::string GetDescription() const override`
- **Description:** Get command description
- **Returns:** "Resize Note [pitch name]" (e.g., "Resize Note C4")

##### Private Member Functions:

###### `size_t FindNoteIndex(uint64_t tick, uint8_t pitch, MidiEvent eventType)`
- **Description:** Re-locate note after sorting
- **Parameters:**
  - `tick` - Tick position
  - `pitch` - MIDI pitch
  - `eventType` - NOTE_ON or NOTE_OFF
- **Returns:** Index in track

##### Private Member Variables:

- `Track& mTrack` - Target track reference
- `size_t mNoteOnIndex`, `mNoteOffIndex` - Current indices
- `uint64_t mOldDuration`, `mNewDuration` - Old and new durations
- `uint64_t mNoteOnTick` - Note start tick
- `uint8_t mPitch` - Note pitch

---

### **src/Commands/RecordCommand.h**

Make entire recording takes undoable.

#### **class RecordCommand : public Command**

**Purpose:** Add all recorded notes to tracks as a single undoable operation.

##### Public Member Functions:

###### `RecordCommand(TrackSet& trackSet, const Track& recordedNotes)`
- **Description:** Constructor
- **Parameters:**
  - `trackSet` - TrackSet reference
  - `recordedNotes` - Copy of recording buffer

###### `void Execute() override`
- **Description:** Add all recorded notes to tracks and sort
- **Actions:**
  - Distributes events to appropriate tracks by channel
  - Sorts each modified track

###### `void Undo() override`
- **Description:** Remove all recorded notes
- **Actions:**
  - For each event, finds and removes from track
  - Searches from end of track backward (more efficient)

###### `std::string GetDescription() const override`
- **Description:** Get command description
- **Returns:** "Record N notes" (e.g., "Record 24 notes")

##### Private Member Variables:

- `TrackSet& mTrackSet` - TrackSet reference
- `Track mRecordedNotes` - Copy of recording buffer

---

### **src/Commands/QuantizeCommand.h**

Snap all notes to grid.

#### **class QuantizeCommand : public Command**

**Purpose:** Quantize all note positions in a track.

##### Public Member Functions:

###### `QuantizeCommand(Track& track, uint64_t gridSize)`
- **Description:** Constructor
- **Parameters:**
  - `track` - Target track
  - `gridSize` - Grid size in ticks (e.g., 240 = 16th note)

###### `void Execute() override`
- **Description:** Quantize all events and sort
- **Actions:**
  - Stores original ticks for undo
  - For each event, snaps tick to nearest grid line
  - Sorts track

###### `void Undo() override`
- **Description:** Restore original ticks
- **Actions:**
  - Restores original tick values
  - Sorts track

###### `std::string GetDescription() const override`
- **Description:** Get command description
- **Returns:** "Quantize N notes to [grid size]" (e.g., "Quantize 12 notes to 240")

##### Private Member Variables:

- `Track& mTrack` - Target track reference
- `uint64_t mGridSize` - Grid size in ticks
- `std::vector<uint64_t> mOriginalTicks` - Original timestamps for undo

---

### **src/Commands/PasteCommand.h**

Paste clipboard notes at target position.

#### **class PasteCommand : public Command**

**Purpose:** Paste notes from clipboard at playhead position.

##### Public Member Functions:

###### `PasteCommand(TrackSet& trackSet, const std::vector<AppModel::ClipboardNote>& clipboardNotes, uint64_t pasteTick)`
- **Description:** Constructor
- **Parameters:**
  - `trackSet` - TrackSet reference
  - `clipboardNotes` - Clipboard data
  - `pasteTick` - Insertion position

###### `void Execute() override`
- **Description:** Add clipboard notes at paste position
- **Actions:**
  - For each clipboard note, creates note-on and note-off events
  - Offsets by `pasteTick`
  - Adds to appropriate track
  - Stores indices for undo
  - Sorts tracks

###### `void Undo() override`
- **Description:** Remove pasted notes
- **Actions:**
  - For each pasted note, removes from track
  - Removes in reverse order to maintain indices

###### `std::string GetDescription() const override`
- **Description:** Get command description
- **Returns:** "Paste N notes" (e.g., "Paste 8 notes")

##### Private Member Variables:

- `TrackSet& mTrackSet` - TrackSet reference
- `std::vector<AppModel::ClipboardNote> mClipboardNotes` - Clipboard data
- `uint64_t mPasteTick` - Insertion position
- `std::vector<PastedNoteInfo> mPastedNotes` - Indices for undo

##### Nested Types:

###### `struct PastedNoteInfo`
- **Purpose:** Track pasted note locations for undo
- **Members:**
  - `int trackIndex` - Track number
  - `size_t noteOnIndex`, `noteOffIndex` - Indices in track
  - `TimedMidiEvent noteOn`, `noteOff` - Event data

---

### **src/Commands/DeleteMultipleNotesCommand.h**

Batch delete notes efficiently.

#### **class DeleteMultipleNotesCommand : public Command**

**Purpose:** Delete multiple notes in a single undoable operation.

##### Public Member Functions:

###### `DeleteMultipleNotesCommand(TrackSet& trackSet, const std::vector<NoteToDelete>& notesToDelete)`
- **Description:** Constructor
- **Parameters:**
  - `trackSet` - TrackSet reference
  - `notesToDelete` - Vector of notes to delete

###### `void Execute() override`
- **Description:** Delete all notes in descending order
- **Actions:**
  - Sorts notes by track and index (descending)
  - Removes note-off events first (higher indices)
  - Removes note-on events
  - Deletion in descending order maintains indices

###### `void Undo() override`
- **Description:** Re-add all deleted notes
- **Actions:**
  - For each deleted note, adds back to track
  - Sorts tracks

###### `std::string GetDescription() const override`
- **Description:** Get command description
- **Returns:** "Delete N notes" (e.g., "Delete 15 notes")

##### Private Member Variables:

- `TrackSet& mTrackSet` - TrackSet reference
- `std::vector<NoteToDelete> mNotesToDelete` - Notes to delete (with data for undo)

##### Nested Types:

###### `struct NoteToDelete`
- **Purpose:** Note deletion data
- **Members:**
  - `int trackIndex` - Track number
  - `size_t noteOnIndex`, `noteOffIndex` - Indices in track
  - `TimedMidiEvent noteOn`, `noteOff` - Event data (for undo)

---

## 6. RtMidiWrapper (MIDI Abstraction)

### **src/RtMidiWrapper/MidiMessage/MidiMessage.h**

MIDI message abstraction and factory methods.

#### Namespace: `MidiInterface`

All MIDI wrapper classes are in the `MidiInterface` namespace.

---

#### **enum MidiEvent**

**Purpose:** MIDI event type constants.

##### Values:

- `NOTE_OFF = 0x80` - Note off message
- `NOTE_ON = 0x90` - Note on message
- `AFTERTOUCH = 0xA0` - Polyphonic aftertouch
- `CONTROL_CHANGE = 0xB0` - Control change (CC)
- `PROGRAM_CHANGE = 0xC0` - Program change (patch select)
- `CHANNEL_PRESSURE = 0xD0` - Channel aftertouch
- `PITCH_BEND = 0xE0` - Pitch bend

---

#### **enum ChannelMessage**

**Purpose:** Control change message constants.

##### Values:

- `VOLUME = 0x07` - CC 7 (volume)
- `ALL_SOUND_OFF = 0x78` - CC 120 (all sound off)
- `RESET_CONTROLLERS = 0x79` - CC 121 (reset controllers)
- `ALL_NOTES_OFF = 0x7B` - CC 123 (all notes off)

---

#### Type Alias:

- `using ubyte = unsigned char`

---

#### **class MidiMessage**

**Purpose:** Encapsulate MIDI message data with factory methods.

##### Public Member Variables:

- `ubyte mData[3]` - MIDI status and data bytes [status, data1, data2]
- `double timestamp` - RtMidi timestamp (seconds since start)

##### Constructors:

###### `MidiMessage()`
- **Description:** Default constructor
- **Actions:** Initializes all bytes to 0

###### `MidiMessage(ubyte d0, ubyte d1, ubyte d2)`
- **Description:** Construct from bytes
- **Parameters:**
  - `d0` - Status byte
  - `d1` - Data byte 1
  - `d2` - Data byte 2

###### `MidiMessage(const MidiMessage& other)`
- **Description:** Copy constructor

###### `MidiMessage& operator=(const MidiMessage& other)`
- **Description:** Copy assignment operator

##### Public Member Functions:

###### `void setChannel(ubyte channel)`
- **Description:** Set channel bits in status byte
- **Parameters:**
  - `channel` - MIDI channel (0-15)
- **Actions:** Sets lower 4 bits of mData[0]

###### `void setTimestamp(double ts)`
- **Description:** Set timestamp
- **Parameters:**
  - `ts` - Timestamp in seconds

###### `bool isNoteOn() const`
- **Description:** Check if message is note-on
- **Returns:** True if status byte upper nibble is 0x9 and velocity > 0

###### `bool isNoteOff() const`
- **Description:** Check if message is note-off
- **Returns:** True if status byte upper nibble is 0x8 or (0x9 with velocity 0)

###### `std::string getString()`
- **Description:** Debug string representation
- **Returns:** String like "NoteOn Ch:1 Pitch:60 Vel:64"

###### `MidiEvent getEventType() const`
- **Description:** Extract event type from status byte
- **Returns:** MidiEvent enum value (upper nibble of mData[0])

###### `ubyte getChannel() const`
- **Description:** Extract channel from status byte
- **Returns:** Channel number (0-15, lower nibble of mData[0])

###### `ubyte getPitch() const`
- **Description:** Get note pitch
- **Returns:** mData[1] (note number 0-127)

##### Static Factory Methods:

###### `static MidiMessage NoteOn(ubyte keyNumber, ubyte velocity, ubyte channel = 0)`
- **Description:** Create note-on message
- **Parameters:**
  - `keyNumber` - MIDI note (0-127)
  - `velocity` - Velocity (1-127, 0 = note-off)
  - `channel` - MIDI channel (0-15)
- **Returns:** MidiMessage

###### `static MidiMessage NoteOff(ubyte keyNumber, ubyte channel = 0)`
- **Description:** Create note-off message
- **Parameters:**
  - `keyNumber` - MIDI note (0-127)
  - `channel` - MIDI channel (0-15)
- **Returns:** MidiMessage with velocity 0

###### `static MidiMessage NoteOff(const MidiMessage& other)`
- **Description:** Convert note-on to note-off
- **Parameters:**
  - `other` - Note-on message
- **Returns:** Note-off with same pitch and channel

###### `static MidiMessage ProgramChange(SoundSet patch, ubyte channel = 0)`
- **Description:** Create program change message from SoundSet enum
- **Parameters:**
  - `patch` - SoundSet enum value
  - `channel` - MIDI channel (0-15)
- **Returns:** MidiMessage

###### `static MidiMessage ProgramChange(ubyte patchNumber, ubyte channel = 0)`
- **Description:** Create program change message from patch number
- **Parameters:**
  - `patchNumber` - Patch number (0-127)
  - `channel` - MIDI channel (0-15)
- **Returns:** MidiMessage

###### `static MidiMessage ControlChange(ubyte ctrlNum, ubyte ctrlVal, ubyte channel = 0)`
- **Description:** Create control change message
- **Parameters:**
  - `ctrlNum` - Controller number (0-127)
  - `ctrlVal` - Controller value (0-127)
  - `channel` - MIDI channel (0-15)
- **Returns:** MidiMessage

###### `static MidiMessage AllNotesOff(ubyte channel = 0)`
- **Description:** Create all-notes-off message (CC 123)
- **Parameters:**
  - `channel` - MIDI channel (0-15)
- **Returns:** MidiMessage

###### `static const std::string& getSoundName(ubyte ss)`
- **Description:** Get instrument name from patch number
- **Parameters:**
  - `ss` - Patch number (0-127)
- **Returns:** Instrument name string (e.g., "Acoustic Grand Piano")

---

### **src/RtMidiWrapper/MidiMessage/SoundMaps.h**

General MIDI instrument and percussion definitions.

#### **enum SoundSet** (in MidiInterface namespace)

**Purpose:** 128 General MIDI instrument patches.

##### Categories and Values:

**Piano (0-7):**
- `ACOUSTIC_GRAND_PIANO = 0`
- `BRIGHT_ACOUSTIC_PIANO = 1`
- `ELECTRIC_GRAND_PIANO = 2`
- `HONKY_TONK_PIANO = 3`
- `ELECTRIC_PIANO_1 = 4`
- `ELECTRIC_PIANO_2 = 5`
- `HARPSICHORD = 6`
- `CLAVINET = 7`

**Chromatic Percussion (8-15):**
- `CELESTA = 8`
- `GLOCKENSPIEL = 9`
- `MUSIC_BOX = 10`
- `VIBRAPHONE = 11`
- `MARIMBA = 12`
- `XYLOPHONE = 13`
- `TUBULAR_BELLS = 14`
- `DULCIMER = 15`

**Organ (16-23):**
- `DRAWBAR_ORGAN = 16`
- `PERCUSSIVE_ORGAN = 17`
- `ROCK_ORGAN = 18`
- `CHURCH_ORGAN = 19`
- `REED_ORGAN = 20`
- `ACCORDION = 21`
- `HARMONICA = 22`
- `TANGO_ACCORDION = 23`

**Guitar (24-31):**
- `ACOUSTIC_GUITAR_NYLON = 24`
- `ACOUSTIC_GUITAR_STEEL = 25`
- `ELECTRIC_GUITAR_JAZZ = 26`
- `ELECTRIC_GUITAR_CLEAN = 27`
- `ELECTRIC_GUITAR_MUTED = 28`
- `OVERDRIVEN_GUITAR = 29`
- `DISTORTION_GUITAR = 30`
- `GUITAR_HARMONICS = 31`

**Bass (32-39):**
- `ACOUSTIC_BASS = 32`
- `ELECTRIC_BASS_FINGER = 33`
- `ELECTRIC_BASS_PICK = 34`
- `FRETLESS_BASS = 35`
- `SLAP_BASS_1 = 36`
- `SLAP_BASS_2 = 37`
- `SYNTH_BASS_1 = 38`
- `SYNTH_BASS_2 = 39`

**Strings (40-47):**
- `VIOLIN = 40`
- `VIOLA = 41`
- `CELLO = 42`
- `CONTRABASS = 43`
- `TREMOLO_STRINGS = 44`
- `PIZZICATO_STRINGS = 45`
- `ORCHESTRAL_HARP = 46`
- `TIMPANI = 47`

**Ensemble (48-55):**
- `STRING_ENSEMBLE_1 = 48`
- `STRING_ENSEMBLE_2 = 49`
- `SYNTH_STRINGS_1 = 50`
- `SYNTH_STRINGS_2 = 51`
- `CHOIR_AAHS = 52`
- `VOICE_OOHS = 53`
- `SYNTH_VOICE = 54`
- `ORCHESTRA_HIT = 55`

**Brass (56-63):**
- `TRUMPET = 56`
- `TROMBONE = 57`
- `TUBA = 58`
- `MUTED_TRUMPET = 59`
- `FRENCH_HORN = 60`
- `BRASS_SECTION = 61`
- `SYNTH_BRASS_1 = 62`
- `SYNTH_BRASS_2 = 63`

**Reed (64-71):**
- `SOPRANO_SAX = 64`
- `ALTO_SAX = 65`
- `TENOR_SAX = 66`
- `BARITONE_SAX = 67`
- `OBOE = 68`
- `ENGLISH_HORN = 69`
- `BASSOON = 70`
- `CLARINET = 71`

**Pipe (72-79):**
- `PICCOLO = 72`
- `FLUTE = 73`
- `RECORDER = 74`
- `PAN_FLUTE = 75`
- `BLOWN_BOTTLE = 76`
- `SHAKUHACHI = 77`
- `WHISTLE = 78`
- `OCARINA = 79`

**Synth Lead (80-87):**
- `LEAD_1_SQUARE = 80`
- `LEAD_2_SAWTOOTH = 81`
- `LEAD_3_CALLIOPE = 82`
- `LEAD_4_CHIFF = 83`
- `LEAD_5_CHARANG = 84`
- `LEAD_6_VOICE = 85`
- `LEAD_7_FIFTHS = 86`
- `LEAD_8_BASS_LEAD = 87`

**Synth Pad (88-95):**
- `PAD_1_NEW_AGE = 88`
- `PAD_2_WARM = 89`
- `PAD_3_POLYSYNTH = 90`
- `PAD_4_CHOIR = 91`
- `PAD_5_BOWED = 92`
- `PAD_6_METALLIC = 93`
- `PAD_7_HALO = 94`
- `PAD_8_SWEEP = 95`

**Synth Effects (96-103):**
- `FX_1_RAIN = 96`
- `FX_2_SOUNDTRACK = 97`
- `FX_3_CRYSTAL = 98`
- `FX_4_ATMOSPHERE = 99`
- `FX_5_BRIGHTNESS = 100`
- `FX_6_GOBLINS = 101`
- `FX_7_ECHOES = 102`
- `FX_8_SCI_FI = 103`

**Ethnic (104-111):**
- `SITAR = 104`
- `BANJO = 105`
- `SHAMISEN = 106`
- `KOTO = 107`
- `KALIMBA = 108`
- `BAG_PIPE = 109`
- `FIDDLE = 110`
- `SHANAI = 111`

**Percussive (112-119):**
- `TINKLE_BELL = 112`
- `AGOGO = 113`
- `STEEL_DRUMS = 114`
- `WOODBLOCK = 115`
- `TAIKO_DRUM = 116`
- `MELODIC_TOM = 117`
- `SYNTH_DRUM = 118`
- `REVERSE_CYMBAL = 119`

**Sound Effects (120-127):**
- `GUITAR_FRET_NOISE = 120`
- `BREATH_NOISE = 121`
- `SEASHORE = 122`
- `BIRD_TWEET = 123`
- `TELEPHONE_RING = 124`
- `HELICOPTER = 125`
- `APPLAUSE = 126`
- `GUNSHOT = 127`

---

#### **enum PercussionMap** (in MidiInterface namespace)

**Purpose:** General MIDI drum sounds on channel 10 (percussion channel).

##### Values (35-81):

- `ACOUSTIC_BASS_DRUM = 35`
- `BASS_DRUM_1 = 36`
- `SIDE_STICK = 37`
- `ACOUSTIC_SNARE = 38`
- `HAND_CLAP = 39`
- `ELECTRIC_SNARE = 40`
- `LOW_FLOOR_TOM = 41`
- `CLOSED_HI_HAT = 42`
- `HIGH_FLOOR_TOM = 43`
- `PEDAL_HI_HAT = 44`
- `LOW_TOM = 45`
- `OPEN_HI_HAT = 46`
- `LOW_MID_TOM = 47`
- `HI_MID_TOM = 48`
- `CRASH_CYMBAL_1 = 49`
- `HIGH_TOM = 50`
- `RIDE_CYMBAL_1 = 51`
- `CHINESE_CYMBAL = 52`
- `RIDE_BELL = 53`
- `TAMBOURINE = 54`
- `SPLASH_CYMBAL = 55`
- `COWBELL = 56`
- `CRASH_CYMBAL_2 = 57`
- `VIBRASLAP = 58`
- `RIDE_CYMBAL_2 = 59`
- `HI_BONGO = 60`
- `LOW_BONGO = 61`
- `MUTE_HI_CONGA = 62`
- `OPEN_HI_CONGA = 63`
- `LOW_CONGA = 64`
- `HIGH_TIMBALE = 65`
- `LOW_TIMBALE = 66`
- `HIGH_AGOGO = 67`
- `LOW_AGOGO = 68`
- `CABASA = 69`
- `MARACAS = 70`
- `SHORT_WHISTLE = 71`
- `LONG_WHISTLE = 72`
- `SHORT_GUIRO = 73`
- `LONG_GUIRO = 74`
- `CLAVES = 75`
- `HI_WOOD_BLOCK = 76`
- `LOW_WOOD_BLOCK = 77`
- `MUTE_CUICA = 78`
- `OPEN_CUICA = 79`
- `MUTE_TRIANGLE = 80`
- `OPEN_TRIANGLE = 81`

---

#### Constants:

###### `const std::string SoundNames[128]`
- **Purpose:** Array of instrument name strings
- **Usage:** `MidiMessage::getSoundName(patchNumber)` returns `SoundNames[patchNumber]`

---

### **src/RtMidiWrapper/MidiDevice/MidiIn.h**

MIDI input device wrapper.

#### **struct Range** (in MidiInterface namespace)

**Purpose:** MIDI note range filter (currently unused).

##### Members:

- `ubyte low` - Low note (e.g., 36 = C2)
- `ubyte high` - High note (e.g., 96 = C7)

---

#### **class MidiIn** (in MidiInterface namespace)

**Purpose:** Wrapper for RtMidiIn with port management.

##### Public Member Functions:

###### `MidiIn()`
- **Description:** Constructor - open first available MIDI input port
- **Actions:**
  - Creates RtMidiIn object
  - Sets error callback
  - Gets port count and names
  - Opens port 0 if available

###### `~MidiIn()`
- **Description:** Destructor - close port and cleanup
- **Actions:**
  - Closes MIDI port
  - Deletes RtMidiIn object

###### `ubyte getCurrentPort()`
- **Description:** Get current port index
- **Returns:** Port number (0-based)

###### `void changePort(ubyte p)`
- **Description:** Close and reopen different port
- **Parameters:**
  - `p` - Port index to switch to
- **Actions:**
  - Closes current port
  - Opens new port
  - Updates `mPortNum`

###### `unsigned int getNumPorts() const`
- **Description:** Get number of available input ports
- **Returns:** Port count

###### `const std::vector<std::string>& getPortNames() const`
- **Description:** Get list of port names
- **Returns:** Reference to port name vector

###### `bool checkForMessage()`
- **Description:** Poll for incoming MIDI message
- **Returns:** True if message received
- **Actions:**
  - Calls `RtMidiIn::getMessage()`
  - Converts to MidiMessage
  - Stores in `mMessage`

###### `MidiMessage& getMessage()`
- **Description:** Get last received message
- **Returns:** Reference to MidiMessage
- **Usage:** Call after `checkForMessage()` returns true

###### `void setMidiInCallback(void (*callback)(double, std::vector<unsigned char>*, void*))`
- **Description:** Set callback mode (unused in MidiWorks)
- **Parameters:**
  - `callback` - Callback function
- **Actions:**
  - Sets RtMidiIn callback

###### `void cancelCallback()`
- **Description:** Cancel callback mode
- **Actions:**
  - Calls `RtMidiIn::cancelCallback()`

##### Private Member Functions:

###### `void fillPortNames()`
- **Description:** Populate port name list
- **Actions:**
  - Iterates over ports
  - Calls `RtMidiIn::getPortName()`
  - Stores in `mPortNames` vector

##### Private Member Variables:

- `ubyte mPortNum` - Current port index (default 0)
- `RtMidiIn* mInstrument` - RtMidi object
- `ubyte mNumPorts` - Number of available ports
- `Range mRange` - Note range filter (unused, default 36-96)
- `std::vector<std::string> mPortNames` - Port name list
- `MidiMessage mMessage` - Last received message

---

### **src/RtMidiWrapper/MidiDevice/MidiOut.h**

MIDI output device wrapper.

#### **class MidiOut** (in MidiInterface namespace)

**Purpose:** Wrapper for RtMidiOut with port management.

##### Public Member Functions:

###### `MidiOut()`
- **Description:** Constructor - open first available MIDI output port (or virtual port)
- **Actions:**
  - Creates RtMidiOut object
  - Sets error callback
  - Gets port count and names
  - Opens port 0 if available
  - If no ports, opens virtual port "MidiWorks Output"

###### `~MidiOut()`
- **Description:** Destructor - close port and cleanup
- **Actions:**
  - Closes MIDI port
  - Deletes RtMidiOut object

###### `ubyte getCurrentPort()`
- **Description:** Get current port index
- **Returns:** Port number (0-based)

###### `void changePort(ubyte p)`
- **Description:** Close and reopen different port
- **Parameters:**
  - `p` - Port index to switch to
- **Actions:**
  - Closes current port
  - Opens new port
  - Updates `mPortNum`

###### `void sendMessage(MidiMessage mm)`
- **Description:** Send MIDI message
- **Parameters:**
  - `mm` - MidiMessage to send
- **Actions:**
  - Converts MidiMessage to std::vector<unsigned char>
  - Calls `RtMidiOut::sendMessage()`

###### `unsigned int getNumPorts() const`
- **Description:** Get number of available output ports
- **Returns:** Port count

###### `const std::vector<std::string>& getPortNames() const`
- **Description:** Get list of port names
- **Returns:** Reference to port name vector

##### Private Member Functions:

###### `void fillPortNames()`
- **Description:** Populate port name list
- **Actions:**
  - Iterates over ports
  - Calls `RtMidiOut::getPortName()`
  - Stores in `mPortNames` vector

##### Private Member Variables:

- `ubyte mPortNum` - Current port index (default 0)
- `RtMidiOut* mPlayer` - RtMidi object
- `ubyte mNumPorts` - Number of available ports
- `std::vector<std::string> mPortNames` - Port name list

---

### **src/RtMidiWrapper/MidiDevice/MidiError.h**

RtMidi error handling callback.

#### Standalone Functions (in MidiInterface namespace):

###### `static void midiErrorCallback(RtMidiError::Type type, const std::string& errorText, void* userData)`
- **Description:** Error handler for RtMidi
- **Parameters:**
  - `type` - Error type enum
  - `errorText` - Error description
  - `userData` - User data pointer (unused)
- **Actions:**
  - Prints error to stderr if type is not WARNING
  - Exits application on DRIVER_ERROR, SYSTEM_ERROR, THREAD_ERROR

---

### **src/RtMidiWrapper/MidiDevice/MidiInCallback.h**

Legacy MIDI input callback (unused in MidiWorks).

#### Global Variables:

- `static std::vector<MidiInterface::MidiMessage> MidiInQueue` - Unused global queue

#### Standalone Functions:

###### `static void midiInCallback(double deltaTime, std::vector<unsigned char>* message, void* userData)`
- **Description:** Debug print MIDI input (unused)
- **Parameters:**
  - `deltaTime` - Time since last message
  - `message` - Raw MIDI bytes
  - `userData` - User data pointer (unused)
- **Actions:**
  - Prints MIDI bytes to stdout
- **Note:** Not used in MidiWorks (polling mode used instead)

---

## Statistics

### File Summary

**Total Files:** 40+
- Header files (.h): 33
- Implementation files (.cpp): 6
- Third-party (RtMidi): 2 files

### Component Summary

**Total Classes:** 30+
- Application: 1 (App)
- AppModel: 4 (AppModel, Transport, SoundBank, TrackSet)
- MainFrame: 1 (MainFrame)
- Panels: 11 (TransportPanel, SoundBankPanel, ChannelControlsPanel, ChannelTrackRow, MidiSettingsPanel, MidiCanvasPanel, LogPanel, UndoHistoryPanel, ShortcutsPanel, DummyPanel, + 3 empty placeholders)
- Commands: 8 (Command, AddNoteCommand, DeleteNoteCommand, MoveNoteCommand, ResizeNoteCommand, RecordCommand, QuantizeCommand, PasteCommand, DeleteMultipleNotesCommand)
- RtMidiWrapper: 3 (MidiMessage, MidiIn, MidiOut)

**Key Structs:** 15+
- TimedMidiEvent, MidiChannel, Transport::BeatInfo, AppModel::ClipboardNote, AppModel::ActiveNote, PanelInfo, MidiCanvasPanel::NoteInfo, PasteCommand::PastedNoteInfo, DeleteMultipleNotesCommand::NoteToDelete, Range

**Key Enums:** 7
- Transport::State (9 states)
- PanePosition (6 positions)
- MidiCanvasPanel::MouseMode (6 modes)
- MidiEvent (7 event types)
- ChannelMessage (4 messages)
- SoundSet (128 GM instruments)
- PercussionMap (47 drum sounds)

### MIDI Constants

- **Channels:** 15 user channels (0-14) + 1 metronome channel (15)
- **MIDI Notes:** 128 (0-127)
- **GM Instruments:** 128 (0-127)
- **Timing:** 960 ticks per quarter note (PPQN)
- **Default Tempo:** 120 BPM
- **Default Time Signature:** 4/4
- **Update Rate:** 10ms (100 Hz)
- **Undo Stack Size:** 50 actions

### Memory Management

- **wxWidgets:** Parent-child ownership (automatic cleanup)
- **AppModel:** `std::shared_ptr` for shared ownership
- **UI Components:** Raw pointers (wxWidgets manages lifetime)
- **Commands:** `std::unique_ptr` for exclusive ownership in stacks
- **MIDI Devices:** `std::shared_ptr` for shared ownership

---

## Conclusion

This document provides a comprehensive reference for the MidiWorks codebase architecture and every function. The architecture follows a clean Model-View separation with a robust undo/redo system based on the Command pattern. The 10ms update loop drives all application logic, MIDI I/O, and UI updates.

Key architectural strengths:
- **Clear separation of concerns** between AppModel (data) and Panels (UI)
- **Efficient playback** with iterator-based track scanning
- **Full undo/redo** for all editing operations
- **Flexible panel system** with dockable, resizable windows
- **Project persistence** with JSON save/load
- **Loop recording** with overlap detection

The codebase is well-organized by layer (App, Model, View, Commands, MIDI) and uses modern C++20 features while maintaining compatibility with the wxWidgets framework.
