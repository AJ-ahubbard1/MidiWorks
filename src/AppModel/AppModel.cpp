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

	CheckMidiInQueue();
}
/* Checks for Midi In Messages
   If a SoundBank Channel is active, the message will playback.
   If a SoundBank Channel is set to record: the message will be added to the RecordingSession.
   This session stores midi messages temporarily during recording,
   when finished recording, the buffer is added to the track and sorted by timestamp.
 */
void AppModel::CheckMidiInQueue()
{
	if (!mMidiInputManager.GetDevice().checkForMessage()) return;

	MidiMessage mm = mMidiInputManager.GetDevice().getMessage();
	auto currentTick = mTransport.GetCurrentTick();
	auto channels = mSoundBank.GetAllChannels();
	bool solosFound = mSoundBank.SolosFound();

	// Notify callback of MIDI event (if registered)
	const auto& logCallback = mMidiInputManager.GetLogCallback();
	if (logCallback)
	{
		logCallback({mm, currentTick});
	}

	for (MidiChannel& c : channels)
	{
		if (mSoundBank.ShouldChannelPlay(c, true))
		{
			MidiMessage routed = mm;
			routed.setChannel(c.channelNumber);
			mSoundBank.GetMidiOutDevice()->sendMessage(routed);

			if (c.record && IsMusicalMessage(routed) && mTransport.IsRecording())
			{
				mRecordingSession.AddEvent({routed, currentTick});

				// Track active notes for loop recording
				ubyte status = routed.mData[0] & 0xF0;
				ubyte pitch = routed.getPitch();
				ubyte channel = routed.getChannel();

				if (status == 0x90 && routed.mData[2] > 0)  // NoteOn with velocity > 0
				{
					mRecordingSession.StartNote(pitch, channel, currentTick);
				}
				else if (status == 0x80 || (status == 0x90 && routed.mData[2] == 0))  // NoteOff
				{
					mRecordingSession.StopNote(pitch, channel);
				}
			}
		}
	}
}

SoundBank& AppModel::GetSoundBank() { return mSoundBank; }
Transport& AppModel::GetTransport() { return mTransport; }
TrackSet& AppModel::GetTrackSet() { return mTrackSet; }
RecordingSession& AppModel::GetRecordingSession() { return mRecordingSession; }
ProjectManager& AppModel::GetProjectManager() { return mProjectManager; }
Clipboard& AppModel::GetClipboard() { return mClipboard; }
UndoRedoManager& AppModel::GetUndoRedoManager() { return mUndoRedoManager; }
MidiInputManager& AppModel::GetMidiInputManager() { return mMidiInputManager; }
MetronomeService& AppModel::GetMetronomeService() { return mMetronomeService; }

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

// TRANSPORT STATE HANDLERS

void AppModel::HandleStopRecording()
{
	mTransport.Stop();
	mTransport.SetState(Transport::State::Stopped);

	// Create RecordCommand to make recording undoable
	if (!mRecordingSession.IsEmpty())
	{
		auto cmd = std::make_unique<RecordCommand>(mTrackSet, mRecordingSession.GetBuffer());
		mUndoRedoManager.ExecuteCommand(std::move(cmd));
		mRecordingSession.Clear();
	}

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
	std::vector<MidiMessage> messages;

	uint64_t lastTick = mTransport.GetCurrentTick();
	mTransport.UpdatePlayBack(GetDeltaTimeMs());
	uint64_t currentTick = mTransport.GetCurrentTick();

	// Loop-back logic (check BEFORE metronome to avoid double-click at loop boundary)
	if (mTransport.mLoopEnabled && currentTick >= mTransport.mLoopEndTick - 1)
	{
		mTransport.ShiftToTick(mTransport.mLoopStartTick);
		mTrackSet.FindStart(mTransport.mLoopStartTick);
		currentTick = mTransport.GetCurrentTick();  // Update currentTick after loop-back
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
	PlayMessages(messages);
}

void AppModel::HandleClickedRecord()
{
	// See HandleClickedPlay for reason behind GetDeltaTimeMs call
	GetDeltaTimeMs();
	mTrackSet.FindStart(mTransport.StartPlayBack());

	// Initialize recording buffer iterator for loop playback
	if (mTransport.mLoopEnabled)
	{
		mRecordingSession.InitializeLoopPlayback(mTransport.GetCurrentTick());
	}

	mTransport.SetState(Transport::State::Recording);
}

void AppModel::HandleRecording()
{
	std::vector<MidiMessage> messages;

	uint64_t lastTick = mTransport.GetCurrentTick();
	mTransport.UpdatePlayBack(GetDeltaTimeMs());
	uint64_t currentTick = mTransport.GetCurrentTick();

	// Loop-back logic (check BEFORE metronome to avoid double-click at loop boundary)
	if (mTransport.mLoopEnabled && currentTick >= mTransport.mLoopEndTick - 1)
	{
		// Auto-close any notes still held at loop end to prevent stuck notes
		uint64_t loopEndTick = mTransport.mLoopEndTick - 1;
		mRecordingSession.CloseAllActiveNotes(loopEndTick);

		// Merge overlapping notes before jumping back
		TrackSet::MergeOverlappingNotes(mRecordingSession.GetBuffer());

		mTransport.ShiftToTick(mTransport.mLoopStartTick);
		mTrackSet.FindStart(mTransport.mLoopStartTick);
		currentTick = mTransport.GetCurrentTick();  // Update currentTick after loop-back

		// Reset recording buffer iterator to loop start position
		mRecordingSession.ResetLoopPlayback(mTransport.mLoopStartTick);
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
	if (mTransport.mLoopEnabled)
	{
		auto loopMessages = mRecordingSession.GetLoopPlaybackMessages(currentTick);
		messages.insert(messages.end(), loopMessages.begin(), loopMessages.end());
	}

	PlayMessages(messages);
}

void AppModel::HandleFastForwardRewind()
{
	mTransport.ShiftCurrentTime();
}

