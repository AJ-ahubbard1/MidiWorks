#pragma once
#include "Command.h"
#include "AppModel/TrackSet/TrackSet.h"
#include <algorithm>
#include <vector>
#include <map>
using namespace MidiInterface;

/// <summary>
/// Command to delete multiple notes across multiple tracks.
/// Handles batch deletion efficiently while maintaining undo capability.
/// </summary>
class DeleteMultipleNotesCommand : public Command
{
public:
	struct NoteToDelete {
		int trackIndex = 0;
		size_t noteOnIndex = 0;
		size_t noteOffIndex = 0;
		TimedMidiEvent noteOn;
		TimedMidiEvent noteOff;
	};

	DeleteMultipleNotesCommand(TrackSet& trackSet, const std::vector<NoteToDelete>& notesToDelete)
		: mTrackSet(trackSet)
		, mNotesToDelete(notesToDelete)
	{
	}

	void Execute() override
	{
		// Group notes by track with their indices
		std::map<int, std::vector<std::pair<size_t, size_t>>> indicesByTrack; // track -> [(noteOnIndex, noteOffIndex), ...]
		for (const auto& note : mNotesToDelete)
		{
			indicesByTrack[note.trackIndex].push_back({note.noteOnIndex, note.noteOffIndex});
		}

		// Delete from each track
		for (auto& [trackIndex, noteIndices] : indicesByTrack)
		{
			// Collect all indices to delete
			std::vector<size_t> indicesToDelete;
			for (const auto& [noteOnIdx, noteOffIdx] : noteIndices)
			{
				indicesToDelete.push_back(noteOnIdx);
				indicesToDelete.push_back(noteOffIdx);
			}

			// Sort indices in descending order
			std::sort(indicesToDelete.begin(), indicesToDelete.end(),
				[](size_t a, size_t b) { return a > b; });

			Track& track = mTrackSet.GetTrack(trackIndex);

			// Delete in descending index order (highest index first)
			// This ensures that deleting an element doesn't affect the validity of lower indices
			for (size_t idx : indicesToDelete)
			{
				if (idx < track.size())
				{
					track.erase(track.begin() + idx);
				}
			}
		}
	}

	void Undo() override
	{
		// Re-add all deleted notes
		for (const auto& note : mNotesToDelete)
		{
			Track& track = mTrackSet.GetTrack(note.trackIndex);

			// Add both events back
			track.push_back(note.noteOn);
			track.push_back(note.noteOff);

			// Re-sort track by tick
			TrackSet::SortTrack(track);
		}
	}

	std::string GetDescription() const override
	{
		int noteCount = static_cast<int>(mNotesToDelete.size());
		return "Delete " + std::to_string(noteCount) + " note" + (noteCount != 1 ? "s" : "");
	}

private:
	TrackSet& mTrackSet;
	std::vector<NoteToDelete> mNotesToDelete;
};
