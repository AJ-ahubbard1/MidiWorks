// AppModel.h
#pragma once
#include <unordered_map>
#include <memory>
#include <chrono>
#include "SoundBank.h"
#include "Transport.h"
#include "TrackSet.h"

class AppModel
{

public:
	std::shared_ptr<MidiIn> mMidiIn;
	TimedMidiEvent			mLogMessage{MidiMessage::NoteOff(0), 0};
	bool					mUpdateLog = false;
	bool					mMetronomeEnabled = true;  // Metronome on by default

	AppModel()
	{
		mMidiIn = std::make_shared<MidiIn>();
		InitializeMetronome();
	}

	void InitializeMetronome()
	{
		// Set channel 16 to woodblock sound for metronome
		// We're using channel 16 (index 15) to avoid conflicts with user channels
		// Program 115 = Woodblock (percussive, short click sound)
		auto player = mSoundBank.GetMidiOutDevice();
		player->sendMessage(MidiMessage::ProgramChange(115, METRONOME_CHANNEL));
	}

	// Called inside of MainFrame::OnTimer event
	void Update()
	{
		static std::vector<MidiMessage> messages;
		switch (mTransport.mState)
		{
		case Transport::State::StopRecording:
			mTransport.Stop();
			mTransport.mState = Transport::State::Stopped;
			mTrackSet.FinalizeRecording(mRecordingBuffer);
			break;

		case Transport::State::StopPlaying:
			mTransport.Stop();
			mTransport.mState = Transport::State::Stopped;
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
			mTransport.mState = Transport::State::Recording;
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

	/*  Checks for Midi In Messages
		If a SoundBank Channel is active, the message will playback.
		If a SoundBank Channel is set to record: the message will be pushed to the mRecordingBuffer.
		This buffer is used to temporarily store the midi messages during recording, 
		when finished recording, the buffer is added to the track and sorted by timestamp. 
	*/
	void CheckMidiInQueue()
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

	SoundBank& GetSoundBank() { return mSoundBank; }
	Transport& GetTransport() { return mTransport; }

	Track& GetTrack(ubyte c) { return mTrackSet.GetTrack(c); }

private:
	std::chrono::steady_clock::time_point mLastTick = std::chrono::steady_clock::now();
	SoundBank	mSoundBank;
	Transport	mTransport;
	TrackSet	mTrackSet;
	Track		mRecordingBuffer;

	uint64_t GetDeltaTimeMs()
	{
		auto now = std::chrono::steady_clock::now();
		auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - mLastTick).count();
		mLastTick = now;
		return static_cast<uint64_t>(delta);
	}

	bool IsMusicalMessage(const MidiMessage& msg)
	{
		using MidiInterface::MidiEvent;
		auto event = msg.getEventType();
		
		return (event >= NOTE_OFF && event <= PITCH_BEND && event != PROGRAM_CHANGE);

	}

	void PlayMessages(std::vector<MidiMessage> msgs)
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

	void PlayMetronomeClick(bool isDownbeat)
	{
		auto player = mSoundBank.GetMidiOutDevice();

		// Different pitches and velocities for downbeat vs other beats
		ubyte note = isDownbeat ? 76 : 72;      // High E vs High C (woodblock sounds good high-pitched)
		ubyte velocity = isDownbeat ? 127 : 90; // Downbeat louder

		// Send note on metronome channel (channel 16)
		player->sendMessage(MidiMessage::NoteOn(note, velocity, METRONOME_CHANNEL));

		// Note: Woodblock has short natural decay, but could add explicit note-off if needed
	}
};

