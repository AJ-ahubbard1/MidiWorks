// AppModel.cpp
#include "AppModel.h"

AppModel::AppModel()
{
	mMidiIn = std::make_shared<MidiIn>();
	InitializeMetronome();
}

void AppModel::InitializeMetronome()
{
	// Set channel 16 to woodblock sound for metronome
	// We're using channel 16 (index 15) to avoid conflicts with user channels
	// Program 115 = Woodblock (percussive, short click sound)
	auto player = mSoundBank.GetMidiOutDevice();
	player->sendMessage(MidiMessage::ProgramChange(115, METRONOME_CHANNEL));
}

// Called inside of MainFrame::OnTimer event
void AppModel::Update()
{
	static std::vector<MidiMessage> messages;
	switch (mTransport.mState)
	{
		case Transport::State::StopRecording:
			mTransport.Stop();
			mTransport.mState = Transport::State::Stopped;
			mTrackSet.FinalizeRecording(mRecordingBuffer);
			SilenceAllChannels();
			break;

		case Transport::State::StopPlaying:
			mTransport.Stop();
			mTransport.mState = Transport::State::Stopped;
			SilenceAllChannels();
			break;

		case Transport::State::Stopped:
			break;

		case Transport::State::ClickedPlay:
			GetDeltaTimeMs();
			mTrackSet.FindStart(mTransport.StartPlayBack());
			mTransport.mState = Transport::State::Playing;
			break;

		case Transport::State::Playing:
		{
			uint64_t lastTick = mTransport.GetCurrentTick();
			mTransport.UpdatePlayBack(GetDeltaTimeMs());
			uint64_t currentTick = mTransport.GetCurrentTick();

			// Metronome
			if (mMetronomeEnabled)
			{
				auto beat = mTransport.CheckForBeat(lastTick, currentTick);
				if (beat.beatOccurred)
				{
					PlayMetronomeClick(beat.isDownbeat);
				}
			}

			messages = mTrackSet.PlayBack(currentTick);
			PlayMessages(messages);
			break;
		}
		case Transport::State::ClickedRecord:
	    	GetDeltaTimeMs();
			mTrackSet.FindStart(mTransport.StartPlayBack());
			mTransport.mState= Transport::State::Recording;
			break;
		case Transport::State::Recording:
		{
			uint64_t lastTick = mTransport.GetCurrentTick();
			mTransport.UpdatePlayBack(GetDeltaTimeMs());
			uint64_t currentTick = mTransport.GetCurrentTick();

			// Metronome
			if (mMetronomeEnabled)
			{
				auto beat = mTransport.CheckForBeat(lastTick, currentTick);
				if (beat.beatOccurred)
				{
					PlayMetronomeClick(beat.isDownbeat);
				}
			}

			messages = mTrackSet.PlayBack(currentTick);
			PlayMessages(messages);
			break;
		}
		case Transport::State::FastForwarding:
		case Transport::State::Rewinding:
			mTransport.ShiftCurrentTime();
			break;
	}

	CheckMidiInQueue();
}
/* Checks for Midi In Messages
   If a SoundBank Channel is active, the message will playback.
   If a SoundBank Channel is set to record: the message will be pushed to the mRecordingBuffer.
   This buffer is used to temporarily store the midi messages during recording, 
   when finished recording, the buffer is added to the track and sorted by timestamp. 
 */
void AppModel::CheckMidiInQueue()
{
	if (!mMidiIn->checkForMessage()) return;

	MidiMessage mm = mMidiIn->getMessage();
	auto currentTick = mTransport.GetCurrentTick();
	auto channels = mSoundBank.GetAllChannels();
	bool solosFound = mSoundBank.SolosFound();
	mLogMessage = {mm, currentTick}; // logging midi in messages 
	mUpdateLog = true;
	for (MidiChannel& c : channels)
	{
		bool shouldPlay = solosFound ? c.solo : (c.record && !c.mute);
		if (shouldPlay)
		{
			MidiMessage routed = mm;
			routed.setChannel(c.channelNumber);
			mSoundBank.GetMidiOutDevice()->sendMessage(routed);

			if (c.record && IsMusicalMessage(routed) && mTransport.mState == Transport::State::Recording)
			{
				mRecordingBuffer.push_back({routed, currentTick});
			}
		}
	}
}

SoundBank& AppModel::GetSoundBank() { return mSoundBank; }
Transport& AppModel::GetTransport() { return mTransport; }
TrackSet& AppModel::GetTrackSet() { return mTrackSet; }
Track& AppModel::GetRecordingBuffer() { return mRecordingBuffer; }
Track& AppModel::GetTrack(ubyte c) { return mTrackSet.GetTrack(c); }

// Command Pattern - Undo/Redo System
void AppModel::ExecuteCommand(std::unique_ptr<Command> cmd)
{
	cmd->Execute();
	mUndoStack.push_back(std::move(cmd));

	// Clear redo stack - can't redo after new action
	mRedoStack.clear();

	// Limit stack size to prevent unbounded memory growth
	if (mUndoStack.size() > MAX_UNDO_STACK_SIZE)
	{
		mUndoStack.erase(mUndoStack.begin());
	}
}

void AppModel::Undo()
{
	if (mUndoStack.empty()) return;

	// Get command from undo stack
	auto cmd = std::move(mUndoStack.back());
	mUndoStack.pop_back();

	// Undo the command
	cmd->Undo();

	// Move to redo stack
	mRedoStack.push_back(std::move(cmd));
}

void AppModel::Redo()
{
	if (mRedoStack.empty()) return;

	// Get command from redo stack
	auto cmd = std::move(mRedoStack.back());
	mRedoStack.pop_back();

	// Re-execute the command
	cmd->Execute();

	// Move back to undo stack
	mUndoStack.push_back(std::move(cmd));
}

bool AppModel::CanUndo() const { return !mUndoStack.empty(); }
bool AppModel::CanRedo() const { return !mRedoStack.empty(); }

const std::vector<std::unique_ptr<Command>>& AppModel::GetUndoStack() const { return mUndoStack; }
const std::vector<std::unique_ptr<Command>>& AppModel::GetRedoStack() const { return mRedoStack; }

void AppModel::ClearUndoHistory()
{
	mUndoStack.clear();
	mRedoStack.clear();
}

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
	auto player = mSoundBank.GetMidiOutDevice();
	auto channels = mSoundBank.GetAllChannels();

	bool solosFound = mSoundBank.SolosFound();

	for (auto& mm : msgs)
	{
		ubyte c = mm.getChannel();
		auto& channel = mSoundBank.GetChannel(c);
		bool shouldPlay = solosFound ? channel.solo : !channel.mute;
		if (shouldPlay)
		{
			player->sendMessage(mm);
		}
	}
}

void AppModel::PlayMetronomeClick(bool isDownbeat)
{
	auto player = mSoundBank.GetMidiOutDevice();

	// Different pitches and velocities for downbeat vs other beats
	ubyte note = isDownbeat ? 76 : 72;      // High E vs High C (woodblock sounds good high-pitched)
	ubyte velocity = isDownbeat ? 127 : 90; // Downbeat louder

	// Send note on metronome channel (channel 16)
	player->sendMessage(MidiMessage::NoteOn(note, velocity, METRONOME_CHANNEL));

	// Note: Woodblock has short natural decay, but could add explicit note-off if needed
}

void AppModel::SilenceAllChannels()
{
	auto player = mSoundBank.GetMidiOutDevice();

	for (ubyte c = 0; c < 15; c++)
	{
		player->sendMessage(MidiMessage::AllNotesOff(c));
	}
}

