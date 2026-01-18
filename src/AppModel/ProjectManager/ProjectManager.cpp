// ProjectManager.cpp
#include "ProjectManager.h"
#include "AppModel/Transport/Transport.h"
#include "AppModel/SoundBank/SoundBank.h"
#include "AppModel/SoundBank/ChannelColors.h"
#include "AppModel/TrackSet/TrackSet.h"
#include "AppModel/RecordingSession/RecordingSession.h"
#include "MidiConstants.h"
#include "External/json.hpp"
#include <fstream>
#include "External/midifile/MidiFile.h"

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
	try 
	{
		json project;

		// Metadata
		project["version"] = "1.0";
		project["appVersion"] = "0.3";

		// 1. Transport
		auto beatSettings = mTransport.GetBeatSettings();
		project["transport"] = {
			{"tempo", beatSettings.tempo},
			{"timeSignature", {
				beatSettings.timeSignatureNumerator,
				beatSettings.timeSignatureDenominator
			}},
			{"currentTick", mTransport.GetCurrentTick()}
		};

		// 2. Channels (CHANNEL_COUNT channels, 0-14)
		project["channels"] = json::array();
		auto channels = mSoundBank.GetAllChannels();
		for (const auto& ch : channels) 
		{
			project["channels"].push_back({
				{"channelNumber", ch.channelNumber},
				{"programNumber", ch.programNumber},
				{"volume", ch.volume},
				{"mute", ch.mute},
				{"solo", ch.solo},
				{"record", ch.record},
				{"minimized", ch.minimized},
				{"customName", ch.customName},
				{"customColor", {
					{"r", ch.customColor.Red()},
					{"g", ch.customColor.Green()},
					{"b", ch.customColor.Blue()}
				}}
			});
		}

		// 3. Tracks (CHANNEL_COUNT tracks, one per channel)
		project["tracks"] = json::array();
		for (int i = 0; i < MidiConstants::CHANNEL_COUNT; i++) 
		{
			Track& track = mTrackSet.GetTrack(i);
			json trackJson;
			trackJson["channel"] = i;
			trackJson["events"] = json::array();

			for (const auto& event : track) 
			{
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
		if (!file.is_open())
		{
			if (mErrorCallback)
			{
				mErrorCallback("Save Failed", "Could not open file for writing: " + filepath);
			}
			return false;
		}

		file << project.dump(4);
		file.close();

		// Update state
		mCurrentProjectPath = filepath;
		MarkClean();

		return true;
	}
	catch (const std::exception& e)
	{
		if (mErrorCallback)
		{
			mErrorCallback("Save Failed", std::string("Error saving project: ") + e.what());
		}
		return false;
	}
}

bool ProjectManager::LoadProject(const std::string& filepath)
{
	try
	{
		// Read file
		std::ifstream file(filepath);
		if (!file.is_open())
		{
			if (mErrorCallback)
			{
				mErrorCallback("Load Failed", "Could not open file: " + filepath);
			}
			return false;
		}

		json project = json::parse(file);
		file.close();

		// Check version compatibility
		std::string version = project.value("version", "1.0");
		// TODO: Handle different versions if needed

		// 1. Transport
		Transport::BeatSettings beatSettings;
		beatSettings.tempo = project["transport"]["tempo"];
		beatSettings.timeSignatureNumerator = project["transport"]["timeSignature"][0];
		beatSettings.timeSignatureDenominator = project["transport"]["timeSignature"][1];
		mTransport.SetBeatSettings(beatSettings);

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

			// Load optional fields (for backwards compatibility)
			if (chJson.contains("minimized")) {
				ch.minimized = chJson["minimized"];
			}
			if (chJson.contains("customName")) {
				ch.customName = chJson["customName"];
			}
			if (chJson.contains("customColor")) {
				unsigned char r = chJson["customColor"]["r"];
				unsigned char g = chJson["customColor"]["g"];
				unsigned char b = chJson["customColor"]["b"];
				ch.customColor = wxColour(r, g, b);
			}
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
	catch (json::parse_error& e)
	{
		if (mErrorCallback)
		{
			mErrorCallback("Load Failed", std::string("Project file is corrupted or not valid JSON: ") + e.what());
		}
		return false;
	}
	catch (json::type_error& e)
	{
		if (mErrorCallback)
		{
			mErrorCallback("Load Failed", std::string("Project file has invalid data format: ") + e.what());
		}
		return false;
	}
	catch (json::out_of_range& e)
	{
		if (mErrorCallback)
		{
			mErrorCallback("Load Failed", std::string("Project file is missing required data: ") + e.what());
		}
		return false;
	}
	catch (const std::exception& e)
	{
		if (mErrorCallback)
		{
			mErrorCallback("Load Failed", std::string("Error loading project: ") + e.what());
		}
		return false;
	}
}

void ProjectManager::ClearProject()
{
	// Stop playback
	mTransport.SetState(Transport::State::Stopped);

	// Reset transport to defaults
	mTransport.SetBeatSettings(Transport::BeatSettings());  // Reset to defaults
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
		ch.minimized = false;
		ch.customName = "";
		ch.customColor = TRACK_COLORS[i];
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

bool ProjectManager::ExportMIDI(const std::string& filepath)
{
	try 
	{
		smf::MidiFile midifile;
		std::vector<ubyte> midievent;
		midievent.resize(3);

		auto beatSettings = mTransport.GetBeatSettings();

		midifile.setTPQ(960);	// Ticks Per Quarter

		// Add tempo and time signature to track 0
		midifile.addTempo(0, 0, beatSettings.tempo);
		midifile.addTimeSignature(0, 0, beatSettings.timeSignatureNumerator, beatSettings.timeSignatureDenominator);

		// Export each track
		for (int i = 0; i < MidiConstants::CHANNEL_COUNT; i++)
		{
			Track& track = mTrackSet.GetTrack(i);
			if (track.empty()) continue;

			int tracknum = midifile.addTrack();

			// Add program change
			midifile.addPatchChange(tracknum, 0, i, mSoundBank.GetChannel(i).programNumber);

			// Add all events
			for (const auto& event : track)
			{
				midievent[0] = event.mm.mData[0];
				midievent[1] = event.mm.mData[1];
				midievent[2] = event.mm.mData[2];
				midifile.addEvent(tracknum, event.tick, midievent);
			}
		}

		midifile.write(filepath);
		return true;
	}
	catch (const std::exception& e)
	{
		if (mErrorCallback)
		{
			mErrorCallback("Export Failed", std::string("Error exporting MIDI file: ") + e.what());
		}
		return false;
	}
}

bool ProjectManager::ImportMIDI(const std::string& filepath)
{
	try
	{
		smf::MidiFile midifile;
		if (!midifile.read(filepath))
		{
			if (mErrorCallback)
			{
				mErrorCallback("Import Failed", "Could not read MIDI file: " + filepath);
			}
			return false;
		}

		// Calculate tick conversion ratio (needed for event import)
		// Formula: newTick = oldTick * (targetPPQN / sourcePPQN)
		int sourcePPQN = midifile.getTicksPerQuarterNote();
		double tickConversion = (double)MidiConstants::TICKS_PER_QUARTER / (double)sourcePPQN;

		// Log import metadata for debugging
		std::ofstream logFile("import-midi.log", std::ios::app);
		if (logFile.is_open())
		{
			logFile << "\n========================================\n";
			logFile << "Import: " << filepath << "\n";
			logFile << "========================================\n";

			// Log original PPQN before conversion
			logFile << "Original PPQN: " << sourcePPQN << "\n";
			logFile << "Track Count: " << midifile.getTrackCount() << "\n";
			logFile << "File Duration (ticks): " << midifile.getFileDurationInTicks() << "\n";
			logFile << "File Duration (quarters): " << midifile.getFileDurationInQuarters() << "\n";

			// Log all tempo events
			logFile << "\nTempo Events:\n";
			for (int track = 0; track < midifile.getTrackCount(); track++)
			{
				for (int event = 0; event < midifile[track].size(); event++)
				{
					if (midifile[track][event].isTempo())
					{
						logFile << "  Track " << track
						        << ", Tick " << midifile[track][event].tick
						        << ": " << midifile[track][event].getTempoBPM() << " BPM\n";
					}
				}
			}

			// Log all time signature events
			logFile << "\nTime Signature Events:\n";
			for (int track = 0; track < midifile.getTrackCount(); track++)
			{
				for (int event = 0; event < midifile[track].size(); event++)
				{
					if (midifile[track][event].isTimeSignature())
					{
						int numerator = midifile[track][event][3];
						int denominator = (int)pow(2, midifile[track][event][4]);
						logFile << "  Track " << track
						        << ", Tick " << midifile[track][event].tick
						        << ": " << numerator << "/" << denominator << "\n";
					}
				}
			}

			// Log conversion info
			logFile << "\nConverting to PPQN: " << MidiConstants::TICKS_PER_QUARTER << "\n";
			logFile << "Tick Conversion Ratio: " << tickConversion << "x\n";
			logFile.close();
		}

		// Clear existing tracks before importing
		for (int i = 0; i < MidiConstants::CHANNEL_COUNT; i++)
		{
			mTrackSet.GetTrack(i).clear();
		}

		// Extract tempo from first tempo event (if present)
		double tempo = 120.0;  // Default tempo
		for (int track = 0; track < midifile.getTrackCount(); track++)
		{
			for (int event = 0; event < midifile[track].size(); event++)
			{
				if (midifile[track][event].isTempo())
				{
					tempo = midifile[track][event].getTempoBPM();
					break;
				}
			}
			if (tempo != 120.0) break;
		}

		// Extract time signature from first time signature event (if present)
		int timeSignatureNumerator = 4;    // Default 4/4
		int timeSignatureDenominator = 4;
		for (int track = 0; track < midifile.getTrackCount(); track++)
		{
			for (int event = 0; event < midifile[track].size(); event++)
			{
				if (midifile[track][event].isTimeSignature())
				{
					timeSignatureNumerator = midifile[track][event][3];
					timeSignatureDenominator = (int)pow(2, midifile[track][event][4]);
					break;
				}
			}
			if (timeSignatureNumerator != 4) break;
		}

		// Update Transport settings
		Transport::BeatSettings beatSettings;
		beatSettings.tempo = tempo;
		beatSettings.timeSignatureNumerator = timeSignatureNumerator;
		beatSettings.timeSignatureDenominator = timeSignatureDenominator;
		mTransport.SetBeatSettings(beatSettings);

		// Import each track's events
		for (int trackNum = 0; trackNum < midifile.getTrackCount(); trackNum++)
		{
			for (int eventNum = 0; eventNum < midifile[trackNum].size(); eventNum++)
			{
				auto& midiEvent = midifile[trackNum][eventNum];

				// Skip meta events (tempo, time signature, etc.)
				if (midiEvent.isMetaMessage())
				{
					continue;
				}

				// Get the MIDI channel from the event (0-15)
				int channel = midiEvent.getChannelNibble();
				if (channel >= MidiConstants::CHANNEL_COUNT)
				{
					continue;  // Skip invalid channels
				}

				// Handle program change events
				if (midiEvent.isTimbre())
				{
					int programNumber = midiEvent[1];
					mSoundBank.GetChannel(channel).programNumber = programNumber;
					continue;
				}

				// Import note events (note on/off)
				if (midiEvent.isNoteOn() || midiEvent.isNoteOff())
				{
					TimedMidiEvent timedEvent;
					// Convert tick from source PPQN to MidiWorks PPQN (960)
					timedEvent.tick = (uint64_t)(midiEvent.tick * tickConversion);
					timedEvent.mm.mData[0] = midiEvent[0];
					timedEvent.mm.mData[1] = midiEvent[1];
					timedEvent.mm.mData[2] = midiEvent[2];

					// Normalize NOTE_ON velocity 0 to explicit NOTE_OFF (0x80)
					// This ensures our internal format is consistent (we always use NOTE_OFF)
					if (timedEvent.mm.isNoteOn() && timedEvent.mm.getVelocity() == 0)
					{
						timedEvent.mm = MidiMessage::NoteOff(timedEvent.mm.getPitch(), channel);
					}

					mTrackSet.GetTrack(channel).push_back(timedEvent);
				}
			}
		}

		// Apply the imported program changes to MIDI device
		mSoundBank.ApplyChannelSettings();

		// Mark project as dirty (imported data is unsaved)
		MarkDirty();

		return true;
	}
	catch (const std::exception& e)
	{
		if (mErrorCallback)
		{
			mErrorCallback("Import Failed", std::string("Error importing MIDI file: ") + e.what());
		}
		return false;
	}
}
