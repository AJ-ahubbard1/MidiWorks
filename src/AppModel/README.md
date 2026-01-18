# AppModel.h

**Location:** `src/AppModel/AppModel.h`

## Overview

`AppModel` is the central orchestrator of the MidiWorks application. It contains all application state and business logic, serving as the "Model" in the Model-View architecture. This class is updated every 10ms by `MainFrame::OnTimer()` and coordinates MIDI I/O, recording, playback, and transport control.

## Key Responsibilities

1. **Transport State Management** - Handles play/record/stop state machine
2. **MIDI Input Processing** - Routes incoming MIDI to appropriate channels
3. **Recording Management** - Buffers incoming MIDI during recording sessions
4. **Playback Coordination** - Schedules and plays back recorded tracks
5. **Time Management** - Tracks elapsed time for accurate playback timing

## Architecture

```
AppModel (Central Orchestrator)
  │
  ├─→ MidiIn (MIDI input device)
  ├─→ SoundBank (16 MIDI channels + output device)
  ├─→ Transport (playback state machine + timing)
  ├─→ TrackSet (16 tracks with recorded MIDI data)
  └─→ RecordingBuffer (temporary storage during recording)
```

## Public Members

### State
```cpp
std::shared_ptr<MidiIn> mMidiIn;
```
Shared pointer to MIDI input device. Shared to allow UI panels to access the same instance.

```cpp
TimedMidiEvent mLogMessage;
bool mUpdateLog;
```
Used for MIDI event logging. When `CheckMidiInQueue()` receives a message, it updates these fields to notify the LogPanel to display the event.

### Methods

#### `void Update()`
**Called by:** `MainFrame::OnTimer()` every 10ms

The main update loop that drives the entire application. Implements a state machine based on `mTransport.mState`:

| State | Action |
|-------|--------|
| `StopRecording` | Stops transport, finalizes recording buffer into tracks |
| `StopPlaying` | Stops transport, transitions to Stopped |
| `Stopped` | Idle state, no action |
| `ClickedPlay` | Initializes playback (resets delta time, positions track iterators) |
| `Playing` | Updates transport time, retrieves scheduled messages from tracks, plays them |
| `ClickedRecord` | Initializes recording (resets delta time, positions track iterators) |
| `Recording` | Updates transport time, plays back existing tracks while recording new input |
| `FastForwarding` / `Rewinding` | Shifts current time position |

After processing the state machine, `CheckMidiInQueue()` is called to handle incoming MIDI.

#### `void CheckMidiInQueue()`
**Purpose:** Processes incoming MIDI messages and routes them appropriately.

**Flow:**
1. Polls MIDI input device for new messages
2. If message received:
   - Updates `mLogMessage` for logging
   - Iterates through all 16 channels in SoundBank
   - For each channel with record enabled (or solo active):
     - Routes message to that channel (changes MIDI channel number)
     - Sends to MIDI output device (for real-time monitoring)
     - If recording and message is "musical" (notes, CC, pitch bend), adds to recording buffer

**Solo Logic:**
- If any channel has solo enabled, only solo channels will play/record
- Otherwise, channels with record enabled and not muted will play/record

**Musical Messages:**
Messages that get recorded: Note On/Off, Control Change, Aftertouch, Pitch Bend
Messages NOT recorded: Program Change (handled separately as channel state)

#### `SoundBank& GetSoundBank()`
Returns reference to SoundBank (16 MIDI channels + output device).

#### `Transport& GetTransport()`
Returns reference to Transport (state machine + timing).

#### `Track& GetTrack(ubyte c)`
Returns reference to a specific track by channel number (0-15).

## Private Members

### State
```cpp
std::chrono::steady_clock::time_point mLastTick;
```
Timestamp of last update, used to calculate delta time for accurate playback.

```cpp
SoundBank mSoundBank;
Transport mTransport;
TrackSet mTrackSet;
```
Core application state components.

```cpp
Track mRecordingBuffer;
```
Temporary buffer for MIDI messages recorded during a recording session. When recording stops, this buffer is sorted by timestamp and merged into the appropriate track(s) via `TrackSet::FinalizeRecording()`.

### Methods

#### `uint64_t GetDeltaTimeMs()`
Calculates milliseconds elapsed since last call. Used to advance playback position accurately regardless of timer drift.

**Pattern:**
- On first play/record, resets timer to now
- On subsequent calls, returns elapsed ms and updates timer

#### `bool IsMusicalMessage(const MidiMessage& msg)`
Determines if a MIDI message should be recorded to a track.

**Returns true for:**
- Note On/Off
- Control Change
- Aftertouch (channel and polyphonic)
- Pitch Bend

**Returns false for:**
- Program Change (handled as channel state, not recorded events)
- System messages

#### `void PlayMessages(std::vector<MidiMessage> msgs)`
Plays back a collection of MIDI messages retrieved from tracks during playback.

**Solo/Mute Logic:**
- If solo channels exist: only play messages from solo channels
- Otherwise: play messages from non-muted channels

## State Machine Diagram

```
          ┌─────────┐
          │ Stopped │◄─────────────┐
          └────┬────┘              │
               │                   │
    ┌──────────┼──────────┐        │
    │          │          │        │
    ▼          ▼          ▼        │
ClickedPlay  ClickedRecord  Rewind/FastForward
    │          │
    ▼          ▼
  Playing   Recording ────┐
    │          │          │
    ▼          ▼          ▼
StopPlaying  StopRecording
    │          │
    └──────────┴───────────────────┘
```

## Update Loop Timeline

```
Every 1ms:
  MainFrame::OnTimer()
    │
    └─→ AppModel::Update()
        ├─→ Process Transport State Machine
        │   ├─ [Playing] TrackSet::PlayBack() → get scheduled messages
        │   ├─ [Playing] PlayMessages() → send to MIDI out
        │   ├─- [Recording] TrackSet::PlayBack() → get scheduled messages
        │   └─- [Recording] PlayMessages() → send to MIDI out
        │
        └─→ CheckMidiInQueue()
            ├─→ MidiIn::checkForMessage() → poll input device
            ├─→ Route message to enabled channels
            ├─→ Send to MidiOut (real-time monitoring)
            └─→ [If Recording] Add to mRecordingBuffer
```

## Recording Flow

1. User clicks Record button → `mTransport.mState = ClickedRecord`
2. Next Update():
   - Reset delta time
   - Position track iterators at start
   - Transition to `Recording` state
3. While Recording:
   - `Update()` plays back existing tracks
   - `CheckMidiInQueue()` routes input and adds to `mRecordingBuffer`
4. User clicks Stop → `mTransport.mState = StopRecording`
5. Next Update():
   - Call `mTrackSet.FinalizeRecording(mRecordingBuffer)`
   - Sorts buffer by timestamp
   - Merges into appropriate channel tracks
   - Clears buffer
   - Transition to `Stopped` state

## Playback Flow

1. User clicks Play → `mTransport.mState = ClickedPlay`
2. Next Update():
   - Reset delta time
   - Position track iterators at start via `mTrackSet.FindStart()`
   - Transition to `Playing` state
3. While Playing:
   - Calculate delta time since last update
   - Advance transport tick count
   - Retrieve messages scheduled at current tick via `mTrackSet.PlayBack()`
   - Send messages to MIDI output (respecting mute/solo)
4. User clicks Stop → `mTransport.mState = StopPlaying`
5. Next Update():
   - Transition to `Stopped` state

## Design Notes

### Why Timer-Based Instead of Callbacks?
The 10ms timer-based approach makes the update flow predictable and easy to debug. MIDI input polling (vs. callbacks) avoids thread synchronization issues at the cost of slightly higher latency (~10ms worst case).

### Why Recording Buffer?
During recording, incoming MIDI messages arrive asynchronously. The buffer accumulates them until recording stops, then they're sorted by timestamp and merged into tracks. This ensures chronological ordering and simplifies the recording state machine.

### Why IsMusicalMessage()?
Program Change messages are treated as channel state (handled by SoundBank) rather than recordable events. This keeps tracks focused on performance data while channel configuration is managed separately.

## Dependencies

- **Transport.h** - Playback state machine, tempo, tick counting
- **SoundBank.h** - 16 MIDI channels, mute/solo/record state, MIDI output device
- **TrackSet.h** - 16 tracks (vectors of timed MIDI events), playback scheduling
- **MidiIn** (from RtMidiWrapper) - MIDI input device abstraction
- **MidiMessage** (from RtMidiWrapper) - MIDI message structure

## Related Files

- `Transport.h` - State enum and timing calculations
- `SoundBank.h` - Channel management and MIDI output
- `TrackSet.h` - Track storage and playback scheduling
- `MainFrame.cpp` - Creates AppModel and calls Update() on timer
