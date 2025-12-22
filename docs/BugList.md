# Bug List

Track bugs and issues discovered during testing of MidiWorks.

## Status Legend
- **Open** - Bug identified, not yet fixed
- **In Progress** - Currently being worked on
- **Fixed** - Bug has been resolved
- **Won't Fix** - Intentional behavior or low priority

---

## Bugs

### #1 - Unable to pan to the right with right mouse drag when playhead is at tick 0
**Status:** Open
**Priority:** Medium
**Found:** 2025-12-17

**Description:**
Cannot pan the canvas to the right using right-click drag when the playhead is at tick 0.

**Steps to Reproduce:**
1. Open MidiWorks
2. Ensure playhead is at tick 0 (stopped state)
3. Right-click and drag to the right on the MidiCanvas

**Expected Behavior:**
Canvas should pan to the right, allowing user to view negative space or position the viewport.

**Actual Behavior:**
Canvas does not pan to the right. Panning to the left works normally.

**Notes:**
May be related to auto-pan logic that resets offset when playhead is at tick 0 (see MidiCanvas.cpp:96-101).

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

### #3 - Loop region not visible when start tick at 0 and end tick at end of 4th measure
**Status:** Open
**Priority:** Medium
**Found:** 2025-12-17

**Description:**
Loop region does not render on the canvas when loop start is at tick 0 and loop end is at the end of the 4th measure. The loop region only becomes visible after dragging the loop boundary edges.

**Steps to Reproduce:**
1. Open MidiWorks
2. Set loop start to tick 0
3. Set loop end to end of 4th measure (tick 15360 for 4/4 time at 960 PPQ)
4. Observe the MidiCanvas

**Expected Behavior:**
Loop region should be visible as a semi-transparent overlay on the canvas.

**Actual Behavior:**
Loop region is not drawn. Only becomes visible after manually dragging loop boundary edges.

**Notes:**
Likely a rendering issue in DrawLoopRegion() (MidiCanvas.cpp:393). May be related to coordinate calculation or loop boundary detection at these specific positions.

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

### #6 - Fast forward and rewind buttons too slow initially
**Status:** Open
**Priority:** Medium
**Found:** 2025-12-17

**Description:**
The fast forward and rewind transport buttons start at a speed that is too slow, making it tedious to navigate through longer compositions.

**Expected Behavior:**
FF/REW buttons should start at a faster initial speed or ramp up more quickly to allow efficient navigation.

**Actual Behavior:**
Initial speed is too slow for practical use.

**Notes:**
See Transport state machine in AppModel::Update() (cases `FastForwarding` and `Rewinding` at AppModel.cpp:192-195). May need to adjust initial speed or acceleration curve.

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

## Fixed Bugs

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
