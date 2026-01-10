# Getting to Version 2.0 - Professional DAW Roadmap

**Goal:** Transform MidiWorks from a functional MVP into a professional-grade MIDI composition tool with stability, polish, and advanced features.

**Starting Point:** Version 1.0 MVP Complete! 🎉

**Target User:** Serious composers who want a reliable, feature-rich DAW for professional MIDI work.

**v2.0 Definition:** "I can compose complex multi-track projects, export to industry-standard formats,
and integrate with my existing audio production workflow - all with professional stability and polish."

---

## Journey Overview

**Version 1.0** ✅ COMPLETE → **Version 1.1** → **Version 1.2** → **Version 1.3** → **Version 2.0** 🎯

### Milestone Breakdown

- **v1.1 - "Production Ready"** (3-4 weeks)
  - Error handling, stability, track management, MIDI export

- **v1.2 - "Professional Editing"** (3-4 weeks)
  - Velocity editing, transpose, timeline improvements, MIDI import, solo visual filtering

- **v1.3 - "Musical Intelligence"** (2-3 weeks)
  - Humanize, scale tools, chord helpers, advanced editing

- **v2.0 - "Professional Integration"** (4-6 weeks)
  - VST support, automation, tempo changes, advanced features

**Total Timeline:** 3-4 months of focused development

---

## 🎉 Recent Progress (January 2026)

**Major milestone achieved - v1.1 is 67% complete!** Recent additions:

**MIDI Import/Export:**
- ✅ Export MidiWorks projects to standard .mid files
- ✅ Import MIDI files from other DAWs and sources
- ✅ Share your compositions with other musicians
- ✅ Tested and verified with Finale notation software

**Track Organization:**
- ✅ Custom track naming (rename channels for clarity)
- ✅ Custom track colors (visual organization with color picker)
- ✅ Track minimize feature (collapse tracks to focus on active ones)
- ✅ All track customizations persist in project files

This unlocks collaboration, visual organization, and integration with the broader music production ecosystem!

---

## Current State (v1.0 ✅)

**What Works Great:**
- ✅ Full piano roll editing (add/move/resize/delete)
- ✅ Multi-selection and clipboard operations
- ✅ Complete undo/redo system
- ✅ Loop recording with overdub
- ✅ Quantize with triplets
- ✅ 15-track recording
- ✅ Channel mixer
- ✅ Save/load projects
- ✅ Keyboard shortcuts
- ✅ Visual feedback
- ✅ **Global time signature control (3/4, 6/8, 7/8, etc.) - January 2026**

**Known Limitations:**
- ⚠️ No error handling (crashes on device disconnect, file errors)
- ~~⚠️ No track organization (naming, colors, show/hide)~~ ✅ **COMPLETE!**
- ~~⚠️ No MIDI file import/export (can't share with other DAWs)~~ ✅ **COMPLETE! (January 2026)**
- ⚠️ No velocity editing (all notes same dynamics)
- ⚠️ No transpose functionality
- ⚠️ Timeline is tick-based (hard to navigate)
- ⚠️ No performance optimization (may lag with 1000+ notes)
- ⚠️ No tempo changes throughout song (single global tempo/time signature only)

---

## Version 1.1 - "Production Ready" 🎯

**Goal:** "MidiWorks won't crash, projects are organized, and I can export MIDI files."

**Target Date:** 3-4 weeks from v1.0

### Critical Stability Features

#### 1.1.1 Error Handling ⭐⭐⭐ CRITICAL
**Why:** App crashes are unacceptable for production use

**Tasks:**
- [ ] Add error callback system to AppModel
- [ ] Handle file I/O errors gracefully
  - [ ] Show error dialog on save failure
  - [ ] Show error dialog on load failure (corrupt JSON, missing file)
  - [ ] Show error dialog on file permissions issues
- [ ] Handle MIDI device errors
  - [ ] Detect MIDI device disconnect during use
  - [ ] Show "MIDI device lost" warning
  - [ ] Allow user to reconnect or continue without device
  - [ ] Gracefully handle no MIDI devices on startup
- [ ] Add track index bounds checking
  - [ ] Validate 0-14 range in TrackSet methods
  - [ ] Use `.at()` with exception handling where appropriate
- [ ] Add basic logging system
  - [ ] Log errors to file (MidiWorks.log)
  - [ ] Include timestamp, error type, context

**Implementation Strategy:**

Break this down into manageable phases. Phase 1 covers 80% of real-world crashes.

---

**PHASE 1: Foundation (3 hours) - DO THIS FIRST**

**Step 1: Add Error Callback System (30 min - 1 hour)**

This is the foundation everything else builds on.

```cpp
// AppModel.h
enum class ErrorLevel { Info, Warning, Error };
using ErrorCallback = std::function<void(const std::string& title,
                                          const std::string& message,
                                          ErrorLevel level)>;
void SetErrorCallback(ErrorCallback callback);

private:
    ErrorCallback mErrorCallback;
```

Wire it up in MainFrame:
```cpp
// MainFrame.cpp - In constructor
mAppModel->SetErrorCallback([this](const std::string& title,
                                     const std::string& msg,
                                     ErrorLevel level) {
    wxMessageBox(msg, title, wxOK | wxICON_ERROR);
});
```

**Step 2: Protect File Operations (1-2 hours)**

Focus on `ProjectManager::SaveProject()` and `ProjectManager::LoadProject()`:

```cpp
// In SaveProject
std::ofstream file(filepath);
if (!file.is_open()) {
    if (mErrorCallback) {
        mErrorCallback("Save Failed",
                      "Could not open file: " + filepath,
                      ErrorLevel::Error);
    }
    return false;
}

// Wrap JSON serialization in try-catch
try {
    nlohmann::json j = /* your serialization */;
    file << j.dump(4);
    return true;
} catch (const std::exception& e) {
    if (mErrorCallback) {
        mErrorCallback("Save Failed",
                      std::string("Error writing file: ") + e.what(),
                      ErrorLevel::Error);
    }
    return false;
}
```

Protection for LoadProject:
```cpp
try {
    nlohmann::json j = nlohmann::json::parse(file);
    // ... load data from j
} catch (const nlohmann::json::parse_error& e) {
    if (mErrorCallback) {
        mErrorCallback("Load Failed",
                      "Corrupt project file: " + std::string(e.what()),
                      ErrorLevel::Error);
    }
    return false;
} catch (const std::exception& e) {
    if (mErrorCallback) {
        mErrorCallback("Load Failed",
                      std::string("Error loading file: ") + e.what(),
                      ErrorLevel::Error);
    }
    return false;
}
```

**Phase 1 Testing:**
- [ ] Try to save to `C:\Windows\test.mwp` (permission denied)
- [ ] Create a corrupt JSON file and try to load it
- [ ] Try to load a non-existent file
- [ ] Verify error dialogs show user-friendly messages

**STOP HERE and test thoroughly. This covers 80% of crashes.**

---

**PHASE 2: MIDI Device Handling (2 hours) - DO THIS NEXT**

**Step 3: MIDI Device Disconnect Detection (1-2 hours)**

This is the #1 cause of crashes during actual use.

**Option A - Simple (Start Here):**
Wrap all `MidiOut` calls in try-catch or null checks:

```cpp
// In SoundBank::PlayNote (or wherever you send MIDI)
void SoundBank::PlayNote(ubyte pitch, ubyte velocity, ubyte channel)
{
    if (!mMidiOut || !mMidiOut->isPortOpen()) {
        // Device disconnected - silently fail or show warning once
        return;
    }

    try {
        mMidiOut->sendMessage(/* ... */);
    } catch (const RtMidiError& e) {
        if (mErrorCallback) {
            mErrorCallback("MIDI Device Error",
                          "Device may have been disconnected",
                          ErrorLevel::Warning);
        }
        // Optionally: clear mMidiOut so we don't keep trying
    }
}
```

**Option B - Proactive (Later):**
Add periodic device polling in `AppModel::Update()` to detect disconnects early.

**Step 4: Startup with No MIDI Devices (30 min)**

Handle the case where user has no MIDI devices at all.

```cpp
// In MidiSettingsPanel or wherever you initialize MIDI
if (mMidiOut->getPortCount() == 0) {
    wxMessageBox("No MIDI devices found. You can still use MidiWorks, "
                 "but you won't hear audio output.",
                 "No MIDI Devices",
                 wxOK | wxICON_INFORMATION);
    // Continue anyway - app works fine without MIDI output
}
```

**Phase 2 Testing:**
- [ ] Start playback, then unplug MIDI device (or close device in another app)
- [ ] Verify warning appears instead of crash
- [ ] Disable all MIDI devices in Device Manager
- [ ] Launch MidiWorks - should show friendly message, not crash
- [ ] Verify app continues to work (save/load, editing, etc.)

---

**PHASE 3: Optional Enhancements (1 hour)**

**Step 5: Basic Logging System**

Helps debug issues users report.

```cpp
// Logger.h - Simple approach
class Logger {
public:
    static void Log(const std::string& message) {
        std::ofstream log("MidiWorks.log", std::ios::app);
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        log << std::ctime(&time_t) << ": " << message << "\n";
    }

    static void Error(const std::string& message) {
        Log("[ERROR] " + message);
    }

    static void Warning(const std::string& message) {
        Log("[WARNING] " + message);
    }
};

// Usage in error callback
mAppModel->SetErrorCallback([this](const std::string& title,
                                     const std::string& msg,
                                     ErrorLevel level) {
    Logger::Error(title + ": " + msg);
    wxMessageBox(msg, title, wxOK | wxICON_ERROR);
});
```

**Step 6: Track Index Bounds Checking**

Add validation in critical TrackSet methods:

```cpp
// In TrackSet.cpp
Track& TrackSet::GetTrack(int index) {
    if (index < 0 || index >= USER_TRACK_COUNT) {
        if (mErrorCallback) {
            mErrorCallback("Internal Error",
                          "Invalid track index: " + std::to_string(index),
                          ErrorLevel::Error);
        }
        // Return a safe default or throw
        return mTracks.at(0);  // Or use exception
    }
    return mTracks[index];
}
```

---

**Testing Checklist - "Break It" Tests:**
- [ ] Save to read-only folder
- [ ] Load corrupted JSON file (manually edit and break syntax)
- [ ] Load non-existent file
- [ ] Unplug MIDI device during playback
- [ ] Launch with no MIDI devices
- [ ] Load project from older version (if format changed)
- [ ] Try to load empty file
- [ ] Try to load text file as .mwp
- [ ] Disconnect MIDI input during recording

**Estimated Effort:**
- Phase 1: 3 hours (CRITICAL - do this first)
- Phase 2: 2 hours (HIGH PRIORITY)
- Phase 3: 1 hour (NICE TO HAVE)
- **Total: 6 hours / ~1 day**

**Status:** ❌ TODO

---

#### 1.1.2 Track Naming ⭐⭐⭐ HIGH PRIORITY ✅ COMPLETE
**Why:** Can't tell what "Track 3" contains in a 15-track project

**Tasks:**
- [x] Add `std::string mName` to Track struct
- [x] Add default names ("Track 1", "Track 2", etc.)
- [x] Add `TrackSet::SetTrackName(int index, std::string name)`
- [x] Add track name to project save/load
- [x] Create track name UI
  - [x] Add text input to channel controls (or separate track list panel)
  - [x] Allow inline editing (double-click to rename)
  - [x] Show track name in piano roll (header area)
- [x] Add track name to undo history descriptions
  - [x] "Add note to Piano Track" instead of "Add note to Track 0"

**Implementation:**
- Custom track names stored in MidiChannel
- Names persist in project save/load (JSON)
- UI allows easy renaming in channel controls

**Estimated Effort:** 1 day
**Status:** ✅ COMPLETE

---

#### 1.1.3 Track Colors ⭐⭐ MEDIUM PRIORITY ✅ COMPLETE
**Why:** Visual organization helps in complex projects

**Tasks:**
- [x] Add `wxColour mColor` to Track struct (or to channel settings)
- [x] Create default color palette (15 distinct colors - already exists in MidiCanvas!)
- [x] Add color picker to channel controls
  - [x] wxColourPickerCtrl for custom colors
  - [x] Palette of preset colors for quick selection
- [x] Use track color in piano roll visualization (already done!)
- [x] Save/load track colors in project file
- [x] Add track color to mixer panel
  - [x] Color indicator strip on each channel control

**Implementation:**
- Custom colors stored in MidiChannel.customColor
- Colors persist in project save/load (JSON with RGB values)
- wxColourPickerCtrl in channel controls for easy customization
- Piano roll displays notes in track colors

**Estimated Effort:** 0.5 days
**Status:** ✅ COMPLETE

---

#### 1.1.4 Show/Hide Tracks (Minimize) ⭐⭐ MEDIUM PRIORITY ✅ COMPLETE
**Why:** Focus on specific instruments in busy projects

**Tasks:**
- [x] Add `bool mVisible` to Track or MidiChannel
- [x] Add eye icon toggle button to channel controls
- [x] Update MidiCanvas to skip hidden tracks in Draw()
- [x] Update playback to skip hidden tracks (optional - maybe only visual hide)
- [x] Save/load track visibility in project
- [x] Add "Show All" / "Hide All" buttons to mixer

**Implementation:**
- Implemented as "minimize" feature in channel controls
- MidiChannel.minimized flag controls visibility
- Minimized tracks collapse in UI, showing only essential controls
- Persists in project save/load (JSON)
- Tracks continue to play when minimized (visual organization only)

**Estimated Effort:** 0.5 days
**Status:** ✅ COMPLETE

---

#### 1.1.5 Clear Track ⭐⭐ MEDIUM PRIORITY
**Why:** Need to delete all notes on a track quickly

**Tasks:**
- [ ] Add "Clear Track" button to channel controls (or context menu)
- [ ] Create ClearTrackCommand for undo support
- [ ] Show confirmation dialog ("Delete all notes on Track 3: Piano?")
- [ ] Clear track should be undoable

**Implementation Notes:**
```cpp
// Commands/ClearTrackCommand.h
class ClearTrackCommand : public Command {
    Track& mTrack;
    std::vector<TimedMidiEvent> mBackup;  // For undo

    void Execute() override {
        mBackup = mTrack.events;  // Backup for undo
        mTrack.events.clear();
    }

    void Undo() override {
        mTrack.events = mBackup;
    }
};
```

**Estimated Effort:** 0.5 days
**Status:** ❌ TODO

---

#### 1.1.6 MIDI File Export ⭐⭐⭐ HIGH PRIORITY ✅ COMPLETE
**Why:** Share work with other musicians and DAWs

**Tasks:**
- [x] Research MIDI file format libraries
  - [x] Consider: midifile library (https://github.com/craigsapp/midifile)
  - [x] Or implement SMF (Standard MIDI File) format manually
- [x] Implement export to .mid file
  - [x] Convert internal ticks to MIDI delta times
  - [x] Set tempo in MIDI file header
  - [x] Set time signature
  - [x] Export all 15 tracks as separate MIDI tracks
  - [x] Include program change events for each track
- [x] Add File → Export MIDI menu item
- [x] Show export options dialog
  - [x] File path selection
  - [x] Format options (Format 0 vs Format 1)
  - [x] Optional: Track selection (export only specific tracks)
- [x] Test with other DAWs
  - [x] Import into Reaper, FL Studio, Ableton, etc.
  - [x] Verify tempo, notes, tracks all correct

**MIDI File Format Notes:**
- **Format 0:** Single track (all channels mixed)
- **Format 1:** Multiple tracks (separate tracks for each instrument) - RECOMMENDED ✅ Implemented
- **PPQN:** Use 960 (already our internal resolution) ✅ Implemented

**Implementation:**
```cpp
// ProjectManager::ExportMIDI() - Implemented in ProjectManager.cpp
// Uses midifile library
// Exports tempo, time signature, program changes, and all MIDI events
// Successfully tested with Finale notation software
```

**Estimated Effort:** 2 days (1 day if using library)
**Status:** ✅ COMPLETE (January 2026)

---

### Version 1.1 Summary

**Total Tasks:** 6 major features (Error handling, Track naming, Track colors, Show/hide, Clear track, MIDI export)
**Estimated Effort:** 5-7 days of focused work
**Target:** 2-3 weeks (part-time)

**Success Criteria:**
- [ ] App doesn't crash on common errors
- [ ] Tracks have meaningful names
- [ ] Can export projects to .mid files
- [ ] Can organize tracks visually
- [ ] User sees helpful error messages instead of crashes

**Next Milestone:** Once v1.1 is complete, move to v1.2 for editing enhancements.

---

## Version 1.2 - "Professional Editing" 🎯

**Goal:** "I can edit dynamics, transpose melodies, and navigate projects easily."

**Target Date:** 6-8 weeks from v1.0 (3-4 weeks after v1.1)

### Enhanced Editing Features

#### 1.2.1 MIDI File Import ⭐⭐⭐ HIGH PRIORITY ✅ COMPLETE
**Why:** Bring in drum loops, existing compositions, collaborate with others

**Tasks:**
- [x] Implement MIDI file parsing
  - [x] Use same library as export (midifile)
  - [x] Read tempo from MIDI file
  - [x] Read time signature
  - [x] Parse all tracks
  - [x] Convert MIDI delta times to internal ticks
- [x] Add File → Import MIDI menu item
- [x] Show import options dialog
  - [x] "Replace current project" vs "Merge into current project"
  - [x] Track mapping (which MIDI track → which MidiWorks track)
- [x] Handle edge cases
  - [x] Multi-tempo MIDI files (take first tempo or average?)
  - [x] Files with > 15 tracks (prompt user to select 15)
  - [x] Files with different PPQN (convert to 960)
- [x] Test with various MIDI files
  - [x] Simple piano melody
  - [x] Full drum loops
  - [x] Complex multi-track compositions

**Implementation:**
```cpp
// ProjectManager::ImportMIDI() - Implemented in ProjectManager.cpp
// Uses midifile library
// Clears existing tracks, imports tempo/time signature, and all MIDI events
// Maps MIDI events by channel (0-14)
// Handles program changes and applies to SoundBank
// Includes warning dialog to prevent accidental data loss
```

**Estimated Effort:** 2 days
**Status:** ✅ COMPLETE (January 2026)

---

#### 1.2.2 Multi-Note Resize ⭐⭐ MEDIUM PRIORITY
**Why:** Quickly adjust duration of multiple notes simultaneously

**Tasks:**
- [ ] Detect when multiple notes are selected during resize operation
- [ ] Apply same duration change to all selected notes
- [ ] Create ResizeMultipleNotesCommand for undo support
- [ ] Preserve relative timing between notes (all grow/shrink by same amount)

**Implementation Notes:**
```cpp
// When user resizes one note in a multi-selection:
// 1. Calculate delta duration (new duration - old duration)
// 2. Apply same delta to all selected notes
// 3. Execute as single undoable command

class ResizeMultipleNotesCommand : public Command {
    std::vector<NoteResizeInfo> mResizes;
    int64_t mDurationDelta;

    void Execute() override {
        for (auto& resize : mResizes) {
            uint64_t newDuration = resize.oldDuration + mDurationDelta;
            // Apply new duration to note
        }
    }
};
```

**Behavior:**
- Works like multi-note move: user drags one note's edge, all selected notes resize by same amount
- Maintains musical relationships (if one note is half the length of another, that ratio is preserved)
- Single undo/redo operation for all resizes

**Estimated Effort:** 0.5 days
**Status:** ❌ TODO

---

#### 1.2.3 Velocity Editing ⭐⭐⭐ HIGH PRIORITY
**Why:** Dynamics are essential for expressive, realistic MIDI

**Status:** 🔄 PARTIALLY COMPLETE - Single-note editing complete (January 2026), multi-note editing TODO

**Phase 1: Single Note Editing** ✅ COMPLETE (January 2026)
- [x] Velocity lanes below piano roll (Option A approach)
  - [x] Velocity editor panel appears when notes selected
  - [x] Shows in bottom 25% of canvas (dark charcoal background)
  - [x] Vertical slider controls per selected note
- [x] Create EditNoteVelocityCommand for undo support
- [x] Visual feedback during editing
  - [x] Orange highlight on active control
  - [x] Real-time velocity value display
  - [x] Control position updates with mouse drag
- [x] Integration with piano roll
  - [x] Click and drag velocity control to edit
  - [x] Velocity range 1-127 (clamped)
  - [x] Full undo/redo support

**Implementation (Completed):**
```cpp
// Commands/EditNoteVelocityCommand (in NoteEditCommands.h)
class EditNoteVelocityCommand : public Command {
    Track& mTrack;
    size_t mNoteOnIndex;
    ubyte mOldVelocity;
    ubyte mNewVelocity;

    void Execute() override {
        mTrack[mNoteOnIndex].mm.mData[2] = mNewVelocity;
    }

    void Undo() override {
        mTrack[mNoteOnIndex].mm.mData[2] = mOldVelocity;
    }
};
```

**Phase 2: Multi-Note Velocity Editing** (TODO)

When multiple notes are selected, provide intuitive ways to edit velocities together.

**Option 1: Relative Adjustment (Default Behavior)** ⭐ RECOMMENDED
- Drag any velocity control
- **All** selected notes adjust by the same delta (+10, -15, etc.)
- Preserves relative differences between velocities
- Example: Notes at [100, 80, 60] dragged up +20 → [120, 100, 80]

**Benefits:**
- ✅ Preserves musical dynamics between notes
- ✅ Feels natural and intuitive
- ✅ Most common in professional DAWs (Ableton, FL Studio, Cubase)
- ✅ No modifiers required

**Implementation:**
```cpp
// When dragging a velocity control with multiple notes selected:
void MidiCanvasPanel::OnMouseMove(wxMouseEvent& event)
{
    if (mMouseMode == MouseMode::EditingVelocity &&
        mVelocityEditNote.found &&
        mSelectedNotes.size() > 1)  // Multi-note editing
    {
        // Calculate delta from original velocity
        int velocityDelta = mVelocityEditNote.velocity - mOriginalVelocity;

        // Update all selected notes by same delta
        for (auto& note : mSelectedNotes)
        {
            int newVelocity = note.velocity + velocityDelta;
            newVelocity = std::clamp(newVelocity, 1, 127);
            // Store in temporary preview state
        }

        Refresh();
    }
}

// On mouse up: Create EditMultipleNoteVelocitiesCommand
class EditMultipleNoteVelocitiesCommand : public Command {
    std::vector<NoteVelocityChange> mChanges;
    int mVelocityDelta;

    void Execute() override {
        for (auto& change : mChanges) {
            Track& track = mTrackSet.GetTrack(change.trackIndex);
            int newVel = std::clamp(
                static_cast<int>(change.originalVelocity) + mVelocityDelta,
                1, 127
            );
            track[change.noteOnIndex].mm.mData[2] = newVel;
        }
    }
};
```

**Option 2: Modifier Key Behavior** (Advanced Flexibility)
- **Default drag**: Relative adjustment (all notes by delta)
- **Ctrl+drag**: Set all to same absolute value (normalize dynamics)

**Benefits:**
- ✅ Flexible: supports both relative and absolute editing
- ✅ Doesn't break current single-note workflow
- ✅ Discoverable (tooltip/status bar hint)

**Implementation:**
```cpp
void MidiCanvasPanel::OnMouseMove(wxMouseEvent& event)
{
    if (mMouseMode == MouseMode::EditingVelocity && mSelectedNotes.size() > 1)
    {
        ubyte targetVelocity = CalculateVelocityFromMouseY(event.GetY());

        if (event.ControlDown())
        {
            // CTRL: Absolute mode - set all to same value
            for (auto& note : mSelectedNotes)
            {
                // Preview all notes at targetVelocity
            }
        }
        else
        {
            // Default: Relative mode - adjust all by delta
            int velocityDelta = targetVelocity - mOriginalVelocity;
            for (auto& note : mSelectedNotes)
            {
                // Preview note with clamped(original + delta)
            }
        }
    }
}
```

**Tasks for Phase 2:**
- [ ] Add EditMultipleNoteVelocitiesCommand class
- [ ] Detect multi-note selection in velocity editor
- [ ] Implement Option 1 (relative adjustment) as default
- [ ] (Optional) Add Option 2 (Ctrl modifier for absolute mode)
- [ ] Update visual feedback for all controls during multi-edit
- [ ] Add status bar hint: "Drag: Relative | Ctrl+Drag: Absolute"

**Future Enhancements (Optional):**
- [ ] Add velocity visualization to notes in piano roll
  - [ ] Color intensity based on velocity (darker = louder)
  - [ ] Or: Note brightness variation
- [ ] Add velocity to Quantize panel
  - [ ] "Quantize Velocity" - round to nearest 10, 16, etc.
- [ ] Velocity curve editing (draw velocity ramps)
- [ ] Velocity scaling (compress/expand dynamic range)

**Estimated Effort:**
- Phase 1 (Single-note): ✅ 1 day (COMPLETE)
- Phase 2 (Multi-note): 1-2 days
- Total: 2-3 days

**Status:**
- Phase 1: ✅ COMPLETE (January 2026)
- Phase 2: ❌ TODO

---

#### 1.2.4 Transpose ⭐⭐⭐ HIGH PRIORITY
**Why:** Essential for trying melodies in different keys

**Tasks:**
- [ ] Add Edit → Transpose menu item
- [ ] Create transpose dialog
  - [ ] Semitones spinner (-12 to +12)
  - [ ] Preview button (play transposed notes without committing)
  - [ ] Apply to: "Selected Notes" or "Entire Track" or "All Tracks"
- [ ] Create TransposeCommand for undo support
- [ ] Add keyboard shortcut (Ctrl+Up/Down for +/-1 semitone?)
- [ ] Implement pitch limiting
  - [ ] Clamp to 0-127 range
  - [ ] Show warning if notes will be clamped

**UI Mockup:**
```
┌─ Transpose ──────────────────────┐
│ Semitones: [▼] [-2      ] [▲]    │
│                                   │
│ Apply to:                         │
│ ○ Selected notes (5 notes)        │
│ ○ Current track (Track 3)         │
│ ○ All tracks                      │
│                                   │
│ [Preview] [Cancel] [OK]           │
└───────────────────────────────────┘
```

**Implementation Notes:**
```cpp
// Commands/TransposeCommand.h
class TransposeCommand : public Command {
    std::vector<NoteLocation> mNotes;
    int mSemitones;

    void Execute() override {
        for (auto& note : mNotes) {
            Track& track = mTrackSet.GetTrack(note.trackIndex);
            int newPitch = std::clamp(
                static_cast<int>(note.pitch) + mSemitones,
                0, 127
            );
            track[note.noteOnIndex].mm.mData[1] = newPitch;
            track[note.noteOffIndex].mm.mData[1] = newPitch;
        }
        // Sort tracks after transpose
        for (auto& note : mNotes) {
            mTrackSet.SortTrack(note.trackIndex);
        }
    }

    void Undo() override {
        for (auto& note : mNotes) {
            Track& track = mTrackSet.GetTrack(note.trackIndex);
            track[note.noteOnIndex].mm.mData[1] = note.pitch;
            track[note.noteOffIndex].mm.mData[1] = note.pitch;
        }
        for (auto& note : mNotes) {
            mTrackSet.SortTrack(note.trackIndex);
        }
    }
};
```

**Estimated Effort:** 1 day
**Status:** ❌ TODO

---

#### 1.2.5 Timeline Improvements ⭐⭐ MEDIUM PRIORITY
**Why:** Tick-based navigation is confusing, need measure/beat display

**Tasks:**
- [ ] Add timeline ruler above piano roll
  - [ ] Show measure numbers (1, 2, 3, 4...)
  - [ ] Show beat subdivisions (1.1, 1.2, 1.3, 1.4)
  - [ ] Calculate from tempo and time signature
- [ ] Click timeline to seek
  - [ ] OnLeftDown on ruler → calculate tick → SetPlayheadPosition
- [ ] Add scrubbing (drag playhead while hearing audio)
  - [ ] Hold and drag playhead
  - [ ] Play notes as playhead passes them
  - [ ] **Advanced:** This is tricky, may defer to v1.3
- [ ] Improve time display
  - [ ] Add "Bars:Beats:Ticks" format (in addition to MM:SS)
  - [ ] Example: "5:3:480" = Measure 5, Beat 3, Tick 480

**Timeline Ruler Mockup:**
```
Measures: |----1----|----2----|----3----|----4----|
Beats:    | .  .  . | .  .  . | .  .  . | .  .  . |
```

**Implementation Notes:**
```cpp
// In TransportPanel or new TimelinePanel
void DrawTimeline(wxDC& dc)
{
    int ticksPerMeasure = MidiConstants::TICKS_PER_QUARTER * mTransport.mTimeSignatureNumerator;

    for (int measure = 0; measure < visibleMeasures; measure++)
    {
        uint64_t measureTick = measure * ticksPerMeasure;
        int x = TickToScreenX(measureTick);

        // Draw measure line
        dc.DrawLine(x, 0, x, height);
        dc.DrawText(wxString::Format("%d", measure + 1), x + 2, 2);

        // Draw beat subdivisions
        for (int beat = 1; beat < mTransport.mTimeSignatureNumerator; beat++)
        {
            uint64_t beatTick = measureTick + (beat * MidiConstants::TICKS_PER_QUARTER);
            int beatX = TickToScreenX(beatTick);
            dc.DrawLine(beatX, height/2, beatX, height);
        }
    }
}
```

**Estimated Effort:** 2 days
**Status:** ❌ TODO

---

#### 1.2.6 Global Time Signature Control ⭐⭐ MEDIUM PRIORITY ✅ COMPLETE
**Why:** Support composing in different meters (3/4 waltz, 6/8 jig, 7/8 odd time, etc.)

**Tasks:**
- [x] Add time signature controls to TransportPanel
  - [x] wxChoice for numerator (1-21)
  - [x] wxChoice for denominator (2, 4, 8, 16, 32)
- [x] Update Transport::BeatSettings with time signature
- [x] Calculate ticks per beat and measure from time signature
- [x] Update canvas grid to respect time signature
- [x] Update metronome beat detection to respect time signature
- [x] Persist time signature in project save/load
- [x] Export/import time signature in MIDI files

**Implementation:**
- Time signature stored in `Transport::BeatSettings` struct
- `GetTicksPerBeat()` calculates based on denominator: `TICKS_PER_QUARTER * 4 / denominator`
- `GetTicksPerMeasure()` calculates: `GetTicksPerBeat() * numerator`
- Canvas grid automatically updates based on beat settings (MidiCanvas.cpp:386-387)
- Metronome `CheckForBeat()` respects time signature for downbeat detection
- Measure navigation (jump to next/previous measure) works correctly
- MIDI import extracts time signature from imported files (ProjectManager.cpp:461-462)
- MIDI export writes time signature meta event (ProjectManager.cpp:321)
- Settings persist in project JSON (save/load)

**Limitation:** This implements a single **global** time signature for the entire project. For multiple time signature changes throughout a song (e.g., 4/4 verse → 3/4 chorus), see section 2.0.3 (Tempo Track).

**Estimated Effort:** 1 day
**Status:** ✅ COMPLETE (January 2026)

---

#### 1.2.7 Solo Visual Filtering ⭐⭐⭐ HIGH PRIORITY ✅ COMPLETE
**Why:** Overlapping notes from different tracks make rectangular selection grab wrong notes. Need visual isolation to edit individual tracks precisely.

**Status:** ✅ Core functionality COMPLETE (January 2026) - Hidden mode implemented. Dimmed and Normal modes can be added later as enhancements.

**Problem Statement:**
When multiple tracks have notes at the same pitch and time, selecting a range of notes from one track accidentally grabs notes from other tracks. This makes editing individual tracks in dense multi-track projects frustrating and error-prone.

**Tasks:**
- [ ] Add visual filtering mode setting
  - [ ] Add option to control solo visual behavior: "Normal" / "Dimmed" / "Hidden"
  - [ ] Store preference in project settings or app preferences
  - [ ] Add UI control (dropdown in canvas toolbar or settings panel)
- [ ] Implement solo-based visual filtering
  - [ ] When solo is active, check visual filtering mode
  - [ ] **Dimmed mode:** Draw non-soloed notes at 20-30% opacity
  - [ ] **Hidden mode:** Don't draw non-soloed notes at all
  - [ ] **Normal mode:** Current behavior (all visible regardless of solo)
- [ ] Filter note interaction based on solo state
  - [ ] `FindNoteAtPosition()` - only return notes from soloed tracks
  - [ ] `FindNotesInRectangle()` - only return notes from soloed tracks
  - [ ] Mouse hover - only highlight notes from soloed tracks
- [ ] Update selection logic
  - [ ] Rectangular selection only grabs soloed track notes
  - [ ] Single-click selection only selects soloed track notes
  - [ ] Prevent editing non-soloed tracks when solo is active
- [ ] Handle edge cases
  - [ ] Multiple solos - show all soloed tracks (existing solo logic)
  - [ ] No solo active - all tracks visible and editable (current behavior)
  - [ ] Existing note selections when enabling solo - clear or filter?

**Implementation Notes:**
```cpp
// MidiCanvas.h
enum class SoloVisualMode
{
    Normal,    // Solo only affects playback (current behavior)
    Dimmed,    // Non-solo tracks drawn at low opacity, not selectable
    Hidden     // Non-solo tracks invisible and not selectable
};

// AppModel.h or MidiCanvas.h
SoloVisualMode mSoloVisualMode = SoloVisualMode::Normal;

// MidiCanvas.cpp - DrawTrackNotes()
void MidiCanvasPanel::DrawTrackNotes(wxGraphicsContext* gc)
{
    bool solosActive = mAppModel->GetSoundBank().SolosFound();
    std::vector<NoteLocation> allNotes = mTrackSet.GetAllNotes();

    for (const auto& note : allNotes)
    {
        if (note.trackIndex >= USER_TRACK_COUNT) continue;

        // Check if track should be drawn based on solo state
        bool isSoloed = mAppModel->GetSoundBank().GetChannel(note.trackIndex).solo;

        if (solosActive && !isSoloed)
        {
            // Non-soloed track when solo is active
            if (mSoloVisualMode == SoloVisualMode::Hidden)
            {
                continue;  // Skip drawing completely
            }
            else if (mSoloVisualMode == SoloVisualMode::Dimmed)
            {
                // Draw with low opacity
                auto trackColor = mAppModel->GetSoundBank().GetChannelColor(note.trackIndex);
                trackColor.Set(trackColor.Red(), trackColor.Green(), trackColor.Blue(), 50);  // ~20% opacity
                gc->SetBrush(wxBrush(trackColor));
                DrawNote(gc, note);
                continue;
            }
        }

        // Normal drawing (soloed track or no solo active or Normal mode)
        auto trackColor = mAppModel->GetSoundBank().GetChannelColor(note.trackIndex);
        gc->SetBrush(wxBrush(trackColor));
        DrawNote(gc, note);
    }
}

// MidiCanvas.cpp - FindNoteAtPosition()
NoteLocation MidiCanvasPanel::FindNoteAtPosition(int screenX, int screenY)
{
    uint64_t clickTick = ScreenXToTick(screenX);
    ubyte clickPitch = ScreenYToPitch(screenY);
    NoteLocation found = mTrackSet.FindNoteAt(clickTick, clickPitch);

    // Filter by solo if filtering is active
    if (mSoloVisualMode != SoloVisualMode::Normal &&
        mAppModel->GetSoundBank().SolosFound())
    {
        if (!found.IsEmpty())
        {
            bool isSoloed = mAppModel->GetSoundBank().GetChannel(found.trackIndex).solo;
            if (!isSoloed)
            {
                return NoteLocation{};  // Return empty - note not selectable
            }
        }
    }

    return found;
}

// Similar filtering for FindNotesInRectangle()
std::vector<NoteLocation> MidiCanvasPanel::FindNotesInRectangle(wxPoint start, wxPoint end)
{
    // ... existing conversion code ...
    std::vector<NoteLocation> found = mTrackSet.FindNotesInRegion(minTick, maxTick, minPitch, maxPitch);

    // Filter by solo if filtering is active
    if (mSoloVisualMode != SoloVisualMode::Normal &&
        mAppModel->GetSoundBank().SolosFound())
    {
        found.erase(
            std::remove_if(found.begin(), found.end(), [&](const NoteLocation& note) {
                return !mAppModel->GetSoundBank().GetChannel(note.trackIndex).solo;
            }),
            found.end()
        );
    }

    return found;
}
```

**UI Options:**

**Option A: Dropdown in Canvas Toolbar**
```
[Piano Roll] | Solo Visual: [Dimmed ▼] | Grid Snap: [✓] | ...
```

**Option B: Toggle Button Cycle**
- Click button to cycle: Normal → Dimmed → Hidden → Normal
- Icon changes to indicate current mode
- Tooltip shows current mode

**Option C: Settings Panel**
- Add to app preferences
- Set once and forget

**Recommendation:** Start with **Option C** (settings preference) for simplicity, add **Option A** later if users want quick toggling.

**Benefits:**
- ✅ Solves the overlapping notes selection problem
- ✅ Maintains visual context (Dimmed mode) when needed
- ✅ Provides maximum focus (Hidden mode) when needed
- ✅ Doesn't break existing behavior (Normal mode is default)
- ✅ Leverages existing solo infrastructure
- ✅ User choice for different workflows

**Testing:**
- [ ] Create project with overlapping notes (e.g., bass and kick drum on C2)
- [ ] Solo one track, test each visual mode
- [ ] Verify rectangular selection only grabs soloed track notes
- [ ] Test with multiple solos active
- [ ] Test mode switching during playback
- [ ] Verify dimmed notes are not selectable

**Estimated Effort:** 1-2 days
**Status:** ✅ COMPLETE (January 2026) - Core hidden mode implemented

**Future Enhancements (Optional):**
- [ ] Add Dimmed mode (draw non-soloed notes at low opacity)
- [ ] Add Normal mode (all tracks visible regardless of solo)
- [ ] Add UI toggle to switch between modes

---

### Version 1.2 Summary

**Total Tasks:** 7 major features (MIDI import, Multi-note resize, Velocity editing, Transpose, Timeline, Global time signature, Solo visual filtering)
**Estimated Effort:** 9.5-11.5 days of focused work
**Target:** 3-4 weeks (part-time)

**Success Criteria:**
- [x] Can import MIDI files from other sources ✅
- [ ] Can resize multiple selected notes simultaneously
- [x] Can edit note dynamics (velocity) 🔄 (Single-note complete, multi-note TODO)
- [ ] Can transpose melodies to different keys
- [ ] Can navigate projects with measure/beat timeline
- [x] Can compose in different time signatures (3/4, 6/8, 7/8, etc.) ✅
- [x] Can isolate individual tracks for precise editing (solo visual filtering) ✅
- [ ] Timeline is clear and intuitive

**Next Milestone:** Once v1.2 is complete, move to v1.3 for musical intelligence features.

---

## Version 1.3 - "Musical Intelligence" 🎯

**Goal:** "MidiWorks helps me compose with smart tools that understand music theory."

**Target Date:** 10-12 weeks from v1.0 (2-3 weeks after v1.2)

### Musical Tools & Advanced Editing

#### 1.3.1 Humanize ⭐⭐ MEDIUM PRIORITY
**Why:** Perfect timing sounds robotic, humanize adds realism

**Tasks:**
- [ ] Add Edit → Humanize menu item
- [ ] Create humanize dialog
  - [ ] Timing randomization slider (0-100 ticks variance)
  - [ ] Velocity randomization slider (0-20 velocity variance)
  - [ ] Apply to: Selected notes / Current track / All tracks
- [ ] Create HumanizeCommand for undo support
- [ ] Use random number generator with seed for reproducibility
  - [ ] Allow "re-roll" to try different humanization

**Implementation Notes:**
```cpp
// Commands/HumanizeCommand.h
class HumanizeCommand : public Command {
    struct NoteHumanization {
        int trackIndex;
        size_t noteOnIndex;
        size_t noteOffIndex;
        uint64_t oldStartTick;
        uint64_t newStartTick;
        uint8_t oldVelocity;
        uint8_t newVelocity;
    };

    std::vector<NoteHumanization> mChanges;
    int mTimingVariance;  // Max ticks to shift
    int mVelocityVariance;  // Max velocity to shift

    void Execute() override {
        std::mt19937 rng(std::random_device{}());

        for (auto& change : mChanges) {
            Track& track = mTrackSet.GetTrack(change.trackIndex);

            // Randomize timing
            std::uniform_int_distribution<int> timingDist(-mTimingVariance, mTimingVariance);
            int tickShift = timingDist(rng);
            change.newStartTick = std::max(0LL, static_cast<int64_t>(change.oldStartTick) + tickShift);

            // Randomize velocity
            std::uniform_int_distribution<int> velDist(-mVelocityVariance, mVelocityVariance);
            int velShift = velDist(rng);
            change.newVelocity = std::clamp(
                static_cast<int>(change.oldVelocity) + velShift,
                1, 127
            );

            // Apply changes
            uint64_t duration = change.oldEndTick - change.oldStartTick;
            track[change.noteOnIndex].tick = change.newStartTick;
            track[change.noteOnIndex].mm.mData[2] = change.newVelocity;
            track[change.noteOffIndex].tick = change.newStartTick + duration;
        }

        // Sort track after humanize
        mTrackSet.SortTrack(change.trackIndex);
    }
};
```

**Estimated Effort:** 1 day
**Status:** ❌ TODO

---

#### 1.3.2 Legato / Staccato ⭐⭐ MEDIUM PRIORITY
**Why:** Quick articulation adjustments

**Tasks:**
- [ ] Add Edit → Articulation submenu
  - [ ] Legato (connect notes)
  - [ ] Staccato (shorten notes to 50% duration)
  - [ ] Custom duration (% of original)
- [ ] Create ArticulationCommand
- [ ] **Legato:** Extend each note's end to next note's start
- [ ] **Staccato:** Shorten each note to 50% duration (or custom %)

**Implementation Notes:**
```cpp
// For Legato: Extend note end to next note start
void ApplyLegato(std::vector<NoteLocation>& notes)
{
    // Sort notes by start time
    std::sort(notes.begin(), notes.end(), [](auto& a, auto& b) {
        return a.startTick < b.startTick;
    });

    for (size_t i = 0; i < notes.size() - 1; i++)
    {
        auto& current = notes[i];
        auto& next = notes[i + 1];

        // Extend current note to just before next note
        Track& track = mTrackSet.GetTrack(current.trackIndex);
        track[current.noteOffIndex].tick = next.startTick - 1;
    }
}

// For Staccato: Shorten all notes
void ApplyStaccato(std::vector<NoteLocation>& notes, float percentage)
{
    for (auto& note : notes)
    {
        uint64_t duration = note.endTick - note.startTick;
        uint64_t newDuration = duration * percentage;

        Track& track = mTrackSet.GetTrack(note.trackIndex);
        track[note.noteOffIndex].tick = note.startTick + newDuration;
    }
}
```

**Estimated Effort:** 1 day
**Status:** ❌ TODO

---

#### 1.3.3 Scale Constraining ⭐⭐ LOW PRIORITY
**Why:** Ensure notes fit in a specific scale (C major, A minor, etc.)

**Tasks:**
- [ ] Create scale definition system
  - [ ] Define common scales (Major, Minor, Dorian, Mixolydian, etc.)
  - [ ] Store as intervals from root note
- [ ] Add View → Scale Highlight
  - [ ] Dropdown to select scale + root note
  - [ ] Highlight scale notes in piano roll (green background)
  - [ ] Dim non-scale notes (gray background)
- [ ] Add Edit → Snap to Scale
  - [ ] Moves non-scale notes to nearest scale note
  - [ ] Create SnapToScaleCommand for undo

**Scale Definitions:**
```cpp
struct Scale {
    std::string name;
    std::vector<int> intervals;  // Semitones from root
};

const std::vector<Scale> SCALES = {
    {"Major", {0, 2, 4, 5, 7, 9, 11}},
    {"Natural Minor", {0, 2, 3, 5, 7, 8, 10}},
    {"Harmonic Minor", {0, 2, 3, 5, 7, 8, 11}},
    {"Dorian", {0, 2, 3, 5, 7, 9, 10}},
    {"Mixolydian", {0, 2, 4, 5, 7, 9, 10}},
    {"Pentatonic Major", {0, 2, 4, 7, 9}},
    {"Pentatonic Minor", {0, 3, 5, 7, 10}},
};

bool IsInScale(uint8_t pitch, uint8_t rootNote, const Scale& scale)
{
    int semitone = (pitch - rootNote) % 12;
    return std::find(scale.intervals.begin(), scale.intervals.end(), semitone) != scale.intervals.end();
}
```

**Estimated Effort:** 2 days
**Status:** ❌ TODO

---

#### 1.3.4 Chord Helper (Optional) ⭐ LOW PRIORITY
**Why:** Quick chord entry for composition

**Tasks:**
- [ ] Add "Insert Chord" dialog
  - [ ] Text input for chord name (e.g., "Cmaj7", "Am", "G7")
  - [ ] Position (tick) to insert
  - [ ] Duration
  - [ ] Velocity
- [ ] Parse chord notation
  - [ ] Major, minor, 7th, maj7, dim, aug, sus, etc.
- [ ] Generate notes for chord
- [ ] Insert via AddNoteCommand (to preserve undo)

**Chord Parser Example:**
```cpp
struct Chord {
    std::string name;
    std::vector<int> intervals;
};

Chord ParseChord(const std::string& chordName)
{
    // Parse chord like "Cmaj7" → root=C, type=maj7
    // Return intervals: major 7th = {0, 4, 7, 11}
}
```

**Estimated Effort:** 2 days
**Status:** ❌ TODO (Optional)

---

### Version 1.3 Summary

**Total Tasks:** 3-4 features (Humanize, Legato/Staccato, Scale tools, Chord helper)
**Estimated Effort:** 4-6 days of focused work
**Target:** 2-3 weeks (part-time)

**Success Criteria:**
- [ ] Can humanize MIDI for realistic feel
- [ ] Can apply legato/staccato articulations
- [ ] Can visualize and constrain to musical scales
- [ ] (Optional) Can insert chords quickly

**Next Milestone:** Once v1.3 is complete, move to v2.0 for professional integration features.

---

## Version 2.0 - "Professional Integration" 🎯 🎉

**Goal:** "MidiWorks integrates seamlessly with my professional audio production workflow."

**Target Date:** 3-4 months from v1.0

### Professional Features

#### 2.0.1 VST/VST3 Plugin Support ⭐⭐⭐ MAJOR FEATURE
**Why:** Use professional synthesizers and effects

**WARNING:** This is a MAJOR feature - likely 2-4 weeks of work

**Tasks:**
- [ ] Research VST SDK
  - [ ] VST3 SDK (recommended - more modern)
  - [ ] Consider JUCE framework (handles VST hosting complexity)
- [ ] Design plugin architecture
  - [ ] Plugin scanner (find VST .dll/.vst3 files)
  - [ ] Plugin manager (load/unload plugins)
  - [ ] Plugin window hosting (show plugin UI)
- [ ] Implement VST hosting
  - [ ] Load VST plugin
  - [ ] Send MIDI events to plugin
  - [ ] Receive audio from plugin
  - [ ] Render audio to WAV/MP3 (if adding audio support)
- [ ] Add plugin UI
  - [ ] Plugin selector per channel
  - [ ] "Open Plugin UI" button
  - [ ] Plugin preset management
- [ ] Save/load plugin settings in project

**Decision Point:** Audio Rendering?
- VST plugins output audio, not MIDI
- Need to add audio rendering capability
- This is a HUGE scope increase

**Recommendation:**
- **Phase 1:** VST for MIDI preview only (hear better sounds while composing)
- **Phase 2:** Add audio rendering and export to WAV

**Estimated Effort:** 3-4 weeks (using JUCE), or defer to v3.0
**Status:** ❌ TODO (MAJOR)

---

#### 2.0.2 MIDI CC Automation ⭐⭐⭐ HIGH PRIORITY
**Why:** Control modulation, expression, sustain pedal, etc.

**Tasks:**
- [ ] Design automation lanes UI
  - [ ] Show CC lanes below piano roll
  - [ ] Dropdown to select CC type (Modulation=1, Volume=7, Pan=10, Expression=11, etc.)
  - [ ] Draw automation curve with breakpoints
- [ ] Add CC events to tracks
  - [ ] Store as TimedMidiEvent with CONTROL_CHANGE type
  - [ ] Sort with note events
- [ ] Implement automation editing
  - [ ] Add automation points (click to add)
  - [ ] Move automation points (drag)
  - [ ] Delete automation points (right-click or Delete key)
  - [ ] Linear interpolation between points
- [ ] Create automation commands for undo
- [ ] Playback automation
  - [ ] Send CC messages during playback
  - [ ] Interpolate values between points
- [ ] Save/load automation in project

**Common MIDI CCs:**
- CC 1: Modulation
- CC 7: Volume
- CC 10: Pan
- CC 11: Expression
- CC 64: Sustain Pedal (on/off)
- CC 91: Reverb
- CC 93: Chorus

**Estimated Effort:** 3-4 days
**Status:** ❌ TODO

---

#### 2.0.3 Tempo Track (Tempo & Time Signature Changes) ⭐⭐⭐ HIGH PRIORITY
**Why:** Support real-world compositions with tempo/meter changes (ritardando, accelerando, complex meters)

**Motivation:**
Testing with imported MIDI files revealed that most professional compositions have tempo and time signature changes throughout the song. Currently, MidiWorks only supports a single global tempo and time signature, which limits both composition flexibility and accurate MIDI import.

**Note:** ✅ Global time signature control is **complete** (see v1.2.5). This section covers adding **multiple** tempo/time signature events at different ticks throughout the song.

**Key Insight:** A tempo track is conceptually similar to MIDI tracks - it's a collection of timed events at specific ticks. The difference is that it stores song structure metadata (tempo, time signature) instead of note on/off messages.

**Architecture Decision:**
Store tempo track in `Transport` class (not `TrackSet`) because:
- Transport already owns timing/tempo logic
- Can reuse existing `Transport::BeatSettings` struct
- Transport naturally queries tempo during `UpdatePlayBack()`
- Cleaner separation: TrackSet = note data, Transport = timing data

**Data Structure:**
```cpp
// Transport.h
struct TempoTrackEvent {
    uint64_t tick;
    BeatSettings settings;  // Reuse existing struct (tempo, timeSignatureNumerator, timeSignatureDenominator)
};

class Transport {
private:
    std::vector<TempoTrackEvent> mTempoTrack;  // Sorted by tick
    BeatSettings mCurrentBeatSettings;         // Cached active settings

public:
    // Query tempo at specific tick
    BeatSettings GetBeatSettingsAtTick(uint64_t tick) const;

    // Add/remove tempo events
    void AddTempoEvent(uint64_t tick, const BeatSettings& settings);
    void RemoveTempoEvent(uint64_t tick);

    // Get all events for UI display
    const std::vector<TempoTrackEvent>& GetTempoTrack() const;
};
```

**Implementation Tasks:**

**Phase 1: Core Data Structure (1 day)**
- [ ] Add `TempoTrackEvent` struct to Transport.h
- [ ] Add `mTempoTrack` vector to Transport class
- [ ] Implement `GetBeatSettingsAtTick()` - binary search to find active tempo
- [ ] Implement `AddTempoEvent()` and `RemoveTempoEvent()`
- [ ] Initialize with default tempo event at tick 0 on project creation

**Phase 2: Playback Integration (1 day)**
- [ ] Update `Transport::UpdatePlayBack()` to query tempo track
  - [ ] Check if current tick crossed a tempo event
  - [ ] Update `mCurrentBeatSettings` when tempo changes
  - [ ] Recalculate tick-to-time conversion with new tempo
- [ ] Handle tempo changes during playback (don't snap playback position)
- [ ] Test with gradually changing tempos (ritardando/accelerando)

**Phase 3: MIDI Import/Export (1 day)**
- [ ] Update `ProjectManager::ImportMIDI()` to populate tempo track
  - [ ] Currently extracts first tempo event only (line 437-450)
  - [ ] Change to store ALL tempo events in tempo track
  - [ ] Extract ALL time signature events
  - [ ] Store as `TempoTrackEvent` at correct ticks (with PPQN conversion)
- [ ] Update `ProjectManager::ExportMIDI()` to write tempo track
  - [ ] Export tempo events as MIDI meta messages (type 0x51)
  - [ ] Export time signature events as MIDI meta messages (type 0x58)
  - [ ] Write to track 0 as per MIDI standard

**Phase 4: Project Save/Load (0.5 days)**
- [ ] Update `ProjectManager::SaveProject()` JSON format
  - [ ] Add "tempoTrack" array to JSON
  - [ ] Store tick and BeatSettings for each event
- [ ] Update `ProjectManager::LoadProject()` to restore tempo track
- [ ] Handle backwards compatibility (if no tempo track, use single global tempo)

**Phase 5: UI - Canvas Grid Display (1 day)**
- [ ] Update `MidiCanvas` to query tempo track for grid drawing
  - [ ] Call `Transport::GetBeatSettingsAtTick()` for visible region
  - [ ] Draw grid with correct time signature (3/4 shows 3 beats, 4/4 shows 4)
  - [ ] Update bar lines to match time signature
  - [ ] Show visual indicator when time signature changes mid-canvas
- [ ] Update measure calculation to account for variable time signatures

**Phase 6: UI - Tempo Lane Visualization (1-2 days)**
- [ ] Add tempo lane above piano roll (similar to automation lanes)
- [ ] Draw tempo curve showing BPM over time
- [ ] Draw time signature markers at change points
- [ ] Color-code regions by time signature for visual clarity

**Phase 7: UI - Tempo Editing (1-2 days)**
- [ ] Add tempo event editing UI
  - [ ] Right-click timeline → "Add Tempo Change"
  - [ ] Dialog: Set BPM and time signature
  - [ ] Click tempo marker to edit or delete
- [ ] Create `TempoChangeCommand` for undo support
  - [ ] Store old and new tempo events
  - [ ] Execute/Undo updates tempo track
- [ ] Add keyboard shortcuts (Ctrl+T = Add Tempo Change)

**Phase 8: Metronome Integration (0.5 days)**
- [ ] Update `MetronomeService` to query tempo track
- [ ] Update beat detection to handle variable time signatures
- [ ] Ensure downbeat detection works across time signature changes

**Advanced Features (Optional - Phase 9):**
- [ ] Tempo ramping (gradual tempo changes)
  - [ ] Add `TempoRampType` enum (None, Linear, Exponential)
  - [ ] Linear interpolation between tempo points
  - [ ] Draw tempo curve instead of stepped changes
- [ ] Tempo tap (tap keyboard to set tempo)
- [ ] Common tempo presets (Largo, Andante, Allegro, Presto, etc.)
- [ ] Visual tempo indicator during playback (flashing on beat)

**Example Usage:**
```cpp
// User imports MIDI file with tempo changes
ProjectManager::ImportMIDI("symphony.mid");

// Tempo track is populated:
// Tick 0:     120 BPM, 4/4
// Tick 3840:  140 BPM, 4/4  (accelerando)
// Tick 7680:  140 BPM, 3/4  (time signature change to waltz)
// Tick 11520: 100 BPM, 3/4  (ritardando)

// During playback, Transport automatically switches tempos
// Canvas grid updates to show 3/4 meter starting at tick 7680
// Metronome plays 3 beats per measure in waltz section
```

**Files to Modify:**
- `src/AppModel/Transport/Transport.h` - Add TempoTrackEvent, tempo track vector, query methods
- `src/AppModel/Transport/Transport.cpp` - Implement tempo track logic and playback integration
- `src/AppModel/ProjectManager/ProjectManager.cpp` - Update Import/Export/Save/Load
- `src/Panels/MidiCanvas/MidiCanvas.cpp` - Update grid rendering
- `src/Commands/TempoChangeCommand.h` - New command for undo support
- `src/AppModel/MetronomeService/MetronomeService.cpp` - Query tempo track for beat detection

**Testing Checklist:**
- [ ] Create project with multiple tempo changes, verify playback speed changes
- [ ] Create project with time signature changes (4/4 → 3/4 → 6/8), verify grid display
- [ ] Import MIDI file with tempo changes, verify tempo track populated correctly
- [ ] Export project with tempo track, verify MIDI file has correct tempo meta events
- [ ] Undo/redo tempo changes, verify tempo track state
- [ ] Save/load project with tempo track, verify persistence
- [ ] Test edge cases:
  - [ ] Tempo change at tick 0 (start of song)
  - [ ] Multiple tempo changes in rapid succession
  - [ ] Tempo change during loop playback
  - [ ] Very fast tempo (200+ BPM) and very slow tempo (40 BPM)

**Estimated Effort:** 6-8 days (full implementation with UI)
- Core + Playback + Import/Export + Save/Load: 3-4 days
- UI (visualization + editing): 3-4 days

**Status:** ❌ TODO

**Related:**
- MIDI Import already extracts tempo/time signature events (partial support exists)
- Grid rendering needs to become tempo-aware
- This unlocks accurate playback of imported classical/film score MIDI files

---

#### 2.0.5 Markers & Sections ⭐⭐ MEDIUM PRIORITY
**Why:** Navigate and organize song structure

**Tasks:**
- [ ] Add marker system
  - [ ] Create/delete markers at any tick
  - [ ] Name markers ("Intro", "Verse 1", "Chorus", etc.)
  - [ ] Show markers in timeline
- [ ] Add section/region system
  - [ ] Define regions (start tick, end tick, name)
  - [ ] Show regions as colored bars above timeline
- [ ] Add marker navigation
  - [ ] Jump to next/previous marker (keyboard shortcuts)
  - [ ] Click marker to seek
- [ ] Save/load markers in project

**Estimated Effort:** 2 days
**Status:** ❌ TODO

---

#### 2.0.6 MIDI Bank Selection ⭐ LOW PRIORITY
**Why:** Access hundreds of instrument variations in soundfonts (Timbres of Heaven, etc.)

**Tasks:**
- [ ] Add bank number to MidiChannel struct
  - [ ] `ubyte bankMSB = 0;` (CC 0 - Bank Select MSB)
  - [ ] `ubyte bankLSB = 0;` (CC 32 - Bank Select LSB, usually 0)
- [ ] Add bank selector UI to ChannelControls
  - [ ] wxSpinCtrl or wxChoice for bank number (0-127)
  - [ ] Place above or next to program change dropdown
  - [ ] Label: "Bank:" with current bank number
- [ ] Update program change logic
  - [ ] Send bank select messages BEFORE program change
  - [ ] Order: CC 0 (MSB) → CC 32 (LSB) → Program Change
- [ ] Save/load bank numbers in project file
  - [ ] Add to JSON serialization in ProjectManager
- [ ] Add bank presets (optional)
  - [ ] Dropdown with common banks: "GM (0)", "Room Kits (8)", "Power Kits (16)", etc.
  - [ ] Or just use numeric spinner for flexibility

**MIDI Bank Select Messages:**
```cpp
// Example: Select Bank 8, Program 0 (Room Standard Kit)
void SoundBank::SendBankAndProgram(ubyte channel)
{
    MidiChannel& ch = mChannels[channel];

    // 1. Bank Select MSB (CC 0)
    auto bankMSB = MidiMessage::ControlChange(0, ch.bankMSB, channel);
    mMidiOut->sendMessage(bankMSB);

    // 2. Bank Select LSB (CC 32) - usually 0
    auto bankLSB = MidiMessage::ControlChange(32, ch.bankLSB, channel);
    mMidiOut->sendMessage(bankLSB);

    // 3. Program Change
    auto pc = MidiMessage::ProgramChange(ch.programNumber, channel);
    mMidiOut->sendMessage(pc);
}
```

**Common Soundfont Banks (Timbres of Heaven):**

**Drums (Channel 10):**
- Bank 0: Standard Kits
- Bank 8: Room Kits
- Bank 16: Power Kits
- Bank 24: Electronic Kits
- Bank 25: TR-808 Kits
- Bank 32: Jazz Kits
- Bank 40: Brush Kits
- Bank 48: Orchestra Kits
- Bank 56: SFX Kits
- Bank 127: Sound Effects

**Melodic Channels:**
- Bank 0: GM Standard (128 instruments)
- Bank 1-127: Variations (different pianos, guitars, synths, etc.)

**UI Mockup:**
```
┌─ Channel 1 ──────────────────┐
│ Bank: [0   ▼]                 │
│ Program: [Acoustic Grand ▼]   │
│ Volume: [||||||||||||] 100    │
│ [M] [S] [R]                   │
└───────────────────────────────┘
```

**Implementation Notes:**
- Bank select only matters if soundfont supports multiple banks
- Standard GM devices (bank 0) ignore bank select messages - safe to send
- Some devices use MSB only, some use both MSB+LSB
- Send bank select every time program changes (some devices reset bank after PC)

**Testing:**
- [ ] Test with Timbres of Heaven soundfont
- [ ] Verify bank switching works (different piano sounds, drum kits, etc.)
- [ ] Test with standard GM synth (should ignore bank select gracefully)
- [ ] Verify bank numbers persist in save/load

**Estimated Effort:** 1-2 days
**Status:** ❌ TODO (Low Priority - Nice to Have)

---

#### 2.0.7 Performance Optimization ⭐⭐ MEDIUM PRIORITY
**Why:** Smooth performance with 1000+ notes

**Tasks:**
- [ ] Profile with large projects
  - [ ] Create test project with 5000+ notes
  - [ ] Measure FPS, frame time, input latency
- [ ] Optimize piano roll rendering
  - [ ] Viewport culling (only draw visible notes)
  - [ ] Spatial hash or quad-tree for fast lookup
  - [ ] Batch draw calls
- [ ] Optimize note finding
  - [ ] Spatial indexing for FindNoteAtPosition
  - [ ] Cache note bounds
- [ ] Optimize track sorting
  - [ ] Binary search insertion instead of full sort
  - [ ] Only sort affected ranges

**Estimated Effort:** 3-4 days (after profiling)
**Status:** ❌ TODO

---

### Version 2.0 Summary

**Total Tasks:** 7 major features (VST, CC automation, Tempo automation, Time sig changes, Markers, Bank selection, Performance)
**Estimated Effort:** 2-4 weeks of focused work (depends on VST scope)
**Target:** 4-6 weeks (part-time)

**Success Criteria:**
- [ ] Can use VST plugins for better sounds (or defer to v3.0)
- [ ] Can automate modulation, expression, etc.
- [ ] Can create tempo changes and ritardando
- [ ] Can organize projects with markers
- [ ] Performance is smooth with large projects (1000+ notes)
- [ ] MidiWorks feels professional and polished

**🎉 VERSION 2.0 COMPLETE! 🎉**

---

## Progress Tracking

### Overall Progress: 33% (Making Great Progress!)

| Version | Status | Progress | Completion Date |
|---------|--------|----------|-----------------|
| v1.0 | ✅ COMPLETE | 100% | December 2025 |
| v1.1 | 🔄 IN PROGRESS | 67% (4/6) | Target: January 2026 |
| v1.2 | 🔄 IN PROGRESS | 64% (4.5/7) | Target: February 2026 |
| v1.3 | ❌ TODO | 0% | Target: March 2026 |
| v2.0 | ❌ TODO | 0% | Target: April 2026 |

### v1.1 Progress: 4/6 Complete (67%)

- [ ] Error Handling
- [x] Track Naming ✅
- [x] Track Colors ✅
- [x] Show/Hide Tracks (Minimize) ✅
- [ ] Clear Track
- [x] MIDI File Export ✅ (January 2026)

### v1.2 Progress: 4.5/7 Complete (64%)

- [x] MIDI File Import ✅ (January 2026)
- [ ] Multi-Note Resize
- [x] Velocity Editing 🔄 (Single-note complete, multi-note TODO - January 2026)
- [ ] Transpose
- [ ] Timeline Improvements
- [x] Global Time Signature Control ✅ (January 2026)
- [x] Solo Visual Filtering ✅ (January 2026)

### v1.3 Progress: 0/4 Complete (0%)

- [ ] Humanize
- [ ] Legato/Staccato
- [ ] Scale Constraining
- [ ] Chord Helper (optional)

### v2.0 Progress: 0/7 Complete (0%)

- [ ] VST Support (optional - defer to v3.0?)
- [ ] MIDI CC Automation
- [ ] Tempo Automation
- [ ] Time Signature Changes
- [ ] Markers & Sections
- [ ] Bank Selection (low priority)
- [ ] Performance Optimization

---

## Quick Wins (Start Here!)

If you want immediate results, tackle these first:

1. ~~**Track Naming** (0.5-1 day) - Instant usability boost~~ ✅ **COMPLETE!**
2. ~~**MIDI File Export** (1-2 days) - Share your work!~~ ✅ **COMPLETE!**
3. **Error Handling** (1-2 days) - No more crashes
4. **Transpose** (1 day) - Common composition need

**Progress: 2/4 complete!** ✅ MIDI Import/Export and Track Organization now available!

**Remaining: 2-3 days = 1 week** for even more improvement!

---

## Staying Motivated

### Celebrate Milestones
- ✅ v1.1 Complete → Share a .mid export on social media!
- ✅ v1.2 Complete → Compose a song using velocity editing
- ✅ v1.3 Complete → Use humanize to make a realistic performance
- ✅ v2.0 Complete → You have a professional DAW! 🎉

### Use Your Own Tool
- Compose with MidiWorks every week
- Note what frustrates you
- Fix the most painful issues first

### Focus on Value
- Each feature should make composition easier
- Don't over-engineer
- Ship > Perfect

### One Feature at a Time
- Don't start new feature until current one works
- Finish > Perfect

---

## Final Thoughts

**You've already proven you can do this.** You built v1.0 from scratch with:
- Complete piano roll editing
- Multi-selection and clipboard
- Undo/redo system
- Loop recording with overdub
- Quantize
- Save/load

**v2.0 is just more of the same:** One feature at a time, test frequently, ship when ready.

**You're not building a DAW. You're already USING a DAW.** Now you're just making it better.

**Timeline Reality Check:**
- **Aggressive:** 2-3 months (full-time focus)
- **Realistic:** 4-6 months (part-time, with other commitments)
- **Comfortable:** 6-12 months (steady progress, no burnout)

Choose your pace. The journey is the fun part. 🎵

**Now go make v1.1 happen!** Start with Track Naming - it's quick, easy, and will make you smile every time you see "Piano" instead of "Track 0". 😊

---

*Made with ❤️ for you, the composer who just achieved v1.0 MVP. You've got this!*
