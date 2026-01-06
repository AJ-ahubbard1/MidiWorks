# Bug List

Track bugs and issues discovered during testing of MidiWorks.

## Status Legend
- **Open** - Bug identified, not yet fixed
- **In Progress** - Currently being worked on
- **Fixed** - Bug has been resolved
- **Won't Fix** - Intentional behavior or low priority

---

## Bugs

### #31 - Turn off playhead autoscroll when loop is enabled
**Status:** Open
**Priority:** Medium
**Found:** 2026-01-05

**Description:**
During loop playback, the playhead autoscroll causes the view to constantly jump back to the loop start position. This creates a distracting visual experience as the content repeatedly scrolls forward then jumps back.

**Expected Behavior:**
When loop is enabled, the canvas should remain stationary (no autoscroll) and let the playhead move across the visible loop region. This would provide a stable, predictable view during loop playback.

**Notes:**
The current fixed playhead autoscroll (implemented in bug #1) works well for linear playback but becomes problematic during looping. Consider adding a setting or automatically disabling autoscroll when loop mode is active.

---

### #32 - Need faster way to scroll/move loop region
**Status:** Open
**Priority:** Medium
**Found:** 2026-01-05

**Description:**
Currently, adjusting the loop region over long distances requires dragging the start and end boundaries individually, which is tedious for large compositions. Users need a faster workflow to reposition the entire loop region.

**Proposed Solutions:**
1. **Drag entire loop region** - Click and drag the middle of the loop overlay to move both start and end together
2. **Keyboard shortcuts** - Shift+Arrow keys to move loop region by measures
3. **Numeric input** - Dialog to type exact tick/measure values for loop boundaries
4. **Selection to loop** - Right-click on selection → "Set as loop region"

**Notes:**
Option 1 (drag entire region) would be the most intuitive and matches DAW standards. Options could be combined for best workflow.

---

### #33 - Show record-enabled status when channel is minimized
**Status:** Open
**Priority:** Low
**Found:** 2026-01-05

**Description:**
When a channel is minimized in the mixer (using the +/- button from bug #10), there's no visual indication of whether the channel is record-enabled. Users must expand the channel to check the record checkbox state.

**Expected Behavior:**
Minimized channels should show a visual indicator (red dot, icon, background color change, etc.) when record is enabled, allowing users to see recording routing at a glance without expanding all channels.

**Proposed Solutions:**
1. **Red border/outline** around minimized channel when record enabled
2. **Record icon (●)** next to the +/- button when record enabled
3. **Background color tint** (subtle red) for record-enabled minimized channels
4. **Status text** - Show "[R]" or "REC" next to channel name when minimized

**Notes:**
This complements the minimize feature (bug #10) by maintaining full functionality when channels are collapsed. Low priority but improves workflow efficiency.

---

### #36 - UI doesn't refresh after MIDI import
**Status:** Closed
**Priority:** Medium
**Found:** 2026-01-05
**Fixed:** 2026-01-05

**Description:**
After importing a MIDI file, the UI panels don't update to reflect the imported data. The data is correctly loaded into the model (program changes, tempo, time signature), but the visual displays remain unchanged until the user manually interacts with them.

**Affected UI Elements:**
1. **SoundBank channel controls** - Don't update to show imported instrument/patch names
   - Program numbers are set correctly in the model
   - Patch selector dropdowns still show previous selection
   - User must manually refresh or switch patches to see correct instrument

2. **Transport panel** - Doesn't update to show imported tempo and time signature
   - Tempo is set correctly in Transport model
   - Display still shows previous tempo value (e.g., 120 BPM instead of imported 140 BPM)
   - Time signature display may also be stale

**Root Cause:**
The `ImportMIDI()` method updates the model data but doesn't notify the UI panels to refresh their displays:

```cpp
// ProjectManager.cpp:497-498 - Sets model, no UI refresh
int programNumber = midiEvent[1];
mSoundBank.GetChannel(channel).programNumber = programNumber;

// ProjectManager.cpp:468-471 - Sets model, no UI refresh
Transport::BeatSettings beatSettings;
beatSettings.tempo = tempo;
beatSettings.timeSignatureNumerator = timeSignatureNumerator;
beatSettings.timeSignatureDenominator = timeSignatureDenominator;
mTransport.SetBeatSettings(beatSettings);
```

**Expected Behavior:**
After importing a MIDI file, all UI panels should immediately update to reflect the imported settings:
- Channel controls should show the imported patch/instrument names
- Transport display should show the imported tempo and time signature
- User sees the correct state without manual intervention

**Proposed Solution:**

**Option 1: Update panels at end of import** (Simple)
```cpp
// At end of ImportMIDI() in ProjectManager.cpp
// Notify panels to refresh from model
if (mUpdateUICallback) {
    mUpdateUICallback();  // Trigger MainFrame to update all panels
}
```

**Option 2: Add update callbacks to ProjectManager** (More targeted)
```cpp
// Add callback setters to ProjectManager
void SetSoundBankUpdateCallback(std::function<void()> callback);
void SetTransportUpdateCallback(std::function<void()> callback);

// Call after import
if (mSoundBankUpdateCallback) mSoundBankUpdateCallback();
if (mTransportUpdateCallback) mTransportUpdateCallback();
```

**Option 3: Panels poll model in Update()** (Automatic, but wasteful)
- Have panels check for model changes every frame
- Inefficient but requires no callback infrastructure

**Recommendation:**
Option 1 is simplest - add a single callback that triggers MainFrame to call `Update()` on all panels. This reuses existing infrastructure and ensures all panels stay in sync.

**Solution Implemented:**
No callback was needed. The fix was implemented directly in `MainFrameEventHandlers.cpp:295-304` in the `OnImportMidiFile()` event handler. After `ImportMIDI()` succeeds, the UI panels are updated directly:

```cpp
if (mAppModel->GetProjectManager().ImportMIDI(path))
{
    wxMessageBox("MIDI file imported successfully", "Import Complete", wxOK | wxICON_INFORMATION);

    UpdateTitle();  // Update title to reflect dirty state

    // Show new tempo from midifile
    if (mTransportPanel)
    {
        mTransportPanel->UpdateTempoDisplay();
    }

    // Show patch changes from midifile
    if (mSoundBankPanel)
    {
        mSoundBankPanel->UpdateFromModel();
    }
}
```

This approach is simpler than using callbacks because:
- MainFrame calls `ImportMIDI()` and knows when it succeeds
- MainFrame can directly update its own panels
- No indirection or callback registration needed
- Matches existing pattern used in `OnOpen()` and `OnNew()` handlers

**Files Modified:**
- `src/MainFrame/MainFrameEventHandlers.cpp` - Added panel updates in OnImportMidiFile()

**Related:**
- Bug #34, #35 - MIDI import functionality (data import works, just UI refresh missing)

---

---

---

### #25 - DrumMachine plays even when panel is hidden
**Status:** Won't Fix
**Priority:** N/A
**Found:** 2025-12-28
**Resolved:** 2025-12-29

**Description:**
The DrumMachine pattern plays back during loop playback even when the DrumMachine panel is not visible.

**Resolution:**
This is **intentional behavior** that matches professional DAW standards. Instruments and patterns should play regardless of UI panel visibility - this is the expected workflow in all major DAWs (Ableton, FL Studio, Logic, etc.).

**Proper Solution:**
Use the **mute button** (implemented in bug #24 Option 5) to control drum machine playback. This provides:
- Standard DAW workflow (mute/solo controls audio routing, not UI visibility)
- Ability to edit patterns while muted
- Clear separation between UI state and audio routing
- Familiar behavior for users coming from other DAWs

**Why Panel Visibility Should NOT Control Playback:**
- Users may want to hear drums while working in other panels (piano roll, mixer, etc.)
- Closing a panel to silence it is non-standard and confusing
- CPU usage from drum machine playback is negligible
- Mute button provides explicit, discoverable control

**Related:**
- Bug #24 Option 5 - Mute/Solo buttons (provides the proper playback control mechanism)

---

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

## Fixed Bugs

### #10 - Add ability to hide/show soundbank channels
**Status:** Fixed
**Priority:** Low
**Found:** 2025-12-17
**Fixed:** 2026-01-04

**Description:**
The SoundBankPanel displayed all 15 channels at once, taking up significant vertical space. Users often only work with a few channels at a time and would benefit from being able to minimize unused channels.

**Solution:**
Implemented minimize/expand functionality for each channel control panel:

**1. Added Minimize Button** (ChannelControls.h:37, 51)
- Toggle button ("-" when expanded, "+" when minimized)
- Positioned in top row next to channel label

**2. Minimize State Tracking** (SoundBank.h - MidiChannel struct)
- Added `minimized` boolean field to persist state per channel
- Allows state to be saved/loaded with project files

**3. Show/Hide Controls Logic** (ChannelControls.h:107-146)
- OnMinimizeButton event handler:
  - Hides/shows: patch selector, volume slider, mute/solo/record checkboxes
  - Updates button label
  - Maintains panel width while shrinking height
  - Triggers proper layout recalculation

**4. Layout Recalculation** (ChannelControls.h:133-145)
```cpp
// Set minimum width to prevent horizontal shrinking
SetMinSize(wxSize(currentWidth, -1));

// Trigger layout recalculation
GetSizer()->Layout();
Fit();

// Update parent layout (SoundBankPanel is wxScrolledWindow)
if (GetParent())
{
    GetParent()->Layout();
    GetParent()->FitInside();  // Recalculates virtual size
}
```

**Key Implementation Details:**
- Used `SetMinSize(wxSize(currentWidth, -1))` to constrain width while allowing height to vary
- Called `FitInside()` on parent to properly update scrolled window's virtual size
- Minimized state persists across app sessions when saved in project file

**Files Modified:**
- `src/Panels/ChannelControls.h` - Added minimize button and collapse logic
- `src/AppModel/SoundBank/SoundBank.h` - Added minimized field to MidiChannel

**Benefits:**
- Cleaner, less cluttered interface
- Easier to focus on active channels
- Better use of screen real estate
- Faster scrolling through mixer
- State persists with project files

**Not Implemented:**
- "Hide empty channels" feature - may be added later
- Keyboard shortcut to toggle all - may be added later

---

### #11 - Add custom colors and titles to channels
**Status:** Fixed
**Priority:** Low
**Found:** 2025-12-17
**Fixed:** 2026-01-04

**Description:**
Channels were identified only by number (0-14) and program name. Users working with multiple channels needed custom colors and descriptive names for better organization and visual identification.

**Solution:**
Implemented comprehensive custom color and name system for all channels:

**1. Data Model Updates** (SoundBank.h)
- Added `std::string customName` - Custom channel name (empty = use default "Channel N")
- Added `wxColour customColor` - Custom channel color
- Created `ChannelColors.h` - Defines default colors (15 unique colors from existing TRACK_COLORS)
- Initialized all channels with default colors in `SoundBank` constructor

**2. Color Swatch UI** (ChannelControls.h)
- Added 15x15 pixel color swatch panel next to channel label
- Click color swatch to open `wxColourDialog` for color selection
- Color updates immediately upon selection
- Layout: `[■] Channel Name [+/-]`

**3. Channel Name Editing** (ChannelControls.h)
- Double-click channel label to rename
- Opens `wxTextEntryDialog` with current name
- Updates `mChannel.customName` and refreshes label display
- Empty name restores default "Channel N" format

**4. Display Logic**
- Label displays custom name if set, otherwise "Channel N"
- Color swatch always displays current channel color
- `UpdateFromModel()` refreshes both color and name when loading projects

**5. Project Serialization** (ProjectManager.cpp)
- **SaveProject**: Saves customName, customColor (as RGB), and minimized state
- **LoadProject**: Restores values with backwards compatibility
- **ClearProject**: Resets to defaults (empty name, TRACK_COLORS[i])

**6. MidiCanvas Integration**
- Updated note rendering to use channel custom colors (user reported already implemented)
- Notes in piano roll now display with channel's custom color
- Improves visual identification in multi-track compositions

**Code Implementation:**
```cpp
// MidiChannel struct (SoundBank.h)
struct MidiChannel
{
    // ... existing fields ...
    std::string customName = "";       // Empty = use default "Channel N"
    wxColour customColor;              // Initialized in constructor
};

// Color swatch click handler (ChannelControls.h)
void OnColorSwatchClicked(wxMouseEvent& event)
{
    wxColourDialog dialog(this, &colorData);
    if (dialog.ShowModal() == wxID_OK)
    {
        mChannel.customColor = dialog.GetColourData().GetColour();
        mColorSwatch->SetBackgroundColour(mChannel.customColor);
        mColorSwatch->Refresh();
    }
}

// Label double-click handler (ChannelControls.h)
void OnLabelDoubleClicked(wxMouseEvent& event)
{
    wxTextEntryDialog dialog(this, "Enter channel name:", "Rename Channel", currentName);
    if (dialog.ShowModal() == wxID_OK)
    {
        mChannel.customName = dialog.GetValue().ToStdString();
        mLabel->SetLabel(displayName);
    }
}

// Project serialization (ProjectManager.cpp)
project["channels"].push_back({
    // ... other fields ...
    {"customName", ch.customName},
    {"customColor", {
        {"r", ch.customColor.Red()},
        {"g", ch.customColor.Green()},
        {"b", ch.customColor.Blue()}
    }}
});
```

**Files Modified:**
- `src/AppModel/SoundBank/SoundBank.h` - Added customName and customColor fields
- `src/AppModel/SoundBank/SoundBank.cpp` - Initialize colors with TRACK_COLORS
- `src/AppModel/SoundBank/ChannelColors.h` (new) - Default color definitions
- `src/Panels/ChannelControls.h` - Color swatch UI and rename dialog
- `src/AppModel/ProjectManager/ProjectManager.cpp` - Serialization support

**Benefits:**
- Better visual organization in complex projects
- Easier to identify channels at a glance (both in mixer and piano roll)
- Standard DAW feature now available (matches Ableton, FL Studio, Reaper)
- Improves workflow for multi-instrument compositions
- Custom settings persist across sessions
- Backwards compatible with old project files

**User Experience:**
- **Click color swatch** → Opens color picker → Updates immediately
- **Double-click channel name** → Rename dialog → Updates label
- **Custom colors applied** to both mixer strip and piano roll notes
- **Settings saved** automatically with project files

---

### #30 - Collision detection doesn't account for separate channels
**Status:** Fixed
**Priority:** High
**Found:** 2026-01-03
**Fixed:** 2026-01-04

**Description:**
The collision detection system prevented notes from being added or moved if they overlapped with existing notes in time and pitch, regardless of channel/track. This blocked valid multi-track operations where different instruments should be allowed to play the same pitch simultaneously.

**Problem:**
- `IsRegionCollisionFree()` searched ALL tracks for collisions
- Prevented adding/moving notes even when collision was on a different channel
- Example: Couldn't add C4 on channel 1 if C4 already existed on channel 0
- Affected note add, move, multi-move, and resize previews

**Solution:**
Implemented channel-aware collision detection throughout the system:

**1. Updated TrackSet::FindNotesInRegion** (TrackSet.h/cpp)
- Added optional `trackIndex` parameter (default = -1 for "all tracks")
- When trackIndex specified, only searches that specific track
- Backwards compatible - existing calls without parameter still work

```cpp
std::vector<NoteLocation> FindNotesInRegion(
    uint64_t minTick, uint64_t maxTick,
    ubyte minPitch, ubyte maxPitch,
    int trackIndex = -1) const;  // -1 = search all tracks
```

**2. Updated AppModel::IsRegionCollisionFree** (AppModel.h/cpp)
- Added `int channel` parameter to both overloads
- Passes channel to `FindNotesInRegion` for channel-specific search
- Both single-note and multi-note exclusion variants updated

```cpp
bool IsRegionCollisionFree(uint64_t startTick, uint64_t endTick, ubyte pitch,
                           int channel, const NoteLocation* excludeNote = nullptr) const;
bool IsRegionCollisionFree(uint64_t startTick, uint64_t endTick, ubyte pitch,
                           int channel, const std::vector<NoteLocation>& excludeNotes) const;
```

**3. Updated All Call Sites** (AppModel.cpp)
- **SetNoteMovePreview**: Uses `note.trackIndex` for channel
- **SetMultipleNotesMovePreview**: Uses `note.trackIndex` for each note being moved
- **SetNoteResizePreview**: Uses `note.trackIndex` for channel

**4. Fixed Note Add Preview** (NoteEditor.cpp)
- Checks collision in ALL record-enabled channels
- Since notes are added to all record-enabled tracks, must verify all are collision-free
- Iterates through `GetRecordEnabledChannels()` and checks each

```cpp
auto channels = mSoundBank.GetRecordEnabledChannels();
for (const MidiChannel* channel : channels)
{
    int trackIndex = static_cast<int>(channel->channelNumber);
    auto conflicts = mTrackSet.FindNotesInRegion(snappedTick, endTick, pitch, pitch, trackIndex);
    if (!conflicts.empty())
        return;  // Collision in this channel, reject preview
}
```

**Files Modified:**
- `src/AppModel/TrackSet/TrackSet.h` - Added trackIndex parameter to FindNotesInRegion
- `src/AppModel/TrackSet/TrackSet.cpp` - Implemented channel-specific search
- `src/AppModel/AppModel.h` - Added channel parameter to IsRegionCollisionFree methods
- `src/AppModel/AppModel.cpp` - Updated implementations and all 3 call sites
- `src/AppModel/NoteEditor/NoteEditor.cpp` - Fixed SetNoteAddPreview to check all record channels

**Benefits:**
- ✓ Users can now layer instruments at same pitch/time on different channels
- ✓ Multi-track compositions work as expected (professional DAW behavior)
- ✓ No more false collision rejections from other channels
- ✓ Collision detection still prevents overlaps WITHIN each channel
- ✓ All note operations (add, move, multi-move, resize) now channel-aware

**Impact:**
Removes a major workflow limitation in multi-track editing. Users can now:
- Layer bass + piano at same pitch
- Create harmonies with multiple instruments
- Work with complex multi-instrument arrangements
- Follow standard DAW editing patterns

**Related:**
- Bug #4 (Collision prevention strategy) - Original implementation that didn't consider multi-channel scenario

---

### #27 - App initializes with tick 0 and playhead off left screen edge
**Status:** Fixed
**Priority:** Medium
**Found:** 2026-01-03
**Fixed:** 2026-01-03

**Description:**
When the application first launches, the MidiCanvas initializes with the playhead positioned off the left edge of the screen, making tick 0 invisible to the user.

**Root Cause:**
The MidiCanvas `OnSize` event is called before the canvas's width is properly set. This timing issue causes the initial offset calculation to be incorrect, resulting in the playhead and starting tick (0) being positioned off-screen.

**Solution:**
Set `mOriginOffset.x = INT32_MAX` in `OnSize()` to force `ClampOffset()` to recalculate and snap the offset to the proper maximum value. This ensures tick 0 is positioned at the playhead location (20% from left edge) regardless of when `OnSize` fires during initialization.

**Code Implementation:**
```cpp
// MidiCanvasEventHandlers.cpp - OnSize()
// Force ClampOffset() to reset horizontal position to show tick 0 at playhead
// (handles timing issues where OnSize fires before canvas width is initialized)
mOriginOffset.x = INT32_MAX;

// Center viewport vertically on middle octaves
mOriginOffset.y = (MidiConstants::MAX_MIDI_NOTE * mNoteHeight - canvasHeight) * 0.5;

ClampOffset(); // Apply offset bounds and finalize positioning
```

**Files Modified:**
- `src/Panels/MidiCanvas/MidiCanvasEventHandlers.cpp` - Added INT32_MAX reset in OnSize()

**Benefits:**
- Application now starts with tick 0 and playhead properly visible
- Robust solution that handles timing issues during initialization
- Minimal code change with clear intent

---

### #13 - Add ability to copy notes between channels
**Status:** Fixed
**Priority:** Medium
**Found:** 2025-12-17
**Fixed:** 2026-01-03

**Description:**
Users needed the ability to copy notes from one channel/instrument to another for doubling lines, layering instruments, or rearranging parts. The original copy-paste operations only worked within the same channel.

**Use Cases:**
- Double a bass line on electric bass + synth bass
- Copy a melody to multiple instruments for layering
- Move a part from one instrument to another
- Create harmony by copying and transposing to different channel

**Solution:**
Implemented cross-track pasting using **Ctrl+Shift+V** with the `PasteToTracksCommand`:
- Pastes clipboard notes to **all record-enabled channels** (instead of original channels)
- Each clipboard note is pasted to ALL target tracks for easy layering/doubling
- Uses `SeparateOverlappingNotes` to handle collisions (like loop recording overdub)
- Full undo support with track snapshots

**Workflow:**
1. Select notes from any channel(s) → Copy (Ctrl+C)
2. Enable record on target channel(s) in mixer
3. Paste to record tracks (Ctrl+Shift+V)
4. Notes appear in all record-enabled channels at paste position

**Code Implementation:**
```cpp
// PasteToTracksCommand.h - Cross-track paste command
void Execute() override
{
    // Paste each clipboard note to ALL target tracks
    for (int targetTrack : mTargetTracks)
    {
        for (const auto& clipNote : mClipboardNotes)
        {
            // Create note events using target track's channel
            noteOn.mm = MidiMessage::NoteOn(clipNote.pitch, clipNote.velocity, targetTrack);
            noteOff.mm = MidiMessage::NoteOff(clipNote.pitch, targetTrack);
            track.push_back(noteOn);
            track.push_back(noteOff);
        }
    }

    // Handle overlaps with overdub behavior
    TrackSet::SeparateOverlappingNotes(track);
}
```

**Files Modified:**
- `src/Commands/PasteToTracksCommand.h` (new) - Cross-track paste command
- `src/AppModel/AppModel.h/cpp` - Added `PasteNotesToRecordTracks()` method
- `src/Panels/MidiCanvas/MidiCanvasEventHandlers.cpp` - Added Ctrl+Shift+V handler

**Benefits:**
- Fundamental DAW workflow now available
- Easy instrument doubling and layering
- Consistent with existing record-enabled channel pattern
- Single keyboard shortcut (Ctrl+Shift+V)
- Full undo/redo support

---

### #12 - Add virtual piano keyboard with visual feedback
**Status:** Fixed
**Priority:** Medium
**Found:** 2025-12-17
**Fixed:** 2026-01-03

**Description:**
Added an on-screen piano keyboard that shows which notes are currently active during recording and note editing. This provides visual feedback and helps users understand note layout without requiring external reference.

**Implemented Features:**
1. **Visual piano keyboard** - 15% of canvas width on left side
   - White and black keys with proper rendering
   - Octave labels on C notes (C0, C1, C2, etc.)
   - Keys align with piano roll note rows

2. **Active note highlighting:**
   - **Green** - Preview notes during mouse-add operations
   - **Red-orange** - Recording notes (currently held during recording)

**Code Implementation:**
```cpp
// MidiCanvas.cpp - DrawPianoKeyboard()
void MidiCanvasPanel::DrawPianoKeyboard(wxGraphicsContext* gc)
{
    // First pass: Draw white keys
    for (int pitch = 0; pitch <= MAX_MIDI_NOTE; pitch++)
    {
        if (!isBlackKey)
        {
            gc->DrawRectangle(0, y, keyboardWidth, mNoteHeight);
            // Add octave labels on C notes
            if (noteInOctave == 0)
                gc->DrawText("C" + std::to_string(octave), 2, y + 2);
        }
    }

    // Second pass: Draw black keys (on top)
    // Third pass: Highlight active notes
    if (mAppModel->HasNoteAddPreview())
    {
        // Green highlight for preview note
        gc->SetBrush(wxBrush(wxColour(100, 255, 100, 180)));
        gc->DrawRectangle(0, y, keyboardWidth, mNoteHeight);
    }
}
```

**Files Modified:**
- `src/Panels/MidiCanvas/MidiCanvas.h` - Added `DrawPianoKeyboard()` declaration
- `src/Panels/MidiCanvas/MidiCanvas.cpp` - Implemented piano keyboard rendering

**Benefits:**
- Visual confirmation of what's being played/recorded
- Helps beginners understand note layout
- Standard DAW feature (FL Studio, Ableton, Reaper all have this)
- No performance impact (rendered once per frame with existing notes)
- Useful for debugging stuck notes

**Notes:**
Current implementation provides visual feedback for preview and recording notes. Future enhancements could include:
- Playback note highlighting
- Mouse interaction (click keys to trigger sounds)
- Computer keyboard as piano (QWERTY mapping)

---

### #29 - Mouse-added notes only go to first record-enabled channel
**Status:** Fixed
**Priority:** High
**Found:** 2026-01-03
**Fixed:** 2026-01-03

**Description:**
When adding notes to the piano roll by clicking with the mouse, notes are only added to the first record-enabled channel, even when multiple channels have record enabled. This is inconsistent with the MIDI keyboard recording behavior, which correctly records to all record-enabled channels.

**Root Cause:**
The `CreateAddNoteToRecordChannels()` method in NoteEditor.cpp:15-39 contained a @TODO comment acknowledging this limitation. The method retrieved all record-enabled channels but only used `channels[0]`:

```cpp
// For now, we'll add to the first record-enabled channel
// @TODO: Create a compound command to handle multiple channels
const MidiChannel* channel = channels[0];
```

**Solution:**
Modified the existing `AddNoteCommand` to handle multiple tracks instead of creating a new command. The updated implementation:
- Changed constructor to accept `TrackSet&` and `std::vector<int> targetTracks` instead of single `Track&`
- Stores note indices per track (not entire track snapshots) for memory-efficient undo
- Uses `TrackSet::FindNoteAt()` to locate added notes for undo
- Loops through all target tracks in Execute() to add the same note to each
- Handles undo by removing notes from each track in reverse order

This approach is more memory-efficient than track snapshots and reuses existing TrackSet API methods.

**Files Modified:**
- `src/Commands/NoteEditCommands.h` - Updated AddNoteCommand interface
- `src/Commands/NoteEditCommands.cpp` - Implemented multi-track add logic
- `src/AppModel/NoteEditor/NoteEditor.cpp` - Updated CreateAddNoteToRecordChannels to pass all record-enabled tracks

**Benefits:**
- Mouse-added notes now correctly go to all record-enabled channels
- Consistent with MIDI keyboard recording behavior
- Memory-efficient undo using note indices instead of track snapshots
- Single undoable action for adding note to multiple tracks

**Related:**
This bug was discovered after implementing cross-track pasting (Ctrl+Shift+V), which uses the same pattern of adding notes to multiple record-enabled tracks.

---

### #4 - Collision prevention strategy for all note operations
**Status:** Fixed
**Priority:** Medium
**Found:** 2025-12-17
**Fixed:** 2026-01-02

**Description:**
Implemented a comprehensive collision prevention strategy across all note operations to replace the inconsistent overlap handling during loop recording.

**Solution:**
Implemented collision detection and prevention across all note editing operations:

**1. Collision Detection Helper Methods** (AppModel.h/cpp)
- Added `IsRegionCollisionFree()` with two overloads:
  - Single note version for move/resize operations
  - Multi-note version for batch move operations
- Both methods use `TrackSet::FindNotesInRegion()` to check for overlaps
- Made as const methods for better encapsulation

**2. Preview Collision Prevention**
- **`SetNoteMovePreview()`** - Prevents showing preview if destination collides
- **`SetMultipleNotesMovePreview()`** - All-or-nothing collision validation:
  - Checks each note in selection for collisions
  - Excludes all selected notes from collision check (allows notes to swap positions)
  - Rejects entire move if ANY note would collide with non-selected notes
  - Validates bounds (pitch 0-127, tick >= 0)
- **`SetNoteResizePreview()`** - Prevents resize preview if extension would overlap

**3. Mouse Click Note Addition** - Inherently handled
- Clicking on existing note starts move operation instead of adding
- No collision possible when clicking empty space

**4. Copy-Paste with Overdub Behavior** (PasteCommand.h)
- Paste now calls `TrackSet::SeparateOverlappingNotes()` after adding notes
- Uses same collision resolution as loop recording (consistent behavior)
- Stores complete track snapshots of affected tracks for undo
- Trade-off: Memory-intensive for very large projects, but simple and correct for typical use

**5. TrackSet Method Improvements**
- Made `FindNoteAt()`, `FindNotesInRegion()`, and `GetAllNotes()` const (read-only operations)

**6. Code Cleanup**
- Replaced 13 instances of manual `std::sort()` with `TrackSet::SortTrack()` across 6 command files
- Eliminated code duplication and improved maintainability

**Files Modified:**
- `src/AppModel/AppModel.h/cpp` - Added collision detection helpers
- `src/AppModel/TrackSet/TrackSet.h/cpp` - Made query methods const
- `src/Commands/PasteCommand.h` - Implemented overdub with track snapshots
- `src/Commands/DeleteMultipleNotesCommand.h` - Use TrackSet::SortTrack
- `src/Commands/MoveMultipleNotesCommand.h` - Use TrackSet::SortTrack
- `src/Commands/QuantizeCommand.h` - Use TrackSet::SortTrack
- `src/Commands/RecordCommand.h` - Use TrackSet::SortTrack
- `src/Commands/NoteEditCommands.cpp` - Use TrackSet::SortTrack

**Benefits:**
- ✓ Consistent collision handling across all note operations
- ✓ Prevents invalid MIDI data (overlapping notes)
- ✓ Matches professional DAW behavior (overdub paste)
- ✓ Cleaner codebase with reduced duplication
- ✓ Better encapsulation with const-correct methods

**Known Limitation:**
Paste undo stores complete track snapshots, which may use significant memory in very large projects (thousands of events per track). This is acceptable for typical projects and can be optimized later if needed.

---

### #24 - DrumMachine enhancements (pitch controls, clear button, ticks display)
**Status:** Fixed
**Priority:** Medium
**Found:** 2025-12-28
**Fixed:** 2025-12-30

**Description:**
The DrumMachine panel needed several usability improvements: customizable pitches for each row, a quick way to clear all enabled pads, and a display showing ticks per column to make the variable-resolution design discoverable.

**Solution:**
Implemented three key enhancements:

**1. Pitch Controls (Option 1)** ⭐
- Added wxSpinCtrl next to each drum row label
- Range: 0-127 (full MIDI pitch range)
- Initialized with row's current pitch (GM drum map defaults)
- Updates immediately when changed
- Marks pattern as dirty for regeneration

**2. Clear Button (Option 3)** ⭐
- Added "Clear All" button to top controls row
- Disables all pads in all rows with single click
- Calls `DrumMachine::Clear()` and refreshes button colors
- Provides quick way to start fresh pattern

**3. Ticks Per Column Display (Option 4)** ⭐
- Added wxStaticText label next to column count spinner
- Shows calculated ticks per column value
- Updates when column count or loop region changes
- Called from constructor, OnColumnCountChanged(), UpdateFromModel(), and loop changed callback
- Made `CalculatePadDuration()` public in DrumMachine class

**Grid Layout:**
```
[Empty] [Pitch] [1] [2] [3] ... [N]  (headers)
[Kick]  [36▼]   [□] [□] [□] ... [□]  (row with pitch control)
[Snare] [38▼]   [□] [□] [□] ... [□]
```

**Files Modified:**
- `src/Panels/DrumMachine/DrumMachinePanel.h` - Added pitch spinner vector, clear button, ticks label
- `src/Panels/DrumMachine/DrumMachinePanel.cpp` - Implemented all three features
- `src/AppModel/DrumMachine/DrumMachine.h` - Added GetPitch/SetPitch, made CalculatePadDuration public
- `src/MainFrame/MainFrame.cpp` - Added UpdateTicksPerColumnDisplay to loop changed callback

**Benefits:**
- Users can now adapt drum machine to any drum kit or percussion setup
- Quick pattern reset with Clear button improves workflow
- Ticks/column display makes variable-resolution design discoverable and educational
- Pattern automatically regenerates when pitch changes (via mPatternDirty flag)

**Not Implemented:**
- Option 2 (MIDI Listen Mode) - Optional advanced feature, not needed for core functionality
- Option 5 (Mute button) - Already existed in the panel before this bug was filed

---

### #2 - Closing app very slow
**Status:** Fixed
**Priority:** Medium
**Found:** 2025-12-17
**Fixed:** 2025-12-29

**Description:**
Application took an unusually long time to close when exiting, appearing to hang for several seconds before finally exiting.

**Root Cause:**
MainFrame's update timer (`mTimer`) fires every 1ms and calls `MidiCanvasPanel->Update()`, which triggers `Refresh()` and repaints. During app shutdown, the timer continued running while panels were being destroyed, causing hundreds of paint operations on dying widgets. This created a significant delay during the destruction process.

**Solution:**
Added `mTimer.Stop()` in `MainFrame::OnClose()` before panels are destroyed (MainFrameEventHandlers.cpp:281). This prevents the timer from firing during shutdown, eliminating the repeated paint operations that were causing the slowdown.

**Files Modified:**
- `src/MainFrame/MainFrameEventHandlers.cpp` - Added timer stop in OnClose handler

**Benefits:**
- Application now closes instantly (< 1 second)
- Clean shutdown without redundant UI updates
- Prevents potential crashes from updating destroyed widgets

---

### #21 - Quantize can create overlapping or zero-length notes
**Status:** Fixed
**Priority:** Medium
**Found:** 2025-12-24
**Fixed:** 2025-12-24

**Description:**
The `QuantizeCommand` quantized all MIDI events independently by snapping them to the nearest grid point. This caused note-on and note-off events to snap to the same tick (creating zero-length notes) or consecutive notes to overlap.

**Solution:**
Implemented duration-aware quantization algorithm using `TrackSet::GetNotesFromTrack()` to handle short vs long notes intelligently:

**Algorithm:**
```cpp
std::vector<NoteLocation> notes = TrackSet::GetNotesFromTrack(mTrack);

for (const auto& note : notes)
{
    uint64_t duration = note.endTick - note.startTick;
    uint64_t quantizedStart = RoundToGrid(note.startTick);

    if (duration < mGridSize)
    {
        // Short note (grace note/ornament): extend to one grid snap minimum
        mTrack[note.noteOnIndex].tick = quantizedStart;
        mTrack[note.noteOffIndex].tick = quantizedStart + mGridSize - NOTE_SEPARATION_TICKS;
    }
    else
    {
        // Long note: quantize both start and end independently
        mTrack[note.noteOnIndex].tick = quantizedStart;
        mTrack[note.noteOffIndex].tick = RoundToGrid(note.endTick);
    }
}

// Post-process to fix any remaining overlaps
TrackSet::SeparateOverlappingNotes(mTrack);
```

**Benefits:**
- Short notes (< grid size) are extended to one grid snap minimum - prevents grace notes from disappearing
- Long notes preserve their relative duration while snapping to grid
- Musically intelligent - respects performance intent (short decorative vs sustained notes)
- Uses `NOTE_SEPARATION_TICKS` constant for consistency with rest of codebase
- Post-processes with `SeparateOverlappingNotes` as safety net

**Files Modified:**
- `src/Commands/QuantizeCommand.h` - Complete rewrite of Execute() method with duration-aware algorithm

---

### #22 - Show MIDI Events tool incorrectly identifies Note Off events
**Status:** Fixed
**Priority:** Low
**Found:** 2025-12-24
**Fixed:** 2025-12-24

**Description:**
The "Show MIDI Events" debug tool in MidiCanvas displayed incorrect event types for Note Off events. Mouse-added notes correctly showed "Note Off", but keyboard-recorded notes showed "Other" instead of "Note Off".

**Root Cause:**
MIDI has two representations for Note Off:
1. **Explicit Note Off** - Status byte 0x80
2. **Implicit Note Off** - Note On (0x90) with velocity 0

The tooltip detection logic in `DrawMidiEventTooltip()` (MidiCanvas.cpp:592) only checked `event.isNoteOn` without considering velocity:
```cpp
// WRONG - doesn't check velocity!
if (event.isNoteOn) {
    eventType = "Note On";
}
```

This caused keyboard-recorded notes (which use implicit Note Off) to show as "Note On" because the status byte was 0x90, even though velocity was 0.

**Solution:**
Implemented a comprehensive refactoring:

**1. Refactored MidiEventDebugInfo struct** (MidiCanvas.h:77-81)
- Replaced individual fields (`pitch`, `velocity`, `trackIndex`, `isNoteOn`) with single `TimedMidiEvent` field
- Eliminated data duplication - `TimedMidiEvent` already contains all this information
- Reduced struct from 7 fields to 3 fields

**Before:**
```cpp
struct MidiEventDebugInfo {
    uint64_t tick;
    ubyte pitch;
    ubyte velocity;
    int trackIndex;
    bool isNoteOn;
    int screenX;
    int screenY;
};
```

**After:**
```cpp
struct MidiEventDebugInfo {
    TimedMidiEvent timedEvent;  // Contains mm and tick
    int screenX;
    int screenY;
};
```

**2. Simplified population code** (MidiCanvas.cpp:567)
```cpp
// Before (7 lines):
MidiEventDebugInfo debugInfo;
debugInfo.tick = event.tick;
debugInfo.pitch = event.mm.getPitch();
debugInfo.velocity = event.mm.mData[2];
debugInfo.trackIndex = event.mm.getChannel();
debugInfo.isNoteOn = isNoteOn;
debugInfo.screenX = screenX;
debugInfo.screenY = screenY;

// After (1 line using aggregate initialization):
mDebugEvents.push_back({event, screenX, screenY});
```

**3. Fixed event type detection** (MidiCanvas.cpp:583-594)
```cpp
auto& midiMsg = event.timedEvent.mm;
ubyte velocity = midiMsg.getVelocity();

if (midiMsg.isNoteOn() && velocity > 0)  // ✓ Real Note On
{
    eventType = "Note On";
}
else if (!midiMsg.isNoteOn() || (midiMsg.isNoteOn() && velocity == 0))  // ✓ Both forms
{
    eventType = "Note Off";
}
else
{
    eventType = "Other";
}
```

**Files Modified:**
- `src/Panels/MidiCanvas/MidiCanvas.h` - Refactored `MidiEventDebugInfo` struct
- `src/Panels/MidiCanvas/MidiCanvas.cpp` - Simplified population, fixed event type detection

**Benefits:**
- ✓ Correctly identifies both explicit and implicit Note Off events
- ✓ Cleaner code with reduced redundancy (7 fields → 3 fields, 7 lines → 1 line)
- ✓ Better maintainability using helper methods from `MidiMessage` class
- ✓ Single source of truth - data stored once in `TimedMidiEvent`

---

### #23 - Loop recording creates orphaned Note Off events for held notes
**Status:** Fixed
**Priority:** High
**Found:** 2025-12-24
**Fixed:** 2025-12-24

**Description:**
When loop recording, if a user held down a note across the loop boundary, the system created orphaned Note Off events with no matching Note On, resulting in invalid MIDI data.

**Problem Scenario:**
```
User holds C4 starting at tick 100
Loop end is at tick 960

Tick 100: Note On (C4) recorded
Tick 960: Loop wraps
         → CloseAllActiveNotes() creates Note Off (C4) at tick 960
         → Loop jumps to tick 0
         → USER STILL HOLDING KEY PHYSICALLY!
Tick 50:  User releases key
         → MIDI device sends Note Off (C4)
         → Orphaned Note Off recorded with no matching Note On! ✗
```

**Result:** Bad MIDI data with unpaired Note Off events that could cause playback issues.

**Root Cause:**
The `CloseAllActiveNotes()` method created Note Off events at loop boundaries to prevent stuck notes, but didn't account for notes that were still physically held on the MIDI keyboard after the loop wrapped. The `ActiveNote` struct also lacked velocity information needed to recreate Note On events.

**Solution:**
Implemented comprehensive refactoring to handle notes held across loop boundaries:

**1. Added `getVelocity()` helper to MidiMessage** (MidiMessage.h:162-165)
```cpp
ubyte getVelocity() const
{
    return mData[2];
}
```

**2. Extended ActiveNote struct with velocity** (RecordingSession.h:36-41)
```cpp
// Before:
struct ActiveNote {
    ubyte pitch;
    ubyte channel;
    uint64_t startTick;
};

// After:
struct ActiveNote {
    ubyte channel;
    ubyte pitch;
    ubyte velocity;  // NEW - needed to recreate Note On
    uint64_t startTick;
};
```

**3. Updated note tracking to capture velocity** (RecordingSession.cpp:48)
```cpp
// StartNote() now takes velocity parameter
ubyte velocity = msg.getVelocity();
StartNote(channel, pitch, velocity, currentTick);
```

**4. Renamed and enhanced CloseAllActiveNotes → WrapActiveNotesAtLoop** (RecordingSession.cpp:70-86)

The new name better describes what the method does: wrapping notes at loop boundaries.

**Before:**
```cpp
void CloseAllActiveNotes(uint64_t endTick)
{
    for (const auto& note : mActiveNotes)
    {
        MidiMessage noteOff = MidiMessage::NoteOff(note.pitch, note.channel);
        mBuffer.push_back({noteOff, endTick});
    }
    mActiveNotes.clear();  // ✗ Loses track of held notes!
}
```

**After:**
```cpp
void WrapActiveNotesAtLoop(uint64_t endTick, uint64_t loopStartTick)
{
    for (auto& note : mActiveNotes)  // Not const - we modify startTick
    {
        // Close the note at loop end
        MidiMessage noteOff = MidiMessage::NoteOff(note.pitch, note.channel);
        mBuffer.push_back({noteOff, endTick});

        // Reopen the note at loop start (user still holding key!)
        MidiMessage noteOn = MidiMessage::NoteOn(note.pitch, note.velocity, note.channel);
        mBuffer.push_back({noteOn, loopStartTick});

        // Update active note's start tick for eventual release
        note.startTick = loopStartTick;
    }
    // Don't clear mActiveNotes - notes are still physically held!
}
```

**5. Updated call site** (AppModel.cpp:342)
```cpp
// Before:
mRecordingSession.CloseAllActiveNotes(loopSettings.endTick - NOTE_SEPARATION_TICKS);

// After:
mRecordingSession.WrapActiveNotesAtLoop(
    loopSettings.endTick - NOTE_SEPARATION_TICKS,
    loopSettings.startTick
);
```

**6. Fixed operation ordering** (AppModel.cpp:336-342)

Moved `SeparateOverlappingNotes()` to run BEFORE `WrapActiveNotesAtLoop()` to prevent the overlap detection from incorrectly processing the synthetic wrap events:
```cpp
// Fix overlapping same-pitch notes first
TrackSet::SeparateOverlappingNotes(mRecordingSession.GetBuffer());

// Then wrap any notes still held at loop end
mRecordingSession.WrapActiveNotesAtLoop(noteOffTick, loopSettings.startTick);
```

**Expected Behavior After Fix:**
```
User holds C4 from tick 100 through loop boundary

Iteration 1: Note On (tick 100) → Note Off (tick 960) ✓
Iteration 2: Note On (tick 0) → Note Off (tick 50) ✓  [when user releases]
```

**Files Modified:**
- `src/RtMidiWrapper/MidiMessage/MidiMessage.h` - Added `getVelocity()` helper
- `src/AppModel/RecordingSession/RecordingSession.h` - Extended `ActiveNote` struct, renamed method
- `src/AppModel/RecordingSession/RecordingSession.cpp` - Updated `StartNote()`, `RecordEvent()`, implemented `WrapActiveNotesAtLoop()`
- `src/AppModel/AppModel.cpp` - Updated call site and operation ordering

**Additional Improvements:**
- Improved code readability by using `isNoteOn()`, `isNoteOff()`, `getPitch()`, `getChannel()`, and `getVelocity()` helper methods instead of raw byte manipulation
- Refactored `SeparateOverlappingNotes()` to use references and helper methods for better performance and clarity

**Benefits:**
- ✓ Proper MIDI data with all Note On/Off pairs matched
- ✓ No orphaned events during loop recording
- ✓ Correctly handles multiple notes held simultaneously across loop boundaries
- ✓ Preserves original note velocity when reopening at loop start
- ✓ Cleaner, more maintainable code using helper methods

**Related:**
This fix also addressed the broader refactoring mentioned in bug #8 (Loop recording active note tracking reliability) by simplifying and clarifying the active note tracking logic.

---

### #9 - MIDI input latency and recording timestamp accuracy
**Status:** Fixed
**Priority:** High
**Found:** 2025-12-17
**Fixed:** 2025-12-24

**Description:**
MIDI input latency was causing recorded notes to be timestamped later than when they were actually played, resulting in recordings that sounded "late" or "sluggish" on playback.

**Root Cause:**
The main issue was timestamp accuracy - notes were timestamped with `mTransport.GetCurrentTick()` which reflected the current playback time, not the actual key press time. With a 10ms update timer, this introduced up to 10ms of polling latency, which could accumulate and become noticeable during recording.

**Solution:**
Reduced the update timer interval from 10ms to 1ms (MainFrame timer), increasing the polling rate from 100 Hz to 1000 Hz. This reduced maximum polling latency from 10ms to 1ms, which is well below the ~20-30ms threshold where humans start to perceive timing delays.

**Impact:**
- Recording latency is now imperceptible during normal use
- Timestamps are captured within 1ms of actual MIDI input
- No noticeable "late" feeling when recording with MIDI keyboard
- Professional-level timing accuracy for rhythm-critical parts

**Notes:**
This fix resolves the latency issue for typical use cases. If latency becomes noticeable again on very large compositions with heavy CPU load, we may need to revisit this and implement additional solutions such as:
- Manual recording offset calibration
- ASIO support for lower-level MIDI access
- Dedicated MIDI input thread
- Latency measurement and compensation

For now, the 1ms timer provides sufficient accuracy for professional-quality MIDI recording.

---

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

### #15 - Inconsistent encapsulation in Transport class
**Status:** Fixed
**Priority:** Medium
**Found:** 2025-12-22
**Fixed:** 2025-12-23

**Description:**
Transport class exposed public member variables (mTempo, mLoopEnabled, mLoopStartTick, mLoopEndTick) while other component classes used proper getters/setters. This broke encapsulation and was inconsistent with the rest of the codebase architecture.

**Solution:**
Implemented comprehensive encapsulation refactoring using struct-based settings:

1. **Created BeatSettings struct** - Groups tempo and time signature settings:
```cpp
struct BeatSettings
{
    double tempo = MidiConstants::DEFAULT_TEMPO;
    int timeSignatureNumerator = MidiConstants::DEFAULT_TIME_SIGNATURE_NUMERATOR;
    int timeSignatureDenominator = MidiConstants::DEFAULT_TIME_SIGNATURE_DENOMINATOR;
};
```

2. **Created LoopSettings struct** - Groups loop-related settings:
```cpp
struct LoopSettings
{
    bool enabled = false;
    uint64_t startTick = 0;
    uint64_t endTick = MidiConstants::DEFAULT_LOOP_END;
};
```

3. **Added proper API methods:**
   - `BeatSettings GetBeatSettings() const` / `void SetBeatSettings(const BeatSettings&)`
   - `LoopSettings GetLoopSettings() const` / `void SetLoopSettings(const LoopSettings&)`
   - Kept existing convenience methods (`SetLoopStart/End`, `GetLoopStart/End`)

4. **Updated all call sites** across 6 files:
   - Transport.cpp - Internal references updated
   - TransportPanel.h - UI controls updated
   - ProjectManager.cpp - Serialization updated
   - AppModel.cpp - Loop logic updated
   - MidiCanvas.cpp - Loop rendering updated

**Benefits:**
- Proper encapsulation matching rest of codebase
- Atomic updates of related settings
- Cleaner code with struct-based grouping
- Easier to extend (validation, change notifications, etc.)
- Follows Single Responsibility Principle

---

### #17 - Mixed responsibilities in CheckMidiInQueue
**Status:** Fixed
**Priority:** Medium
**Found:** 2025-12-22
**Fixed:** 2025-12-23

**Description:**
`CheckMidiInQueue()` handled multiple concerns in a single 44-line method: MIDI input polling, routing, playback, recording, and note tracking. This violated Single Responsibility Principle and made the method hard to test and maintain.

**Solution:**
Implemented comprehensive refactoring following Single Responsibility Principle by splitting responsibilities across focused methods and components:

**1. Added MidiInputManager::PollAndNotify()**
- Moved message polling and callback notification to MidiInputManager
- Returns `std::optional<MidiMessage>`
- Encapsulates all input polling concerns in one place

```cpp
std::optional<MidiMessage> MidiInputManager::PollAndNotify(uint64_t currentTick)
{
    if (!mMidiIn->checkForMessage()) return std::nullopt;
    MidiMessage mm = mMidiIn->getMessage();
    if (mLogCallback) { mLogCallback({mm, currentTick}); }
    return mm;
}
```

**2. Added RecordingSession::RecordEvent()**
- Moved recording logic to RecordingSession where it belongs
- Encapsulates recording buffer management AND active note tracking
- Better separation of concerns - all recording logic in one component

```cpp
void RecordingSession::RecordEvent(const MidiMessage& msg, uint64_t currentTick)
{
    AddEvent({msg, currentTick});
    // Parse status and call StartNote/StopNote for note tracking
}
```

**3. Added AppModel::RouteAndPlayMessage()**
- Private helper method for message routing and playback
- Handles channel iteration, routing, and MIDI output
- Calls RecordingSession::RecordEvent() when recording

**4. Renamed CheckMidiInQueue() → HandleIncomingMidi()**
- More descriptive name matching Handle* pattern
- Moved from public to private (only called from Update())
- Simplified to 4 lines of pure orchestration

```cpp
void AppModel::HandleIncomingMidi()
{
    auto message = mMidiInputManager.PollAndNotify(mTransport.GetCurrentTick());
    if (!message) return;
    RouteAndPlayMessage(*message, mTransport.GetCurrentTick());
}
```

**Files Modified:**
- `MidiInputManager.h/cpp` - Added PollAndNotify
- `RecordingSession.h/cpp` - Added RecordEvent
- `AppModel.h/cpp` - Added RouteAndPlayMessage, renamed to HandleIncomingMidi

**Benefits:**
- Single Responsibility - Each method has one clear purpose
- Testability - Helper methods can be tested in isolation
- Maintainability - Recording changes don't affect routing
- Readability - High-level flow visible at a glance
- Better Encapsulation - RecordingSession owns all recording concerns

**Before:** 44 lines, 11 responsibilities mixed together
**After:** 4 lines of clean orchestration, responsibilities delegated to focused methods

---

### #7 - Add measure navigation to arrow keys
**Status:** Fixed
**Priority:** Medium
**Found:** 2025-12-17
**Fixed:** 2025-12-23

**Description:**
Keyboard shortcuts to jump to next/previous measure for faster navigation.

**Solution:**
Implemented measure navigation with arrow keys:

1. **Added Transport Methods:**
   - `JumpToNextMeasure()` - Jumps playhead to next measure boundary (Transport.h:73, Transport.cpp:132-147)
   - `JumpToPreviousMeasure()` - Jumps playhead to previous measure boundary (Transport.h:74, Transport.cpp:149-165)
   - Smart boundary detection: if at measure start, jump full measure; if mid-measure, jump to current measure boundary

2. **Keyboard Bindings:**
   - Left Arrow → Jump to previous measure
   - Right Arrow → Jump to next measure
   - Bindings added in MainFrame.h (ID_KEYBOARD_PREVIOUS_MEASURE, ID_KEYBOARD_NEXT_MEASURE)
   - Event handlers in MainFrameEventHandlers.cpp (OnPreviousMeasure, OnNextMeasure)

3. **Measure Calculation:**
   - Correctly uses time signature: 960 ticks per quarter note
   - Default 4/4 time: 3840 ticks per measure (960 * 4)
   - Adapts to custom time signatures via `GetTicksPerMeasure()`

4. **Documentation:**
   - Added to ShortcutsPanel.h Transport Controls section
   - Users can now discover the arrow key shortcuts in the shortcuts reference panel

**Files Modified:**
- `src/AppModel/Transport/Transport.h/cpp` - Added measure navigation methods
- `src/MainFrame/MainFrame.h` - Added keyboard event IDs
- `src/MainFrame/MainFrame.cpp` - Added keyboard bindings
- `src/MainFrame/MainFrameEventHandlers.cpp` - Added event handlers
- `src/Panels/ShortcutsPanel.h` - Documented shortcuts
- `docs/BugList.md` - Moved to Fixed Bugs

**Benefits:**
- Faster navigation through long compositions
- Professional DAW-style workflow
- Intuitive keyboard shortcuts matching standard DAW behavior
- Measure-aware navigation respects time signature changes
- Smooth user experience with smart boundary detection

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

### #5 - Add multiple note move feature
**Status:** Fixed
**Priority:** High
**Found:** 2025-12-17
**Fixed:** 2025-12-28

**Description:**
Users could select multiple notes and copy-paste them, but there was no way to move multiple selected notes together. This is a fundamental piano roll editing operation present in all professional DAWs.

**Solution:**
Implemented comprehensive multi-note drag-and-drop system:

**1. Created MoveMultipleNotesCommand** (Commands/MoveMultipleNotesCommand.h)
- Applies tick delta and pitch delta to all selected notes simultaneously
- Stores original positions for undo capability
- Handles pitch clamping (0-127) and negative tick protection
- Efficiently sorts affected tracks after move

**2. Extended NoteEditor with multi-note support** (AppModel/NoteEditor/)
- Added `CreateMoveMultipleNotes()` - Creates batch move command
- Added `MultiNoteEditPreview` struct for drag preview state
- Added `SetMultipleNotesMovePreview()`, `GetMultiNoteEditPreview()`, `HasMultiNoteEditPreview()`

**3. Added AppModel wrapper methods** (AppModel/)
- `MoveMultipleNotes()` - Executes multi-note move command
- `SetMultipleNotesMovePreview()` - Sets preview state during drag
- `GetMultiNoteEditPreview()`, `HasMultiNoteEditPreview()` - Preview accessors

**4. Updated MidiCanvas event handlers** (Panels/MidiCanvas/)
- Added `MouseMode::MovingMultipleNotes` state
- Modified `OnLeftDown()` - Detects when clicked note is part of selection, starts multi-note move
- Modified `OnMouseMove()` - Calculates tick/pitch delta during drag, updates preview
- Modified `OnLeftUp()` - Finalizes multi-note move using preview delta
- Updated `DrawNoteEditPreview()` - Renders preview for all selected notes during drag

**5. Fixed uint64_t underflow bug** (bonus fix during implementation)
- `ScreenXToTick()` - Added bounds checking to prevent negative coordinates from wrapping to huge positive values
- This fixed multi-select rectangle extending past tick 0 incorrectly selecting all notes

**Usage:**
1. Select multiple notes using rectangle selection (Shift + drag) or Ctrl+A
2. Click on any selected note (not on resize edge)
3. Drag to move all selected notes together
4. Release to commit the move (or Ctrl+Z to undo)

**Technical Details:**
- Preserves note durations during move
- Clamps pitch to valid MIDI range (0-127)
- Prevents negative tick values (clamps to 0)
- Maintains track separation (notes stay in their original channels)
- Full undo/redo support via command pattern
- Real-time visual preview with semi-transparent overlay
- Efficient batch operation (single command for all notes)

**Files Modified:**
- `src/Commands/MoveMultipleNotesCommand.h` (new)
- `src/AppModel/NoteEditor/NoteEditor.h/cpp`
- `src/AppModel/AppModel.h/cpp`
- `src/Panels/MidiCanvas/MidiCanvas.h`
- `src/Panels/MidiCanvas/MidiCanvas.cpp` (ScreenXToTick fix + preview drawing)
- `src/Panels/MidiCanvas/MidiCanvasEventHandlers.cpp`

**Benefits:**
- Fundamental DAW workflow now available
- Massive productivity improvement for editing multi-note patterns
- Consistent with single-note move behavior
- Professional-grade editing experience

---

### #8 - Loop recording active note tracking reliability
**Status:** Fixed
**Priority:** High
**Found:** 2025-12-17
**Fixed:** 2025-12-28

**Description:**
The loop recording system had complex logic for tracking active notes and auto-closing them at loop boundaries. This complexity introduced potential failure modes including duplicate note-offs, buffer corruption, stuck notes, and race conditions in state management.

**Solution:**
The reliability issues were resolved through comprehensive RecordingSession refactoring (implemented as part of bug #23 fix):

**1. Extended ActiveNote struct with velocity tracking** (RecordingSession.h:36-41)
- Added `velocity` field to enable proper note recreation across loop boundaries
- Allows reopening notes with original velocity after loop wrap

**2. Renamed and enhanced CloseAllActiveNotes → WrapActiveNotesAtLoop**
- Method now properly handles notes held across loop boundaries
- Closes note at loop end AND reopens at loop start (user still holding key)
- Updates active note start tick for eventual release
- Prevents orphaned Note Off events

**3. Improved operation ordering** (AppModel.cpp:336-342)
- `SeparateOverlappingNotes()` runs BEFORE `WrapActiveNotesAtLoop()`
- Prevents overlap detection from incorrectly processing synthetic wrap events

**4. Simplified active note tracking** (RecordingSession.cpp)
- Used helper methods: `isNoteOn()`, `isNoteOff()`, `getPitch()`, `getVelocity()`
- Cleaner code with better maintainability
- RecordingSession owns all recording concerns (proper encapsulation)

**Results:**
- ✓ No duplicate note-offs - wrap logic handles loop boundaries correctly
- ✓ Active notes properly managed across loop boundaries
- ✓ Notes held during mid-loop stop are properly tracked
- ✓ Clean state management with RecordingSession owning recording logic
- ✓ All note-ons have matching note-offs
- ✓ No stuck notes or buffer corruption

**Files Modified:**
- `src/RtMidiWrapper/MidiMessage/MidiMessage.h` - Added `getVelocity()` helper
- `src/AppModel/RecordingSession/RecordingSession.h` - Extended `ActiveNote` struct
- `src/AppModel/RecordingSession/RecordingSession.cpp` - Implemented `WrapActiveNotesAtLoop()`
- `src/AppModel/AppModel.cpp` - Updated call site and operation ordering

**Related:**
This fix was implemented as part of bug #23 (Loop recording creates orphaned Note Off events for held notes).

---

### #26 - Drum machine playback only works with loop start at tick 0
**Status:** Fixed
**Priority:** High
**Found:** 2025-12-30
**Fixed:** 2025-12-30

**Description:**
The DrumMachine playback only functioned correctly when the loop region started at tick 0. Moving the loop start to any other position caused drum machine playback to fail completely.

**Root Cause:**
The drum pattern is generated starting at tick 0 (relative to the pattern itself). When checking which events should play, the code was comparing pattern ticks (starting at 0) directly against playback ticks (which could be anywhere in the composition). This caused all events to be filtered out when the loop didn't start at tick 0.

**Solution:**
Added tick offset in `PlayDrumMachinePattern()` method (AppModel.cpp:354-356):
```cpp
// Drum pattern starts at 0, need to offset by loop's start tick for
// pattern to match loop region
uint64_t tick = event.tick + loopSettings.startTick;
```

This ensures drum pattern events are offset by the loop's starting tick, so they align with the loop region regardless of where it's positioned.

**Files Modified:**
- `src/AppModel/AppModel.cpp` - Added tick offset in PlayDrumMachinePattern()

**Benefits:**
- Drum machine now works correctly at any loop position
- Pattern playback properly adapts to loop start tick
- No change to pattern generation - just corrects playback timing

---

### #16 - Magic number sentinel value in PasteNotes
**Status:** Fixed
**Priority:** Low
**Found:** 2025-12-22
**Fixed:** 2026-01-03

**Description:**
`PasteNotes()` used UINT64_MAX as a sentinel value to indicate "use current tick", which was unclear and error-prone. This violated modern C++ idioms and made the code harder to understand.

**Problematic Code:**
```cpp
void AppModel::PasteNotes(uint64_t pasteTick = UINT64_MAX)
{
    if (!mClipboard.HasData()) return;
    if (pasteTick == UINT64_MAX)
        pasteTick = mTransport.GetCurrentTick();
    // ...
}
```

**Solution:**
Replaced magic number with C++17 `std::optional` for clear, type-safe optional parameters:

```cpp
void AppModel::PasteNotes(std::optional<uint64_t> pasteTick = std::nullopt)
{
    if (!mClipboard.HasData()) return;
    uint64_t tick = pasteTick.value_or(mTransport.GetCurrentTick());
    auto cmd = std::make_unique<PasteCommand>(mTrackSet, mClipboard.GetNotes(), tick);
    mUndoRedoManager.ExecuteCommand(std::move(cmd));
}
```

**Files Modified:**
- `src/AppModel/AppModel.h` - Added `#include <optional>`, updated method signature
- `src/AppModel/AppModel.cpp` - Updated implementation to use `value_or()`

**Benefits:**
- Self-documenting code - `std::nullopt` clearly indicates "use default"
- Type-safe - Impossible to accidentally pass UINT64_MAX as valid tick
- Idiomatic C++17 - Uses standard library feature designed for this use case
- Cleaner logic - Single line using `value_or()` instead of conditional

---

### #20 - Loop boundary check uses magic number
**Status:** Fixed
**Priority:** Low
**Found:** 2025-12-22
**Fixed:** 2026-01-03

**Description:**
Loop boundary check in `HandlePlaybackCore()` used inline logic `loopSettings.enabled && currentTick >= loopSettings.endTick` which duplicated loop detection logic and wasn't properly encapsulated in the Transport class.

**Problematic Code:**
```cpp
// AppModel.cpp:521
if (loopSettings.enabled && currentTick >= loopSettings.endTick)
{
    // loop-back logic
}
```

**Issues:**
1. **Poor encapsulation** - Loop boundary logic exposed outside Transport class
2. **Unclear intent** - Inline condition doesn't express "should we loop back?"
3. **Fragile** - Changes to loop logic require updating all call sites

**Solution:**
Extracted loop boundary check to named method in Transport class:

```cpp
// Transport.h
bool ShouldLoopBack(uint64_t currentTick) const;

// Transport.cpp
bool Transport::ShouldLoopBack(uint64_t currentTick) const
{
    return mLoopSettings.enabled && currentTick >= mLoopSettings.endTick;
}

// AppModel.cpp - Usage
if (mTransport.ShouldLoopBack(currentTick))
{
    // loop-back logic
}
```

**Files Modified:**
- `src/AppModel/Transport/Transport.h` - Added `ShouldLoopBack()` declaration
- `src/AppModel/Transport/Transport.cpp` - Implemented method
- `src/AppModel/AppModel.cpp` - Replaced inline check with method call

**Benefits:**
- Better encapsulation - Loop logic owned by Transport class
- Self-documenting - Method name clearly expresses intent
- Easier to maintain - Changes to loop logic centralized in one place
- Testable - Loop boundary logic can be unit tested in isolation

---

### #28 - Note add preview collision detection broken due to type overflow
**Status:** Fixed
**Priority:** Medium
**Found:** 2026-01-03
**Fixed:** 2026-01-03

**Description:**
The collision detection for note add preview in the MidiCanvas was not working, allowing users to see preview notes hovering over existing notes. The collision check was implemented but always failed due to a critical type mismatch.

**Root Cause:**
In MidiCanvasEventHandlers.cpp:335, the `endTick` variable was declared as `ubyte` (unsigned byte, range 0-255) instead of `uint64_t`:
```cpp
ubyte endTick = newTick + GetGridSize();  // WRONG - overflow!
```

Since `newTick` is a `uint64_t` that can be much larger than 255, this caused severe overflow/truncation. For example, if `newTick` was 960, the calculation would wrap around to a small value, causing `IsRegionCollisionFree()` to check the wrong region entirely.

**Solution:**
Fixed the type declaration and improved the collision check implementation (MidiCanvasEventHandlers.cpp:335-338):

```cpp
// Check if new note has any collisions on trackset
uint64_t snappedTick = ApplyGridSnap(newTick);
uint64_t endTick = snappedTick + GetGridSize() - MidiConstants::NOTE_SEPARATION_TICKS;
bool noCollisions = mAppModel->IsRegionCollisionFree(snappedTick, endTick, newPitch);
if (!noCollisions) return;
```

**Key Changes:**
1. Fixed type from `ubyte` to `uint64_t` to prevent overflow
2. Added grid snapping to check collision at the actual placement position (matches OnLeftUp behavior)
3. Subtracted `NOTE_SEPARATION_TICKS` to match the actual note duration that will be created
4. Removed outdated @TODO comment

**Files Modified:**
- `src/Panels/MidiCanvas/MidiCanvasEventHandlers.cpp` - Fixed collision check in OnMouseMove

**Benefits:**
- Preview note now correctly prevents hovering over existing notes
- Collision detection matches the actual note that will be created
- Consistent with grid snap settings
- Proper separation padding enforcement

---

### #34 - MIDI import doesn't recognize NOTE_ON velocity 0 as NOTE_OFF
**Status:** Fixed
**Priority:** High
**Found:** 2026-01-05
**Fixed:** 2026-01-05

**Description:**
When importing MIDI files, notes that used NOTE_ON messages with velocity 0 (a common MIDI convention for note-offs) were not being properly recognized as note-off events. This caused imported notes to not display correctly in the piano roll because the note pairing logic couldn't match them.

**Root Cause:**
MIDI has two representations for Note Off:
1. **Explicit Note Off** - Status byte 0x80 (what MidiWorks uses internally)
2. **Implicit Note Off** - NOTE_ON (0x90) with velocity 0 (common in MIDI files for running status optimization)

The `isNoteOff()` method only checked for explicit NOTE_OFF (0x80), missing the velocity 0 case.

**Solution:**
Implemented two-part fix:

**1. Enhanced `isNoteOff()` API** (MidiMessage.h:97-104)
```cpp
bool isNoteOff() const
{
    // Note Off can be either:
    // 1. Explicit NOTE_OFF message (0x80)
    // 2. NOTE_ON message with velocity 0 (common MIDI convention)
    return getEventType() == MidiEvent::NOTE_OFF || (isNoteOn() && getVelocity() == 0);
}
```

**2. Normalized on import** (ProjectManager.cpp:444-449)
```cpp
// Normalize NOTE_ON velocity 0 to explicit NOTE_OFF (0x80)
// This ensures our internal format is consistent (we always use NOTE_OFF)
if (timedEvent.mm.isNoteOn() && timedEvent.mm.getVelocity() == 0)
{
    timedEvent.mm = MidiMessage::NoteOff(timedEvent.mm.getPitch(), channel);
}
```

**Files Modified:**
- `src/RtMidiWrapper/MidiMessage/MidiMessage.h` - Enhanced `isNoteOff()` to handle both cases
- `src/AppModel/ProjectManager/ProjectManager.cpp` - Convert NOTE_ON velocity 0 to NOTE_OFF during import

**Benefits:**
- ✓ Imported MIDI files from all sources now display correctly
- ✓ Internal format is consistent (always explicit NOTE_OFF)
- ✓ Handles both MIDI conventions automatically
- ✓ `isNoteOff()` acts as safety check throughout codebase

**Design Decision:**
Chose to normalize at import boundary rather than handling both formats throughout the codebase. This keeps internal representation simple and canonical.

---

### #35 - MIDI import timing incorrect due to missing PPQN conversion
**Status:** Fixed
**Priority:** High
**Found:** 2026-01-05
**Fixed:** 2026-01-05

**Description:**
Imported MIDI files played back at incorrect tempos. Files with different PPQN (Pulses Per Quarter Note) values than MidiWorks' internal 960 PPQN had all their note timings wrong, causing them to play too fast or too slow.

**Problem Examples:**
- **480 PPQN file**: Quarter note = 480 ticks → Imported as 480 ticks → Plays at **half speed** (should be 960)
- **192 PPQN file**: Quarter note = 192 ticks → Imported as 192 ticks → Plays at **1/5 speed** (should be 960)
- **960 PPQN file**: Works correctly by accident (matches internal PPQN)

**Root Cause:**
The import code directly copied tick values from the MIDI file without converting them to MidiWorks' internal 960 PPQN resolution:

```cpp
timedEvent.tick = midiEvent.tick;  // ❌ No conversion!
```

Initially assumed the midifile library's `setTicksPerQuarterNote()` would automatically scale tick values, but it only sets metadata - it doesn't rescale existing event ticks.

**Solution:**
Implemented manual tick conversion with comprehensive logging:

**1. Calculate conversion ratio** (ProjectManager.cpp:364-367)
```cpp
// Calculate tick conversion ratio (needed for event import)
// Formula: newTick = oldTick * (targetPPQN / sourcePPQN)
int sourcePPQN = midifile.getTicksPerQuarterNote();
double tickConversion = (double)MidiConstants::TICKS_PER_QUARTER / (double)sourcePPQN;
```

**2. Apply conversion during import** (ProjectManager.cpp:506-507)
```cpp
// Convert tick from source PPQN to MidiWorks PPQN (960)
timedEvent.tick = (uint64_t)(midiEvent.tick * tickConversion);
```

**3. Added comprehensive import logging** (ProjectManager.cpp:362-429)
Created `import-midi.log` that records:
- Original PPQN from file
- Track count and duration
- All tempo events with timestamps and BPM
- All time signature events
- **Tick conversion ratio** (e.g., "2.0x" for 480→960)

**Example Conversions:**
- **480 PPQN file**: Ratio = 960/480 = 2.0x (tick 480 → 960) ✓
- **192 PPQN file**: Ratio = 960/192 = 5.0x (tick 192 → 960) ✓
- **960 PPQN file**: Ratio = 960/960 = 1.0x (no change) ✓

**Files Modified:**
- `src/AppModel/ProjectManager/ProjectManager.cpp` - Added tick conversion, import logging

**Benefits:**
- ✓ MIDI files from all sources now play at correct tempo
- ✓ Manual conversion is explicit and predictable
- ✓ Import log shows exactly what conversion happened
- ✓ Handles any PPQN value (480, 192, 96, 960, etc.)
- ✓ Works with all DAW exports (FL Studio, Ableton, Logic, Cubase, etc.)

**Testing:**
User confirmed fix by importing MIDI file that previously played at wrong tempo - now plays correctly at proper speed.

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
