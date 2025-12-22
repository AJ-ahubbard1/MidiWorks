// ProjectManager.cpp
#include "ProjectManager.h"
#include "AppModel/Transport/Transport.h"
#include "AppModel/SoundBank/SoundBank.h"
#include "AppModel/TrackSet/TrackSet.h"
#include "AppModel/RecordingSession/RecordingSession.h"
#include "MidiConstants.h"
#include "External/json.hpp"
#include <fstream>

using json = nlohmann::json;

ProjectManager::ProjectManager(
	Transport& transport,
	SoundBank& soundBank,
	TrackSet& trackSet,
	RecordingSession& recordingSession
)
	: mTransport(transport)
	, mSoundBank(soundBank)
	, mTrackSet(trackSet)
	, mRecordingSession(recordingSession)
	, mIsDirty(false)
{
}

bool ProjectManager::SaveProject(const std::string& filepath)
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

		// 2. Channels (CHANNEL_COUNT channels, 0-14)
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

		// 3. Tracks (CHANNEL_COUNT tracks, one per channel)
		project["tracks"] = json::array();
		for (int i = 0; i < MidiConstants::CHANNEL_COUNT; i++) {
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
		MarkClean();

		return true;
	}
	catch (const std::exception& e) {
		// Log error or show dialog
		return false;
	}
}

bool ProjectManager::LoadProject(const std::string& filepath)
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
		if (mClearUndoHistoryCallback) {
			mClearUndoHistoryCallback();
		}

		// Update state
		mCurrentProjectPath = filepath;
		MarkClean();

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

void ProjectManager::ClearProject()
{
	// Stop playback
	mTransport.Stop();
	mTransport.SetState(Transport::State::Stopped);

	// Reset transport to defaults
	mTransport.mTempo = MidiConstants::DEFAULT_TEMPO;
	mTransport.Reset();

	// Clear all tracks
	for (int i = 0; i < MidiConstants::CHANNEL_COUNT; i++) {
		mTrackSet.GetTrack(i).clear();
	}

	// Clear recording buffer
	mRecordingSession.Clear();

	// Reset channels to defaults
	for (int i = 0; i < MidiConstants::CHANNEL_COUNT; i++) {
		auto& ch = mSoundBank.GetChannel(i);
		ch.programNumber = 0;
		ch.volume = MidiConstants::DEFAULT_VOLUME;
		ch.mute = false;
		ch.solo = false;
		ch.record = false;
	}
	mSoundBank.ApplyChannelSettings();

	// Silence all channels
	mSoundBank.SilenceAllChannels();

	// Clear undo/redo
	if (mClearUndoHistoryCallback) {
		mClearUndoHistoryCallback();
	}

	// Reset project state
	mCurrentProjectPath.clear();
	MarkClean();
}

bool ProjectManager::IsProjectDirty() const
{
	return mIsDirty;
}

void ProjectManager::MarkDirty()
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

void ProjectManager::MarkClean()
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

const std::string& ProjectManager::GetCurrentProjectPath() const
{
	return mCurrentProjectPath;
}

void ProjectManager::SetDirtyStateCallback(DirtyStateCallback callback)
{
	mDirtyStateCallback = callback;
}

void ProjectManager::SetClearUndoHistoryCallback(ClearUndoHistoryCallback callback)
{
	mClearUndoHistoryCallback = callback;
}
