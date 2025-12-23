# Bug List

Track bugs and issues discovered during testing of MidiWorks.

## Status Legend
- **Open** - Bug identified, not yet fixed
- **In Progress** - Currently being worked on
- **Fixed** - Bug has been resolved
- **Won't Fix** - Intentional behavior or low priority

---

## Bugs

---

### #2 - Closing app very slow
**Status:** Open
**Priority:** Medium
**Found:** 2025-12-17

**Description:**
Application takes an unusually long time to close when exiting.

**Steps to Reproduce:**
1. Open MidiWorks
2. Use the app normally (create/edit notes, playback, etc.)
3. Close the application via File > Exit or window close button

**Expected Behavior:**
Application should close promptly (within 1-2 seconds).

**Actual Behavior:**
Application takes several seconds to close, appearing to hang before finally exiting.

**Notes:**
May be related to cleanup/destruction of resources (MIDI devices, wxAuiManager, panels, or timer cleanup). Check MainFrame destructor and OnClose event handler.

---


### #4 - Remove overlapping notes, move to collision prevention strategy
**Status:** Open
**Priority:** Medium
**Found:** 2025-12-17

**Description:**
Current implementation uses `MergeOverlappingNotes()` to merge overlapping notes during loop recording. This approach is inconsistent and should be replaced with a collision prevention strategy across all note operations.

**Current Issues:**
- Loop recording merges consecutive NoteOns of same pitch/channel
- Mouse-click note addition doesn't check for collisions
- Drag operations don't validate collision-free destination
- Copy-paste operations don't handle colliding notes

**Proposed Solution:**
Implement collision detection helper and prevent note collisions across all operations:
1. Mouse click to add note - check before adding
2. Dragging existing note - validate destination before move
3. Resizing note - check if extended duration would overlap
4. Loop recording - prevent recording over existing notes
5. Copy-paste - replace/overwrite strategy (delete colliding notes, then paste)

**Notes:**
See `AppModel::MergeOverlappingNotes()` at AppModel.cpp:933. This method should be removed once collision prevention is implemented. Most professional DAWs use a "paste overwrites" behavior for copy-paste collisions.

---

### #5 - Add multiple note move feature
**Status:** Open
**Priority:** High
**Found:** 2025-12-17

**Description:**
Currently users can select multiple notes and copy-paste them, but there's no way to move multiple selected notes. This is a fundamental piano roll editing operation.

**Expected Behavior:**
After selecting multiple notes, user should be able to drag them all together to move them to a new position (changing pitch and/or timing).

**Proposed Solution:**
Implement multi-note drag operation similar to single note move, but applying the delta to all selected notes. Should use a command pattern (e.g., `MoveMultipleNotesCommand`) for undo/redo support.

**Notes:**
Should respect collision prevention strategy once bug #4 is implemented.

---

---

### #7 - Add measure navigation to arrow keys
**Status:** Open
**Priority:** Medium
**Found:** 2025-12-17

**Description:**
Need keyboard shortcuts to jump to next/previous measure for faster navigation.

**Proposed Feature:**
- Left arrow: Move playhead to start of previous measure
- Right arrow: Move playhead to start of next measure

**Notes:**
Should be added to keyboard shortcut system and documented in ShortcutsPanel. Measure boundaries are calculated based on time signature and tick position (960 ticks per quarter note, default 4/4 time = 3840 ticks per measure).

---

### #8 - Loop recording active note tracking reliability
**Status:** Open
**Priority:** High
**Found:** 2025-12-17

**Description:**
The loop recording system has complex logic for tracking active notes and auto-closing them at loop boundaries. This complexity introduces several potential failure modes that could result in stuck notes or recording buffer corruption.

**Potential Issues:**
1. **Active note auto-close at loop end** (AppModel.cpp:124-131) - Could create duplicate note-offs if one is already in buffer
2. **Recording buffer iterator** (AppModel.cpp:172-186) - Could go out of bounds if buffer is modified during playback
3. **Mid-loop stop** - If recording stops while notes are still held, are they properly closed?
4. **Race conditions** - Complex state management between `mActiveNotes`, `mRecordingBuffer`, and `mRecordingBufferIterator`

**Steps to Reproduce:**
Hard to reproduce reliably, but likely scenarios:
1. Start loop recording with loop enabled
2. Hold a note across loop boundary
3. Stop recording mid-loop while holding notes
4. Check for stuck notes or corrupted recording buffer

**Expected Behavior:**
- All note-ons should have matching note-offs
- No stuck notes should persist after stopping
- Recording buffer should remain valid and sorted

**Actual Behavior:**
Potential for stuck notes or buffer corruption under edge cases.

**Proposed Solution:**
1. Add defensive checks in loop recording logic
2. Add explicit note cleanup in `StopRecording` state
3. Add assertions to verify `mRecordingBufferIterator` bounds
4. Consider simplifying the active note tracking logic

**Notes:**
See AppModel.cpp:124-154 (auto-close logic) and AppModel.cpp:233-251 (active note tracking in CheckMidiInQueue). This is a critical issue because stuck notes are very noticeable and disruptive to the user experience.

---

### #9 - MIDI input latency and recording timestamp accuracy
**Status:** Open
**Priority:** High
**Found:** 2025-12-17

**Description:**
MIDI input latency is currently uncompensated, causing recorded notes to be timestamped later than when they were actually played. This results in recordings that sound "late" or "sluggish" on playback, even if the performance was accurate.

**Root Causes:**
1. **System latency** - OS audio/MIDI stack introduces delay (typically 10-50ms)
2. **Audio buffer latency** - Larger buffers = more stable but higher latency
3. **Timestamp accuracy** - Notes timestamped with `mTransport.GetCurrentTick()` which reflects current playback time, not actual key press time
4. **Monitoring latency** - Delay between pressing key and hearing sound affects timing perception

**Current Behavior:**
- `CheckMidiInQueue()` timestamps incoming MIDI with current transport tick (AppModel.cpp:211)
- No latency measurement or compensation
- No user control over audio buffer size
- No direct monitoring option

**Impact on User:**
- Recorded notes consistently late relative to metronome/backing tracks
- Frustrating recording experience, especially for rhythm-critical parts
- Users may overcompensate by playing early, creating inconsistent timing

**Potential Solutions:**
1. **Latency measurement** - Loopback test or manual calibration UI
2. **Timestamp compensation** - Subtract measured latency from recorded note timestamps
3. **ASIO support** - Use ASIO API on Windows for lower latency (RtMidi supports this)
4. **Buffer size control** - Let users adjust audio buffer size vs latency tradeoff
5. **Direct monitoring** - Route MIDI input directly to output with minimal buffering
6. **Latency indicator** - Display current system latency in UI so users understand the issue

**Implementation Phases:**

**Phase 1 - Manual Offset (Quick Win, ~1 hour)**
- Add "Recording Offset" setting (milliseconds slider in MidiSettingsPanel)
- Subtract offset from all recorded timestamps in CheckMidiInQueue()
- User calibrates manually: record with metronome, adjust until notes align
- Solves 80% of the problem with minimal complexity
- Store offset in project settings

**Phase 2 - Automatic Measurement (Moderate complexity)**
- Implement loopback latency test: send MIDI note out, loop back in, measure round-trip
- Divide by 2 for input latency estimate
- Add "Calibrate Latency" button in settings
- Requires user to connect MIDI out → MIDI in with cable/virtual port
- Display measured latency in UI
- Auto-populate recording offset with measured value

**Phase 3 - ASIO Support (Professional, high complexity)**
- Add ASIO API support on Windows (RtMidi already supports this)
- Add audio API selection dropdown in settings (Windows MM / ASIO)
- Handle lower-level callback model required by ASIO
- Expose buffer size control for latency/stability tradeoff
- Display current system latency in real-time
- Typical latency reduction: 40ms → 5-10ms

**Technical Considerations:**
- Need to measure round-trip latency (input → processing → output)
- Latency can vary by MIDI device and system load
- May need per-device latency calibration
- Timestamp compensation must account for tempo changes

**Notes:**
This is a complex problem that affects all real-time audio applications. Most professional DAWs provide latency compensation and ASIO support. Research solutions used by Reaper, Ableton, FL Studio. Consider implementing in phases: (1) measurement/display, (2) basic compensation, (3) advanced features.

---

### #10 - Add ability to hide/show soundbank channels
**Status:** Open
**Priority:** Low
**Found:** 2025-12-17

**Description:**
The SoundBankPanel currently displays all 15 channels at once, taking up significant vertical space. Users often only work with a few channels at a time and would benefit from being able to hide unused channels.

**Proposed Feature:**
- Add collapse/expand button or checkbox for each channel
- Option to "hide empty channels" (channels with no recorded notes)
- Save visibility state with project file
- Keyboard shortcut to toggle all channels visibility

**Benefits:**
- Cleaner, less cluttered interface
- Easier to focus on active channels
- Better use of screen real estate
- Faster scrolling through mixer

**Implementation Notes:**
- Could use wxCollapsiblePane for each ChannelControlsPanel
- Or simple Show()/Hide() with layout refresh
- Store visibility state in channel data structure or separate UI state

**Notes:**
See SoundBankPanel.h for current implementation. Consider how this interacts with solo/mute workflow.

---

### #11 - Add custom colors and titles to channels
**Status:** Open
**Priority:** Low
**Found:** 2025-12-17

**Description:**
Channels are currently identified only by number (0-14) and program name. Users working with multiple channels would benefit from custom colors and descriptive names for better organization.

**Proposed Feature:**
- **Custom channel titles** - User-editable text field (e.g., "Bass", "Piano", "Drums")
- **Custom channel colors** - Color picker for each channel
  - Apply color to channel strip in mixer
  - Apply color to notes in piano roll (MidiCanvas)
  - Apply color to track headers/labels

**Benefits:**
- Better visual organization in complex projects
- Easier to identify channels at a glance
- Standard DAW feature (Ableton, FL Studio, Reaper all have this)
- Improves workflow for multi-instrument compositions

**UI Mockup Ideas:**
- Double-click channel number to edit title
- Right-click channel strip → "Set Color"
- Color swatch button on each ChannelControlsPanel

**Implementation Notes:**
- Add `std::string customName` and `wxColour customColor` to MidiChannel struct (SoundBank.h)
- Update ChannelControlsPanel to display custom name if set
- Update MidiCanvas note rendering to use channel color
- Store in project file (SaveProject/LoadProject in AppModel.cpp)
- Default colors: use color wheel (HSV with evenly spaced hues)

**Notes:**
Color-coding notes in piano roll significantly improves readability in dense arrangements. This is a quality-of-life feature that distinguishes professional DAWs.

---

### #12 - Add virtual piano keyboard with visual feedback
**Status:** Open
**Priority:** Medium
**Found:** 2025-12-17

**Description:**
Add an on-screen piano keyboard that shows which notes are currently playing and allows mouse-based note triggering. This provides visual feedback during playback/recording and enables note input without a MIDI keyboard.

**Proposed Features:**
1. **Visual feedback** - Highlight keys that are currently playing
   - Show notes from `mActiveNotes` vector (during recording)
   - Show preview notes from `mIsPreviewingNote`/`mPreviewPitch`
   - Show notes during playback
   - Color-code by channel (integrate with bug #11)

2. **Mouse interaction** - Click keys to trigger sounds
   - Left-click to play note on record-enabled channels
   - Release to stop note
   - Use same velocity as preview notes (`mPreviewVelocity`)

3. **Keyboard shortcuts** - Computer keyboard as piano
   - Map QWERTY keys to piano keys (e.g., A-K = white keys, W-I = black keys)
   - Adjustable octave range

**UI Placement:**
- Vertical piano strip on left side of MidiCanvas (standard DAW layout)
- Or dockable panel for flexibility
- Piano keys should align with piano roll note rows

**Benefits:**
- Visual confirmation of what's playing (especially useful during recording)
- Note input without MIDI keyboard
- Standard DAW feature (FL Studio, Ableton, Reaper all have this)
- Helps beginners understand note layout
- Useful for testing/debugging stuck notes

**Implementation Notes:**
- New panel or widget: `VirtualPianoPanel` or `PianoKeyboard`
- Poll `AppModel::mActiveNotes` to determine which keys to highlight
- Poll `AppModel::mIsPreviewingNote` and `mPreviewPitch` for preview state
- Mouse down → `AppModel::PlayPreviewNote(pitch)`
- Mouse up → `AppModel::StopPreviewNote()`
- Render piano keys (88 keys or configurable range, e.g., C1-C7)
- Update in sync with timer (10ms refresh)

**Technical Considerations:**
- `mActiveNotes` is currently used only during recording - may need to track active notes during playback too
- Need to handle multiple simultaneous notes (polyphony)
- Key highlighting should be channel-aware (different colors per channel)
- Consider performance impact of rendering 88 keys at 100fps

**Notes:**
This is a highly visible, user-facing feature that significantly improves usability. Medium priority because it enhances both input and feedback. See AppModel.h for `mActiveNotes`, `mIsPreviewingNote`, and preview note methods.

---

### #13 - Add ability to copy notes between channels
**Status:** Open
**Priority:** Medium
**Found:** 2025-12-17

**Description:**
Currently, copy-paste operations only work within the same channel. Users need the ability to copy notes from one channel/instrument to another for doubling lines, layering instruments, or rearranging parts.

**Use Cases:**
- Double a bass line on electric bass + synth bass
- Copy a melody to multiple instruments for layering
- Move a part from one instrument to another
- Create harmony by copying and transposing to different channel

**Proposed Solutions:**

**Option 1: Paste to Active Channel**
- Select notes from any channel(s) → Copy
- Select/activate destination channel (click channel in mixer?)
- Paste → notes appear in active channel at paste position
- Pro: Simple, standard DAW behavior
- Con: Need UI concept of "active/selected channel"

**Option 2: "Copy to Channel..." Context Menu**
- Select notes → Right-click → "Copy to Channel..." → Choose from dropdown
- Creates copy in destination channel immediately
- Pro: Explicit, clear to user
- Con: Extra menu interaction

**Option 3: Paste with Channel Override**
- Select notes → Copy
- Paste (Ctrl+V) → pastes to original channels
- "Paste to Channel..." (Ctrl+Shift+V) → dialog to choose destination
- Pro: Preserves current behavior, adds option
- Con: Extra keyboard shortcut to learn

**Implementation Notes:**
- Current clipboard: `std::vector<ClipboardNote>` includes `trackIndex`
- `PasteCommand` currently respects original `trackIndex` from clipboard
- Need to modify `PasteCommand` to accept optional channel override parameter
- Or add new `PasteToChannelCommand`
- Consider multi-channel selection: if selecting notes from channels 0, 1, 2 and pasting to channel 5, should it paste all to 5, or to 5, 6, 7?

**Recommended Approach:**
Option 1 (paste to active channel) + ability to paste to multiple record-enabled channels:
- Add "active channel" concept to UI (highlight active channel in mixer)
- Paste to all record-enabled channels (existing pattern in `AddNoteToRecordChannels()`)
- Matches existing mental model in app

**Notes:**
This is a fundamental DAW workflow feature. See `AppModel::CopyNotesToClipboard()` and `PasteCommand` for current implementation. Consider interaction with bug #5 (multi-note move).

---

---

### #15 - Inconsistent encapsulation in Transport class
**Status:** Open
**Priority:** Medium
**Found:** 2025-12-22

**Description:**
Transport class exposes public member variables (mTempo, mLoopEnabled, mLoopStartTick, mLoopEndTick) while other component classes use proper getters/setters. This breaks encapsulation and is inconsistent with the rest of the codebase architecture.

**Problematic Code:**
- Transport.h:19-26 - Public member variables
- AppModel.cpp:320, 365 - Direct access: `if (mTransport.mLoopEnabled && currentTick >= mTransport.mLoopEndTick - 1)`

**Issues:**
1. **Breaks encapsulation** - Internal state exposed to external modification
2. **Inconsistent with other components** - SoundBank, TrackSet, etc. use getters
3. **No validation** - Direct access bypasses any validation logic
4. **Hard to refactor** - Public members create tight coupling

**Expected Behavior:**
Transport should use getters/setters like other component classes for consistency and proper encapsulation.

**Proposed Solution:**
1. Make member variables private
2. Add getter/setter methods (some already exist like GetLoopStart/GetLoopEnd)
3. Add missing getters: `IsLoopEnabled()`, `GetTempo()`, `SetTempo(double tempo)`
4. Update AppModel to use getters instead of direct access

**Notes:**
Transport already has some getters (GetLoopStart, GetLoopEnd) but is inconsistent. Complete the encapsulation for all public members. This improves maintainability and follows Single Responsibility Principle.

---

### #16 - Magic number sentinel value in PasteNotes
**Status:** Open
**Priority:** Low
**Found:** 2025-12-22

**Description:**
`PasteNotes()` uses UINT64_MAX as a sentinel value to indicate "use current tick", which is unclear and error-prone.

**Problematic Code:**
- AppModel.cpp:223: `void AppModel::PasteNotes(uint64_t pasteTick = UINT64_MAX)`
- AppModel.cpp:227: `if (pasteTick == UINT64_MAX)`

**Issues:**
1. **Unclear intent** - Magic number doesn't convey meaning
2. **Error-prone** - Easy to accidentally pass UINT64_MAX thinking it's a valid tick
3. **Not idiomatic C++** - C++17 provides std::optional for exactly this use case

**Expected Behavior:**
Use std::optional to clearly express "optional parameter with default behavior".

**Proposed Solution:**
```cpp
void AppModel::PasteNotes(std::optional<uint64_t> pasteTick = std::nullopt)
{
    if (!mClipboard.HasData()) return;

    uint64_t tick = pasteTick.value_or(mTransport.GetCurrentTick());
    auto cmd = std::make_unique<PasteCommand>(mTrackSet, mClipboard.GetNotes(), tick);
    mUndoRedoManager.ExecuteCommand(std::move(cmd));
}
```

**Notes:**
This is a minor refactoring but improves code clarity and type safety. Low priority because current implementation works, but should be addressed during next refactoring pass.

---

### #17 - Mixed responsibilities in CheckMidiInQueue
**Status:** Open
**Priority:** Medium
**Found:** 2025-12-22

**Description:**
`CheckMidiInQueue()` handles multiple concerns: MIDI input polling, routing, playback, recording, and note tracking. This violates Single Responsibility Principle and makes the method hard to test and maintain.

**Problematic Code:**
- AppModel.cpp:59-103 - 44-line method handling:
  - MIDI input polling
  - Callback notification
  - Channel iteration and solo logic
  - MIDI routing and playback
  - Recording buffer management
  - Active note tracking (NoteOn/NoteOff)

**Issues:**
1. **Too many responsibilities** - Difficult to understand at a glance
2. **Hard to test** - Can't test recording logic in isolation from routing logic
3. **Poor maintainability** - Changes to recording affect routing and vice versa
4. **Nested complexity** - Deep nesting with multiple conditions

**Expected Behavior:**
Extract recording logic into separate focused method(s).

**Proposed Solution:**
```cpp
void AppModel::CheckMidiInQueue()
{
    if (!mMidiInputManager.GetDevice().checkForMessage()) return;

    MidiMessage mm = mMidiInputManager.GetDevice().getMessage();
    auto currentTick = mTransport.GetCurrentTick();

    NotifyMidiEventCallback(mm, currentTick);
    RouteAndPlayMidiMessage(mm, currentTick);
}

void AppModel::RouteAndPlayMidiMessage(const MidiMessage& mm, uint64_t currentTick)
{
    auto channels = mSoundBank.GetAllChannels();
    for (MidiChannel& c : channels)
    {
        if (mSoundBank.ShouldChannelPlay(c, true))
        {
            MidiMessage routed = mm;
            routed.setChannel(c.channelNumber);
            mSoundBank.GetMidiOutDevice()->sendMessage(routed);

            if (c.record && IsMusicalMessage(routed) && mTransport.GetState() == Transport::State::Recording)
            {
                RecordMidiEvent(routed, currentTick);
            }
        }
    }
}

void AppModel::RecordMidiEvent(const MidiMessage& msg, uint64_t currentTick)
{
    mRecordingSession.AddEvent({msg, currentTick});

    // Track active notes for loop recording
    ubyte status = msg.mData[0] & 0xF0;
    ubyte pitch = msg.getPitch();
    ubyte channel = msg.getChannel();

    if (status == 0x90 && msg.mData[2] > 0)  // NoteOn
    {
        mRecordingSession.StartNote(pitch, channel, currentTick);
    }
    else if (status == 0x80 || (status == 0x90 && msg.mData[2] == 0))  // NoteOff
    {
        mRecordingSession.StopNote(pitch, channel);
    }
}
```

**Notes:**
This refactoring improves readability, testability, and maintainability. Each method has a single, clear purpose. Medium priority - would be good to address during next refactoring session.

---

---

### #19 - Inconsistent documentation across component classes
**Status:** Open
**Priority:** Low
**Found:** 2025-12-22

**Description:**
Component classes have inconsistent documentation standards. UndoRedoManager has excellent comprehensive documentation, while AppModel, Transport, NoteEditor, and other classes have minimal or no class-level documentation.

**Examples:**
- **Good:** UndoRedoManager.h:8-26 - Excellent class documentation with responsibilities, usage example, and method descriptions
- **Missing:** AppModel.h - No class-level documentation explaining responsibilities or usage
- **Missing:** Transport.h - No documentation explaining state machine or responsibilities
- **Partial:** NoteEditor.h - Brief inline comments but no class overview

**Issues:**
1. **Inconsistent standards** - New contributors don't know which style to follow
2. **Harder to understand** - Classes like AppModel are complex but undocumented
3. **Knowledge gaps** - Important design decisions not captured in code

**Expected Behavior:**
All major component classes should have UndoRedoManager-level documentation including:
- Class purpose and responsibilities
- Key design patterns (e.g., Transport's state machine)
- Usage examples where helpful
- Important lifecycle/ownership notes

**Proposed Solution:**
Add comprehensive documentation to all component classes following UndoRedoManager.h as the template:
1. AppModel - Document update loop, component orchestration, MIDI flow
2. Transport - Document state machine, transitions, tick/time conversion
3. NoteEditor - Document command creation pattern, preview system
4. SoundBank - Document channel management, solo/mute logic
5. TrackSet - Document playback scheduling, iterator management
6. RecordingSession - Document loop recording, active note tracking
7. ProjectManager - Document save/load, dirty state tracking
8. MetronomeService - Document beat detection and click generation
9. Clipboard - Document copy format and paste behavior

**Notes:**
This improves maintainability and onboarding. Low priority but should be done incrementally as classes are modified. See CLAUDE.md for existing architecture documentation.

---

### #1 - Unable to pan to the right with right mouse drag when playhead is at tick 0
**Status:** Fixed
**Priority:** Medium
**Found:** 2025-12-17
**Fixed:** 2025-12-22

**Description:**
Canvas could not pan to the right when playhead was at tick 0 due to auto-reset logic that forced the view back to the origin. Additionally, there was no auto-scroll during fast forward/rewind operations.

**Solution:**
Implemented a comprehensive auto-scroll overhaul with professional fixed playhead scrolling:

**1. Removed Problematic Auto-Reset Logic**
- Deleted the forced reset in MidiCanvas.cpp Update() that prevented manual panning at tick 0
- Users now have full manual pan control when stopped

**2. Implemented Fixed Playhead Auto-Scroll**
- Playhead now locks at 20% from left edge during playback/recording/FF/REW
- Content (grid and notes) scrolls smoothly underneath the fixed playhead
- Industry-standard behavior matching FL Studio, Ableton Live, Cubase

**3. Added FF/REW Auto-Scroll**
- Canvas automatically follows playhead during fast forward and rewind
- Smooth scrolling in both directions (forward and backward)
- Works seamlessly with the accelerating shift speed

**4. Enhanced Transport State Queries**
- Added helper methods to Transport: `IsMoving()`, `IsFastForwarding()`, `IsRewinding()`
- Replaced all direct state comparisons with cleaner helper method calls
- Improved code readability and maintainability

**5. Added MAX_SHIFT_SPEED Cap**
- Prevents runaway acceleration during FF/REW
- Caps at 500.0 ms/frame (configurable in Transport.h:80)
- Smooth acceleration from 20.0 to 500.0

**6. Handled Edge Cases**
- Reset button while stopped: View properly repositions to show tick 0 at playhead
- Window resize: Maintains proper offset initialization via OnSize event
- Initial load: Playhead correctly positioned at target from startup

**Key Code Changes:**
```cpp
// Transport.h - New helper methods
bool IsMoving() const;  // Returns true during play/record/FF/REW
bool IsFastForwarding() const;
bool IsRewinding() const;

// MidiCanvas.cpp - Fixed playhead auto-scroll
if (mTransport.IsMoving() || tickChanged)
{
    int targetPlayheadX = canvasWidth * AUTOSCROLL_TARGET_POSITION;  // 20%
    mOriginOffset.x = targetPlayheadX - (currentTick / mTicksPerPixel);
}

// ClampOffset() - Allow positive offsets for fixed playhead
int maxOffsetX = targetPlayheadX;  // Was: 0
```

**Impact:**
- Smooth, professional auto-scroll experience
- FF/REW operations now visually track playhead movement
- Manual panning works correctly in all scenarios
- Cleaner codebase with reusable helper methods

---

### #20 - Loop boundary check uses magic number -1
**Status:** Open
**Priority:** Low
**Found:** 2025-12-22

**Description:**
Loop boundary checks in HandlePlaying and HandleRecording use `mTransport.mLoopEndTick - 1` without explaining why the `-1` is necessary.

**Problematic Code:**
- AppModel.cpp:320: `if (mTransport.mLoopEnabled && currentTick >= mTransport.mLoopEndTick - 1)`
- AppModel.cpp:365: `if (mTransport.mLoopEnabled && currentTick >= mTransport.mLoopEndTick - 1)`

**Issues:**
1. **Unclear intent** - Why subtract 1? Off-by-one correction? Timing compensation?
2. **Duplicated logic** - Same magic number appears twice
3. **Fragile** - If the reason for `-1` is forgotten, future refactoring might break it

**Expected Behavior:**
Either add a clear comment explaining the `-1`, or extract to a named method/constant that expresses intent.

**Proposed Solution:**

**Option 1: Add explanatory comment**
```cpp
// Check slightly before loop end to avoid timing edge cases
// where the loop boundary is crossed between timer ticks
if (mTransport.mLoopEnabled && currentTick >= mTransport.mLoopEndTick - 1)
```

**Option 2: Extract to named method (better)**
```cpp
bool Transport::ShouldLoopBack(uint64_t currentTick) const
{
    // Check one tick before loop end to ensure smooth loop transition
    return mLoopEnabled && currentTick >= mLoopEndTick - 1;
}

// In AppModel:
if (mTransport.ShouldLoopBack(currentTick))
{
    // loop-back logic
}
```

**Notes:**
Low priority style/clarity issue. Option 2 is preferred as it encapsulates the loop logic in Transport where it belongs and makes the intent self-documenting.

---

## Fixed Bugs

### #14 - Static local variables in HandlePlaying/HandleRecording
**Status:** Fixed
**Priority:** High
**Found:** 2025-12-22
**Fixed:** 2025-12-22

**Description:**
`HandlePlaying()` and `HandleRecording()` used static local variables for message buffers, which could cause issues with re-entrancy, testing, and future threading scenarios.

**Solution:**
Removed the `static` keyword from the vector declarations in both methods (AppModel.cpp:313 and AppModel.cpp:358). Changed to regular local variables:
```cpp
std::vector<MidiMessage> messages;  // Was: static std::vector<MidiMessage> messages;
```

The performance impact of creating/destroying the vector on each call (100 times/second) is negligible (<0.0001% CPU time). Modern compilers and memory allocators handle this efficiently, and the code is now safer and more maintainable.

---

### #18 - Type mismatch in Transport DEFAULT_SHIFT_SPEED
**Status:** Fixed
**Priority:** Low
**Found:** 2025-12-22
**Fixed:** 2025-12-22

**Description:**
Transport class declared `DEFAULT_SHIFT_SPEED` and `mShiftAccel` as `const double` but initialized with float literals (`5.0f`, `1.01f`), causing unnecessary implicit type conversions.

**Solution:**
Removed the `f` suffix from double literals on Transport.h:75 and Transport.h:77:
```cpp
const double DEFAULT_SHIFT_SPEED = 5.0;   // Was: 5.0f
double mShiftAccel = 1.01;                 // Was: 1.01f
```

This eliminates type mismatches and makes the code consistent with C++ conventions (double literals don't need the `f` suffix).

---

### #6 - Fast forward and rewind buttons too slow initially
**Status:** Fixed
**Priority:** Medium
**Found:** 2025-12-17
**Fixed:** 2025-12-22

**Description:**
The fast forward and rewind transport buttons started at a speed that was too slow, making it tedious to navigate through longer compositions. Additionally, the shift speed didn't reset when the user released the FF/REW buttons, causing subsequent uses to start at the previously accelerated speed.

**Solution:**
Two fixes were applied:

1. **Increased initial speed** - Changed `DEFAULT_SHIFT_SPEED` from `5.0` to `20.0` in Transport.h:75, doubling the initial FF/REW speed.

2. **Fixed shift speed reset bug** - Added `mTransport.ResetShiftRate()` call in `TransportPanel::StopTransport()` (called on `wxEVT_LEFT_UP` when FF/REW buttons are released). This ensures the shift speed returns to `DEFAULT_SHIFT_SPEED` after each FF/REW operation instead of persisting the accelerated speed.

**Code changes:**
```cpp
// Transport.h:75
const double DEFAULT_SHIFT_SPEED = 20.0;  // Was: 5.0

// TransportPanel.h - StopTransport() method
void StopTransport(wxMouseEvent& event)
{
    mTransport.ResetShiftRate();  // ← Added this line

    switch (mPreviousState)
    {
        // ... restore previous state
    }
    event.Skip();
}
```

Users can now navigate long compositions more efficiently, and the FF/REW behavior is consistent on every use.

---

### #3 - Loop region not visible when start tick at 0 and end tick at end of 4th measure
**Status:** Fixed
**Priority:** Medium
**Found:** 2025-12-17
**Fixed:** 2025-12-22

**Description:**
Loop region did not render on the canvas when loop start was at tick 0 and loop end was at the end of the 4th measure. The loop region only became visible after dragging the loop boundary edges.

**Solution:**
Removed early return logic in `DrawLoopRegion()` (MidiCanvas.cpp:393) that was preventing the loop region from being drawn. The method was exiting prematurely before rendering the loop overlay, which blocked the draw loop from executing under certain boundary conditions.

The fix ensures the loop region is now properly rendered as a semi-transparent overlay whenever loop mode is enabled, regardless of the loop start/end tick positions.

---

### #X - [Fixed Bug Title]
**Status:** Fixed
**Priority:** High/Medium/Low
**Found:** YYYY-MM-DD
**Fixed:** YYYY-MM-DD

**Description:**
What the bug was.

**Solution:**
How it was fixed.

---

## Template (Copy for new bugs)

```markdown
### #N - [Bug Title]
**Status:** Open
**Priority:** High/Medium/Low
**Found:** YYYY-MM-DD

**Description:**


**Steps to Reproduce:**
1.

**Expected Behavior:**


**Actual Behavior:**


**Notes:**

```
