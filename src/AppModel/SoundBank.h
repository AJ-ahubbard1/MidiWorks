#pragma once
#include <span>
#include "RtMidiWrapper/RtMidiWrapper.h"
using namespace MidiInterface;
struct MidiChannel
{
	ubyte channelNumber;
	ubyte programNumber = 0;
	ubyte volume = 100;
	bool mute = false;
	bool solo = false;
	bool record = false;
};

class SoundBank
{
public:
	SoundBank()
	{
		for (ubyte c = 0; c < 16; c++)
		{
			mChannels[c].channelNumber = c;
			mChannels[c].programNumber = c * 8;
		}
		mMidiOut = std::make_shared<MidiOut>();
		ApplyChannelSettings();
	}

	void SetMidiOutDevice(std::shared_ptr<MidiOut> device)
	{
		mMidiOut = std::move(device);
		ApplyChannelSettings();
	}

	std::shared_ptr<MidiOut> GetMidiOutDevice() const
	{
		return mMidiOut;
	}

	void ApplyChannelSettings()
	{
		if (!mMidiOut) return;

		for (auto& c : mChannels)
		{
			auto pc = MidiMessage::ProgramChange(c.programNumber, c.channelNumber);
			auto vol = MidiMessage::ControlChange(VOLUME, c.volume, c.channelNumber);
			mMidiOut->sendMessage(pc);
			mMidiOut->sendMessage(vol);
		}
	}

	MidiChannel& GetChannel(ubyte c) { return mChannels[c]; }

	std::span<MidiChannel> GetAllChannels() 
	{
		return std::span<MidiChannel>(mChannels);
	}

	bool SolosFound() const
	{
		for (auto & c : mChannels)
		{
			if (c.solo)
			{
				return true;
			}
		}
		return false;
	}

private:
	std::shared_ptr<MidiOut> mMidiOut;
	MidiChannel mChannels[16];
};

