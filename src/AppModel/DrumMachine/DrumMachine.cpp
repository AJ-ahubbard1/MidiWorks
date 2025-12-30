#include "DrumMachine.h"

DrumMachine::DrumMachine()
{
	InitializeDefaultRows();
}

void DrumMachine::SetColumnCount(int columns)
{
    mColumnCount = columns;
    // Resize all rows to new column count
    for (auto& row : mRows)
    {
        row.pads.resize(columns);
    }
    mPatternDirty = true;
}

void DrumMachine::UpdatePattern(uint64_t loopDuration)
{
    if (mLastLoopDuration != loopDuration)
    {
        mLastLoopDuration = loopDuration;
        mPatternDirty = true;
    }
}

const Track& DrumMachine::GetPattern()
{
    if (mPatternDirty)
    {
        RegeneratePattern();
        mPatternDirty = false;
    }
    return mPattern;
}

void DrumMachine::AddRow(const std::string& name, ubyte pitch)
{
    DrumRow row;
    row.name = name;
    row.pitch = pitch;
    row.pads.resize(mColumnCount);  // Initialize with empty pads
    mRows.push_back(row);
    mPatternDirty = true;
}

void DrumMachine::RemoveRow(size_t rowIndex)
{
    if (rowIndex >= mRows.size()) return;
    mRows.erase(mRows.begin() + rowIndex);
    mPatternDirty = true;
}

void DrumMachine::TogglePad(size_t rowIndex, size_t columnIndex)
{
    if (rowIndex >= mRows.size() || columnIndex >= mColumnCount) return;

    DrumPad& pad = GetPad(rowIndex, columnIndex);
    pad.enabled = !pad.enabled;
    mPatternDirty = true;
}

void DrumMachine::EnablePad(size_t rowIndex, size_t columnIndex)
{
    if (rowIndex >= mRows.size() || columnIndex >= mColumnCount) return;

    DrumPad& pad = GetPad(rowIndex, columnIndex);
    if (!pad.enabled)  // Only enable if not already enabled
    {
        pad.enabled = true;
        mPatternDirty = true;
    }
}

void DrumMachine::SetPadVelocity(size_t rowIndex, size_t columnIndex, ubyte velocity)
{
    DrumPad& pad = GetPad(rowIndex, columnIndex);
    pad.velocity = velocity;
    mPatternDirty = true;
}

bool DrumMachine::IsPadEnabled(size_t rowIndex, size_t columnIndex) const
{
	return GetPad(rowIndex, columnIndex).enabled;
}

void DrumMachine::Clear()
{
    for (auto& row : mRows)
    {
        for (auto& pad : row.pads)
        {
            pad.enabled = false;
        }
    }
    mPattern.clear();
    mPatternDirty = true;
}

bool DrumMachine::IsColumnOnMeasure(int column, uint64_t ticksPerMeasure) const
{
	uint64_t ticksPerColumn = CalculatePadDuration(mLastLoopDuration);
	uint64_t columnTick = column * ticksPerColumn;

	return columnTick % ticksPerMeasure == 0;
}

int DrumMachine::GetColumnAtTick(uint64_t tick, uint64_t loopStartTick) const
{
	// Calculate tick position relative to loop start
	if (tick < loopStartTick) return -1;

	uint64_t tickInLoop = tick - loopStartTick;
	uint64_t ticksPerColumn = CalculatePadDuration(mLastLoopDuration);

	if (ticksPerColumn == 0) return -1;

	// Round to nearest column instead of truncating
	int column = static_cast<int>(std::round(static_cast<double>(tickInLoop) / ticksPerColumn));

	// Clamp to valid column range
	if (column < 0 || column >= mColumnCount)
		return -1;

	return column;
}

void DrumMachine::InitializeDefaultRows()
{
    // General MIDI drum map (Channel 10 standard)
    AddRow("Kick", 35);  // Acoustic Bass Drum
    AddRow("Snare", 38);  // Acoustic Snare
    AddRow("Clap", 39);  // Hand Clap
    AddRow("Closed HH", 42);  // Closed Hi-Hat
    AddRow("Open HH", 46);  // Open Hi-Hat
    AddRow("Low Tom", 45);  // Low Tom
    AddRow("Crash", 49);  // Crash Cymbal 1
    AddRow("Ride", 51);  // Ride Cymbal 1
}

void DrumMachine::RegeneratePattern()
{
    mPattern.clear();
    uint64_t padDuration = CalculatePadDuration(mLastLoopDuration);

    for (size_t rowIdx = 0; rowIdx < mRows.size(); rowIdx++)
    {
        const DrumRow& row = GetRow(rowIdx);
        for (size_t colIdx = 0; colIdx < mColumnCount; colIdx++)  // Fixed: was mRows.size()
        {
            const DrumPad& pad = GetPad(rowIdx, colIdx);
            if (pad.enabled)
            {
                uint64_t tick = colIdx * padDuration;
                // Create Note On
                MidiMessage noteOn = MidiMessage::NoteOn(row.pitch, pad.velocity, mChannel);
                mPattern.push_back({noteOn, tick});

                // Create Note Off (duration = half of total pad duration)
                MidiMessage noteOff = MidiMessage::NoteOff(row.pitch, mChannel);
                mPattern.push_back({noteOff, tick + padDuration / 2});
            }
        }
    }
    TrackSet::SortTrack(mPattern);
}


uint64_t DrumMachine::CalculatePadDuration(uint64_t loopDuration) const
{
	return loopDuration / mColumnCount;
}

