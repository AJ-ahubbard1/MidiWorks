# MidiWorks Refactoring Guide - Version 1.0 Cleanup

This guide organizes all DRY and Separation of Concerns violations into actionable refactoring tasks.

## Refactoring Principles

1. **Model First**: Create new methods in AppModel/TrackSet/SoundBank before modifying views
2. **Test Often**: After each group of changes, test the functionality
3. **One Todo at a Time**: Complete each todo fully before moving to the next
4. **Dependencies**: Follow the order below - later tasks depend on earlier ones

---

## Phase 1: Foundation - Create Model Methods (Todos 1-8)

These create the centralized methods that eliminate duplication and provide proper encapsulation.

### Todo 1: Create AppModel::PlayNote() and StopNote()

**Current Problem:** MIDI messages sent from 8+ locations (AppModel, MidiCanvas, ChannelControls)

**Add to AppModel.h:**
```cpp
// MIDI Playback
void PlayNote(uint8_t pitch, uint8_t velocity, uint8_t channel);
void StopNote(uint8_t pitch, uint8_t channel);
```

**Add to AppModel.cpp:**
```cpp
void AppModel::PlayNote(uint8_t pitch, uint8_t velocity, uint8_t channel)
{
    auto midiOut = mSoundBank.GetMidiOutDevice();
    midiOut->sendMessage(MidiMessage::NoteOn(pitch, velocity, channel));
}

void AppModel::StopNote(uint8_t pitch, uint8_t channel)
{
    auto midiOut = mSoundBank.GetMidiOutDevice();
    midiOut->sendMessage(MidiMessage::NoteOff(pitch, channel));
}
```

**Note:** Later you'll replace direct `sendMessage()` calls throughout the codebase with these methods.

---

### Todo 2: Create AppModel::StopPlaybackIfActive()

**Current Problem:** Identical 9-line code block repeated 3 times in MainFrame (OnUndo, OnRedo, OnQuantize)

**Add to AppModel.h:**
```cpp
// Transport Control
void StopPlaybackIfActive();
```

**Add to AppModel.cpp:**
```cpp
void AppModel::StopPlaybackIfActive()
{
    if (mTransport.mState == Transport::State::Playing)
    {
        mTransport.mState = Transport::State::StopPlaying;
    }
    else if (mTransport.mState == Transport::State::Recording)
    {
        mTransport.mState = Transport::State::StopRecording;
    }
}
```

**Replace in MainFrame.cpp (3 locations):**
```cpp
// OLD (lines 254-262, 274-282, 294-301):
auto& transport = mAppModel->GetTransport();
if (transport.mState == Transport::State::Playing)
    transport.mState = Transport::State::StopPlaying;
else if (transport.mState == Transport::State::Recording)
    transport.mState = Transport::State::StopRecording;

// NEW:
mAppModel->StopPlaybackIfActive();
```

---

### Todo 3: Create TrackSet::FindNoteAt(tick, pitch)

**Current Problem:** MidiCanvas::FindNoteAtPosition() iterates through all tracks manually

**Add to TrackSet.h:**
```cpp
struct NoteLocation {
    bool found = false;
    int trackIndex = -1;
    size_t noteOnIndex = 0;
    size_t noteOffIndex = 0;
    uint64_t startTick = 0;
    uint64_t endTick = 0;
    uint8_t pitch = 0;
};

NoteLocation FindNoteAt(uint64_t tick, uint8_t pitch);
```

**Add to TrackSet.cpp (you'll need to create this file):**
```cpp
TrackSet::NoteLocation TrackSet::FindNoteAt(uint64_t tick, uint8_t pitch)
{
    NoteLocation result;

    // Search through all tracks
    for (int trackIndex = 0; trackIndex < 15; trackIndex++)
    {
        Track& track = mTracks[trackIndex];
        if (track.empty()) continue;

        size_t end = track.size();
        for (size_t i = 0; i < end; i++)
        {
            const TimedMidiEvent& noteOn = track[i];
            if (noteOn.mm.getEventType() != MidiEvent::NOTE_ON) continue;

            // Find corresponding note off
            for (size_t j = i + 1; j < end; j++)
            {
                const TimedMidiEvent& noteOff = track[j];
                if (noteOff.mm.getEventType() != MidiEvent::NOTE_OFF ||
                    noteOff.mm.mData[1] != noteOn.mm.mData[1]) continue;

                // Check if click is within this note's bounds
                uint8_t notePitch = noteOn.mm.mData[1];
                uint64_t noteStartTick = noteOn.tick;
                uint64_t noteEndTick = noteOff.tick;

                if (notePitch == pitch && tick >= noteStartTick && tick <= noteEndTick)
                {
                    result.found = true;
                    result.trackIndex = trackIndex;
                    result.noteOnIndex = i;
                    result.noteOffIndex = j;
                    result.startTick = noteStartTick;
                    result.endTick = noteEndTick;
                    result.pitch = notePitch;
                    return result;
                }
                break;
            }
        }
    }

    return result;
}
```

**Note:** You'll need to add TrackSet.cpp to your project and move TrackSet implementation there.

---

### Todo 4: Create TrackSet::FindNotesInRegion()

**Current Problem:** MidiCanvas::FindNotesInRectangle() duplicates search logic

**Add to TrackSet.h:**
```cpp
std::vector<NoteLocation> FindNotesInRegion(uint64_t minTick, uint64_t maxTick,
                                            uint8_t minPitch, uint8_t maxPitch);
```

**Add to TrackSet.cpp:**
```cpp
std::vector<TrackSet::NoteLocation> TrackSet::FindNotesInRegion(
    uint64_t minTick, uint64_t maxTick, uint8_t minPitch, uint8_t maxPitch)
{
    std::vector<NoteLocation> result;

    // Search through all tracks
    for (int trackIndex = 0; trackIndex < 15; trackIndex++)
    {
        Track& track = mTracks[trackIndex];
        if (track.empty()) continue;

        size_t end = track.size();
        for (size_t i = 0; i < end; i++)
        {
            const TimedMidiEvent& noteOn = track[i];
            if (noteOn.mm.getEventType() != MidiEvent::NOTE_ON) continue;

            // Find corresponding note off
            for (size_t j = i + 1; j < end; j++)
            {
                const TimedMidiEvent& noteOff = track[j];
                if (noteOff.mm.getEventType() != MidiEvent::NOTE_OFF ||
                    noteOff.mm.mData[1] != noteOn.mm.mData[1]) continue;

                uint8_t notePitch = noteOn.mm.mData[1];
                uint64_t noteStartTick = noteOn.tick;
                uint64_t noteEndTick = noteOff.tick;

                // Check if note overlaps region
                bool pitchInRange = (notePitch >= minPitch && notePitch <= maxPitch);
                bool timeOverlaps = (noteStartTick <= maxTick && noteEndTick >= minTick);

                if (pitchInRange && timeOverlaps)
                {
                    NoteLocation note;
                    note.found = true;
                    note.trackIndex = trackIndex;
                    note.noteOnIndex = i;
                    note.noteOffIndex = j;
                    note.startTick = noteStartTick;
                    note.endTick = noteEndTick;
                    note.pitch = notePitch;
                    result.push_back(note);
                }
                break;
            }
        }
    }

    return result;
}
```

---

### Todo 5: Create TrackSet::GetAllNotes()

**Current Problem:** MidiCanvas Ctrl+A iterates through all tracks manually

**Add to TrackSet.h:**
```cpp
std::vector<NoteLocation> GetAllNotes();
```

**Add to TrackSet.cpp:**
```cpp
std::vector<TrackSet::NoteLocation> TrackSet::GetAllNotes()
{
    std::vector<NoteLocation> result;

    for (int trackIndex = 0; trackIndex < 15; trackIndex++)
    {
        Track& track = mTracks[trackIndex];
        if (track.empty()) continue;

        size_t end = track.size();
        for (size_t i = 0; i < end; i++)
        {
            const TimedMidiEvent& noteOn = track[i];
            if (noteOn.mm.getEventType() != MidiEvent::NOTE_ON) continue;

            // Find corresponding note off
            for (size_t j = i + 1; j < end; j++)
            {
                const TimedMidiEvent& noteOff = track[j];
                if (noteOff.mm.getEventType() != MidiEvent::NOTE_OFF ||
                    noteOff.mm.mData[1] != noteOn.mm.mData[1]) continue;

                NoteLocation note;
                note.found = true;
                note.trackIndex = trackIndex;
                note.noteOnIndex = i;
                note.noteOffIndex = j;
                note.startTick = noteOn.tick;
                note.endTick = noteOff.tick;
                note.pitch = noteOn.mm.mData[1];
                result.push_back(note);
                break;
            }
        }
    }

    return result;
}
```

---

### Todo 6: Create Track/TrackSet note iterator
## Will save for the end of Phase 1
**Current Problem:** Note-on/note-off pairing logic repeated 6+ times

**Note:** This is more advanced. Consider implementing later if you want to use modern C++ range-based iteration.

**Example API (optional):**
```cpp
// In TrackSet.h
class NoteIterator {
    // ... iterator implementation
};

// Usage:
for (auto note : trackSet.GetNoteIterator(trackIndex)) {
    // note.noteOn, note.noteOff, note.startTick, note.endTick available
}
```

**For now:** Todos 3-5 provide methods that eliminate most duplication. You can skip this todo initially.

---

### Todo 7: Create SoundBank::GetRecordEnabledChannels()

**Current Problem:** Iterating over channels to find record-enabled ones appears 3 times

**Add to SoundBank.h:**
```cpp
std::vector<MidiChannel*> GetRecordEnabledChannels();
```

**Add to SoundBank.cpp (you'll need to create this file):**
```cpp
std::vector<MidiChannel*> SoundBank::GetRecordEnabledChannels()
{
    std::vector<MidiChannel*> result;
    for (auto& channel : mChannels)
    {
        if (channel.record)
        {
            result.push_back(&channel);
        }
    }
    return result;
}
```

---

### Todo 8: Create SoundBank::ShouldChannelPlay()

**Current Problem:** Solo/mute logic duplicated in 2 places with INCONSISTENT implementations (BUG!)

**Add to SoundBank.h:**
```cpp
bool ShouldChannelPlay(const MidiChannel& channel, bool checkRecord = false);
```

**Add to SoundBank.cpp:**
```cpp
bool SoundBank::ShouldChannelPlay(const MidiChannel& channel, bool checkRecord)
{
    bool solosFound = SolosFound();

    if (solosFound)
    {
        // If any channel is solo'd, only solo channels play
        return channel.solo;
    }
    else
    {
        // Normal playback: check mute (and optionally record)
        if (checkRecord)
            return channel.record && !channel.mute;
        else
            return !channel.mute;
    }
}
```

**Replace in AppModel.cpp:**
```cpp
// OLD (line 227):
bool shouldPlay = solosFound ? c.solo : (c.record && !c.mute);

// NEW:
bool shouldPlay = mSoundBank.ShouldChannelPlay(c, true);

// OLD (line 416):
bool shouldPlay = solosFound ? channel.solo : !channel.mute;

// NEW:
bool shouldPlay = mSoundBank.ShouldChannelPlay(channel, false);
```

---
# I am now here! *
## Phase 2: High-Level Business Logic (Todos 9-14)

Move complex business logic from views to AppModel.

### Todo 9-10: Move MIDI Preview to AppModel

**Current Problem:** MidiCanvas directly accesses SoundBank and sends MIDI

**Add to AppModel.h:**
```cpp
void PlayPreviewNote(uint8_t pitch);
void StopPreviewNote();
```

**Add to AppModel.cpp:**
```cpp
void AppModel::PlayPreviewNote(uint8_t pitch)
{
    const uint8_t PREVIEW_VELOCITY = 100;
    auto midiOut = mSoundBank.GetMidiOutDevice();
    auto channels = GetRecordEnabledChannels();

    for (MidiChannel* channel : channels)
    {
        MidiMessage noteOn = MidiMessage::NoteOn(pitch, PREVIEW_VELOCITY, channel->channelNumber);
        midiOut->sendMessage(noteOn);
    }
}

void AppModel::StopPreviewNote()
{
    auto midiOut = mSoundBank.GetMidiOutDevice();
    auto channels = GetRecordEnabledChannels();

    for (MidiChannel* channel : channels)
    {
        // Note: You'll need to track which pitch was previewed
        // Consider adding member variable: uint8_t mPreviewPitch
    }
}
```

**Note:** You may need to track preview state in AppModel.

---

### Todo 11: Create AppModel::AddNoteToRecordChannels()

**Current Problem:** MidiCanvas has complex logic to add notes to all record-enabled channels

**Add to AppModel.h:**
```cpp
void AddNoteToRecordChannels(uint8_t pitch, uint64_t startTick, uint64_t duration);
```

**Add to AppModel.cpp:**
```cpp
void AppModel::AddNoteToRecordChannels(uint8_t pitch, uint64_t startTick, uint64_t duration)
{
    const uint8_t NOTE_VELOCITY = 100;
    auto channels = mSoundBank.GetRecordEnabledChannels();

    for (MidiChannel* channel : channels)
    {
        MidiMessage noteOn = MidiMessage::NoteOn(pitch, NOTE_VELOCITY, channel->channelNumber);
        MidiMessage noteOff = MidiMessage::NoteOff(pitch, channel->channelNumber);

        TimedMidiEvent timedNoteOn{noteOn, startTick};
        TimedMidiEvent timedNoteOff{noteOff, startTick + duration - 1};

        Track& track = mTrackSet.GetTrack(channel->channelNumber);
        auto cmd = std::make_unique<AddNoteCommand>(track, timedNoteOn, timedNoteOff);
        ExecuteCommand(std::move(cmd));
    }
}
```

**Replace in MidiCanvas.cpp (lines 814-836):**
```cpp
// OLD:
auto& soundBank = mAppModel->GetSoundBank();
auto channels = soundBank.GetAllChannels();
for (const MidiChannel& channel : channels)
{
    if (channel.record)
    {
        // ... 15 lines of code
    }
}

// NEW:
mAppModel->AddNoteToRecordChannels(mPreviewPitch, snappedTick, duration);
```

---

### Todo 12-13: Create Loop and Playhead Setters

**Current Problem:** MidiCanvas directly manipulates Transport state

**Add to AppModel.h:**
```cpp
void SetLoopStart(uint64_t tick);
void SetLoopEnd(uint64_t tick);
void SetPlayheadPosition(uint64_t tick);
```

**Add to AppModel.cpp:**
```cpp
void AppModel::SetLoopStart(uint64_t tick)
{
    // Validation: loop start must be before loop end
    if (tick < mTransport.mLoopEndTick)
    {
        mTransport.mLoopStartTick = tick;
    }
}

void AppModel::SetLoopEnd(uint64_t tick)
{
    // Validation: loop end must be after loop start
    if (tick > mTransport.mLoopStartTick)
    {
        mTransport.mLoopEndTick = tick;
    }
}

void AppModel::SetPlayheadPosition(uint64_t tick)
{
    mTransport.ShiftToTick(tick);
}
```

**Replace in MidiCanvas.cpp:**
```cpp
// OLD (lines 979, 992):
mTransport.mLoopStartTick = newTick;
mTransport.mLoopEndTick = newTick;

// NEW:
mAppModel->SetLoopStart(newTick);
mAppModel->SetLoopEnd(newTick);

// OLD (line 928):
mTransport.ShiftToTick(newTick);

// NEW:
mAppModel->SetPlayheadPosition(newTick);
```

---

### Todo 14: Create AppModel::QuantizeAllTracks()

**Current Problem:** MainFrame implements quantize business logic

**Add to AppModel.h:**
```cpp
void QuantizeAllTracks(uint64_t gridSize);
```

**Add to AppModel.cpp:**
```cpp
void AppModel::QuantizeAllTracks(uint64_t gridSize)
{
    // Stop playback if needed (business rule)
    StopPlaybackIfActive();

    // Quantize all non-empty tracks
    for (int i = 0; i < 15; i++)
    {
        Track& track = mTrackSet.GetTrack(i);
        if (!track.empty())
        {
            auto cmd = std::make_unique<QuantizeCommand>(track, gridSize);
            ExecuteCommand(std::move(cmd));
        }
    }
}
```

**Replace in MainFrame.cpp (lines 289-321):**
```cpp
// OLD: 30+ lines of code

// NEW:
void MainFrame::OnQuantize(wxCommandEvent& event)
{
    uint64_t gridSize = mMidiCanvasPanel->GetGridSize();
    mAppModel->QuantizeAllTracks(gridSize);
    mUndoHistoryPanel->UpdateDisplay();
    Refresh();
}
```

---

## Phase 3: Extract Duplicate Code in Views (Todos 15-16)

Create helper methods in views to eliminate duplication.

### Todo 15: Extract MidiCanvas::CopySelectedNotesToClipboard()

**Current Problem:** Clipboard conversion code duplicated in Ctrl+C and Ctrl+X

**Add to MidiCanvas.h:**
```cpp
private:
    void CopySelectedNotesToClipboard();
```

**Add to MidiCanvas.cpp:**
```cpp
void MidiCanvasPanel::CopySelectedNotesToClipboard()
{
    if (mSelectedNotes.empty()) return;

    // Find the earliest start tick
    uint64_t earliestTick = UINT64_MAX;
    for (const auto& note : mSelectedNotes)
    {
        if (note.startTick < earliestTick)
            earliestTick = note.startTick;
    }

    // Convert selected notes to clipboard format
    std::vector<AppModel::ClipboardNote> clipboardNotes;
    clipboardNotes.reserve(mSelectedNotes.size());

    for (const auto& note : mSelectedNotes)
    {
        Track& track = mTrackSet.GetTrack(note.trackIndex);
        const TimedMidiEvent& noteOnEvent = track[note.noteOnIndex];

        AppModel::ClipboardNote clipNote;
        clipNote.relativeStartTick = note.startTick - earliestTick;
        clipNote.duration = note.endTick - note.startTick;
        clipNote.pitch = note.pitch;
        clipNote.velocity = noteOnEvent.mm.mData[2];
        clipNote.trackIndex = note.trackIndex;

        clipboardNotes.push_back(clipNote);
    }

    mAppModel->CopyToClipboard(clipboardNotes);
}
```

**Replace in OnKeyDown:**
```cpp
// Ctrl+C (delete lines 1199-1234):
if (event.ControlDown() && keyCode == 'C' && !mSelectedNotes.empty())
{
    CopySelectedNotesToClipboard();  // ONE LINE!
    return;
}

// Ctrl+X (delete lines 1258-1286):
if (event.ControlDown() && keyCode == 'X' && !mSelectedNotes.empty())
{
    CopySelectedNotesToClipboard();  // ONE LINE!
    // ... then delete code
}
```

---

### Todo 16: Extract MidiCanvas::DeleteSelectedNotes()

**Current Problem:** Delete logic duplicated in Delete key and Ctrl+X

**Add to MidiCanvas.h:**
```cpp
private:
    void DeleteSelectedNotes();
```

**Add to MidiCanvas.cpp:**
```cpp
void MidiCanvasPanel::DeleteSelectedNotes()
{
    if (mSelectedNotes.empty()) return;

    std::vector<DeleteMultipleNotesCommand::NoteToDelete> notesToDelete;
    notesToDelete.reserve(mSelectedNotes.size());

    for (const auto& note : mSelectedNotes)
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

    auto cmd = std::make_unique<DeleteMultipleNotesCommand>(mTrackSet, notesToDelete);
    mAppModel->ExecuteCommand(std::move(cmd));

    ClearSelection();
}
```

**Replace in OnKeyDown:**
```cpp
// Delete key (delete lines 1119-1146):
if (keyCode == WXK_DELETE && !mSelectedNotes.empty())
{
    DeleteSelectedNotes();  // ONE LINE!
    Refresh();
    return;
}

// Ctrl+X (delete lines 1289-1313):
if (event.ControlDown() && keyCode == 'X' && !mSelectedNotes.empty())
{
    CopySelectedNotesToClipboard();
    DeleteSelectedNotes();  // ONE LINE!
    Refresh();
    return;
}
```

---

## Phase 4: Use New Model Methods in Views (Todos 17-25)

Replace all view code that duplicates or violates SoC with calls to new model methods.

### Todo 17-18: Refactor MidiCanvas Find Methods

**Replace MidiCanvas::FindNoteAtPosition():**
```cpp
// OLD (lines 467-521): ~55 lines of iteration code

// NEW:
MidiCanvasPanel::NoteInfo MidiCanvasPanel::FindNoteAtPosition(int screenX, int screenY)
{
    uint64_t clickTick = ScreenXToTick(screenX);
    uint8_t clickPitch = ScreenYToPitch(screenY);

    auto modelNote = mTrackSet.FindNoteAt(clickTick, clickPitch);

    // Convert to view NoteInfo
    NoteInfo result;
    result.valid = modelNote.found;
    if (modelNote.found)
    {
        result.trackIndex = modelNote.trackIndex;
        result.noteOnIndex = modelNote.noteOnIndex;
        result.noteOffIndex = modelNote.noteOffIndex;
        result.startTick = modelNote.startTick;
        result.endTick = modelNote.endTick;
        result.pitch = modelNote.pitch;
    }
    return result;
}
```

**Replace MidiCanvas::FindNotesInRectangle():**
```cpp
// OLD (lines 523-587): ~65 lines of iteration code

// NEW:
std::vector<MidiCanvasPanel::NoteInfo> MidiCanvasPanel::FindNotesInRectangle(wxPoint start, wxPoint end)
{
    // Normalize rectangle
    int minX = std::min(start.x, end.x);
    int maxX = std::max(start.x, end.x);
    int minY = std::min(start.y, end.y);
    int maxY = std::max(start.y, end.y);

    // Convert to tick/pitch ranges
    uint64_t minTick = ScreenXToTick(minX);
    uint64_t maxTick = ScreenXToTick(maxX);
    uint8_t minPitch = ScreenYToPitch(maxY);  // Y is flipped
    uint8_t maxPitch = ScreenYToPitch(minY);

    // Query model
    auto modelNotes = mTrackSet.FindNotesInRegion(minTick, maxTick, minPitch, maxPitch);

    // Convert to view NoteInfo
    std::vector<NoteInfo> result;
    for (const auto& modelNote : modelNotes)
    {
        NoteInfo note;
        note.valid = true;
        note.trackIndex = modelNote.trackIndex;
        note.noteOnIndex = modelNote.noteOnIndex;
        note.noteOffIndex = modelNote.noteOffIndex;
        note.startTick = modelNote.startTick;
        note.endTick = modelNote.endTick;
        note.pitch = modelNote.pitch;
        result.push_back(note);
    }

    return result;
}
```

---

### Todo 19: Refactor MidiCanvas::Draw() to use helper

**Note:** This is optional since Draw() needs to iterate for rendering anyway. You could create a helper:

```cpp
// Helper to iterate note pairs
void MidiCanvasPanel::DrawNotes(wxGraphicsContext* gc, Track& track, const wxColour& color)
{
    gc->SetBrush(wxBrush(color));

    size_t end = track.size();
    for (size_t i = 0; i < end; i++)
    {
        const TimedMidiEvent& noteOn = track[i];
        if (noteOn.mm.getEventType() != MidiEvent::NOTE_ON) continue;

        for (size_t j = i + 1; j < end; j++)
        {
            const TimedMidiEvent& noteOff = track[j];
            if (noteOff.mm.getEventType() != MidiEvent::NOTE_OFF ||
                noteOff.mm.mData[1] != noteOn.mm.mData[1]) continue;

            int x = noteOn.tick / mTicksPerPixel + mOriginOffset.x;
            int y = Flip(noteOn.mm.mData[1] * mNoteHeight) + mOriginOffset.y;
            int w = (noteOff.tick - noteOn.tick) / mTicksPerPixel;
            gc->DrawRectangle(x, y, w, mNoteHeight);
            break;
        }
    }
}

// Then in Draw():
for (int trackIndex = 0; trackIndex < 15; trackIndex++)
{
    Track& track = mTrackSet.GetTrack(trackIndex);
    if (track.empty()) continue;
    DrawNotes(gc, track, trackColors[trackIndex]);
}
```

---

### Todo 20-25: Simple Replacements

These are straightforward find-and-replace operations:

**Todo 20 - OnLeftUp note creation:**
```cpp
// Replace lines 814-836 with:
mAppModel->AddNoteToRecordChannels(mPreviewPitch, snappedTick, duration);
```

**Todo 21 - Preview note calls:**
```cpp
// Replace MidiCanvas::PlayPreviewNote() body with:
void MidiCanvasPanel::PlayPreviewNote(uint8_t pitch)
{
    mAppModel->PlayPreviewNote(pitch);
    mIsPreviewingNote = true;
    mPreviewPitch = pitch;
}
```

**Todo 22 - Loop dragging:**
```cpp
// Replace lines 979, 992 with:
mAppModel->SetLoopStart(newTick);
mAppModel->SetLoopEnd(newTick);
```

**Todo 23 - Select all:**
```cpp
// Replace lines 1157-1196 with:
if (event.ControlDown() && keyCode == 'A')
{
    ClearSelection();

    auto modelNotes = mTrackSet.GetAllNotes();
    for (const auto& modelNote : modelNotes)
    {
        NoteInfo note;
        note.valid = true;
        note.trackIndex = modelNote.trackIndex;
        note.noteOnIndex = modelNote.noteOnIndex;
        note.noteOffIndex = modelNote.noteOffIndex;
        note.startTick = modelNote.startTick;
        note.endTick = modelNote.endTick;
        note.pitch = modelNote.pitch;
        mSelectedNotes.push_back(note);
    }

    Refresh();
    return;
}
```

**Todo 24 - MainFrame stop playback:**
```cpp
// Already covered in Todo 2 - just replace 3 locations
```

**Todo 25 - MainFrame quantize:**
```cpp
// Already covered in Todo 14
```

---

## Phase 5: Constants and Cleanup (Todos 26-29)

### Todo 26-27: Create MidiConstants

**Create new file: src/MidiConstants.h:**
```cpp
#pragma once

namespace MidiConstants
{
    // Timing
    static constexpr int TICKS_PER_QUARTER = 960;
    static constexpr int BEATS_PER_MEASURE = 4;
    static constexpr int TICKS_PER_MEASURE = TICKS_PER_QUARTER * BEATS_PER_MEASURE;

    // MIDI Specification
    static constexpr int CHANNEL_COUNT = 15;  // 16th reserved for metronome
    static constexpr int METRONOME_CHANNEL = 15;
    static constexpr int TOTAL_CHANNELS = 16;
    static constexpr int MIDI_NOTE_COUNT = 128;
    static constexpr int MAX_MIDI_NOTE = 127;
    static constexpr int PROGRAM_COUNT = 128;

    // Default Values
    static constexpr double DEFAULT_TEMPO = 120.0;
    static constexpr int DEFAULT_VOLUME = 100;
    static constexpr int DEFAULT_VELOCITY = 100;
}
```

**Replace magic numbers throughout codebase:**
- `960` → `MidiConstants::TICKS_PER_QUARTER`
- `15` → `MidiConstants::CHANNEL_COUNT`
- `128` → `MidiConstants::MIDI_NOTE_COUNT` or `PROGRAM_COUNT`
- `120` → `MidiConstants::DEFAULT_TEMPO`
- etc.

---

### Todo 28: Consider Transport Encapsulation

**Current:** Transport state is public and directly manipulated

**Consideration:** Make `mState` private and add methods:
```cpp
class Transport
{
public:
    void StartPlayback();
    void StartRecording();
    void Stop();
    bool IsPlaying() const;
    bool IsRecording() const;

private:
    State mState;  // Now private!
};
```

**Note:** This is a larger refactor. Consider doing this after all other todos are complete.

---

### Todo 29: Testing

After each phase, test:
1. **Playback** - Play existing tracks
2. **Recording** - Record new notes
3. **Editing** - Add, move, resize, delete notes
4. **Undo/Redo** - All operations
5. **Copy/Paste** - Note clipboard
6. **Selection** - Rectangle select, Ctrl+A
7. **Quantize** - Grid snapping
8. **Loop Recording** - Loop playback during record
9. **Metronome** - Click track
10. **Solo/Mute** - Channel routing

---

## Summary

**Total Refactoring Tasks:** 29
**Estimated Lines Removed:** 400+ lines of duplicate code
**Estimated Lines Added:** 200 lines of clean, reusable methods
**Net Change:** -200 lines, +100% clarity

**Key Benefits:**
- Centralized MIDI playback (DRY)
- Proper Model-View separation (SoC)
- Fixed solo/mute inconsistency bug
- Eliminated 6+ instances of note-finding duplication
- Reduced MainFrame complexity significantly
- Made AppModel the single source of truth

Good luck with the refactoring! Work through the todos in order, test frequently, and feel free to adjust as needed.
