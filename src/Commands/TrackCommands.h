//==============================================================================
// TrackCommands.h
// Commands for track-level operations (clear, quantize, etc.)
//==============================================================================

#pragma once
#include "Command.h"
#include "AppModel/TrackSet/TrackSet.h"
#include "MidiConstants.h"
#include <algorithm>
#include <vector>
#include <string>
using namespace MidiInterface;

//==============================================================================
// ClearTrackCommand
//==============================================================================

/// <summary>
/// Command to clear all notes from a single track.
/// Stores a complete backup of the track to enable undo.
/// </summary>
class ClearTrackCommand : public Command
{
public:
	/// <summary>
	/// Construct a clear track command.
	/// </summary>
	/// <param name="track">Reference to the track to clear</param>
	/// <param name="trackNumber">Track number for display purposes</param>
	ClearTrackCommand(Track& track, int trackNumber)
		: mTrack(track)
		, mTrackNumber(trackNumber)
	{
	}

	void Execute() override
	{
		// Backup all events before clearing
		mBackup = mTrack;

		// Clear the track
		mTrack.clear();
	}

	void Undo() override
	{
		// Restore backed-up events
		mTrack = mBackup;
	}

	std::string GetDescription() const override
	{
		return "Clear Track " + std::to_string(mTrackNumber + 1);
	}

private:
	Track& mTrack;          // Reference to the track to clear
	int mTrackNumber;       // Track number for description
	Track mBackup;          // Backup storage for undo
};

//==============================================================================
// QuantizeAllCommand
//==============================================================================

/// <summary>
/// Command to quantize all non-empty tracks in the TrackSet.
/// Provides a single undo operation for quantizing the entire project.
/// Uses the same duration-aware quantization algorithm as QuantizeCommand.
/// </summary>
class QuantizeAllCommand : public Command
{
public:
	/// <summary>
	/// Construct a quantize all command.
	/// </summary>
	/// <param name="trackSet">Reference to the TrackSet containing all tracks</param>
	/// <param name="gridSize">Grid size in ticks (e.g., 960 for quarter notes)</param>
	QuantizeAllCommand(TrackSet& trackSet, uint64_t gridSize)
		: mTrackSet(trackSet)
		, mGridSize(gridSize)
	{
		// Store original ticks for all non-empty tracks
		for (int i = 0; i < MidiConstants::CHANNEL_COUNT; i++)
		{
			Track& track = mTrackSet.GetTrack(i);
			if (!track.empty())
			{
				TrackBackup backup;
				backup.trackIndex = i;
				backup.originalTicks.reserve(track.size());
				for (const auto& event : track)
				{
					backup.originalTicks.push_back(event.tick);
				}
				mTrackBackups.push_back(backup);
			}
		}
	}

	void Execute() override
	{
		// Quantize all non-empty tracks
		for (const auto& backup : mTrackBackups)
		{
			Track& track = mTrackSet.GetTrack(backup.trackIndex);
			QuantizeTrack(track);
		}
	}

	void Undo() override
	{
		// Restore original ticks for all backed-up tracks
		for (const auto& backup : mTrackBackups)
		{
			Track& track = mTrackSet.GetTrack(backup.trackIndex);

			// Safety check - track size changed, cannot undo safely
			if (backup.originalTicks.size() != track.size())
			{
				continue;
			}

			// Restore ticks
			for (size_t i = 0; i < track.size(); i++)
			{
				track[i].tick = backup.originalTicks[i];
			}

			// Re-sort track
			TrackSet::SortTrack(track);
		}
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

		// Count total note-on events across all tracks
		int totalNoteCount = 0;
		for (const auto& backup : mTrackBackups)
		{
			const Track& track = mTrackSet.GetTrack(backup.trackIndex);
			for (const auto& event : track)
			{
				ubyte status = event.mm.mData[0] & 0xF0;
				if (status == 0x90 && event.mm.mData[2] > 0) // Note-on with velocity > 0
				{
					totalNoteCount++;
				}
			}
		}

		return "Quantize All (" + std::to_string(totalNoteCount) + " note" +
		       (totalNoteCount != 1 ? "s" : "") + " to " + gridName + ")";
	}

private:
	/// <summary>
	/// Backup structure for a single track's original tick values.
	/// </summary>
	struct TrackBackup
	{
		int trackIndex;
		std::vector<uint64_t> originalTicks;
	};

	/// <summary>
	/// Round a tick value to the nearest grid point.
	/// </summary>
	uint64_t RoundToGrid(uint64_t tick) const
	{
		return ((tick + mGridSize / 2) / mGridSize) * mGridSize;
	}

	/// <summary>
	/// Apply quantization to a single track (same logic as QuantizeCommand).
	/// </summary>
	void QuantizeTrack(Track& track)
	{
		// Get note pairs (note-on + note-off) for intelligent quantization
		std::vector<NoteLocation> notes = TrackSet::GetNotesFromTrack(track);

		// Duration-aware quantization: handle short vs long notes differently
		for (const auto& note : notes)
		{
			uint64_t duration = note.endTick - note.startTick;
			uint64_t quantizedStart = RoundToGrid(note.startTick);

			if (duration < mGridSize)
			{
				// Short note (grace note/ornament): quantize start, extend to one grid snap
				track[note.noteOnIndex].tick = quantizedStart;
				track[note.noteOffIndex].tick = quantizedStart + mGridSize - MidiConstants::NOTE_SEPARATION_TICKS;
			}
			else
			{
				// Long note: quantize both start and end independently
				track[note.noteOnIndex].tick = quantizedStart;
				track[note.noteOffIndex].tick = RoundToGrid(note.endTick) - MidiConstants::NOTE_SEPARATION_TICKS;
			}
		}

		// Post-process to fix any remaining overlaps
		TrackSet::SeparateOverlappingNotes(track);

		// Re-sort track to maintain chronological order
		TrackSet::SortTrack(track);
	}

	TrackSet& mTrackSet;                    // Reference to the TrackSet
	uint64_t mGridSize;                     // Tick amount to quantize notes to
	std::vector<TrackBackup> mTrackBackups; // Original timestamps for all tracks
};

//==============================================================================
// QuantizeMultipleTracksCommand
//==============================================================================

/// <summary>
/// Command to quantize specific tracks in the TrackSet.
/// Provides a single undo operation for quantizing multiple tracks (e.g., all solo tracks).
/// Uses the same duration-aware quantization algorithm as QuantizeCommand.
/// </summary>
class QuantizeMultipleTracksCommand : public Command
{
public:
	/// <summary>
	/// Construct a quantize multiple tracks command.
	/// </summary>
	/// <param name="trackSet">Reference to the TrackSet containing all tracks</param>
	/// <param name="trackIndices">Vector of track indices to quantize</param>
	/// <param name="gridSize">Grid size in ticks (e.g., 960 for quarter notes)</param>
	QuantizeMultipleTracksCommand(TrackSet& trackSet, const std::vector<int>& trackIndices, uint64_t gridSize)
		: mTrackSet(trackSet)
		, mGridSize(gridSize)
	{
		// Store original ticks for specified tracks
		for (int trackIndex : trackIndices)
		{
			Track& track = mTrackSet.GetTrack(trackIndex);
			if (!track.empty())
			{
				TrackBackup backup;
				backup.trackIndex = trackIndex;
				backup.originalTicks.reserve(track.size());
				for (const auto& event : track)
				{
					backup.originalTicks.push_back(event.tick);
				}
				mTrackBackups.push_back(backup);
			}
		}
	}

	void Execute() override
	{
		// Quantize specified tracks
		for (const auto& backup : mTrackBackups)
		{
			Track& track = mTrackSet.GetTrack(backup.trackIndex);
			QuantizeTrack(track);
		}
	}

	void Undo() override
	{
		// Restore original ticks for all backed-up tracks
		for (const auto& backup : mTrackBackups)
		{
			Track& track = mTrackSet.GetTrack(backup.trackIndex);

			// Safety check - track size changed, cannot undo safely
			if (backup.originalTicks.size() != track.size())
			{
				continue;
			}

			// Restore ticks
			for (size_t i = 0; i < track.size(); i++)
			{
				track[i].tick = backup.originalTicks[i];
			}

			// Re-sort track
			TrackSet::SortTrack(track);
		}
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

		// Count total note-on events across specified tracks
		int totalNoteCount = 0;
		int trackCount = static_cast<int>(mTrackBackups.size());
		for (const auto& backup : mTrackBackups)
		{
			const Track& track = mTrackSet.GetTrack(backup.trackIndex);
			for (const auto& event : track)
			{
				ubyte status = event.mm.mData[0] & 0xF0;
				if (status == 0x90 && event.mm.mData[2] > 0) // Note-on with velocity > 0
				{
					totalNoteCount++;
				}
			}
		}

		return "Quantize " + std::to_string(trackCount) + " track" +
		       (trackCount != 1 ? "s" : "") + " (" + std::to_string(totalNoteCount) +
		       " note" + (totalNoteCount != 1 ? "s" : "") + " to " + gridName + ")";
	}

private:
	/// <summary>
	/// Backup structure for a single track's original tick values.
	/// </summary>
	struct TrackBackup
	{
		int trackIndex;
		std::vector<uint64_t> originalTicks;
	};

	/// <summary>
	/// Round a tick value to the nearest grid point.
	/// </summary>
	uint64_t RoundToGrid(uint64_t tick) const
	{
		return ((tick + mGridSize / 2) / mGridSize) * mGridSize;
	}

	/// <summary>
	/// Apply quantization to a single track (same logic as QuantizeCommand).
	/// </summary>
	void QuantizeTrack(Track& track)
	{
		// Get note pairs (note-on + note-off) for intelligent quantization
		std::vector<NoteLocation> notes = TrackSet::GetNotesFromTrack(track);

		// Duration-aware quantization: handle short vs long notes differently
		for (const auto& note : notes)
		{
			uint64_t duration = note.endTick - note.startTick;
			uint64_t quantizedStart = RoundToGrid(note.startTick);

			if (duration < mGridSize)
			{
				// Short note (grace note/ornament): quantize start, extend to one grid snap
				track[note.noteOnIndex].tick = quantizedStart;
				track[note.noteOffIndex].tick = quantizedStart + mGridSize - MidiConstants::NOTE_SEPARATION_TICKS;
			}
			else
			{
				// Long note: quantize both start and end independently
				track[note.noteOnIndex].tick = quantizedStart;
				track[note.noteOffIndex].tick = RoundToGrid(note.endTick) - MidiConstants::NOTE_SEPARATION_TICKS;
			}
		}

		// Post-process to fix any remaining overlaps
		TrackSet::SeparateOverlappingNotes(track);

		// Re-sort track to maintain chronological order
		TrackSet::SortTrack(track);
	}

	TrackSet& mTrackSet;                    // Reference to the TrackSet
	uint64_t mGridSize;                     // Tick amount to quantize notes to
	std::vector<TrackBackup> mTrackBackups; // Original timestamps for specified tracks
};
