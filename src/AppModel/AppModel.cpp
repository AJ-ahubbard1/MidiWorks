// AppModel.cpp
#include "AppModel.h"
#include <algorithm>
#include "Commands/RecordCommand.h"
#include "Commands/NoteEditCommands.h"
#include "Commands/MultiNoteCommands.h"
#include "Commands/TrackCommands.h"
#include "Commands/ClipboardCommands.h"

AppModel::AppModel()
	: mNoteEditor(mTrackSet, mSoundBank)
	, mProjectManager(mTransport, mSoundBank, mTrackSet, mRecordingSession)
	, mMetronomeService(mSoundBank)
{
	mMetronomeService.Initialize();

	// The ProjectManager uses callback to clear the undo history on 
	// ClearProject() method calls and on LoadProject() method calls.
	mProjectManager.SetClearUndoHistoryCallback([this]() {
		mUndoRedoManager.ClearHistory();
	});

	// The UndoRedoManager uses callback to mark projects dirty when 
	// any commands are executed to show the project has unsaved changes
	mUndoRedoManager.SetCommandExecutedCallback([this]() {
		mProjectManager.MarkDirty();
	});
}

// Called inside of MainFrame::OnTimer event
void AppModel::Update()
{
	switch (mTransport.GetState())
	{
	case Transport::State::StopRecording:	HandleStopRecording();		break;
	case Transport::State::StopPlaying:		HandleStopPlaying();		break;
	case Transport::State::Stopped:										break; // Do nothing 
	case Transport::State::ClickedPlay:		HandleClickedPlay();		break;
	case Transport::State::Playing:			HandlePlaying();			break;
	case Transport::State::ClickedRecord:	HandleClickedRecord();		break;
	case Transport::State::Recording:		HandleRecording();			break;
	case Transport::State::FastForwarding:
	case Transport::State::Rewinding:		HandleFastForwardRewind();	break;
	}

	HandleIncomingMidi();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// COMMMAND METHODS: creates the commands and uses UndoRedoManager to execute them, adding them to undo stack 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Adds a note to all channels that have Record enabled
void AppModel::AddNoteToRecordChannels(ubyte pitch, uint64_t startTick, uint64_t duration)
{
	auto cmd = mNoteEditor.CreateAddNoteToRecordChannels(pitch, startTick, duration);
	if (cmd)
	{
		mUndoRedoManager.ExecuteCommand(std::move(cmd));
	}
}

void AppModel::QuantizeAllTracks(uint64_t gridSize)
{
	mTransport.StopPlaybackIfActive();

	auto commands = mNoteEditor.CreateQuantizeAllTracks(gridSize);
	for (auto& cmd : commands)
	{
		mUndoRedoManager.ExecuteCommand(std::move(cmd));
	}
}

void AppModel::DeleteNote(const NoteLocation& note)
{
	auto cmd = mNoteEditor.CreateDeleteNote(note);
	if (cmd)
	{
		mUndoRedoManager.ExecuteCommand(std::move(cmd));
	}
}

void AppModel::DeleteNotes(const std::vector<NoteLocation>& notes)
{
	auto cmd = mNoteEditor.CreateDeleteNotes(notes);
	if (cmd)
	{
		mUndoRedoManager.ExecuteCommand(std::move(cmd));
	}
}

void AppModel::ClearTrack(ubyte trackNumber)
{
	auto cmd = std::make_unique<ClearTrackCommand>(mTrackSet.GetTrack(trackNumber), trackNumber);
	if (cmd)
	{
		mUndoRedoManager.ExecuteCommand(std::move(cmd));
	}
}

void AppModel::MoveNote(const NoteLocation& note, uint64_t newStartTick, ubyte newPitch)
{
	auto cmd = mNoteEditor.CreateMoveNote(note, newStartTick, newPitch);
	if (cmd)
	{
		mUndoRedoManager.ExecuteCommand(std::move(cmd));
	}
}

void AppModel::MoveMultipleNotes(const std::vector<NoteLocation>& notes, int64_t tickDelta, int pitchDelta)
{
	auto cmd = mNoteEditor.CreateMoveMultipleNotes(notes, tickDelta, pitchDelta);
	if (cmd)
	{
		mUndoRedoManager.ExecuteCommand(std::move(cmd));
	}
}

void AppModel::ResizeNote(const NoteLocation& note, uint64_t newDuration)
{
	auto cmd = mNoteEditor.CreateResizeNote(note, newDuration);
	if (cmd)
	{
		mUndoRedoManager.ExecuteCommand(std::move(cmd));
	}
}

void AppModel::EditNoteVelocity(const NoteLocation& note, ubyte newVelocity)
{
	auto cmd = mNoteEditor.CreateEditNoteVelocity(note, newVelocity);
	if (cmd)
	{
		mUndoRedoManager.ExecuteCommand(std::move(cmd));
	}
}

// First checks for collisions at the new note region before moving the preview note
void AppModel::SetNoteMovePreview(const NoteLocation& note, uint64_t newStartTick, ubyte newPitch)
{
	uint64_t newEndTick = newStartTick + note.endTick - note.startTick;

	if (IsRegionCollisionFree(newStartTick, newEndTick, newPitch, note.trackIndex, &note))
	{
		mNoteEditor.SetNoteMovePreview(note, newStartTick, newPitch);
	}
}

void AppModel::SetMultipleNotesMovePreview(const std::vector<NoteLocation>& notes, int64_t tickDelta, int pitchDelta)
{
	// Check each note for collisions
	for (const auto& note : notes)
	{
		// Calculate new position
		int64_t newStartTick = static_cast<int64_t>(note.startTick) + tickDelta;
		int newPitch = static_cast<int>(note.pitch) + pitchDelta;

		// Validate bounds
		if (newStartTick < 0 || newPitch < 0 || newPitch > 127)
			return;  // Out of bounds, reject entire move

		uint64_t newEndTick = newStartTick + (note.endTick - note.startTick);

		// Check if this note would collide (excluding all notes being moved)
		if (!IsRegionCollisionFree(newStartTick, newEndTick, static_cast<ubyte>(newPitch), note.trackIndex, notes))
			return;  // Collision detected, reject entire move
	}

	// All notes are collision-free, allow preview
	mNoteEditor.SetMultipleNotesMovePreview(notes, tickDelta, pitchDelta);
}

void AppModel::SetNoteResizePreview(const NoteLocation& note, uint64_t newEndTick)
{
	if (IsRegionCollisionFree(note.startTick, newEndTick, note.pitch, note.trackIndex, &note))
	{
		mNoteEditor.SetNoteResizePreview(note, newEndTick);
	}
}

// Clipboard operations

// Paste Clipboard notes at given tick position
// param optional: default pasteTick is the transport's playhead
void AppModel::PasteNotes(std::optional<uint64_t> pasteTick)
{
	if (!mClipboard.HasData()) return;

	uint64_t tick = pasteTick.value_or(mTransport.GetCurrentTick());
	auto cmd = std::make_unique<PasteCommand>(mTrackSet, mClipboard.GetNotes(), tick);
	mUndoRedoManager.ExecuteCommand(std::move(cmd));
}

// Paste Clipboard notes to record-enabled tracks at given tick position
// param optional: default pasteTick is the transport's playhead
void AppModel::PasteNotesToRecordTracks(std::optional<uint64_t> pasteTick)
{
	if (!mClipboard.HasData()) return;

	// Get record-enabled channels and convert to track indices
	auto recordChannels = mSoundBank.GetRecordEnabledChannels();
	if (recordChannels.empty()) return;  // No record-enabled tracks

	std::vector<int> targetTracks;
	for (MidiChannel* channel : recordChannels)
	{
		targetTracks.push_back(static_cast<int>(channel->channelNumber));
	}

	// Create and execute paste command
	uint64_t tick = pasteTick.value_or(mTransport.GetCurrentTick());
	auto cmd = std::make_unique<PasteToTracksCommand>(mTrackSet, mClipboard.GetNotes(), tick, targetTracks);
	mUndoRedoManager.ExecuteCommand(std::move(cmd));
}

// Record drum machine pattern to TrackSet within loop region
void AppModel::RecordDrumPatternToTrack()
{
	// Get the current drum pattern
	const Track& pattern = mDrumMachine.GetPattern();
	if (pattern.empty()) return;

	// Create a copy offset by loop start
	std::vector<TimedMidiEvent> buffer;
	uint64_t loopStart = mTransport.GetLoopSettings().startTick;

	for (const auto& event : pattern)
	{
		TimedMidiEvent offsetEvent = event;
		offsetEvent.tick += loopStart;
		buffer.push_back(offsetEvent);
	}

	// Use existing RecordCommand for undo/redo support
	auto cmd = std::make_unique<RecordCommand>(mTrackSet, buffer);
	mUndoRedoManager.ExecuteCommand(std::move(cmd));
}

// Trigger drum pad via keyboard - plays sound immediately and enables pad if loop is playing
int AppModel::TriggerDrumPad(int rowIndex)
{
	// Respect mute state
	if (mDrumMachine.IsMuted()) return -1;

	// Validate row index
	if (rowIndex < 0 || rowIndex >= static_cast<int>(mDrumMachine.GetRowCount())) return -1;

	// Get drum row info
	const DrumRow& row = mDrumMachine.GetRow(rowIndex);
	ubyte channel = mDrumMachine.GetChannel();

	// Play sound immediately (NoteOn only - let drum sound decay naturally)
	MidiMessage noteOn = MidiMessage::NoteOn(row.pitch, 100, channel);  // Default velocity 100
	mSoundBank.GetMidiOutDevice()->sendMessage(noteOn);

	// If loop is playing, enable the pad at the quantized column position
	if (mTransport.IsPlaying() && mTransport.GetLoopSettings().enabled)
	{
		uint64_t currentTick = mTransport.GetCurrentTick();
		uint64_t loopStart = mTransport.GetLoopSettings().startTick;

		int column = mDrumMachine.GetColumnAtTick(currentTick, loopStart);
		if (column >= 0)  // Valid column
		{
			mDrumMachine.EnablePad(rowIndex, column);
			return column;  // Return column index
		}
	}

	return -1;  // No pad was enabled
}

// Send NoteOff for drum pad when key is released
void AppModel::ReleaseDrumPad(int rowIndex)
{
	// Validate row index
	if (rowIndex < 0 || rowIndex >= static_cast<int>(mDrumMachine.GetRowCount())) return;

	// Get drum row info
	const DrumRow& row = mDrumMachine.GetRow(rowIndex);
	ubyte channel = mDrumMachine.GetChannel();

	// Send NoteOff
	MidiMessage noteOff = MidiMessage::NoteOff(row.pitch, channel);
	mSoundBank.GetMidiOutDevice()->sendMessage(noteOff);
}

// Collision detection helper - single note exclusion
bool AppModel::IsRegionCollisionFree(uint64_t startTick, uint64_t endTick, ubyte pitch, int channel, const NoteLocation* excludeNote) const
{
	auto collisions = mTrackSet.FindNotesInRegion(startTick, endTick, pitch, pitch, channel);

	if (collisions.empty())
		return true;

	// If there's only one collision and it's the note we're excluding, that's fine
	if (excludeNote && collisions.size() == 1 &&
		collisions[0].noteOnIndex == excludeNote->noteOnIndex)
		return true;

	return false;
}

// Collision detection helper - multiple note exclusion
bool AppModel::IsRegionCollisionFree(uint64_t startTick, uint64_t endTick, ubyte pitch, int channel, const std::vector<NoteLocation>& excludeNotes) const
{
	auto collisions = mTrackSet.FindNotesInRegion(startTick, endTick, pitch, pitch, channel);

	if (collisions.empty())
		return true;

	// Check if all collisions are in the exclude list
	for (const auto& collision : collisions)
	{
		bool isExcluded = false;
		for (const auto& exclude : excludeNotes)
		{
			if (collision.noteOnIndex == exclude.noteOnIndex)
			{
				isExcluded = true;
				break;
			}
		}

		if (!isExcluded)
			return false;  // Found a collision that's not excluded
	}

	return true;
}

// Private Methods

/* Handles incoming MIDI messages from input device
   Polls for incoming messages, routes them to appropriate channels,
   plays them back, and records them if recording is active.
 */
void AppModel::HandleIncomingMidi()
{
	auto message = mMidiInputManager.PollAndNotify(mTransport.GetCurrentTick());
	if (!message) return;

	RouteAndPlayMessage(*message, mTransport.GetCurrentTick());
}


// Returns the change in time from lastTick's last value to now, then updates lastTick 
uint64_t AppModel::GetDeltaTimeMs()
{
	static std::chrono::steady_clock::time_point lastTick = std::chrono::steady_clock::now();

	auto now = std::chrono::steady_clock::now();
	auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTick).count();
	lastTick = now;
	return static_cast<uint64_t>(delta);
}

void AppModel::RouteAndPlayMessage(const MidiMessage& mm, uint64_t currentTick)
{
	auto channels = mSoundBank.GetAllChannels();
	bool isRecording = mTransport.IsRecording();
	for (MidiChannel& c : channels)
	{
		if (mSoundBank.ShouldChannelPlay(c, true))
		{
			MidiMessage routed = mm;
			routed.setChannel(c.channelNumber);
			mSoundBank.GetMidiOutDevice()->sendMessage(routed);

			if (isRecording && c.record && routed.isMusicalMessage())
			{
				mRecordingSession.RecordEvent(routed, currentTick);
			}
		}
	}
}

std::vector<MidiMessage> AppModel::PlayDrumMachinePattern(uint64_t lastTick, uint64_t currentTick)
{
	std::vector<MidiMessage> messages;

	// Update loop duration (cheap - just sets flag if duration changed)
	auto loopSettings = mTransport.GetLoopSettings();
	uint64_t loopDuration = loopSettings.endTick - loopSettings.startTick;
	mDrumMachine.UpdatePattern(loopDuration);

	// GetPattern() will regenerate if dirty
	const Track& pattern = mDrumMachine.GetPattern();

	// Find events between lastTick and currentTick (prevents missing events if tick jumps)
	for (const auto& event : pattern)
	{
		// Drum pattern starts at 0, need to offset by loop's start tick for
		// pattern to match loop region
		uint64_t tick = event.tick + loopSettings.startTick;
		if (tick >= lastTick && tick < currentTick)
		{
			messages.push_back(event.mm);
		}
		else if (tick > currentTick)
		{
			break;  // Pattern is sorted, no more events in range
		}
	}

	return messages;
}

// TRANSPORT STATE HANDLERS

void AppModel::HandleStopRecording()
{
	mTransport.SetState(Transport::State::Stopped);

	// Close any notes still being held when stopping (prevents orphaned Note Ons)
	if (mRecordingSession.HasActiveNotes())
	{
		mRecordingSession.CloseAllActiveNotes(mTransport.GetCurrentTick());
	}

	// Create RecordCommand to make recording undoable
	if (!mRecordingSession.IsEmpty())
	{
		auto cmd = std::make_unique<RecordCommand>(mTrackSet, mRecordingSession.GetBuffer());
		mUndoRedoManager.ExecuteCommand(std::move(cmd));
	}

	// Always clear to ensure clean state for next recording
	mRecordingSession.Clear();

	mSoundBank.SilenceAllChannels();
}

void AppModel::HandleStopPlaying()
{
	mTransport.SetState(Transport::State::Stopped);
	mSoundBank.SilenceAllChannels();
}

void AppModel::HandleClickedPlay()
{
	// Call GetDeltaTimeMs:
	// This sets mLastTick to now to prepare for playback 
	// If call doesn't happen Delta would be HUGE 
	// (total time when transport state was stopped)
	// causing an unpredictable jump 
	GetDeltaTimeMs();

	mTrackSet.FindStart(mTransport.StartPlayBack());
	mTransport.SetState(Transport::State::Playing);
}


void AppModel::HandlePlaying()
{
	HandlePlaybackCore(false);
}

void AppModel::HandleClickedRecord()
{
	// See HandleClickedPlay for reason behind GetDeltaTimeMs call
	GetDeltaTimeMs();
	// Move trackset iterators to start of playback based on the playhead tick
	mTrackSet.FindStart(mTransport.StartPlayBack());
	mTransport.SetState(Transport::State::Recording);
}

void AppModel::HandleRecording()
{
	HandlePlaybackCore(true);
}

void AppModel::HandleFastForwardRewind()
{
	mSoundBank.SilenceAllChannels();
	mTransport.ShiftCurrentTime();
}

void AppModel::HandlePlaybackCore(bool isRecording)
{
	std::vector<MidiMessage> messages;

	uint64_t lastTick = mTransport.GetCurrentTick();
	mTransport.UpdatePlayBack(GetDeltaTimeMs());
	uint64_t currentTick = mTransport.GetCurrentTick();

	// Get loop settings once
	const auto& loopSettings = mTransport.GetLoopSettings();

	// Loop-back logic (check BEFORE metronome to avoid double-click at loop boundary)
	if (mTransport.ShouldLoopBack(currentTick))
	{
		// === RECORDING-SPECIFIC: Pre-wrap cleanup ===
		// Must happen BEFORE the wrap to finalize events at the old loop end position
		if (isRecording)
		{
			// Fix overlapping same-pitch notes to prevent merging artifacts
			TrackSet::SeparateOverlappingNotes(mRecordingSession.GetBuffer());

			// Wrap any notes still held at loop end to prevent stuck notes
			// note offs will be added at the loop end, note ons will be added at loop start
			uint64_t noteOffTick = loopSettings.endTick - MidiConstants::NOTE_SEPARATION_TICKS;
			mRecordingSession.WrapActiveNotesAtLoop(noteOffTick, loopSettings.startTick);
		}

		// === COMMON: Perform the loop wrap (both playback and recording) ===
		mTransport.ShiftToTick(loopSettings.startTick);
		mTrackSet.FindStart(loopSettings.startTick);
		uint64_t diff = currentTick - mTransport.GetCurrentTick();
		currentTick = mTransport.GetCurrentTick();  // Update currentTick after loop-back
		lastTick -= diff;

		// === RECORDING-SPECIFIC: Post-wrap setup ===
		// Must happen AFTER the wrap to initialize playback at the new loop start position
		if (isRecording)
		{
			// Reset recording buffer iterator to loop start position for playback
			mRecordingSession.ResetLoopPlayback(loopSettings.startTick);
		}
	}

	// Metronome
	if (mMetronomeService.IsEnabled())
	{
		auto beat = mTransport.CheckForBeat(lastTick, currentTick);
		if (beat.beatOccurred)
		{
			mSoundBank.PlayMetronomeClick(beat.isDownbeat);
		}
	}

	messages = mTrackSet.PlayBack(currentTick);

	// During loop recording, also play back the recording buffer
	// so you can hear what you recorded in previous loop iterations
	if (isRecording && loopSettings.enabled)
	{
		auto loopMessages = mRecordingSession.GetLoopPlaybackMessages(currentTick);
		messages.insert(messages.end(), loopMessages.begin(), loopMessages.end());
	}

	// Play drum machine pattern during loop playback
	if (loopSettings.enabled && !mDrumMachine.IsMuted())
	{
		auto drumMessages = PlayDrumMachinePattern(lastTick, currentTick);
		messages.insert(messages.end(), drumMessages.begin(), drumMessages.end());
	}

	mSoundBank.PlayMessages(messages);
}
