// DrumMachine.h
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
	DrumMachine() { InitializeDefaultRows(); }

	// Pattern Management
	void SetColumnCount(int columns);
	int GetColumnCount() const { return mColumnCount; }
	void UpdatePattern(uint64_t loopDuration);	// Regenerate Track from Pads
	const Track& GetPattern();  // Returns pattern, regenerates if dirty

	// Row Management
	void AddRow(const std::string& name, ubyte pitch);
	void RemoveRow(size_t rowIndex);
	size_t GetRowCount() const { return mRows.size(); }
	DrumRow& GetRow(size_t index) { return mRows[index]; }
	const DrumRow& GetRow(size_t index) const { return mRows[index]; }
	DrumPad& GetPad(size_t rowIndex, size_t columnIndex) { return mRows[rowIndex].pads[columnIndex]; }
	const DrumPad& GetPad(size_t rowIndex, size_t columnIndex) const { return mRows[rowIndex].pads[columnIndex]; }
	ubyte GetPitch(int row) const { return mRows[row].pitch; }
	void SetPitch(int row, ubyte pitch) { mRows[row].pitch = pitch; mPatternDirty = true; }

	// Pad Manipulation
	void TogglePad(size_t rowIndex, size_t columnIndex);
	void EnablePad(size_t rowIndex, size_t columnIndex);  // Enable specific pad (for live recording)
	void SetPadVelocity(size_t rowIndex, size_t columnIndex, ubyte velocity);
	bool IsPadEnabled(size_t rowIndex, size_t columnIndex) const;
	void Clear();

	// Channel Selection
	void SetChannel(ubyte channel) { mChannel = channel; mPatternDirty = true; }
	ubyte GetChannel() { return mChannel; }

	// Playback Integration
	bool IsMuted() const { return mIsMuted; }
	void SetMuted(bool isMuted) { mIsMuted = isMuted; }

	// Grid Visualization Helper
	bool IsColumnOnMeasure(int column, uint64_t ticksPerMeasure) const;

	// Get column index for a given tick position (for live recording)
	int GetColumnAtTick(uint64_t tick, uint64_t loopStartTick) const;

	// Calculate ticks per column for a given loop duration
	uint64_t CalculatePadDuration(uint64_t loopDuration) const;

private:
	std::vector<DrumRow> mRows;
	Track mPattern;		// Generated from pads for playback
	int mColumnCount = 16;
	ubyte mChannel = 9;	// Channel 10 (index 9) for GM Drums
	bool mIsMuted = true;
	bool mPatternDirty = true;
	uint64_t mLastLoopDuration = 15360;  // Default: 4 measures (3840 * 4)

	void InitializeDefaultRows();
	void RegeneratePattern();  // Actually rebuilds the pattern
};
