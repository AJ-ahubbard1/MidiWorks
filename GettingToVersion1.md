# Getting to Version 1.0 - MVP Roadmap

**Goal:** Ship a usable DAW that makes MIDI composition easier than existing tools, as fast as possible.

**Target User:** You (and composers like you) who want a simple, focused tool for sketching musical ideas quickly.

**MVP Definition:** "I can compose, edit, and save a complete MIDI song without frustration."

---

## Current State (v0.1)

**What Works:**
- ✅ MIDI input/output
- ✅ Basic recording (16 tracks)
- ✅ Basic playback
- ✅ Transport controls
- ✅ Channel mixer (patch, volume, mute, solo)
- ✅ Piano roll visualization (view-only)
- ✅ Dockable panel system
- ✅ Basic metronome with adjustable tempo

**What's Missing:**
- ❌ Can't edit notes (add/move/delete)
- ❌ Can't save/load projects
- ❌ No quantize
- ❌ No undo/redo
- ❌ Can't loop sections

---

## Phase 1: Critical MVP Features
**Goal:** "I can record a simple song, fix mistakes, and save it"

### 1.1 Piano Roll Editing ⭐⭐⭐ HIGHEST PRIORITY
Without this, you can't fix mistakes or compose directly in the DAW.

**Tasks:**
- [ ] **Add notes with mouse click** (place note at cursor position)
- [ ] **Delete notes with right-click** or Delete key
- [ ] **Move notes with drag-and-drop** (change pitch/time)
- [ ] **Resize notes** (change duration by dragging note end)
- [ ] **Select multiple notes** (rectangle selection with mouse drag)
- [ ] **Delete selected notes** (Delete key)
- [ ] **Move selected notes** (drag selection to new position)

**Why Critical:** Can't compose or fix mistakes without editing notes.

**Estimated Effort:** 3-5 days (most complex feature)

**Implementation Notes:**
- Track mouse state: idle, adding, selecting, dragging, resizing
- Convert screen coordinates ↔ tick/pitch
- Add notes to TrackSet when modified
- Consider making edit operations add to undo stack (if undo implemented)

---

### 1.2 Save/Load Projects ⭐⭐⭐ HIGHEST PRIORITY
Without this, all work is lost on close.

**Tasks:**
- [ ] **Define project file format** (suggest JSON for simplicity)
  - Track data (notes with tick, pitch, velocity, duration)
  - Tempo
  - Channel settings (program, volume, mute state)
  - Project metadata (name, created date)
- [ ] **Implement Save** (File → Save / Save As)
- [ ] **Implement Load** (File → Open)
- [ ] **New Project** (File → New, clears all tracks)
- [ ] **Mark project as "dirty"** when edited (add * to title bar)
- [ ] **Prompt to save on close** if unsaved changes

**Why Critical:** Can't compose anything serious if you lose work.

**Estimated Effort:** 2-3 days

**Implementation Notes:**
- Use JSON library (nlohmann/json or similar)
- File extension: `.mwp` (MidiWorks Project)
- Save to `Documents/MidiWorks/` by default
- Consider auto-save every 5 minutes (simple background timer)

**Example JSON Structure:**
```json
{
  "version": "1.0",
  "tempo": 120,
  "timeSignature": [4, 4],
  "tracks": [
    {
      "channel": 0,
      "program": 0,
      "volume": 100,
      "mute": false,
      "solo": false,
      "notes": [
        {"tick": 0, "pitch": 60, "velocity": 100, "duration": 480},
        {"tick": 480, "pitch": 62, "velocity": 90, "duration": 480}
      ]
    }
  ]
}
```

---

### 1.3 Metronome ⭐⭐⭐ HIGHEST PRIORITY
Can't record on beat without this.

**Tasks:**
- [ ] **Reserve Channel 16 for metronome** (as discussed)
- [ ] **Add beat detection to Transport** (CheckForBeat method)
- [ ] **Send click on beats** (downbeat = loud, other beats = quiet)
- [ ] **Add metronome enable/disable toggle** (in TransportPanel or settings)
- [ ] **Set metronome sound on init** (Program Change to side stick)

**Why Critical:** Recording without metronome is frustrating and leads to off-beat notes.

**Estimated Effort:** 1 day

**Implementation Notes:**
- Use MIDI notes 37 (side stick) for downbeat, 31 (sticks) for other beats
- Velocity: 127 for downbeat, 80 for beats
- Initially always on, add toggle later

---

### 1.4 Undo/Redo ⭐⭐ HIGH PRIORITY
Editing without undo is painful.

**Tasks:**
- [ ] **Design command pattern** (abstract Command class)
- [ ] **Implement command stack** (undo/redo stacks in AppModel)
- [ ] **Create commands for edit operations:**
  - AddNoteCommand
  - DeleteNoteCommand
  - MoveNoteCommand
  - ResizeNoteCommand
- [ ] **Keyboard shortcuts** (Ctrl+Z undo, Ctrl+Y or Ctrl+Shift+Z redo)
- [ ] **Limit stack size** (e.g., last 50 actions to prevent memory bloat)

**Why High Priority:** Makes editing forgiving and fast. Encourages experimentation.

**Estimated Effort:** 2-3 days

**Implementation Notes:**
- Command pattern: each edit operation is a Command object with Execute() and Undo()
- Push commands onto stack when executed
- Pop and call Undo() when user presses Ctrl+Z
- Clear redo stack when new command executed after undo

**Example:**
```cpp
class Command {
public:
    virtual void Execute() = 0;
    virtual void Undo() = 0;
};

class AddNoteCommand : public Command {
    Track& mTrack;
    TimedMidiEvent mNote;
public:
    void Execute() override { mTrack.push_back(mNote); }
    void Undo() override { /* remove mNote from mTrack */ }
};
```

---

### 1.5 Loop Playback ⭐⭐ HIGH PRIORITY
Essential for composition workflow - listen to a section repeatedly while tweaking.

**Tasks:**
- [ ] **Add loop start/end markers** (stored in Transport)
- [ ] **UI for setting loop region:**
  - Click-drag in timeline to select region
  - Or: text boxes for manual tick input
  - Display loop region visually (shaded area)
- [ ] **Loop playback logic:**
  - When playback reaches loop end, jump to loop start
  - Continue until user stops
- [ ] **Toggle loop on/off** (button in TransportPanel)

**Why High Priority:** Composing requires iterating on sections. Loop is faster than manually rewinding.

**Estimated Effort:** 1-2 days

**Implementation Notes:**
- Add `mLoopStart`, `mLoopEnd`, `mLoopEnabled` to Transport
- In `Transport::UpdatePlayBack()`, check if current tick >= loop end
- If so, set current tick to loop start
- Visual: draw loop region in MidiCanvasPanel and/or TransportPanel timeline

---

## Phase 2: Important MVP Features
**Goal:** "I can compose efficiently with good workflow"

### 2.1 Quantize ⭐⭐ HIGH PRIORITY
Fix timing mistakes instantly.

**Tasks:**
- [ ] **Implement quantize algorithm:**
  - For each note, round tick to nearest grid value
  - Grid options: 1/4, 1/8, 1/16, 1/32, 1/64 notes
- [ ] **UI for quantize:**
  - Button in toolbar or Edit menu
  - Dropdown for grid size
- [ ] **Apply to selected notes** (or all notes if none selected)
- [ ] **Make quantize undoable** (create QuantizeCommand)

**Why Important:** Recording with perfect timing is hard. Quantize fixes it instantly.

**Estimated Effort:** 1 day

**Implementation Notes:**
```cpp
void Quantize(Track& track, uint64_t gridSize) {
    for (auto& event : track) {
        uint64_t tick = event.tick;
        uint64_t quantized = ((tick + gridSize/2) / gridSize) * gridSize;
        event.tick = quantized;
    }
    std::sort(track.begin(), track.end(), /*by tick*/);
}
```

---

### 2.2 Tempo Control ⭐⭐ HIGH PRIORITY
120 BPM is limiting.

**Tasks:**
- [ ] **Add tempo field to Transport** (currently hardcoded)
- [ ] **UI for tempo:**
  - Numeric text box in TransportPanel
  - Or: tempo slider (60-240 BPM)
- [ ] **Update tick calculations** when tempo changes
- [ ] **Save/load tempo** in project file

**Why Important:** Different genres need different tempos. 120 BPM doesn't fit everything.

**Estimated Effort:** 0.5 days (simple change)

---

### 2.3 Copy/Paste Notes ⭐⭐ MEDIUM PRIORITY
Reuse musical phrases efficiently.

**Tasks:**
- [ ] **Implement clipboard for notes** (vector of selected notes)
- [ ] **Copy (Ctrl+C):** Store selected notes in clipboard
- [ ] **Paste (Ctrl+V):** Insert clipboard notes at current playback position
  - Or: at mouse cursor position in piano roll
- [ ] **Paste maintains relative timing** (offset from first note)
- [ ] **Make paste undoable** (PasteCommand)

**Why Important:** Composition often involves repeating patterns. Copy/paste is faster than re-recording.

**Estimated Effort:** 1 day

---

### 2.4 Snap to Grid ⭐ MEDIUM PRIORITY
Makes note placement precise.

**Tasks:**
- [ ] **Add snap-to-grid toggle** (button or keyboard shortcut)
- [ ] **When enabled, round all edit operations to grid:**
  - Note placement rounds to nearest grid line
  - Note dragging snaps to grid
  - Note resizing snaps to grid
- [ ] **Grid size selector** (1/4, 1/8, 1/16, etc.)
- [ ] **Visual grid lines** in piano roll (already might exist?)

**Why Important:** Placing notes precisely by hand is hard. Snap-to-grid ensures rhythmic accuracy.

**Estimated Effort:** 1 day

---

### 2.5 Keyboard Shortcuts ⭐ MEDIUM PRIORITY
Speed up workflow dramatically.

**Tasks:**
- [ ] **Implement common shortcuts:**
  - `Space` - Play/Pause toggle
  - `Ctrl+S` - Save
  - `Ctrl+O` - Open
  - `Ctrl+N` - New Project
  - `Ctrl+Z` - Undo
  - `Ctrl+Y` - Redo
  - `Ctrl+C` - Copy
  - `Ctrl+V` - Paste
  - `Delete` - Delete selected notes
  - `Ctrl+A` - Select all notes
  - `Home` - Jump to start
  - `L` - Toggle loop
- [ ] **Display shortcuts in menus** (e.g., "Save    Ctrl+S")

**Why Important:** Mouse-only workflow is slow. Shortcuts make power users efficient.

**Estimated Effort:** 1 day (mostly wxWidgets accelerator table setup)

---

## Phase 3: Polish & Usability
**Goal:** "The DAW feels professional and pleasant to use"

### 3.1 Visual Feedback Improvements
- [ ] **Show notes while recording** (add to track in real-time, not just on finalize)
- [ ] **Highlight selected notes** (different color/border)
- [ ] **Show loop region** (shaded background in piano roll)
- [ ] **Playhead cursor** in piano roll (vertical line at current tick)
- [ ] **Note preview on hover** (show pitch/duration in tooltip)
- [ ] **Grid lines** in piano roll (faint lines at beat boundaries)

**Estimated Effort:** 2-3 days

---

### 3.2 Better Transport Controls
- [ ] **Timeline ruler** showing measures/beats (not just ticks)
- [ ] **Click timeline to seek** (jump playback position)
- [ ] **Scrubbing** (drag playhead to hear audio while dragging)
- [ ] **Display tempo in transport panel**
- [ ] **Display time signature** (hardcoded to 4/4 for now)

**Estimated Effort:** 2 days

---

### 3.3 Track Management
- [ ] **Track naming** (give tracks descriptive names)
- [ ] **Track colors** (visual organization)
- [ ] **Show/hide tracks** (focus on specific instruments)
- [ ] **Clear track** button (delete all notes on a track)

**Estimated Effort:** 1-2 days

---

### 3.4 Velocity Editing
- [ ] **Select notes and adjust velocity** (numeric input or slider)
- [ ] **Velocity lanes** in piano roll (display/edit velocity per note)
- [ ] **Velocity curves** (crescendo/diminuendo for selected notes)

**Estimated Effort:** 2 days

---

### 3.5 MIDI File Import/Export
- [ ] **Export to Standard MIDI File (.mid)**
- [ ] **Import Standard MIDI File** (load into tracks)
- [ ] **Export individual tracks** (stem export)

**Why Important:** Share with other musicians, import drum loops, export to other DAWs.

**Estimated Effort:** 2-3 days (MIDI file format is complex but libraries exist)

---

## Phase 4: Technical Debt (Post-MVP)
**These can wait until after v1.0 ships**

### 4.1 Callback-Based MIDI Input
- [ ] Switch from polling to RtMidi callbacks
- [ ] Add mutex for thread safety
- [ ] Test latency improvement

**Estimated Effort:** 1-2 days

---

### 4.2 Proper Encapsulation
- [ ] Make Transport state private with getter/setters
- [ ] Add state validation
- [ ] Encapsulate AppModel public members

**Estimated Effort:** 2-3 days (refactor existing code)

---

### 4.3 Error Handling
- [ ] Handle MIDI device disconnect gracefully
- [ ] Handle file I/O errors (disk full, permissions)
- [ ] Add error dialogs for user feedback
- [ ] Add logging system for debugging

**Estimated Effort:** 2-3 days

---

### 4.4 Threading
- [ ] Move file I/O to background thread
- [ ] Move MIDI processing off UI thread (if needed)
- [ ] Add progress bars for long operations

**Estimated Effort:** 3-5 days

---

### 4.5 Performance Optimization
- [ ] Profile with large projects (1000+ notes)
- [ ] Optimize piano roll rendering (only draw visible notes)
- [ ] Optimize playback scheduling
- [ ] Consider spatial data structures (quad-tree for note lookup)

**Estimated Effort:** Variable (profile first, optimize bottlenecks)

---

## Recommended Implementation Order

### Sprint 1: "Make It Editable" (1-2 weeks)
1. Piano roll editing (add/delete/move notes)
2. Save/Load projects
3. Undo/Redo

**Goal:** Can compose and save a song.

---

### Sprint 2: "Make It Musical" (1 week)
4. Metronome
5. Quantize
6. Loop playback
7. Tempo control

**Goal:** Composing feels natural and rhythmically accurate.

---

### Sprint 3: "Make It Efficient" (1 week)
8. Copy/Paste
9. Snap to grid
10. Keyboard shortcuts
11. Select all / clear track

**Goal:** Fast workflow for power users.

---

### Sprint 4: "Make It Pretty" (1 week)
12. Visual feedback (selected notes, loop region, playhead)
13. Timeline improvements
14. Track naming/colors
15. Bug fixes and polish

**Goal:** Feels professional and pleasant to use.

---

### Sprint 5: "Make It Shareable" (1 week)
16. MIDI file import/export
17. Velocity editing
18. Documentation and tutorials

**Goal:** Ready for public release.

---

## Total Estimated Time: 5-7 weeks (solo dev)

**Realistic Timeline:**
- Part-time (10-15 hrs/week): ~3-4 months
- Full-time (40 hrs/week): ~5-7 weeks

---

## MVP Success Criteria

You'll know you've reached MVP when you can:
- ✅ Record a 4-track song (bass, chords, melody, drums)
- ✅ Fix timing mistakes with quantize
- ✅ Edit notes that are wrong pitch
- ✅ Copy/paste a chorus section
- ✅ Save and load the project
- ✅ Export to MIDI file to share with others
- ✅ Use keyboard shortcuts for common tasks
- ✅ Work for 1 hour without frustration

---

## What to Skip (For Now)

Don't build these until after MVP ships:
- ❌ VST/plugin support (huge effort, minimal MVP value)
- ❌ Audio rendering (MIDI-only is fine for MVP)
- ❌ Advanced automation (volume/pan curves)
- ❌ Score view (piano roll is sufficient)
- ❌ Collaboration features
- ❌ Cloud sync
- ❌ Mobile app
- ❌ Multiple time signatures per project
- ❌ Tempo automation

**Why skip?** These add months of work but don't enable the core use case: composing MIDI music.

---

## Staying Motivated as a Solo Dev

### Use Your Own Tool
- Compose with MidiWorks every week
- Note what frustrates you
- Fix the most painful issues first

### Ship Early, Ship Often
- Don't wait for perfection
- Release v0.2, v0.3, v0.4 as features land
- Get feedback from real users (friends, online communities)

### Focus on One Feature at a Time
- Don't start new feature until current one works
- Resist feature creep
- Finish > Perfect

### Celebrate Small Wins
- Piano roll editing works? 🎉 That's huge!
- Save/load works? 🎉 You have a real product!
- First exported MIDI file? 🎉 Share it!

### Join the Community
- Share progress on /r/WeAreTheMusicMakers, /r/gamedev, Twitter
- Get feedback and encouragement
- Help others with similar projects

---

## Final Thoughts

**Your advantage as a solo dev:**
- You're your own user - you know exactly what you need
- No meetings, no approvals - just build
- You can iterate fast based on real usage

**Your challenge:**
- Don't over-engineer - ship > perfect
- Fight feature creep - MVP first, then iterate
- Stay motivated - use it, share it, celebrate progress

**You've got this.** The architecture is solid, the vision is clear, and the roadmap is achievable. One feature at a time, and you'll have a usable DAW in 2-3 months.

Now go make composing easier! 🎵
