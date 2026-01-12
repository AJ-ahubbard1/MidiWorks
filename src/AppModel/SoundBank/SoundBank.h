#pragma once
#include <wx/colour.h>
#include <memory>
#include <vector>
#include <span>
#include <string>
#include "RtMidiWrapper/RtMidiWrapper.h"
#include "MidiConstants.h"

using namespace MidiInterface;

struct MidiChannel
{
	ubyte channelNumber = 0;
	ubyte programNumber = 0;
	ubyte volume = MidiConstants::DEFAULT_VOLUME;
	bool mute = false;
	bool solo = false;
	bool record = false;
	bool minimized = false;
	std::string customName = "";  // Custom channel name (empty = use default "Channel N")
	wxColour customColor;  // Custom channel color (initialized in SoundBank constructor)
};

class SoundBank
{
public:

	SoundBank();
	void SetMidiOutDevice(std::shared_ptr<MidiOut> device);
	std::shared_ptr<MidiOut> GetMidiOutDevice() const { return mMidiOut; }
	void ApplyChannelSettings();
	MidiChannel& GetChannel(ubyte ch) { return mChannels[ch]; }
	std::span<MidiChannel> GetAllChannels() { return std::span<MidiChannel>(mChannels); }
	wxColour GetChannelColor(ubyte ch) const { return mChannels[ch].customColor; }
	bool SolosFound() const;
	std::vector<MidiChannel*> GetRecordEnabledChannels();
	std::vector<MidiChannel*> GetSoloChannels();
	bool ShouldChannelPlay(const MidiChannel& channel, bool checkRecord = false) const;

	// MIDI Playback helpers
	void PlayMessages(std::vector<MidiMessage> msgs);
	void PlayNote(ubyte pitch, ubyte velocity, ubyte channel);
	void StopNote(ubyte pitch, ubyte channel);
	void SilenceAllChannels();
	void PlayMetronomeClick(bool isDownbeat);

	// Preview note state (for UI keyboard/mouse hover)
	void PlayPreviewNote(ubyte pitch);
	void StopPreviewNote();
	bool IsPreviewingNote() const { return mIsPreviewingNote; }
	ubyte GetPreviewPitch() const { return mPreviewPitch; }
	ubyte GetPreviewVelocity() const { return mPreviewVelocity; }
	void SetPreviewVelocity(ubyte velocity) { mPreviewVelocity = velocity; }


private:
	// Preview note state members
	ubyte mPreviewVelocity = MidiConstants::DEFAULT_VELOCITY;
	bool mIsPreviewingNote = false;
	ubyte mPreviewPitch = 0;
	std::vector<ubyte> mPreviewChannels;
	std::shared_ptr<MidiOut> mMidiOut;
	MidiChannel mChannels[MidiConstants::CHANNEL_COUNT];  // CHANNEL_COUNT channels (channel 16 reserved for metronome)
};

