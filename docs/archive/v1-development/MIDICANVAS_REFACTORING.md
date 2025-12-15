# MidiWorks Refactoring Guide 2 - MidiCanvas Separation of Concerns

This guide focuses on moving business logic out of MidiCanvas and into AppModel, improving separation of concerns and enabling code reuse for future note editing features.

## Refactoring Principles

1. **Model Owns Data**: AppModel should handle all track data manipulation
2. **Commands Through Model**: Views should never create commands directly
3. **Preview vs Commit**: Separate temporary drag previews from permanent edits
4. **Test Often**: After each todo, test note editing functionality
5. **One Todo at a Time**: Complete each refactoring fully before moving on

---

## Phase 1: Note Manipulation API (Todos 1-4)

These methods move command creation and track manipulation into AppModel, eliminating direct track access from MidiCanvas.

### Todo 1: Create AppModel::DeleteNote() and DeleteNotes()

**Current Problem:** Note deletion logic appears in 2 places:
- OnMiddleDown (lines 744-768): Single note deletion
- DeleteSelectedNotes (lines 990-1017): Multiple note deletion

Both directly access tracks and create commands, duplicating command setup logic.

**Add to AppModel.h:**
```cpp
// Note Editing - Deletion
void DeleteNote(const NoteLocation& note);
void DeleteNotes(const std::vector<NoteLocation>& notes);
```

**Add to AppModel.cpp:**
```cpp
void AppModel::DeleteNote(const NoteLocation& note)
{
    if (!note.found) return;

    Track& track = mTrackSet.GetTrack(note.trackIndex);
    auto cmd = std::make_unique<DeleteNoteCommand>(
        track,
        note.noteOnIndex,
        note.noteOffIndex
    );
    ExecuteCommand(std::move(cmd));
}

void AppModel::DeleteNotes(const std::vector<NoteLocation>& notes)
{
    if (notes.empty()) return;

    // Build deletion list
    std::vector<DeleteMultipleNotesCommand::NoteToDelete> notesToDelete;
    notesToDelete.reserve(notes.size());

    for (const auto& note : notes)
    {
        Track& track = mTrackSet.GetTrack(note.trackIndex);

        DeleteMultipleNotesCommand::NoteToDelete noteData;
        noteData.trackIndex = note.trackIndex;
        noteData.noteOnIndex = note.noteOnIndex;
        noteData.noteOffIndex = note.noteOffIndex;
        noteData.noteOn = track[note.noteOnIndex];
        noteData.noteOff = track[note.noteOffIndex];

        notesToDelete.push_back(noteData);
    }

    // Execute single batch command
    auto cmd = std::make_unique<DeleteMultipleNotesCommand>(mTrackSet, notesToDelete);
    ExecuteCommand(std::move(cmd));
}
```

**Replace in MidiCanvas.cpp:**
```cpp
// OLD OnMiddleDown (lines 753-757):
Track& track = mTrackSet.GetTrack(clickedNote.trackIndex);
auto cmd = std::make_unique<DeleteNoteCommand>(track, clickedNote.noteOnIndex,
                                               clickedNote.noteOffIndex);
mAppModel->ExecuteCommand(std::move(cmd));

// NEW:
mAppModel->DeleteNote(clickedNote);

// OLD DeleteSelectedNotes (lines 994-1014):
std::vector<DeleteMultipleNotesCommand::NoteToDelete> notesToDelete;
// ... 20+ lines of building deletion list
auto cmd = std::make_unique<DeleteMultipleNotesCommand>(mTrackSet, notesToDelete);
mAppModel->ExecuteCommand(std::move(cmd));

// NEW:
mAppModel->DeleteNotes(mSelectedNotes);
```

**Simplify DeleteSelectedNotes() in MidiCanvas.cpp:**
```cpp
void MidiCanvasPanel::DeleteSelectedNotes()
{
    if (mSelectedNotes.empty()) return;

    mAppModel->DeleteNotes(mSelectedNotes);
    ClearSelection();
}
```

---

### Todo 2: Create AppModel::CopyNotesToClipboard()

**Current Problem:** CopySelectedNotesToClipboard (lines 953-988) contains complex business logic:
- Finding earliest tick for relative timing
- Converting NoteLocation to ClipboardNote format
- Direct track access to extract velocity data
- Understanding clipboard data structure (ClipboardNote)

This logic belongs in AppModel, not the view.

**Add to AppModel.h:**
```cpp
// Clipboard Operations
void CopyNotesToClipboard(const std::vector<NoteLocation>& notes);
```

**Add to AppModel.cpp:**
```cpp
void AppModel::CopyNotesToClipboard(const std::vector<NoteLocation>& notes)
{
    if (notes.empty()) return;

    // Find earliest start tick for relative timing
    uint64_t earliestTick = UINT64_MAX;
    for (const auto& note : notes)
    {
        if (note.startTick < earliestTick)
        {
            earliestTick = note.startTick;
        }
    }

    // Convert to clipboard format
    std::vector<ClipboardNote> clipboardNotes;
    clipboardNotes.reserve(notes.size());

    for (const auto& note : notes)
    {
        Track& track = mTrackSet.GetTrack(note.trackIndex);
        const TimedMidiEvent& noteOnEvent = track[note.noteOnIndex];

        ClipboardNote clipNote;
        clipNote.relativeStartTick = note.startTick - earliestTick;
        clipNote.duration = note.endTick - note.startTick;
        clipNote.pitch = note.pitch;
        clipNote.velocity = noteOnEvent.mm.mData[2];  // Extract velocity
        clipNote.trackIndex = note.trackIndex;

        clipboardNotes.push_back(clipNote);
    }

    CopyToClipboard(clipboardNotes);
}
```

**Replace in MidiCanvas.cpp:**
```cpp
// OLD CopySelectedNotesToClipboard (lines 953-988):
void MidiCanvasPanel::CopySelectedNotesToClipboard()
{
    if (mSelectedNotes.empty()) return;

    // Find the earliest start tick...
    // ... 35 lines of conversion logic

    mAppModel->CopyToClipboard(clipboardNotes);
}

// NEW:
void MidiCanvasPanel::CopySelectedNotesToClipboard()
{
    if (mSelectedNotes.empty()) return;
    mAppModel->CopyNotesToClipboard(mSelectedNotes);
}
```

---

### Todo 3: Create AppModel::PasteNotes()

**Current Problem:** OnKeyDown Ctrl+V (lines 1055-1072) creates PasteCommand directly. The view shouldn't know about command internals.

**Add to AppModel.h:**
```cpp
void PasteNotes(uint64_t pasteTick);
```

**Add to AppModel.cpp:**
```cpp
void AppModel::PasteNotes(uint64_t pasteTick)
{
    if (!HasClipboardData()) return;

    auto cmd = std::make_unique<PasteCommand>(
        mTrackSet,
        GetClipboard(),
        pasteTick
    );
    ExecuteCommand(std::move(cmd));
}
```

**Replace in MidiCanvas.cpp:**
```cpp
// OLD OnKeyDown Ctrl+V (lines 1056-1071):
if (event.ControlDown() && keyCode == 'V' && mAppModel->HasClipboardData())
{
    uint64_t pasteTick = mTransport.GetCurrentTick();

    auto cmd = std::make_unique<PasteCommand>(
        mTrackSet,
        mAppModel->GetClipboard(),
        pasteTick
    );
    mAppModel->ExecuteCommand(std::move(cmd));

    ClearSelection();
    Refresh();
    return;
}

// NEW:
if (event.ControlDown() && keyCode == 'V' && mAppModel->HasClipboardData())
{
    uint64_t pasteTick = mTransport.GetCurrentTick();
    mAppModel->PasteNotes(pasteTick);
    ClearSelection();
    Refresh();
    return;
}
```

---

### Todo 4: Create AppModel::MoveNote() and ResizeNote()

**Current Problem:** OnLeftUp (lines 684-737) contains complex finalization logic:
- Manually stores/restores original position
- Directly manipulates track data
- Creates and executes commands
- Duplicates "only create command if changed" logic

**Add to AppModel.h:**
```cpp
// Note Editing - Move and Resize
void MoveNote(const NoteLocation& note, uint64_t newStartTick, ubyte newPitch);
void ResizeNote(const NoteLocation& note, uint64_t newDuration);
```

**Add to AppModel.cpp:**
```cpp
void AppModel::MoveNote(const NoteLocation& note, uint64_t newStartTick, ubyte newPitch)
{
    if (!note.found) return;

    // Only execute if position actually changed
    if (newStartTick == note.startTick && newPitch == note.pitch)
        return;

    Track& track = mTrackSet.GetTrack(note.trackIndex);

    auto cmd = std::make_unique<MoveNoteCommand>(
        track,
        note.noteOnIndex,
        note.noteOffIndex,
        newStartTick,
        newPitch
    );
    ExecuteCommand(std::move(cmd));
}

void AppModel::ResizeNote(const NoteLocation& note, uint64_t newDuration)
{
    if (!note.found) return;

    uint64_t oldDuration = note.endTick - note.startTick;

    // Only execute if duration actually changed
    if (newDuration == oldDuration)
        return;

    Track& track = mTrackSet.GetTrack(note.trackIndex);

    auto cmd = std::make_unique<ResizeNoteCommand>(
        track,
        note.noteOnIndex,
        note.noteOffIndex,
        newDuration
    );
    ExecuteCommand(std::move(cmd));
}
```

**Replace in MidiCanvas.cpp OnLeftUp:**
```cpp
// OLD Moving note (lines 684-710):
else if (mMouseMode == MouseMode::MovingNote && mSelectedNote.found)
{
    Track& track = mTrackSet.GetTrack(mSelectedNote.trackIndex);

    // Get current position (after dragging)
    uint64_t currentTick = track[mSelectedNote.noteOnIndex].tick;
    ubyte currentPitch = track[mSelectedNote.noteOnIndex].mm.mData[1];

    // Restore original position first
    uint64_t duration = mOriginalEndTick - mOriginalStartTick;
    track[mSelectedNote.noteOnIndex].tick = mOriginalStartTick;
    track[mSelectedNote.noteOnIndex].mm.mData[1] = mOriginalPitch;
    track[mSelectedNote.noteOffIndex].tick = mOriginalEndTick;
    track[mSelectedNote.noteOffIndex].mm.mData[1] = mOriginalPitch;

    // Only create command if position actually changed
    if (currentTick != mOriginalStartTick || currentPitch != mOriginalPitch)
    {
        auto cmd = std::make_unique<MoveNoteCommand>(track, mSelectedNote.noteOnIndex,
                                                     mSelectedNote.noteOffIndex,
                                                     currentTick, currentPitch);
        mAppModel->ExecuteCommand(std::move(cmd));
    }

    mSelectedNote.found = false;
}

// NEW:
else if (mMouseMode == MouseMode::MovingNote && mSelectedNote.found)
{
    Track& track = mTrackSet.GetTrack(mSelectedNote.trackIndex);

    // Get current position (after temporary drag updates)
    uint64_t currentTick = track[mSelectedNote.noteOnIndex].tick;
    ubyte currentPitch = track[mSelectedNote.noteOnIndex].mm.mData[1];

    // Restore original position (undo temporary changes)
    uint64_t duration = mOriginalEndTick - mOriginalStartTick;
    track[mSelectedNote.noteOnIndex].tick = mOriginalStartTick;
    track[mSelectedNote.noteOnIndex].mm.mData[1] = mOriginalPitch;
    track[mSelectedNote.noteOffIndex].tick = mOriginalEndTick;
    track[mSelectedNote.noteOffIndex].mm.mData[1] = mOriginalPitch;

    // Execute move through model
    mAppModel->MoveNote(mSelectedNote, currentTick, currentPitch);
    mSelectedNote.found = false;
}

// OLD Resizing note (lines 712-736):
else if (mMouseMode == MouseMode::ResizingNote && mSelectedNote.found)
{
    Track& track = mTrackSet.GetTrack(mSelectedNote.trackIndex);

    // Get current end tick (after dragging)
    uint64_t currentEndTick = track[mSelectedNote.noteOffIndex].tick;

    // Restore original duration first
    track[mSelectedNote.noteOffIndex].tick = mOriginalEndTick;

    // Only create command if duration actually changed
    uint64_t newDuration = currentEndTick - mOriginalStartTick;
    uint64_t oldDuration = mOriginalEndTick - mOriginalStartTick;

    if (newDuration != oldDuration)
    {
        auto cmd = std::make_unique<ResizeNoteCommand>(track, mSelectedNote.noteOnIndex,
                                                       mSelectedNote.noteOffIndex,
                                                       newDuration);
        mAppModel->ExecuteCommand(std::move(cmd));
    }

    mSelectedNote.found = false;
}

// NEW:
else if (mMouseMode == MouseMode::ResizingNote && mSelectedNote.found)
{
    Track& track = mTrackSet.GetTrack(mSelectedNote.trackIndex);

    // Get current end tick (after temporary drag updates)
    uint64_t currentEndTick = track[mSelectedNote.noteOffIndex].tick;

    // Restore original duration (undo temporary changes)
    track[mSelectedNote.noteOffIndex].tick = mOriginalEndTick;

    // Calculate new duration and execute through model
    uint64_t newDuration = currentEndTick - mOriginalStartTick;
    mAppModel->ResizeNote(mSelectedNote, newDuration);
    mSelectedNote.found = false;
}
```

---

## Phase 2: Preview State Management (Todos 5-7)

**Critical Problem:** MidiCanvas directly modifies track data during mouse drag (lines 862-900), which:
- Bypasses the command pattern
- Corrupts track data temporarily
- Makes undo/redo state inconsistent
- Prevents multiple views from showing the same note

**Solution:** Store preview state in AppModel separately from actual track data.

### Todo 5: Add Note Edit Preview to AppModel

**Add to AppModel.h:**
```cpp
// Note Edit Preview (for drag operations)
struct NoteEditPreview
{
    bool isActive = false;
    NoteLocation originalNote;

    // Preview state (what the note looks like during drag)
    uint64_t previewStartTick = 0;
    uint64_t previewEndTick = 0;
    ubyte previewPitch = 0;
};

void SetNoteMovePreview(const NoteLocation& note, uint64_t newStartTick, ubyte newPitch);
void SetNoteResizePreview(const NoteLocation& note, uint64_t newEndTick);
void ClearNoteEditPreview();
const NoteEditPreview& GetNoteEditPreview() const { return mNoteEditPreview; }
bool HasNoteEditPreview() const { return mNoteEditPreview.isActive; }

private:
    NoteEditPreview mNoteEditPreview;
```

**Add to AppModel.cpp:**
```cpp
void AppModel::SetNoteMovePreview(const NoteLocation& note, uint64_t newStartTick, ubyte newPitch)
{
    mNoteEditPreview.isActive = true;
    mNoteEditPreview.originalNote = note;
    mNoteEditPreview.previewStartTick = newStartTick;
    mNoteEditPreview.previewEndTick = newStartTick + (note.endTick - note.startTick);
    mNoteEditPreview.previewPitch = newPitch;
}

void AppModel::SetNoteResizePreview(const NoteLocation& note, uint64_t newEndTick)
{
    mNoteEditPreview.isActive = true;
    mNoteEditPreview.originalNote = note;
    mNoteEditPreview.previewStartTick = note.startTick;
    mNoteEditPreview.previewEndTick = newEndTick;
    mNoteEditPreview.previewPitch = note.pitch;
}

void AppModel::ClearNoteEditPreview()
{
    mNoteEditPreview.isActive = false;
}
```

---

### Todo 6: Update MidiCanvas Mouse Move to Use Preview

**Replace in MidiCanvas.cpp OnMouseMove:**
```cpp
// OLD Moving note (lines 862-883):
if (mMouseMode == MouseMode::MovingNote && mSelectedNote.found)
{
    // Calculate new position based on mouse delta
    int deltaX = pos.x - mDragStartPos.x;
    int deltaY = pos.y - mDragStartPos.y;

    uint64_t newTick = mOriginalStartTick + (deltaX * mTicksPerPixel);
    int pitchDelta = -deltaY / mNoteHeight;
    int newPitch = std::clamp(static_cast<int>(mOriginalPitch) + pitchDelta, 0, MidiConstants::MAX_MIDI_NOTE);

    // PROBLEM: Directly modifies track data!
    Track& track = mTrackSet.GetTrack(mSelectedNote.trackIndex);
    uint64_t duration = mOriginalEndTick - mOriginalStartTick;

    track[mSelectedNote.noteOnIndex].tick = newTick;
    track[mSelectedNote.noteOnIndex].mm.mData[1] = newPitch;
    track[mSelectedNote.noteOffIndex].tick = newTick + duration;
    track[mSelectedNote.noteOffIndex].mm.mData[1] = newPitch;

    Refresh();
    return;
}

// NEW:
if (mMouseMode == MouseMode::MovingNote && mSelectedNote.found)
{
    // Calculate new position based on mouse delta
    int deltaX = pos.x - mDragStartPos.x;
    int deltaY = pos.y - mDragStartPos.y;

    uint64_t newTick = mOriginalStartTick + (deltaX * mTicksPerPixel);
    int pitchDelta = -deltaY / mNoteHeight;
    int newPitch = std::clamp(static_cast<int>(mOriginalPitch) + pitchDelta, 0, MidiConstants::MAX_MIDI_NOTE);

    // Store preview state in model (doesn't modify track data)
    mAppModel->SetNoteMovePreview(mSelectedNote, newTick, static_cast<ubyte>(newPitch));

    Refresh();
    return;
}

// OLD Resizing note (lines 886-900):
if (mMouseMode == MouseMode::ResizingNote && mSelectedNote.found)
{
    uint64_t newEndTick = ScreenXToTick(pos.x);
    if (newEndTick <= mOriginalStartTick) newEndTick = mOriginalStartTick + 100;

    // PROBLEM: Directly modifies track data!
    Track& track = mTrackSet.GetTrack(mSelectedNote.trackIndex);
    track[mSelectedNote.noteOffIndex].tick = newEndTick;

    Refresh();
    return;
}

// NEW:
if (mMouseMode == MouseMode::ResizingNote && mSelectedNote.found)
{
    uint64_t newEndTick = ScreenXToTick(pos.x);
    if (newEndTick <= mOriginalStartTick) newEndTick = mOriginalStartTick + 100;

    // Store preview state in model (doesn't modify track data)
    mAppModel->SetNoteResizePreview(mSelectedNote, newEndTick);

    Refresh();
    return;
}
```

---

### Todo 7: Update MidiCanvas Draw to Render Preview

**Add to MidiCanvas.cpp Draw() method (after drawing normal notes, before selection borders):**
```cpp
// Draw note edit preview (for move/resize drag operations)
if (mAppModel->HasNoteEditPreview())
{
    const auto& preview = mAppModel->GetNoteEditPreview();
    const auto& originalNote = preview.originalNote;

    // Determine color based on track
    wxColour previewColor = trackColors[originalNote.trackIndex];
    previewColor.Set(
        previewColor.Red(),
        previewColor.Green(),
        previewColor.Blue(),
        180  // Semi-transparent
    );

    gc->SetBrush(wxBrush(previewColor));
    gc->SetPen(wxPen(*wxWHITE, 2));  // White border to distinguish preview

    int x = preview.previewStartTick / mTicksPerPixel + mOriginOffset.x;
    int y = Flip(preview.previewPitch * mNoteHeight) + mOriginOffset.y;
    int w = (preview.previewEndTick - preview.previewStartTick) / mTicksPerPixel;

    gc->DrawRectangle(x, y, w, mNoteHeight);
}
```

**Note:** You'll also need to hide the original note during preview. Add this before drawing notes:
```cpp
// Skip drawing the note being previewed (it's drawn separately as preview)
const auto& preview = mAppModel->GetNoteEditPreview();
bool skipNote = false;
if (preview.isActive &&
    trackIndex == preview.originalNote.trackIndex &&
    i == preview.originalNote.noteOnIndex)
{
    skipNote = true;
}
if (skipNote) continue;
```

---

### Todo 8: Clear Preview on Mouse Up

**Add to MidiCanvas.cpp OnLeftUp (before resetting mode):**
```cpp
// Clear preview state before finalizing edit
if (mMouseMode == MouseMode::MovingNote || mMouseMode == MouseMode::ResizingNote)
{
    mAppModel->ClearNoteEditPreview();
}

// Reset mode and refresh
mMouseMode = MouseMode::Idle;
Refresh();
```

**Remove track restoration code since we never modified tracks:**
```cpp
// DELETE these lines (no longer needed since we use preview state):
// Lines 693-698: Restore original position
// Lines 721: Restore original duration
```

**Simplify OnLeftUp to:**
```cpp
else if (mMouseMode == MouseMode::MovingNote && mSelectedNote.found)
{
    const auto& preview = mAppModel->GetNoteEditPreview();
    mAppModel->MoveNote(mSelectedNote, preview.previewStartTick, preview.previewPitch);
    mAppModel->ClearNoteEditPreview();
    mSelectedNote.found = false;
}
else if (mMouseMode == MouseMode::ResizingNote && mSelectedNote.found)
{
    const auto& preview = mAppModel->GetNoteEditPreview();
    uint64_t newDuration = preview.previewEndTick - preview.previewStartTick;
    mAppModel->ResizeNote(mSelectedNote, newDuration);
    mAppModel->ClearNoteEditPreview();
    mSelectedNote.found = false;
}
```

---

## Phase 3: View-Specific Helpers (Optional Cleanup)

These todos are optional but improve code organization within MidiCanvas.

### Todo 9: Consider Coordinate Conversion Helper Struct

**Current:** Coordinate conversion scattered throughout MidiCanvas

**Optional Improvement:**
```cpp
// In MidiCanvas.h
struct CanvasCoordinates
{
    static uint64_t ScreenXToTick(int x, int offset, int ticksPerPixel);
    static ubyte ScreenYToPitch(int y, int offset, int noteHeight, int canvasHeight);
    static int TickToScreenX(uint64_t tick, int offset, int ticksPerPixel);
    static int PitchToScreenY(ubyte pitch, int offset, int noteHeight, int canvasHeight);
};
```

**Note:** This is low priority since coordinate conversion is inherently view logic.

---

### Todo 10: Extract Drawing Helpers

**Current:** Draw() method is 200+ lines (lines 120-304)

**Optional Improvement:**
```cpp
// In MidiCanvas.h (private methods)
void DrawLoopRegion(wxGraphicsContext* gc);
void DrawTrackNotes(wxGraphicsContext* gc, int trackIndex, const wxColour& color);
void DrawRecordingBuffer(wxGraphicsContext* gc);
void DrawNoteAddPreview(wxGraphicsContext* gc);
void DrawNoteEditPreview(wxGraphicsContext* gc);
void DrawSelection(wxGraphicsContext* gc);
void DrawHoverBorder(wxGraphicsContext* gc);
void DrawPlayhead(wxGraphicsContext* gc);
```

Then in Draw():
```cpp
void MidiCanvasPanel::Draw(wxPaintEvent&)
{
    wxAutoBufferedPaintDC dc(this);
    // ... setup code

    wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
    if (!gc) return;

    DrawGrid(gc);
    DrawLoopRegion(gc);

    for (int i = 0; i < 15; i++)
        DrawTrackNotes(gc, i, trackColors[i]);

    DrawRecordingBuffer(gc);
    DrawNoteAddPreview(gc);
    DrawNoteEditPreview(gc);
    DrawSelection(gc);
    DrawHoverBorder(gc);
    DrawPlayhead(gc);

    delete gc;
}
```

---

## Testing Checklist

After completing each phase, verify:

### Phase 1 Tests (Todos 1-4)
- [ ] Delete note with middle-click works
- [ ] Delete key removes selected notes
- [ ] Ctrl+C copies notes to clipboard
- [ ] Ctrl+V pastes notes at playhead
- [ ] Ctrl+X cuts notes
- [ ] Move note with drag works
- [ ] Resize note by edge drag works
- [ ] Undo/Redo works for all operations

### Phase 2 Tests (Todos 5-8)
- [ ] Preview appears when dragging note
- [ ] Preview updates smoothly during drag
- [ ] Preview disappears on mouse up
- [ ] Original note hidden during preview
- [ ] Move finalizes correctly
- [ ] Resize finalizes correctly
- [ ] Track data never corrupted during drag
- [ ] Multiple drags work correctly

### Phase 3 Tests (Optional)
- [ ] All functionality still works after refactoring Draw()
- [ ] No visual regressions

---

## Summary

**Total Refactoring Tasks:** 10 (7 required, 3 optional)

**Key Problems Solved:**
1. **Direct Track Manipulation:** MidiCanvas no longer modifies tracks directly during drag
2. **Command Creation in View:** All commands now created through AppModel
3. **Clipboard Logic in View:** Conversion logic moved to model
4. **Code Duplication:** Delete and clipboard operations unified
5. **Future-Proofing:** Note editing API enables future features (batch edit, MIDI import, etc.)

**Lines of Code Impact:**
- **Removed from MidiCanvas:** ~150 lines of business logic
- **Added to AppModel:** ~120 lines of reusable methods
- **Net Complexity:** -30 lines, +300% clarity

**Benefits:**
- Clean separation: Views render, Model manipulates
- Reusable note editing API for future panels
- Preview state enables multi-view support
- No more temporary track corruption
- Easier to test business logic in isolation

**Next Steps:**
After completing this guide, consider:
- Adding unit tests for AppModel note editing methods
- Creating a TrackEditor panel that reuses this API
- Implementing batch note operations (transpose, velocity adjust)
- Adding note validation (prevent overlapping notes, enforce ranges)

Good luck with Phase 2 refactoring!
