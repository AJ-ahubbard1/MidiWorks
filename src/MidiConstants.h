#pragma once
#include <cstdint>
#include <string>
namespace MidiConstants
{
	// Timing - True constants
	static constexpr int TICKS_PER_QUARTER = 960;  // PPQN (Pulses Per Quarter Note)
	static constexpr int MAX_TICK_VALUE = 100'000'000;				// Prevent overflow if position tick < 0
	static constexpr int NOTE_SEPARATION_TICKS = 10;  // Minimum tick gap to prevent note overlap

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

	// Use const for non-constexpr arrays
	const std::string NUMERATOR_LIST[] = 
	{
		"1", "2", "3", "4", "5", "6", "7", 
		"8", "9", "10", "11", "12", "13", "14", 
		"15", "16", "17", "18", "19", "20", "21"
	};
	const std::string DENOMINATOR_LIST[] = { "2", "4", "8", "16", "32" };

	// Calculated default (4 bars in 4/4 time)
	static constexpr uint64_t DEFAULT_LOOP_END = TICKS_PER_QUARTER * 4 * 4;  // = 15360

	// ========== Note Duration / Grid Size Definitions ==========

	/// <summary>
	/// Represents a note duration with its display label and tick value.
	/// Used for grid snapping, quantization, and UI dropdowns.
	/// </summary>
	struct NoteDuration
	{
		const char* label;
		uint64_t ticks;
	};

	const NoteDuration NOTE_DURATIONS[] = {
		{ "Whole Note", 3840 },
		{ "Half Note", 1920 },
		{ "Quarter Note", 960 },
		{ "Quarter Triplet", 640 },
		{ "Eighth Note", 480 },
		{ "Eighth Triplet", 320 },
		{ "Sixteenth Note", 240 },
		{ "Sixteenth Triplet", 160 },
		{ "Custom", 0 }  // 0 = custom, tick value from spin control
	};

	const int NOTE_DURATIONS_COUNT = sizeof(NOTE_DURATIONS) / sizeof(NOTE_DURATIONS[0]);
	const int DEFAULT_DURATION_INDEX = 2;  // Quarter note

	// ========== Quantization Utilities ==========

	/// <summary>
	/// Round a tick value to the nearest grid point.
	/// </summary>
	/// <param name="tick">The tick value to round</param>
	/// <param name="gridSize">The grid size in ticks</param>
	/// <returns>Rounded tick value</returns>
	inline uint64_t RoundToGrid(uint64_t tick, uint64_t gridSize)
	{
		return ((tick + gridSize / 2) / gridSize) * gridSize;
	}

	/// <summary>
	/// Convert a grid size (in ticks) to a human-readable name.
	/// Looks up the name in NOTE_DURATIONS array.
	/// </summary>
	/// <param name="gridSize">Grid size in ticks</param>
	/// <returns>Pluralized name (e.g., "Quarter Notes") or fallback</returns>
	inline std::string GridSizeToName(uint64_t gridSize)
	{
		// Look up the grid size in NOTE_DURATIONS array
		for (int i = 0; i < NOTE_DURATIONS_COUNT; i++)
		{
			if (NOTE_DURATIONS[i].ticks == gridSize)
				return std::string(NOTE_DURATIONS[i].label) + "s";  // Pluralize
		}
		// Fallback for custom/unknown grid sizes
		return std::to_string(gridSize) + " ticks";
	}
}
