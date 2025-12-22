// MetronomeService.h
#pragma once
#include "AppModel/SoundBank/SoundBank.h"
#include "MidiConstants.h"

/**
 * MetronomeService manages metronome settings and initialization.
 *
 * Responsibilities:
 * - Initialize metronome sound (send program change to channel 16)
 * - Track metronome enabled/disabled state
 *
 * The metronome uses MIDI channel 16 (MidiConstants::METRONOME_CHANNEL)
 * with program 115 (woodblock) for a percussive click sound.
 *
 * Usage:
 *   MetronomeService metronome(soundBank);
 *   metronome.Initialize();
 *   metronome.SetEnabled(true);
 *   if (metronome.IsEnabled()) {
 *       // Play metronome clicks during playback
 *   }
 */
class MetronomeService
{
public:
	/**
	 * Constructor
	 * @param soundBank Reference to SoundBank for MIDI output
	 */
	MetronomeService(SoundBank& soundBank)
		: mSoundBank(soundBank)
		, mEnabled(true)  // Metronome on by default
	{
	}

	/**
	 * Initialize metronome by sending program change
	 * Sets channel 16 to woodblock sound (program 115)
	 * Called during app initialization
	 */
	void Initialize()
	{
		// Set channel 16 to woodblock sound for metronome
		// We're using channel 16 (index METRONOME_CHANNEL) to avoid conflicts with user channels
		// Program 115 = Woodblock (percussive, short click sound)
		auto player = mSoundBank.GetMidiOutDevice();
		player->sendMessage(MidiMessage::ProgramChange(115, MidiConstants::METRONOME_CHANNEL));
	}

	/**
	 * Check if metronome is enabled
	 * @return true if metronome should play during playback/recording
	 */
	bool IsEnabled() const
	{
		return mEnabled;
	}

	/**
	 * Enable or disable metronome
	 * @param enabled true to enable metronome clicks, false to disable
	 */
	void SetEnabled(bool enabled)
	{
		mEnabled = enabled;
	}

private:
	SoundBank& mSoundBank;
	bool mEnabled;
};
