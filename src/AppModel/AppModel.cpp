// AppModel.cpp
#include "AppModel.h"
#include <fstream>
#include "external/json.hpp"
#include "Commands/RecordCommand.h"

using json = nlohmann::json;

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

			// Create RecordCommand to make recording undoable
			if (!mRecordingBuffer.empty())
			{
				auto cmd = std::make_unique<RecordCommand>(mTrackSet, mRecordingBuffer);
				ExecuteCommand(std::move(cmd));
				mRecordingBuffer.clear();
			}

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

	// Notify callback of MIDI event (if registered)
	if (mMidiLogCallback)
	{
		mMidiLogCallback({mm, currentTick});
	}

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

// MIDI Input port management
std::vector<std::string> AppModel::GetMidiInputPortNames() const
{
	return mMidiIn->getPortNames();
}

void AppModel::SetMidiInputPort(int portIndex)
{
	mMidiIn->changePort(portIndex);
}

int AppModel::GetCurrentMidiInputPort() const
{
	return mMidiIn->getCurrentPort();
}

// Logging system
void AppModel::SetLogCallback(MidiLogCallback callback)
{
	mMidiLogCallback = callback;
}

// Dirty state notification
void AppModel::SetDirtyStateCallback(DirtyStateCallback callback)
{
	mDirtyStateCallback = callback;
}

// Metronome settings
bool AppModel::IsMetronomeEnabled() const
{
	return mMetronomeEnabled;
}

void AppModel::SetMetronomeEnabled(bool enabled)
{
	mMetronomeEnabled = enabled;
}

// Command Pattern - Undo/Redo System
void AppModel::ExecuteCommand(std::unique_ptr<Command> cmd)
{
	cmd->Execute();
	mUndoStack.push_back(std::move(cmd));

	// Clear redo stack - can't redo after new action
	mRedoStack.clear();

	// Mark project as dirty (has unsaved changes)
	MarkDirty();

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

// Save/Load Project
bool AppModel::SaveProject(const std::string& filepath)
{
	try {
		json project;

		// Metadata
		project["version"] = "1.0";
		project["appVersion"] = "0.3";

		// 1. Transport
		project["transport"] = {
			{"tempo", mTransport.mTempo},
			{"timeSignature", {
				mTransport.mTimeSignatureNumerator,
				mTransport.mTimeSignatureDenominator
			}},
			{"currentTick", mTransport.GetCurrentTick()}
		};

		// 2. Channels (15 channels, 0-14)
		project["channels"] = json::array();
		auto channels = mSoundBank.GetAllChannels();
		for (const auto& ch : channels) {
			project["channels"].push_back({
				{"channelNumber", ch.channelNumber},
				{"programNumber", ch.programNumber},
				{"volume", ch.volume},
				{"mute", ch.mute},
				{"solo", ch.solo},
				{"record", ch.record}
			});
		}

		// 3. Tracks (15 tracks, one per channel)
		project["tracks"] = json::array();
		for (int i = 0; i < 15; i++) {
			Track& track = mTrackSet.GetTrack(i);
			json trackJson;
			trackJson["channel"] = i;
			trackJson["events"] = json::array();

			for (const auto& event : track) {
				trackJson["events"].push_back({
					{"tick", event.tick},
					{"midiData", {
						event.mm.mData[0],
						event.mm.mData[1],
						event.mm.mData[2]
					}}
				});
			}

			project["tracks"].push_back(trackJson);
		}

		// Write to file (pretty-printed with 4-space indent)
		std::ofstream file(filepath);
		if (!file.is_open()) {
			return false;
		}

		file << project.dump(4);
		file.close();

		// Update state
		mCurrentProjectPath = filepath;
		mIsDirty = false;

		return true;
	}
	catch (const std::exception& e) {
		// Log error or show dialog
		return false;
	}
}

bool AppModel::LoadProject(const std::string& filepath)
{
	try {
		// Read file
		std::ifstream file(filepath);
		if (!file.is_open()) {
			return false;
		}

		json project = json::parse(file);
		file.close();

		// Check version compatibility
		std::string version = project.value("version", "1.0");
		// TODO: Handle different versions if needed

		// 1. Transport
		mTransport.mTempo = project["transport"]["tempo"];
		mTransport.mTimeSignatureNumerator = project["transport"]["timeSignature"][0];
		mTransport.mTimeSignatureDenominator = project["transport"]["timeSignature"][1];

		// Optional: Restore playback position
		if (project["transport"].contains("currentTick")) {
			uint64_t tick = project["transport"]["currentTick"];
			mTransport.Reset();  // Reset to tick 0 for now
			// Note: If you want to restore position, you'll need to add a setter
		}

		// 2. Channels
		int channelIdx = 0;
		for (const auto& chJson : project["channels"]) {
			auto& ch = mSoundBank.GetChannel(channelIdx++);
			ch.programNumber = chJson["programNumber"];
			ch.volume = chJson["volume"];
			ch.mute = chJson["mute"];
			ch.solo = chJson["solo"];
			ch.record = chJson["record"];
		}

		// IMPORTANT: Apply channel settings to MIDI device
		mSoundBank.ApplyChannelSettings();

		// 3. Tracks
		for (const auto& trackJson : project["tracks"]) {
			int channel = trackJson["channel"];
			Track& track = mTrackSet.GetTrack(channel);
			track.clear();

			for (const auto& eventJson : trackJson["events"]) {
				TimedMidiEvent event;
				event.tick = eventJson["tick"];
				event.mm.mData[0] = eventJson["midiData"][0];
				event.mm.mData[1] = eventJson["midiData"][1];
				event.mm.mData[2] = eventJson["midiData"][2];
				track.push_back(event);
			}
		}

		// Clear undo/redo history (don't restore edit history)
		ClearUndoHistory();

		// Update state
		mCurrentProjectPath = filepath;
		mIsDirty = false;

		return true;
	}
	catch (json::parse_error& e) {
		// Invalid JSON
		return false;
	}
	catch (json::type_error& e) {
		// Wrong data type (e.g., expected number, got string)
		return false;
	}
	catch (json::out_of_range& e) {
		// Missing required field
		return false;
	}
	catch (const std::exception& e) {
		// Other errors
		return false;
	}
}

void AppModel::ClearProject()
{
	// Stop playback
	mTransport.Stop();
	mTransport.mState = Transport::State::Stopped;

	// Reset transport to defaults
	mTransport.mTempo = 120.0;
	mTransport.Reset();

	// Clear all tracks
	for (int i = 0; i < 15; i++) {
		mTrackSet.GetTrack(i).clear();
	}

	// Clear recording buffer
	mRecordingBuffer.clear();

	// Reset channels to defaults
	for (int i = 0; i < 15; i++) {
		auto& ch = mSoundBank.GetChannel(i);
		ch.programNumber = 0;
		ch.volume = 100;
		ch.mute = false;
		ch.solo = false;
		ch.record = false;
	}
	mSoundBank.ApplyChannelSettings();

	// Silence all channels
	SilenceAllChannels();

	// Clear undo/redo
	ClearUndoHistory();

	// Reset project state
	mCurrentProjectPath.clear();
	mIsDirty = false;
}

// Project state methods
bool AppModel::IsProjectDirty() const
{
	return mIsDirty;
}

void AppModel::MarkDirty()
{
	if (!mIsDirty)  // Only update if state actually changes
	{
		mIsDirty = true;
		if (mDirtyStateCallback)
		{
			mDirtyStateCallback(true);
		}
	}
}

void AppModel::MarkClean()
{
	if (mIsDirty)  // Only update if state actually changes
	{
		mIsDirty = false;
		if (mDirtyStateCallback)
		{
			mDirtyStateCallback(false);
		}
	}
}

const std::string& AppModel::GetCurrentProjectPath() const
{
	return mCurrentProjectPath;
}

