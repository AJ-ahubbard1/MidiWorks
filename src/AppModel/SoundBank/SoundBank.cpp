#include "SoundBank.h"
#include "ChannelColors.h"

SoundBank::SoundBank()
{
	for (ubyte c = 0; c < MidiConstants::CHANNEL_COUNT; c++)
	{
		mChannels[c].channelNumber = c;
		mChannels[c].programNumber = c * 8;
		mChannels[c].customColor = TRACK_COLORS[c];  // Initialize with default color
	}
	// Drum Track at index 9 
	mChannels[9].programNumber = 0; 
	mChannels[9].customName = "Ch 10 - Percussion";
	mMidiOut = std::make_shared<MidiOut>();
	ApplyChannelSettings();
}

void SoundBank::SetMidiOutDevice(std::shared_ptr<MidiOut> device)
{
	mMidiOut = std::move(device);
	ApplyChannelSettings();
}

void SoundBank::ApplyChannelSettings()
{
	if (!mMidiOut) return;

	// Apply settings for all user channels (0-14)
	for (auto& c : mChannels)
	{
		auto pc = MidiMessage::ProgramChange(c.programNumber, c.channelNumber);
		auto vol = MidiMessage::ControlChange(VOLUME, c.volume, c.channelNumber);
		mMidiOut->sendMessage(pc);
		mMidiOut->sendMessage(vol);
	}

	// Also set metronome sound (channel 15/16) - Program 115 = Woodblock
	auto metronomePc = MidiMessage::ProgramChange(115, MidiConstants::METRONOME_CHANNEL);
	mMidiOut->sendMessage(metronomePc);
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

std::vector<int> SoundBank::GetRecordEnabledChannelNumbers()
{
	std::vector<int> results;
	for (auto& channel : mChannels)
	{
		if (channel.record)
		{
			results.emplace_back(channel.channelNumber);
		}
	}
	return results;
}

std::vector<int> SoundBank::GetSoloChannelNumbers()
{
	std::vector<int> results;
	for (auto& channel : mChannels)
	{
		if (channel.solo)
		{
			results.emplace_back(channel.channelNumber);
		}
	}
}

bool SoundBank::ShouldChannelPlay(const MidiChannel& channel, bool checkRecord) const 
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


void SoundBank::PlayMessages(std::vector<MidiMessage> msgs)
{
	if (msgs.empty()) return;
	
	for (auto& mm : msgs)
	{
		ubyte c = mm.getChannel();
		auto& channel = GetChannel(c);
		if (ShouldChannelPlay(channel, false))
		{
			mMidiOut->sendMessage(mm);
		}
	}
}


void SoundBank::PlayNote(ubyte pitch, ubyte velocity, ubyte channel)
{
	if (!mMidiOut) return;
	mMidiOut->sendMessage(MidiMessage::NoteOn(pitch, velocity, channel));
}

void SoundBank::StopNote(ubyte pitch, ubyte channel)
{
	if (!mMidiOut) return;
	mMidiOut->sendMessage(MidiMessage::NoteOff(pitch, channel));
}

void SoundBank::SilenceAllChannels()
{
	if (!mMidiOut) return;

	for (ubyte c = 0; c < MidiConstants::CHANNEL_COUNT; c++)
	{
		mMidiOut->sendMessage(MidiMessage::AllNotesOff(c));
	}
}

void SoundBank::SilenceNonSoloedChannels()
{
	if (!mMidiOut) return;

	for (ubyte c = 0; c < MidiConstants::CHANNEL_COUNT; c++)
	{
		if (!GetChannel(c).solo)
		{
			mMidiOut->sendMessage(MidiMessage::AllNotesOff(c));
		}
	}

}

void SoundBank::PlayMetronomeClick(bool isDownbeat)
{
	if (!mMidiOut) return;

	// Different pitches and velocities for downbeat vs other beats
	ubyte note = isDownbeat ? 76 : 72;      // High E vs High C
	ubyte velocity = isDownbeat ? MidiConstants::MAX_MIDI_NOTE : 90;

	// Send note on metronome channel (channel 16)
	mMidiOut->sendMessage(MidiMessage::NoteOn(note, velocity, MidiConstants::METRONOME_CHANNEL));
}

void SoundBank::PlayPreviewNote(ubyte pitch)
{
	// Clear previous preview
	mPreviewChannels.clear();

	// Get record-enabled channels
	auto channels = GetRecordEnabledChannelNumbers();

	// Play note on each record-enabled channel
	for (ubyte channel : channels)
	{
		PlayNote(pitch, mPreviewVelocity, channel);
		mPreviewChannels.push_back(channel);
	}

	mIsPreviewingNote = true;
	mPreviewPitch = pitch;
}

void SoundBank::StopPreviewNote()
{
	if (!mIsPreviewingNote) return;

	// Stop note on all channels that are playing the preview
	for (ubyte channelNum : mPreviewChannels)
	{
		StopNote(mPreviewPitch, channelNum);
	}

	mIsPreviewingNote = false;
	mPreviewChannels.clear();
}

