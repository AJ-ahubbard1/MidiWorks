#pragma once
#include <span>
#include "RtMidiWrapper/RtMidiWrapper.h"
#include "MidiConstants.h"
using namespace MidiInterface;
struct MidiChannel
{
	ubyte channelNumber;
	ubyte programNumber = 0;
	ubyte volume = MidiConstants::DEFAULT_VOLUME;
	bool mute = false;
	bool solo = false;
	bool record = false;
};

class SoundBank
{
public:

	SoundBank();
	void SetMidiOutDevice(std::shared_ptr<MidiOut> device);
	std::shared_ptr<MidiOut> GetMidiOutDevice() const;
	void ApplyChannelSettings();
	MidiChannel& GetChannel(ubyte c); 
	std::span<MidiChannel> GetAllChannels();
	bool SolosFound() const;
	std::vector<MidiChannel*> GetRecordEnabledChannels();
	bool ShouldChannelPlay(const MidiChannel& channel, bool checkRecord = false);
	
	
private:
	std::shared_ptr<MidiOut> mMidiOut;
	MidiChannel mChannels[MidiConstants::CHANNEL_COUNT];  // CHANNEL_COUNT channels (channel 16 reserved for metronome)
};

