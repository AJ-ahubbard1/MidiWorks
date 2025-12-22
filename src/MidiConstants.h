#pragma once
#include <cstdint>

namespace MidiConstants
{
	// Timing - True constants
	static constexpr int TICKS_PER_QUARTER = 960;  // PPQN (Pulses Per Quarter Note)

	// MIDI Specification - True constants
	static constexpr int CHANNEL_COUNT = 15;        // 16th reserved for metronome
	static constexpr int METRONOME_CHANNEL = 15;    // Channel 16 (index 15)
	static constexpr int TOTAL_CHANNELS = 16;       // MIDI has 16 channels
	static constexpr int MIDI_NOTE_COUNT = 128;     // MIDI notes 0-127
	static constexpr int MAX_MIDI_NOTE = 127;       // Highest MIDI note
	static constexpr int PROGRAM_COUNT = 128;       // MIDI programs 0-127
	static constexpr int NOTES_PER_OCTAVE = 12;		// 12 chromatic pitches

	// Default Initial Values (can change at runtime)
	static constexpr double DEFAULT_TEMPO = 120.0;
	static constexpr int DEFAULT_TIME_SIGNATURE_NUMERATOR = 4;    // 4/4 time by default
	static constexpr int DEFAULT_TIME_SIGNATURE_DENOMINATOR = 4;
	static constexpr int DEFAULT_VOLUME = 100;
	static constexpr int DEFAULT_VELOCITY = 100;

	// Calculated default (4 bars in 4/4 time)
	static constexpr uint64_t DEFAULT_LOOP_END = TICKS_PER_QUARTER * 4 * 4;  // = 15360
}
