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
  - Velocity editing, transpose, timeline improvements, MIDI import

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

**Known Limitations:**
- ⚠️ No error handling (crashes on device disconnect, file errors)
- ~~⚠️ No track organization (naming, colors, show/hide)~~ ✅ **COMPLETE!**
- ~~⚠️ No MIDI file import/export (can't share with other DAWs)~~ ✅ **COMPLETE! (January 2026)**
- ⚠️ No velocity editing (all notes same dynamics)
- ⚠️ No transpose functionality
- ⚠️ Timeline is tick-based (hard to navigate)
- ⚠️ No performance optimization (may lag with 1000+ notes)

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

**Implementation Notes:**
```cpp
// AppModel.h
using ErrorCallback = std::function<void(const std::string& title,
                                          const std::string& message,
                                          ErrorLevel level)>;
void SetErrorCallback(ErrorCallback callback);

// In SaveProject/LoadProject
if (!file.is_open()) {
    if (mErrorCallback) {
        mErrorCallback("Save Failed",
                      "Could not open file: " + filepath,
                      ErrorLevel::Error);
    }
    return false;
}
```

**Testing:**
- [ ] Disconnect MIDI device during playback
- [ ] Try to save to read-only directory
- [ ] Try to load corrupt JSON file
- [ ] Try to load file with missing fields
- [ ] Run with no MIDI devices connected

**Estimated Effort:** 1-2 days
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

#### 1.2.2 Velocity Editing ⭐⭐⭐ HIGH PRIORITY
**Why:** Dynamics are essential for expressive, realistic MIDI

**Tasks:**
- [ ] Design velocity editing UI
  - [ ] **Option A:** Velocity lanes below piano roll (like Ableton/FL Studio)
  - [ ] **Option B:** Velocity editor panel (separate dockable panel)
  - [ ] **Option C:** Properties panel showing selected note velocity
  - [ ] **Recommendation:** Start with Option C (simpler), add Option A later
- [ ] Add velocity display for selected notes
  - [ ] Show velocity value (0-127) in status bar or properties panel
  - [ ] wxSpinCtrl or wxSlider to adjust velocity
  - [ ] Update all selected notes when changed
- [ ] Create ChangeVelocityCommand for undo support
- [ ] Add velocity visualization in piano roll
  - [ ] Color intensity based on velocity (darker = louder)
  - [ ] Or: Note height/width variation
- [ ] Add velocity to Quantize panel (optional)
  - [ ] "Quantize Velocity" - round to nearest 10, 16, etc.

**UI Mockup (Properties Panel Approach):**
```
┌─ Selected Note Properties ──────┐
│ Count: 5 notes                   │
│ Velocity: [|||||||||||] 100      │
│ Duration: Mixed                  │
│ [Apply]                          │
└──────────────────────────────────┘
```

**Implementation Notes:**
```cpp
// Commands/ChangeVelocityCommand.h
class ChangeVelocityCommand : public Command {
    struct NoteVelocityChange {
        int trackIndex;
        size_t noteOnIndex;
        uint8_t oldVelocity;
        uint8_t newVelocity;
    };

    std::vector<NoteVelocityChange> mChanges;

    void Execute() override {
        for (auto& change : mChanges) {
            Track& track = mTrackSet.GetTrack(change.trackIndex);
            track[change.noteOnIndex].mm.mData[2] = change.newVelocity;
        }
    }

    void Undo() override {
        for (auto& change : mChanges) {
            Track& track = mTrackSet.GetTrack(change.trackIndex);
            track[change.noteOnIndex].mm.mData[2] = change.oldVelocity;
        }
    }
};
```

**Estimated Effort:** 2-3 days
**Status:** ❌ TODO

---

#### 1.2.3 Transpose ⭐⭐⭐ HIGH PRIORITY
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

#### 1.2.4 Timeline Improvements ⭐⭐ MEDIUM PRIORITY
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

### Version 1.2 Summary

**Total Tasks:** 4 major features (MIDI import, Velocity editing, Transpose, Timeline)
**Estimated Effort:** 7-8 days of focused work
**Target:** 3-4 weeks (part-time)

**Success Criteria:**
- [ ] Can import MIDI files from other sources
- [ ] Can edit note dynamics (velocity)
- [ ] Can transpose melodies to different keys
- [ ] Can navigate projects with measure/beat timeline
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

#### 2.0.3 Tempo Automation ⭐⭐ MEDIUM PRIORITY
**Why:** Ritardando, accelerando, tempo changes

**Tasks:**
- [ ] Add tempo track (separate from MIDI tracks)
- [ ] Design tempo automation UI
  - [ ] Tempo lane showing BPM over time
  - [ ] Breakpoints for tempo changes
- [ ] Implement tempo ramping
  - [ ] Linear interpolation between tempo points
  - [ ] Update playback engine to handle variable tempo
- [ ] Create tempo automation commands
- [ ] Save/load tempo automation
- [ ] Export tempo changes to MIDI file

**Estimated Effort:** 2-3 days
**Status:** ❌ TODO

---

#### 2.0.4 Time Signature Changes ⭐ LOW PRIORITY
**Why:** Support complex compositions with changing meters

**Tasks:**
- [ ] Allow multiple time signatures in a project
- [ ] Add time signature markers
- [ ] Update grid display to reflect time signature
- [ ] Update metronome to handle time signature changes
- [ ] Save/load time signature track

**Estimated Effort:** 2 days
**Status:** ❌ TODO

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

#### 2.0.6 Performance Optimization ⭐⭐ MEDIUM PRIORITY
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

**Total Tasks:** 6 major features (VST, CC automation, Tempo automation, Time sig changes, Markers, Performance)
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

### Overall Progress: 25% (Making Great Progress!)

| Version | Status | Progress | Completion Date |
|---------|--------|----------|-----------------|
| v1.0 | ✅ COMPLETE | 100% | December 2025 |
| v1.1 | 🔄 IN PROGRESS | 67% (4/6) | Target: January 2026 |
| v1.2 | 🔄 IN PROGRESS | 25% (1/4) | Target: February 2026 |
| v1.3 | ❌ TODO | 0% | Target: March 2026 |
| v2.0 | ❌ TODO | 0% | Target: April 2026 |

### v1.1 Progress: 4/6 Complete (67%)

- [ ] Error Handling
- [x] Track Naming ✅
- [x] Track Colors ✅
- [x] Show/Hide Tracks (Minimize) ✅
- [ ] Clear Track
- [x] MIDI File Export ✅ (January 2026)

### v1.2 Progress: 1/4 Complete (25%)

- [x] MIDI File Import ✅ (January 2026)
- [ ] Velocity Editing
- [ ] Transpose
- [ ] Timeline Improvements

### v1.3 Progress: 0/4 Complete (0%)

- [ ] Humanize
- [ ] Legato/Staccato
- [ ] Scale Constraining
- [ ] Chord Helper (optional)

### v2.0 Progress: 0/6 Complete (0%)

- [ ] VST Support (optional - defer to v3.0?)
- [ ] MIDI CC Automation
- [ ] Tempo Automation
- [ ] Time Signature Changes
- [ ] Markers & Sections
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
