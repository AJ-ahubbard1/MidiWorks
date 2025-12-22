# DEVELOPMENT.md

## Project Vision

MidiWorks aims to make MIDI composition as easy and intuitive as possible. The focus is on removing friction from the creative process, providing smart tools that get out of the way, and enabling rapid iteration of musical ideas.

## Core Principles

- **Ease of Use First**: Every feature should reduce complexity, not add it
- **Minimal Clicks**: Common operations should be fast and require minimal input
- **Visual Feedback**: Musicians should see what they're creating in real-time
- **Smart Defaults**: Sensible defaults that work for most use cases
- **Keyboard-Driven**: Shortcuts for power users without sacrificing discoverability

---

## Version 1.0 Features

### Piano Roll Editing
- [x] Add notes with mouse click
- [x] Delete notes (middle-click or Delete key)
- [x] Move notes with drag and drop
- [x] Resize notes (duration editing)
- [x] Multi-selection (Shift+Drag rectangle, Ctrl+A select all)
- [x] Copy/Paste/Cut operations (Ctrl+C/V/X)
- [x] Quantize to grid (Q key) with triplet support
- [x] Grid snap with duration selector (whole/half/quarter/eighth/sixteenth + triplets)
- [x] Visual feedback (note hovering, preview notes, selection highlighting)
- [x] Zoom and pan (mouse wheel, shift+wheel, right-click drag)

### Recording & Playback
- [x] 15-track MIDI recording (channel 16 reserved for metronome)
- [x] Loop playback and recording with visual loop region
- [x] Overdub note merging (automatic note collision handling)
- [x] Metronome with downbeat detection (channel 16, woodblock sound)
- [x] Transport controls with keyboard shortcuts (Spacebar, R)
- [x] Tempo control (40-300 BPM with UI spinbox)
- [x] Time display (ticks and MM:SS:mmm format)
- [x] Auto-scroll during playback
- [x] 960 PPQN (ticks per quarter note) resolution

### Undo/Redo System
- [x] Complete command pattern implementation
- [x] Undo/Redo for all editing operations (Ctrl+Z, Ctrl+Y)
- [x] Undo history panel with command descriptions
- [x] 50-command stack size limit

### Project Management
- [x] Save/load projects (.mwp JSON format)
- [x] File menu with keyboard shortcuts (Ctrl+N/O/S/Shift+S)
- [x] Dirty flag tracking with asterisk in title bar
- [x] Unsaved changes prompt on exit/new/open

### Mixing & Channels
- [x] 16 MIDI channel support (15 usable + 1 metronome)
- [x] Per-channel patch selection (128 General MIDI programs)
- [x] Per-channel volume control (0-127)
- [x] Per-channel mute/solo
- [x] Per-channel record enable
- [x] Solo logic (mutes non-solo channels when solo is active)

### User Interface
- [x] Dockable panel system (wxAuiManager)
- [x] Transport panel with tempo control
- [x] Sound bank/mixer panel (16 channel controls)
- [x] MIDI settings panel
- [x] Piano roll panel with full editing capabilities
- [x] MIDI event log panel
- [x] Undo history panel
- [x] Shortcuts reference panel
- [x] Resizable and rearrangeable panels

### MIDI I/O
- [x] MIDI input device selection
- [x] MIDI output device selection
- [x] Real-time MIDI input routing to record-enabled channels
- [x] MIDI event logging with real-time display

---

## Planned Features

### Phase 1: Core Composition Tools
**Goal: Enable basic but complete composition workflow**

#### Essential Recording Features
- [ ] Metronome/click track
- [ ] Count-in before recording
- [ ] Overdub mode (record on top of existing track)
- [ ] Replace mode (clear and record)
- [ ] Punch in/punch out recording
- [ ] Loop recording with multiple takes

#### Essential Editing Features
- [ ] Piano roll editing
  - [ ] Add/delete notes with mouse
  - [ ] Move notes (drag and drop)
  - [ ] Resize notes (duration)
  - [ ] Select multiple notes (rectangle selection)
  - [ ] Copy/paste/duplicate notes
- [ ] Quantize to grid (1/4, 1/8, 1/16, 1/32 notes)
- [ ] Transpose selected notes
- [ ] Velocity editing for selected notes
- [ ] Undo/redo system

#### Playback Enhancements
- [ ] Loop/cycle region playback
- [ ] Adjustable tempo (BPM control)
- [ ] Time signature support
- [ ] Click-and-drag timeline scrubbing

#### Project Management
- [ ] Save/load project files
- [ ] New project wizard
- [ ] Recent projects list
- [ ] Auto-save

---

### Phase 2: Workflow Improvements
**Goal: Speed up composition with smart tools**

#### Smart Composition Tools
- [ ] Snap-to-grid toggle
- [ ] Chord helper (enter chord name, generates notes)
- [ ] Scale constraining (only allow notes in selected scale)
- [ ] Note preview on hover (play note before placing)
- [ ] MIDI input recording with visual metronome
- [ ] Step sequencer mode

#### Advanced Editing
- [ ] Humanize (randomize timing/velocity)
- [ ] Legato (connect note ends to next note start)
- [ ] Staccato (shorten all notes)
- [ ] Velocity curves (crescendo/diminuendo)
- [ ] Time stretch/compress
- [ ] Split/merge tracks

#### UI/UX Enhancements
- [ ] Keyboard shortcuts for all major actions
- [ ] Customizable keyboard shortcuts
- [ ] Track colors for visual organization
- [ ] Track naming
- [ ] Zoom levels (horizontal and vertical)
- [ ] Mini-map overview of full composition
- [ ] Dark/light theme toggle

#### Mixing Features
- [ ] Pan control per channel
- [ ] Channel groups/buses
- [ ] Quick mute/solo all tracks
- [ ] Volume automation
- [ ] Pan automation

---

### Phase 3: Advanced Features
**Goal: Professional-grade composition capabilities**

#### Advanced Recording
- [ ] MIDI track comping (choose best takes)
- [ ] Multi-track recording
- [ ] MIDI merge (combine multiple takes)
- [ ] Input quantize (quantize while recording)
- [ ] Input transpose (transpose while recording)

#### Advanced Playback
- [ ] Tempo automation/tempo track
- [ ] Time signature changes mid-song
- [ ] Markers and sections (Intro, Verse, Chorus, etc.)
- [ ] Arrangement view (drag sections to reorder)

#### MIDI Effects
- [ ] Arpeggiator
- [ ] Note repeat/echo
- [ ] Random note generator
- [ ] Scale quantizer
- [ ] Chord spreader

#### Export & Import
- [ ] Export to MIDI file (.mid)
- [ ] Import MIDI file
- [ ] Export individual tracks
- [ ] Export audio (with VST support)

#### Advanced Editing
- [ ] MIDI CC automation (modulation, expression, etc.)
- [ ] Pitch bend editing
- [ ] Aftertouch editing
- [ ] Drum map editor (for drum notation)
- [ ] Score view (musical notation)

---

### Phase 4: Power User Features
**Goal: Support complex compositions and integration**

#### Integration
- [ ] VST/VST3 plugin support
- [ ] ReWire support
- [ ] MIDI clock sync (send/receive)
- [ ] OSC (Open Sound Control) support
- [ ] Integration with external controllers (MIDI mapping)

#### Templates & Libraries
- [ ] Project templates
- [ ] MIDI clip library
- [ ] Groove templates (for quantize)
- [ ] Custom scale definitions
- [ ] Chord progression library

#### Collaboration
- [ ] Export/import track stems
- [ ] Session notes/comments
- [ ] Version control integration

#### Performance
- [ ] Background audio rendering
- [ ] Freeze tracks to reduce CPU
- [ ] Optimized playback engine
- [ ] Low-latency mode

---

## Roadmap Timeline

### Current Status: Version 1.0 - MVP Complete! 🎉
**Released:** December 2025

MidiWorks v1.0 provides a complete MIDI composition workflow with:
- ✅ Full piano roll editing (add, move, resize, delete notes)
- ✅ Multi-selection with rectangle drag and Ctrl+A
- ✅ Copy/Paste/Cut operations with clipboard
- ✅ Complete undo/redo system with history panel
- ✅ Metronome with downbeat detection
- ✅ Loop playback and recording with overdub note merging
- ✅ Quantize with triplet and custom tick support
- ✅ Save/load projects (.mwp JSON format)
- ✅ Grid snap and duration selector
- ✅ Tempo control (40-300 BPM)
- ✅ 15-track MIDI recording and playback
- ✅ Professional channel mixer (volume, mute, solo, record)
- ✅ Comprehensive keyboard shortcuts
- ✅ Visual feedback (grid, playhead, loop region, selection)
- ✅ Auto-scroll during playback
- ✅ Zoom and pan navigation

**Goal Achieved:** Composers can create, edit, and save complete MIDI compositions with professional workflow tools.

### Next Milestone: v1.1 - "Polish & Performance"
**Planned Features:**
- Error handling improvements (MIDI device disconnect, file I/O errors)
- Performance optimization (viewport culling, spatial indexing)
- MidiCanvas refactoring (separation of drawing/input logic)
- Command merging (cleaner undo history for drag operations)
- Track management (naming, colors, show/hide)

**Goal:** Production-ready stability and polish.

### Future Milestone: v1.2 - "Enhanced Editing"
**Planned Features:**
- Velocity editing UI
- Transpose operations
- Humanize (randomize timing/velocity)
- Timeline improvements (click to seek, scrubbing)
- Better transport controls

**Goal:** Advanced editing capabilities for detailed composition work.

### Future Milestone: v2.0 - "Professional Integration"
**Planned Features:**
- MIDI file import/export (.mid format)
- VST/VST3 plugin support
- Advanced MIDI CC automation
- Tempo automation
- Marker and section support

**Goal:** Full integration with professional audio production workflows.

---

## Design Decisions Log

### Why Timer-Based Updates Instead of Event-Driven?
**Decision:** Use 10ms timer loop for updates
**Rationale:** Simplicity and predictability for early development. Can optimize later.
**Trade-off:** Slightly higher CPU usage, but easier to debug and maintain.

### Why wxWidgets?
**Decision:** Use wxWidgets for GUI framework
**Rationale:** Cross-platform, native look-and-feel, mature and stable, good documentation.
**Trade-off:** Slightly older API patterns, but proven for audio/MIDI applications.

### Why 960 PPQN?
**Decision:** 960 ticks per quarter note
**Rationale:** High resolution for accurate timing, divisible by common note values (1/4, 1/8, 1/16, 1/32, triplets).

---

## How to Contribute Ideas

If you have ideas for features or improvements:
1. Consider: Does this make composition easier/faster?
2. Consider: What problem does this solve?
3. Add to the appropriate phase above with a brief note
4. If it's a major feature, add to "Design Decisions Log" with rationale

---

## Notes on "Ease of Use"

Good examples of easy composition workflows:
- **One-click operations**: Record with spacebar, no menu diving
- **Visual feedback**: See notes as you play them in
- **Smart defaults**: Auto-select sensible quantize values based on tempo
- **Forgiving**: Easy undo, non-destructive editing
- **Fast iteration**: Quickly try different arrangements without committing

Bad patterns to avoid:
- Modal dialogs that interrupt flow
- Hidden features that require reading manuals
- Destructive operations without undo
- Confusing state (is it recording? playing? stopped?)
- Clicks that should be drags, drags that should be clicks
