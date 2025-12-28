#pragma once
#include "../TrackSet/TrackSet.h"
#include <vector>
#include <string>

struct DrumPad
{
	bool enabled = false;
	ubyte velocity = 100; // Midi Velocity (0-127)
	uint64_t tick = 0;	  // Position in ticks
};

struct DrumRow
{
	std::string name;	// Display name (e.g., "Kick", "Snare")
	ubyte pitch;		// Midi Pitch (e.g., 35 for kick in GM drum map)
	std::vector<DrumPad> pads; // One pad per column
};

class DrumMachine
{
public:
	DrumMachine();

	// Pattern Management
	void SetColumnCount(int columns);
	int GetColumnCount() const { return mColumnCount; }
	void UpdatePattern(uint64_t loopDuration);	// Regenerate Track from Pads

	// Row Management
	void AddRow(const std::string& name, ubyte pitch);
	void RemoveRow(size_t rowIndex);
	size_t GetRowCount() const { return mRows.size(); }
	DrumRow& GetRow(size_t index) { return mRows[index]; }
	const DrumRow& GetRow(size_t index) const { return mRows[index]; }
	DrumPad& GetPad(size_t rowIndex, size_t columnIndex) { return mRows[rowIndex].pads[columnIndex]; }
	const DrumPad& GetPad(size_t rowIndex, size_t columnIndex) const { return mRows[rowIndex].pads[columnIndex]; }

	// Pad Manipulation
	void TogglePad(size_t rowIndex, size_t columnIndex);
	void SetPadVelocity(size_t rowIndex, size_t columnIndex, ubyte velocity);
	bool IsPadEnabled(size_t rowIndex, size_t columnIndex) const;

	// Channel Selection
	void SetChannel(ubyte channel) { mChannel = channel; mPatternDirty = true; }
	ubyte GetChannel() { return mChannel; }

	// Playback Integration
	const Track& GetPattern();  // Returns pattern, regenerates if dirty

	// Persistence
	void Clear();

private:
	std::vector<DrumRow> mRows;
	Track mPattern;		// Generated from pads for playback
	int mColumnCount = 16;
	ubyte mChannel = 9;	// Channel 10 (index 9) for GM Drums

	bool mPatternDirty = true;
	uint64_t mLastLoopDuration = 15360;  // Default: 4 measures (3840 * 4)

	void InitializeDefaultRows();
	void RegeneratePattern();  // Actually rebuilds the pattern
	uint64_t CalculatePadDuration(uint64_t loopDuration) const;
};
