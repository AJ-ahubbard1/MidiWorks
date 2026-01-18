// TrackCommands.h
#pragma once
#include "Command.h"
#include "AppModel/TrackSet/TrackSet.h"
#include "MidiConstants.h"
#include <algorithm>
#include <vector>
#include <string>
using namespace MidiInterface;

// Track-Level Commands

/// Clears all notes from a single track.
///
/// Responsibilities:
/// - Clear all events from specified track
/// - Store complete backup for undo
///
/// Usage:
///   auto cmd = std::make_unique<ClearTrackCommand>(track, trackNumber);
///   appModel.ExecuteCommand(std::move(cmd));
class ClearTrackCommand : public Command
{
public:
	/// Construct a clear track command
	/// @param trackNumber Track number for display purposes
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

/// Quantizes all non-empty tracks in the TrackSet.
///
/// Responsibilities:
/// - Quantize all non-empty tracks to grid
/// - Provide single undo operation for entire project quantization
/// - Use duration-aware quantization algorithm
///
/// Usage:
///   auto cmd = std::make_unique<QuantizeAllCommand>(trackSet, gridSize);
///   appModel.ExecuteCommand(std::move(cmd));
class QuantizeAllCommand : public Command
{
public:
	/// Construct a quantize all command
	/// @param gridSize Grid size in ticks (e.g., 960 for quarter notes)
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
		std::string gridName = MidiConstants::GridSizeToName(mGridSize);

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
	/// Backup structure for a single track's original tick values
	struct TrackBackup
	{
		int trackIndex;
		std::vector<uint64_t> originalTicks;
	};

	/// Apply quantization to a single track using shared TrackSet helper
	void QuantizeTrack(Track& track)
	{
		TrackSet::QuantizeTrack(track, mGridSize);
	}

	TrackSet& mTrackSet;                    // Reference to the TrackSet
	uint64_t mGridSize;                     // Tick amount to quantize notes to
	std::vector<TrackBackup> mTrackBackups; // Original timestamps for all tracks
};

/// Quantizes specific tracks in the TrackSet.
///
/// Responsibilities:
/// - Quantize specified tracks to grid (e.g., all solo tracks)
/// - Provide single undo operation for multi-track quantization
/// - Use duration-aware quantization algorithm
///
/// Usage:
///   auto cmd = std::make_unique<QuantizeMultipleTracksCommand>(trackSet, trackIndices, gridSize);
///   appModel.ExecuteCommand(std::move(cmd));
class QuantizeMultipleTracksCommand : public Command
{
public:
	/// Construct a quantize multiple tracks command
	/// @param trackIndices Vector of track indices to quantize
	/// @param gridSize Grid size in ticks (e.g., 960 for quarter notes)
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
		std::string gridName = MidiConstants::GridSizeToName(mGridSize);

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
	/// Backup structure for a single track's original tick values
	struct TrackBackup
	{
		int trackIndex;
		std::vector<uint64_t> originalTicks;
	};

	/// Apply quantization to a single track using shared TrackSet helper
	void QuantizeTrack(Track& track)
	{
		TrackSet::QuantizeTrack(track, mGridSize);
	}

	TrackSet& mTrackSet;                    // Reference to the TrackSet
	uint64_t mGridSize;                     // Tick amount to quantize notes to
	std::vector<TrackBackup> mTrackBackups; // Original timestamps for specified tracks
};
