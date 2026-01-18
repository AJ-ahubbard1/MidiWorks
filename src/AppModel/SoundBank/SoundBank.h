// SoundBank.h
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

/// SoundBank manages MIDI output and channel state.
///
/// Responsibilities:
/// - Manage MIDI output device
/// - Track channel settings (program, volume, mute, solo, record)
/// - Provide playback helpers for notes, messages, and metronome
/// - Handle preview note state for UI feedback
///
/// Usage:
///   SoundBank soundBank;
///   soundBank.GetChannel(0).programNumber = 25;  // Set to acoustic guitar
///   soundBank.ApplyChannelSettings();
///   soundBank.PlayNote(60, 100, 0);  // Play middle C
class SoundBank
{
public:
	SoundBank();

	// MIDI Device

	/// Set the MIDI output device
	void SetMidiOutDevice(std::shared_ptr<MidiOut> device);

	/// Get the MIDI output device
	std::shared_ptr<MidiOut> GetMidiOutDevice() const { return mMidiOut; }

	// Channel Management

	/// Apply all channel settings (program, volume) to MIDI device
	void ApplyChannelSettings();

	/// Get a channel by index (0-14)
	MidiChannel& GetChannel(ubyte ch) { return mChannels[ch]; }

	/// Get all channels as a span
	std::span<MidiChannel> GetAllChannels() { return std::span<MidiChannel>(mChannels); }

	/// Get the color for a channel
	wxColour GetChannelColor(ubyte ch) const { return mChannels[ch].customColor; }

	/// Check if any channel has solo enabled
	bool SolosFound() const;

	/// Get all channels with record enabled
	std::vector<MidiChannel*> GetRecordEnabledChannels();

	/// Get all channels with solo enabled
	std::vector<MidiChannel*> GetSoloChannels();

	/// Check if a channel should play based on mute/solo state
	/// @param checkRecord If true, also requires record to be enabled
	bool ShouldChannelPlay(const MidiChannel& channel, bool checkRecord = false) const;

	// MIDI Playback

	/// Play a vector of MIDI messages (respects mute/solo)
	void PlayMessages(std::vector<MidiMessage> msgs);

	/// Play a single note
	void PlayNote(ubyte pitch, ubyte velocity, ubyte channel);

	/// Stop a single note
	void StopNote(ubyte pitch, ubyte channel);

	/// Send All Notes Off to all channels
	void SilenceAllChannels();

	/// Play a metronome click
	/// @param isDownbeat true for accented downbeat, false for regular beat
	void PlayMetronomeClick(bool isDownbeat);

	// Preview Note (for UI keyboard/mouse hover)

	/// Play a preview note on all record-enabled channels
	void PlayPreviewNote(ubyte pitch);

	/// Stop the currently previewing note
	void StopPreviewNote();

	/// Check if a preview note is currently playing
	bool IsPreviewingNote() const { return mIsPreviewingNote; }

	/// Get the pitch of the preview note
	ubyte GetPreviewPitch() const { return mPreviewPitch; }

	/// Get the velocity used for preview notes
	ubyte GetPreviewVelocity() const { return mPreviewVelocity; }

	/// Set the velocity for preview notes
	void SetPreviewVelocity(ubyte velocity) { mPreviewVelocity = velocity; }

private:
	std::shared_ptr<MidiOut> mMidiOut;
	MidiChannel mChannels[MidiConstants::CHANNEL_COUNT];  // CHANNEL_COUNT channels (channel 16 reserved for metronome)

	// Preview note state
	ubyte mPreviewVelocity = MidiConstants::DEFAULT_VELOCITY;
	bool mIsPreviewingNote = false;
	ubyte mPreviewPitch = 0;
	std::vector<ubyte> mPreviewChannels;
};
