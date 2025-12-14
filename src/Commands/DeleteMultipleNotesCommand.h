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
		// Group notes by track
		std::map<int, std::vector<NoteToDelete>> notesByTrack;
		for (const auto& note : mNotesToDelete)
		{
			notesByTrack[note.trackIndex].push_back(note);
		}

		// Delete from each track in descending index order
		for (auto& [trackIndex, notes] : notesByTrack)
		{
			// Sort by noteOffIndex descending (delete noteOff first, then noteOn)
			std::sort(notes.begin(), notes.end(),
				[](const NoteToDelete& a, const NoteToDelete& b) {
					return a.noteOffIndex > b.noteOffIndex;
				});

			Track& track = mTrackSet.GetTrack(trackIndex);

			// Delete in descending order to maintain valid indices
			for (const auto& note : notes)
			{
				// Delete noteOff first (higher index)
				if (note.noteOffIndex < track.size())
				{
					track.erase(track.begin() + note.noteOffIndex);
				}

				// Delete noteOn second (lower index, still valid after noteOff deletion)
				if (note.noteOnIndex < track.size())
				{
					track.erase(track.begin() + note.noteOnIndex);
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
			std::sort(track.begin(), track.end(),
				[](const TimedMidiEvent& a, const TimedMidiEvent& b) {
					return a.tick < b.tick;
				});
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
