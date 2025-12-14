#pragma once
#include "Command.h"
#include "AppModel/TrackSet/TrackSet.h"
#include <algorithm>
#include <vector>
#include <string>
using namespace MidiInterface;

/// <summary>
/// Command to quantize MIDI events in a track to a grid.
/// Snaps all event timestamps to the nearest grid division, fixing timing imperfections.
///
/// Grid sizes (in ticks, assuming 960 ticks per quarter note):
/// - Whole note: 3840 ticks
/// - Half note: 1920 ticks
/// - Quarter note: 960 ticks
/// - Eighth note: 480 ticks
/// - Sixteenth note: 240 ticks
/// - Thirty-second note: 120 ticks
///
/// Example:
/// - Original note at tick 975
/// - Grid size: 960 (quarter note)
/// - Quantized: 960 (rounds to nearest quarter note)
/// </summary>
class QuantizeCommand : public Command
{
public:
	/// <summary>
	/// Construct a quantize command for a single track.
	/// </summary>
	/// <param name="track">Reference to the track to quantize</param>
	/// <param name="gridSize">Grid size in ticks (e.g., 960 for quarter notes)</param>
	QuantizeCommand(Track& track, uint64_t gridSize)
		: mTrack(track)
		, mGridSize(gridSize)
	{
		// Store original ticks for undo
		mOriginalTicks.reserve(track.size());
		for (const auto& event : track)
		{
			mOriginalTicks.push_back(event.tick);
		}
	}

	void Execute() override
	{
		// Quantize each event to nearest grid division
		for (auto& event : mTrack)
		{
			uint64_t tick = event.tick;
			// Round to nearest multiple of gridSize
			uint64_t quantized = ((tick + mGridSize / 2) / mGridSize) * mGridSize;
			event.tick = quantized;
		}

		// Re-sort track by tick to maintain chronological order
		std::sort(mTrack.begin(), mTrack.end(),
			[](const TimedMidiEvent& a, const TimedMidiEvent& b) {
				return a.tick < b.tick;
			});
	}

	void Undo() override
	{
		// Restore original ticks
		if (mOriginalTicks.size() != mTrack.size())
		{
			// Safety check - track size changed, cannot undo safely
			return;
		}

		for (size_t i = 0; i < mTrack.size(); i++)
		{
			mTrack[i].tick = mOriginalTicks[i];
		}

		// Re-sort track by tick to maintain chronological order
		std::sort(mTrack.begin(), mTrack.end(),
			[](const TimedMidiEvent& a, const TimedMidiEvent& b) {
				return a.tick < b.tick;
			});
	}

	std::string GetDescription() const override
	{
		// Convert grid size to musical notation
		std::string gridName;
		switch (mGridSize)
		{
			case 3840: gridName = "whole notes"; break;
			case 1920: gridName = "half notes"; break;
			case 960:  gridName = "quarter notes"; break;
			case 640:  gridName = "quarter triplets"; break;
			case 480:  gridName = "eighth notes"; break;
			case 320:  gridName = "eighth triplets"; break;
			case 240:  gridName = "sixteenth notes"; break;
			case 160:  gridName = "sixteenth triplets"; break;
			case 120:  gridName = "thirty-second notes"; break;
			default:   gridName = std::to_string(mGridSize) + " ticks"; break;
		}

		// Count note-on events
		int noteCount = 0;
		for (const auto& event : mTrack)
		{
			ubyte status = event.mm.mData[0] & 0xF0;
			if (status == 0x90 && event.mm.mData[2] > 0) // Note-on with velocity > 0
			{
				noteCount++;
			}
		}

		return "Quantize " + std::to_string(noteCount) + " note" +
		       (noteCount != 1 ? "s" : "") + " to " + gridName;
	}

private:
	Track& mTrack;
	uint64_t mGridSize;
	std::vector<uint64_t> mOriginalTicks;  // Original timestamps for undo
};
