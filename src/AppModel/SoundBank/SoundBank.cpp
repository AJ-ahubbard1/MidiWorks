#include "SoundBank.h"

SoundBank::SoundBank()
{
	for (ubyte c = 0; c < MidiConstants::CHANNEL_COUNT; c++)
	{
		mChannels[c].channelNumber = c;
		mChannels[c].programNumber = c * 8;
	}
	mMidiOut = std::make_shared<MidiOut>();
	ApplyChannelSettings();
}

void SoundBank::SetMidiOutDevice(std::shared_ptr<MidiOut> device)
{
	mMidiOut = std::move(device);
	ApplyChannelSettings();
}

std::shared_ptr<MidiOut> SoundBank::GetMidiOutDevice() const
{
	return mMidiOut;
}

void SoundBank::ApplyChannelSettings()
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

MidiChannel& SoundBank::GetChannel(ubyte c) 
{ 
	return mChannels[c]; 
}

std::span<MidiChannel> SoundBank::GetAllChannels()
{
	return std::span<MidiChannel>(mChannels);
}

bool SoundBank::SolosFound() const
{
	for (auto& c : mChannels)
	{
		if (c.solo)
		{
			return true;
		}
	}
	return false;
}

std::vector<MidiChannel*> SoundBank::GetRecordEnabledChannels()
{
	std::vector<MidiChannel*> result;
	for (auto& channel : mChannels)
	{
		if (channel.record)
		{
			result.push_back(&channel);
		}
	}
	return result;
}

bool SoundBank::ShouldChannelPlay(const MidiChannel& channel, bool checkRecord)
{
	if (SolosFound())
	{
		// If any channel is solo'd, only solo channels play
		return channel.solo;
	}
	else
	{
		// Normal playback: check mute (and optionally record)
		if (checkRecord)
		{
			return channel.record && !channel.mute;
		}
		else
		{
			return !channel.mute;
		}
	}
	return false;
}

