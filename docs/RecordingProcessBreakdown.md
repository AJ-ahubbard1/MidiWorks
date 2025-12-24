# Recording Process Breakdown

Complete trace of the recording process in MidiWorks, from user clicking REC to clicking STOP.

---

## COMPLETE RECORDING PROCESS TRACE

### PHASE 1: RECORDING INITIATION

**Step 1: User Clicks REC Button** (TransportPanel.h:155)
```
User clicks → TransportPanel::OnRecord()
            → mTransport.SetState(Transport::State::ClickedRecord)
```

**Step 2: Timer Detects State Change** (MainFrame.cpp:162-168)
```
Every 1ms: wxTimer fires
         → MainFrame::OnTimer()
         → mAppModel->Update()
```

**Step 3: State Machine Routes to Handler** (AppModel.cpp:36-49)
```
AppModel::Update() checks state
                 → case ClickedRecord
                 → HandleClickedRecord()
```

**Step 4: Setup Recording** (AppModel.cpp:297-304)
```cpp
HandleClickedRecord():
  1. GetDeltaTimeMs()  // Reset timer to prevent huge jump
  2. mTrackSet.FindStart(mTransport.StartPlayBack())  // Init playback
  3. mTransport.SetState(Transport::State::Recording)  ★ NOW RECORDING ★
```

**Note:** No loop playback initialization needed here - the buffer is always empty when recording starts 
(thanks to unconditional `Clear()` on stop). Loop playback iterator is initialized by `ResetLoopPlayback()`
when the loop wraps.

---

### PHASE 2: RECORDING LOOP (Every 1ms)

#### 2A: Handle Playback & Timing
```
AppModel::Update() → HandleRecording()
                   → HandlePlaybackCore(isRecording=true)
```

**HandlePlaybackCore does:**
1. Update transport time (lastTick → currentTick)
2. **Check loop wrap:**
   ```cpp
   IF currentTick >= loopEndTick:
     • CloseAllActiveNotes(loopEndTick)  // Prevent stuck notes
     • SeparateOverlappingNotes()        // Fix note overlaps
     • ShiftToTick(loopStartTick)        // Jump back
     • ResetLoopPlayback(loopStartTick)  // Reset iterator
   ```
3. Trigger metronome clicks if beat occurred
4. Get playback messages from TrackSet
5. **IF loop recording:** Add RecordingSession playback messages (hear previous iterations)
6. PlayMessages() → output to MIDI device

#### 2B: Capture Incoming MIDI
```
AppModel::Update() → HandleIncomingMidi()
                   → MidiInputManager.PollAndNotify(currentTick)
```

**MidiInputManager.PollAndNotify** (MidiInputManager.cpp:34-46):
```cpp
1. Check if mMidiIn has message
2. IF message exists:
   - Get MidiMessage from hardware
   - Call mLogCallback (for UI logging)
   - Return message
```

#### 2C: Route & Record Message
```
AppModel::HandleIncomingMidi()
        → RouteAndPlayMessage(msg, currentTick)
```

**RouteAndPlayMessage** (AppModel.cpp:233-251):
```cpp
FOR each channel in SoundBank:
  IF channel should play (not muted/soloed):
    1. Route message to channel (velocity scaling, etc.)
    2. Send to MIDI output (for monitoring - hear what you play)
    3. ★ IF channel.record && IsMusicalMessage && IsRecording():
         RecordingSession.RecordEvent(msg, currentTick)  // RECORD IT!
```

#### 2D: Store in Recording Buffer

**RecordingSession.RecordEvent** (RecordingSession.cpp:36-54):
```cpp
RecordEvent(msg, currentTick):
  1. mBuffer.push_back({msg, currentTick})  // Add to temporary buffer

  2. Track active notes (for loop recording):
     IF NoteOn (status 0x90, velocity > 0):
       mActiveNotes.push_back({pitch, channel, currentTick})

     IF NoteOff (status 0x80 or NoteOn velocity=0):
       Remove from mActiveNotes
```

**Data Structure:**
```cpp
RecordingSession {
  Track mBuffer;  // vector<TimedMidiEvent> - ALL recorded events
  vector<ActiveNote> mActiveNotes;  // Currently held notes
  int mBufferIterator;  // For loop playback
}
```

#### 2E: Visual Update
```
MidiCanvasPanel references mRecordingBuffer
              → Shows "ghost notes" (yellow/preview) in real-time
              → Updates every 1ms
```

---

### PHASE 3: LOOP RECORDING DETAILS

**When Loop Wraps** (currentTick >= loopEndTick):

**RecordingSession.CloseAllActiveNotes** (RecordingSession.cpp:68-81):
```cpp
FOR each note in mActiveNotes:
  Create NoteOff event at loopEndTick
  Add to mBuffer
  Remove from mActiveNotes
// Prevents stuck notes when jumping back to loop start
```

**TrackSet::SeparateOverlappingNotes** (TrackSet.cpp:47-102):
```cpp
// Problem: Same pitch NoteOn NoteOn NoteOff NoteOff
// Solution: Insert early NoteOff before second NoteOn
FOR each consecutive same-pitch NoteOn:
  Find its NoteOff
  IF another NoteOn exists between them:
    Create early NoteOff 1 tick before second NoteOn
    Shift original NoteOff timing
// Prevents note merging artifacts
```

**Loop Playback** (RecordingSession.cpp:119-138):
```cpp
GetLoopPlaybackMessages(currentTick):
  // Returns all events at/before currentTick from mBuffer
  // Advances mBufferIterator
  // Allows hearing previous loop iterations while recording new ones
```

---

### PHASE 4: STOPPING RECORDING

**Step 1: User Clicks Stop/Record Again**
```
TransportPanel::OnStop() OR OnRecord()
              → mTransport.SetState(Transport::State::StopRecording)
```

**Step 2: Finalization Handler** (AppModel.cpp:255-271)
```cpp
HandleStopRecording():
  1. mTransport.Stop()  // Reset playback state
  2. mTransport.SetState(Transport::State::Stopped)

  3. IF mRecordingSession.IsEmpty():
       SKIP command creation (nothing to save)
     ELSE:
       Create RecordCommand and execute ★ FINALIZE RECORDING ★

  4. RecordingSession.Clear()  // ALWAYS called for clean state
  5. SilenceAllChannels()
```

**Step 3: Create Undo/Redo Command**
```cpp
RecordCommand* cmd = new RecordCommand(
  &mTrackSet,
  mRecordingSession.GetBuffer()  // COPY of buffer
);
mUndoRedoManager.ExecuteCommand(cmd);
```

**Step 4: Command Execution** (RecordCommand.h:37-55)
```cpp
RecordCommand::Execute():
  FOR each event in mRecordedNotes:
    channel = event.msg.GetChannel()
    mTrackSet->GetTrack(channel).push_back(event)

  // Sort all affected tracks by tick
  FOR each track that was modified:
    std::sort(track.begin(), track.end())
```

**Result:**
- **RecordingSession buffer** → copied into **RecordCommand** (for undo)
- **RecordCommand** → adds events to **TrackSet** (permanent storage)
- **ProjectManager** → marked dirty (unsaved changes)

**Step 5: Clean Up** (Always Executed)
```cpp
mRecordingSession.Clear():
  mBuffer.clear()         // Empty the recording buffer
  mActiveNotes.clear()    // Clear tracked notes
  mBufferIterator = -1    // Reset iterator to known state

mSoundBank.SilenceAllChannels()  // Stop all playing notes
```

**Key Change:** `Clear()` is now **unconditionally** called on every recording stop, even if nothing was recorded. This ensures the `mBufferIterator` is always in a known state (`-1`) for the next recording, eliminating the need for explicit initialization when recording starts.

---

### PHASE 5: VISUAL UPDATE

**MidiCanvasPanel automatically updates:**
- **Before:** Shows ghost notes from `mRecordingBuffer` (yellow preview)
- **After:** Shows permanent notes from `TrackSet` (colored by channel)
- Refresh happens via `MainFrame::OnTimer() → Update()` loop

---

## COMPLETE DATA FLOW DIAGRAM

```
┌─────────────────────┐
│  User clicks REC    │
└──────────┬──────────┘
           ↓
┌──────────────────────────────────────────────┐
│ Transport.SetState(ClickedRecord)            │
│   ↓ [next 1ms tick]                          │
│ HandleClickedRecord()                        │
│   • Reset delta timer                        │
│   • Initialize TrackSet playback iterators   │
│   • SetState(Recording) ★                    │
└──────────┬───────────────────────────────────┘
           ↓
┌────────────────────────────────────────────────────────┐
│ RECORDING LOOP (every 1ms)                             │
│                                                         │
│  ┌──────────────────────────────────────────────────┐  │
│  │ A. HandlePlaybackCore(isRecording=true)          │  │
│  │    • Update transport time                       │  │
│  │    • Check loop wrap → close notes, reset        │  │
│  │    • Trigger metronome                           │  │
│  │    • Play TrackSet + RecordingBuffer (loop)      │  │
│  └──────────────────────────────────────────────────┘  │
│                                                         │
│  ┌──────────────────────────────────────────────────┐  │
│  │ B. HandleIncomingMidi()                          │  │
│  │    ↓                                             │  │
│  │  MidiInputManager.PollAndNotify(currentTick)     │  │
│  │    • Check hardware device                       │  │
│  │    • Log to UI                                   │  │
│  │    • Return message (if any)                     │  │
│  │    ↓                                             │  │
│  │  RouteAndPlayMessage(msg, currentTick)           │  │
│  │    FOR each SoundBank channel:                   │  │
│  │      • Route to channel (velocity scaling)       │  │
│  │      • Send to MIDI output (monitoring)          │  │
│  │      • IF channel.record:                        │  │
│  │        ★ RecordingSession.RecordEvent() ★        │  │
│  │           • Add to mBuffer                       │  │
│  │           • Track active notes                   │  │
│  └──────────────────────────────────────────────────┘  │
│                                                         │
│  ┌──────────────────────────────────────────────────┐  │
│  │ C. MidiCanvasPanel.Update()                      │  │
│  │    • Draw ghost notes from RecordingBuffer       │  │
│  │    • Draw playhead                               │  │
│  └──────────────────────────────────────────────────┘  │
└────────────┬───────────────────────────────────────────┘
             ↓ [User clicks STOP]
┌──────────────────────────────────────────────────────┐
│ Transport.SetState(StopRecording)                    │
│   ↓ [next 1ms tick]                                  │
│ HandleStopRecording()                                │
│                                                       │
│ 1. IF RecordingSession NOT empty:                    │
│      Create RecordCommand (copy of buffer)           │
│      UndoRedoManager.ExecuteCommand(cmd)             │
│        ↓                                             │
│      RecordCommand.Execute()                         │
│        FOR each event:                               │
│          TrackSet.GetTrack(channel).push_back(event) │
│        Sort all tracks by tick                       │
│                                                       │
│ 2. RecordingSession.Clear() ← ALWAYS called          │
│      • mBuffer.clear()                               │
│      • mActiveNotes.clear()                          │
│      • mBufferIterator = -1                          │
│                                                       │
│ 3. SilenceAllChannels()                              │
│                                                       │
│ Result:                                              │
│   • Events in TrackSet (permanent)                   │
│   • RecordCommand holds copy (undo)                  │
│   • Buffer always empty for next recording           │
│   • Iterator always reset to -1                      │
└──────────────────────────────────────────────────────┘
```

---

## KEY COMPONENTS SUMMARY

### Transport
State machine controlling recording/playback states
- States: `Stopped → ClickedRecord → Recording → StopRecording → Stopped`
- Manages playhead position and loop boundaries

### RecordingSession
Temporary recording buffer with loop support
- Holds all recorded MIDI events until stop
- Tracks active notes to prevent stuck notes at loop boundaries
- Provides loop playback iterator to hear previous iterations
- Cleared after finalization

### MidiInputManager
Hardware MIDI device interface
- Polls hardware device for incoming messages
- Notifies logging callback for UI display
- Returns messages with current tick timestamp

### TrackSet
Permanent MIDI event storage
- 16 channels (0-14 musical, 15 metronome)
- Stores TimedMidiEvents sorted by tick
- Provides PlayBack() iterator for playback

### RecordCommand
Undo/redo wrapper
- Makes entire recording take a single undoable operation
- Holds copy of all recorded events
- Execute: adds events to TrackSet
- Undo: removes events from TrackSet

### MidiCanvasPanel
Visual representation
- References mRecordingBuffer for real-time preview
- Shows "ghost" notes during recording
- Updates every 1ms via timer

---

## KEY INSIGHTS

### 1. Two-Phase Storage
- **During recording:** Events in `RecordingSession.mBuffer` (temporary)
- **After stop:** Events in `TrackSet` (permanent)
- Allows atomic undo/redo of entire recording take

### 2. Real-time Monitoring
- Every incoming message is **immediately** sent to MIDI output
- User hears what they play with minimal latency
- Separate from recording logic

### 3. Loop Recording Complexity
- Must close active notes at loop boundary to prevent stuck notes
- Must separate overlapping same-pitch notes to prevent merging
- Must maintain iterator to play back previous iterations
- All while continuing to record new events

### 4. Command Pattern
- `RecordCommand` wraps the entire recording
- Single undo operation removes all recorded events
- Holds complete copy of recording buffer

### 5. Channel-Based Recording
- Each event routed to specific MIDI channel (0-15)
- TrackSet has 16 separate tracks
- Recording respects per-channel record-enable state

---

## ARCHITECTURE BENEFITS

This architecture provides:
- Clean separation between temporary recording state and permanent storage
- Sophisticated loop recording with overdub capability
- Atomic undo/redo for entire recording takes
- Real-time visual feedback during recording
- Prevention of stuck notes and note merging artifacts
- Minimal latency monitoring of incoming MIDI

---

## DESIGN EVOLUTION: Unconditional Clear() Refactoring

### Previous Design (Before Refactoring)
The original implementation had redundant initialization logic:

```cpp
// On recording start:
HandleClickedRecord():
  if (loopEnabled):
    mRecordingSession.InitializeLoopPlayback(currentTick)
    // Would check if buffer empty and set iterator to -1

// On recording stop:
HandleStopRecording():
  if (!mRecordingSession.IsEmpty()):
    CreateRecordCommand()
    ExecuteCommand()
    mRecordingSession.Clear()  // Only called if buffer had data
  // If buffer was empty, Clear() was skipped!
```

**Problem:** This created uncertainty - the iterator might not be reset if an empty recording was stopped.

### Current Design (After Refactoring)
Simplified with unconditional cleanup:

```cpp
// On recording start:
HandleClickedRecord():
  // No initialization needed - buffer is always empty
  // Iterator is always -1 from previous Clear()

// On recording stop:
HandleStopRecording():
  if (!mRecordingSession.IsEmpty()):
    CreateRecordCommand()
    ExecuteCommand()
  mRecordingSession.Clear()  // ALWAYS called - ensures known state
```

### Why This Works

**Loop Playback Initialization:**
- During **first loop iteration**: Iterator is `-1`, `GetLoopPlaybackMessages()` returns empty (correct - nothing to play back yet)
- When **loop wraps**: `ResetLoopPlayback()` positions iterator to first recorded event
- During **subsequent iterations**: Playback works normally

**Benefits:**
1. **Simpler code** - Removed `InitializeLoopPlayback()` method entirely
2. **Guaranteed state** - Iterator is always `-1` when recording starts
3. **Unconditional cleanup** - No special cases for empty recordings
4. **Easier reasoning** - Known state eliminates edge cases
5. **No performance cost** - Calling `clear()` on empty containers is a no-op

**Key Insight:** `InitializeLoopPlayback()` was always a no-op because the buffer was always empty when recording started (thanks to `Clear()`). The real initialization happens in `ResetLoopPlayback()` when the loop wraps, not when recording begins.
