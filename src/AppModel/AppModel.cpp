// AppModel.cpp
#include "AppModel.h"
#include <algorithm>
#include "Commands/RecordCommand.h"
#include "Commands/QuantizeCommand.h"
#include "Commands/NoteEditCommands.h"
#include "Commands/DeleteMultipleNotesCommand.h"
#include "Commands/PasteCommand.h"

AppModel::AppModel()
	: mNoteEditor(mTrackSet, mSoundBank)
	, mProjectManager(mTransport, mSoundBank, mTrackSet, mRecordingSession)
	, mMetronomeService(mSoundBank)
{
	mLastTick = std::chrono::steady_clock::now();
	mMetronomeService.Initialize();

	// Setup ProjectManager callbacks
	mProjectManager.SetDirtyStateCallback([this](bool dirty) {
		if (mDirtyStateCallback) {
			mDirtyStateCallback(dirty);
		}
	});

	mProjectManager.SetClearUndoHistoryCallback([this]() {
		mUndoRedoManager.ClearHistory();
	});

	// Setup UndoRedoManager callback
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
		case Transport::State::Stopped:										break; 
		case Transport::State::ClickedPlay:		HandleClickedPlay();		break;
		case Transport::State::Playing:			HandlePlaying();			break;
		case Transport::State::ClickedRecord:	HandleClickedRecord();		break;
		case Transport::State::Recording:		HandleRecording();			break;
		case Transport::State::FastForwarding:
		case Transport::State::Rewinding:		HandleFastForwardRewind();	break;
	}

	HandleIncomingMidi();
}
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

void AppModel::StopPlaybackIfActive()
{
	if (mTransport.IsPlaying())
	{
		mTransport.SetState(Transport::State::StopPlaying);
	}
	else if (mTransport.IsRecording())
	{
		mTransport.SetState(Transport::State::StopRecording);
	}
}

void AppModel::QuantizeAllTracks(uint64_t gridSize)
{
	StopPlaybackIfActive();

	auto commands = mNoteEditor.CreateQuantizeAllTracks(gridSize);
	for (auto& cmd : commands)
	{
		mUndoRedoManager.ExecuteCommand(std::move(cmd));
	}
}

void AppModel::AddNoteToRecordChannels(ubyte pitch, uint64_t startTick, uint64_t duration)
{
	auto cmd = mNoteEditor.CreateAddNoteToRecordChannels(pitch, startTick, duration);
	if (cmd)
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

void AppModel::MoveNote(const NoteLocation& note, uint64_t newStartTick, ubyte newPitch)
{
	auto cmd = mNoteEditor.CreateMoveNote(note, newStartTick, newPitch);
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

void AppModel::SetNoteMovePreview(const NoteLocation& note, uint64_t newStartTick, ubyte newPitch)
{
	mNoteEditor.SetNoteMovePreview(note, newStartTick, newPitch);
}

void AppModel::SetNoteResizePreview(const NoteLocation& note, uint64_t newEndTick)
{
	mNoteEditor.SetNoteResizePreview(note, newEndTick);
}

void AppModel::ClearNoteEditPreview()
{
	mNoteEditor.ClearNoteEditPreview();
}

const NoteEditor::NoteEditPreview& AppModel::GetNoteEditPreview() const
{
	return mNoteEditor.GetNoteEditPreview();
}

bool AppModel::HasNoteEditPreview() const
{
	return mNoteEditor.HasNoteEditPreview();
}

// Dirty state notification
void AppModel::SetDirtyStateCallback(DirtyStateCallback callback)
{
	mDirtyStateCallback = callback;
}


// Clipboard operations
void AppModel::CopyNotesToClipboard(const std::vector<NoteLocation>& notes)
{
	mClipboard.CopyNotes(notes, mTrackSet);
}

// Paste Clipboard notes at given tick position
// param optional: default pasteTick is the transport's playhead
void AppModel::PasteNotes(uint64_t pasteTick)
{
	if (!mClipboard.HasData()) return;

	if (pasteTick == UINT64_MAX)
	{
		pasteTick = mTransport.GetCurrentTick();
	}
	auto cmd = std::make_unique<PasteCommand>(mTrackSet, mClipboard.GetNotes(), pasteTick);
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

// Private Methods
uint64_t AppModel::GetDeltaTimeMs()
{
	auto now = std::chrono::steady_clock::now();
	auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - mLastTick).count();
	mLastTick = now;
	return static_cast<uint64_t>(delta);
}

bool AppModel::IsMusicalMessage(const MidiMessage& msg)
{
	using MidiInterface::MidiEvent;
	auto event = msg.getEventType();

	return (event >= NOTE_OFF && event <= PITCH_BEND && event != PROGRAM_CHANGE);

}

void AppModel::PlayMessages(std::vector<MidiMessage> msgs)
{
	if (msgs.empty()) return;

	auto player = mSoundBank.GetMidiOutDevice();
	auto channels = mSoundBank.GetAllChannels();

	bool solosFound = mSoundBank.SolosFound();

	for (auto& mm : msgs)
	{
		ubyte c = mm.getChannel();
		auto& channel = mSoundBank.GetChannel(c);
		if (mSoundBank.ShouldChannelPlay(channel, false))
		{
			player->sendMessage(mm);
		}
	}
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

			if (isRecording && c.record && IsMusicalMessage(routed))
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
	uint64_t loopDuration = mTransport.GetLoopSettings().endTick -
		mTransport.GetLoopSettings().startTick;
	mDrumMachine.UpdatePattern(loopDuration);

	// GetPattern() will regenerate if dirty
	const Track& pattern = mDrumMachine.GetPattern();

	// Find events between lastTick and currentTick (prevents missing events if tick jumps)
	for (const auto& event : pattern)
	{
		if (event.tick >= lastTick && event.tick <= currentTick)
		{
			messages.push_back(event.mm);
		}
		else if (event.tick > currentTick)
		{
			break;  // Pattern is sorted, no more events in range
		}
	}

	return messages;
}

// TRANSPORT STATE HANDLERS

void AppModel::HandleStopRecording()
{
	mTransport.Stop();
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
	mTransport.Stop();
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
	if (loopSettings.enabled && currentTick >= loopSettings.endTick)
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
	if (loopSettings.enabled)
	{
		auto drumMessages = PlayDrumMachinePattern(lastTick, currentTick);
		messages.insert(messages.end(), drumMessages.begin(), drumMessages.end());
	}

	PlayMessages(messages);
}
