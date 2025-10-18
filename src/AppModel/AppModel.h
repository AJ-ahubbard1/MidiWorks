// AppModel.h
#pragma once
#include <unordered_map>
#include <memory>
#include <chrono>
#include "SoundBank.h"
#include "PaneInfo.h"
#include "Transport.h"
#include "TrackSet.h"

class AppModel
{

public:
	std::shared_ptr<MidiIn> mMidiIn;

	AppModel() { mMidiIn = std::make_shared<MidiIn>(); }

	void RegisterPanel(PanelID id, const PanelInfo& info) { mPanels.insert({id, info}); }

	void SetPanelVisibility(PanelID id, bool vis) { mPanels.at(id).isVisible = vis; }
	//bool IsPanelVisible(PanelID id) const { return mPanels.at(id).isVisible; }

	PanelInfo& GetPanelInfo(PanelID id) { return mPanels.at(id); }
	std::unordered_map<PanelID, PanelInfo>& GetAllPanels() { return mPanels; }

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
			mTransport.UpdatePlayBack(GetDeltaTimeMs());
			messages = mTrackSet.PlayBack(mTransport.GetCurrentTick());
			PlayMessages(messages);
			break;

		case Transport::State::ClickedRecord:
			GetDeltaTimeMs();
			mTrackSet.FindStart(mTransport.StartPlayBack());
			mTransport.mState = Transport::State::Recording;
			break;

		case Transport::State::Recording:
			mTransport.UpdatePlayBack(GetDeltaTimeMs());
			messages = mTrackSet.PlayBack(mTransport.GetCurrentTick());
			PlayMessages(messages);
			break;

		case Transport::State::FastForwarding:
		case Transport::State::Rewinding:
			mTransport.ShiftCurrentTime();
			break;
		}

		CheckMidiInQueue();
	}

	// Checks for Midi In Messages
	// If a SoundBank Channel is active, the message will playback.
	// If a SoundBank Channel is set to record, the message will be pushed to the TrackSet.
	void CheckMidiInQueue()
	{
		if (!mMidiIn->checkForMessage()) return;

		MidiMessage mm = mMidiIn->getMessage();
		auto channels = mSoundBank.GetAllChannels();

		bool solosFound = mSoundBank.SolosFound();

		for (MidiChannel& c : channels)
		{
			bool shouldPlay = solosFound ? c.solo : (c.record && !c.mute);
			if (shouldPlay)
			{
				MidiMessage routed = mm;
				routed.setChannel(c.channelNumber);
				mSoundBank.GetMidiOutDevice()->sendMessage(routed);

				if (c.record && IsMusicalMessage(routed))
				{
					TimedMidiEvent tme{routed, mTransport.GetCurrentTick()};
					mRecordingBuffer.push_back(tme);
					//mTrackSet.Record(tme, c.channelNumber);
				}
			}
		}
	}

	SoundBank& GetSoundBank() { return mSoundBank; }
	Transport& GetTransport() { return mTransport; }

	Track& GetTrack(ubyte c) { return mTrackSet.GetTrack(c); }

private:
	std::unordered_map<PanelID, PanelInfo> mPanels;
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
};

