#pragma once
#include "Command.h"
#include "AppModel/TrackSet/TrackSet.h"
#include "MidiConstants.h"
#include <algorithm>
#include <vector>
#include <string>
using namespace MidiInterface;

/// <summary>
/// Command to quantize MIDI events in a track to a grid using duration-aware algorithm.
/// Intelligently handles short notes (grace notes/ornaments) vs long notes (sustained).
///
/// Duration-aware quantization:
/// - Short notes (< grid size): Quantize start, extend to one grid snap minimum
///   - Prevents grace notes/ornaments from disappearing into zero-length notes
///   - Example: 50-tick grace note → one full grid snap (e.g., 960 ticks)
/// - Long notes (>= grid size): Quantize both start and end independently
///   - Preserves musical phrasing while fixing timing
///   - Example: 2000-tick sustained note → snaps to nearest grid points
///
/// Grid sizes (in ticks, assuming 960 ticks per quarter note):
/// - Whole note: 3840 ticks
/// - Half note: 1920 ticks
/// - Quarter note: 960 ticks
/// - Eighth note: 480 ticks
/// - Sixteenth note: 240 ticks
/// - Thirty-second note: 120 ticks
///
/// Post-processes with SeparateOverlappingNotes to ensure NOTE_SEPARATION_TICKS gap.
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
		// Get note pairs (note-on + note-off) for intelligent quantization
		std::vector<NoteLocation> notes = TrackSet::GetNotesFromTrack(mTrack);

		// Duration-aware quantization: handle short vs long notes differently
		for (const auto& note : notes)
		{
			uint64_t duration = note.endTick - note.startTick;
			uint64_t quantizedStart = RoundToGrid(note.startTick);

			if (duration < mGridSize)
			{
				// Short note (grace note/ornament): quantize start, extend to one grid snap
				// This prevents quick decorative notes from disappearing
				mTrack[note.noteOnIndex].tick = quantizedStart;
				mTrack[note.noteOffIndex].tick = quantizedStart + mGridSize - MidiConstants::NOTE_SEPARATION_TICKS;
			}
			else
			{
				// Long note: quantize both start and end independently
				mTrack[note.noteOnIndex].tick = quantizedStart;
				mTrack[note.noteOffIndex].tick = RoundToGrid(note.endTick) - MidiConstants::NOTE_SEPARATION_TICKS;
			}
		}

		// Post-process to fix any remaining overlaps
		TrackSet::SeparateOverlappingNotes(mTrack);

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
	/// <summary>
	/// Helper to round a tick value to the nearest grid point
	/// </summary>
	uint64_t RoundToGrid(uint64_t tick) const
	{
		return ((tick + mGridSize / 2) / mGridSize) * mGridSize;
	}

	Track& mTrack;
	uint64_t mGridSize;
	std::vector<uint64_t> mOriginalTicks;  // Original timestamps for undo
};
