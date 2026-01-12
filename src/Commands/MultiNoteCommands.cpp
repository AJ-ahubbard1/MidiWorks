//==============================================================================
// MultiNoteCommands.cpp
// Implementation of batch note operation commands
//==============================================================================

#include "MultiNoteCommands.h"

//==============================================================================
// DeleteMultipleNotesCommand Implementation
//==============================================================================

void DeleteMultipleNotesCommand::Execute()
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

		// Sort indices in descending order to delete from highest to lowest
		// This ensures deleting an element doesn't affect the validity of lower indices
		std::sort(indicesToDelete.begin(), indicesToDelete.end(),
			[](size_t a, size_t b) { return a > b; });

		Track& track = mTrackSet.GetTrack(trackIndex);

		// Delete in descending index order (highest index first)
		for (size_t idx : indicesToDelete)
		{
			if (idx < track.size())
			{
				track.erase(track.begin() + idx);
			}
		}
	}
}

void DeleteMultipleNotesCommand::Undo()
{
	// Re-add all deleted notes
	for (const auto& note : mNotesToDelete)
	{
		Track& track = mTrackSet.GetTrack(note.trackIndex);

		// Add both events back
		track.push_back(note.noteOn);
		track.push_back(note.noteOff);

		// Re-sort track to maintain chronological order
		TrackSet::SortTrack(track);
	}
}

std::string DeleteMultipleNotesCommand::GetDescription() const
{
	int noteCount = static_cast<int>(mNotesToDelete.size());
	return "Delete " + std::to_string(noteCount) + " note" + (noteCount != 1 ? "s" : "");
}

//==============================================================================
// MoveMultipleNotesCommand Implementation
//==============================================================================

void MoveMultipleNotesCommand::Execute()
{
	// Move each note by the specified delta
	for (const auto& noteInfo : mNotesToMove)
	{
		Track& track = mTrackSet.GetTrack(noteInfo.trackIndex);

		// Calculate new position with delta (clamp to valid ranges)
		int64_t newTickSigned = static_cast<int64_t>(noteInfo.originalStartTick) + mTickDelta;
		uint64_t newTick = (newTickSigned < 0) ? 0 : static_cast<uint64_t>(newTickSigned);

		int newPitchSigned = static_cast<int>(noteInfo.originalPitch) + mPitchDelta;
		ubyte newPitch = std::clamp(newPitchSigned, 0, MidiConstants::MAX_MIDI_NOTE);

		// Update note-on event
		if (noteInfo.noteOnIndex < track.size())
		{
			track[noteInfo.noteOnIndex].tick = newTick;
			track[noteInfo.noteOnIndex].mm.mData[1] = newPitch;  // Set pitch
		}

		// Update note-off event (maintain duration)
		if (noteInfo.noteOffIndex < track.size())
		{
			track[noteInfo.noteOffIndex].tick = newTick + noteInfo.duration;
			track[noteInfo.noteOffIndex].mm.mData[1] = newPitch;  // Set pitch
		}
	}

	// Re-sort all affected tracks to maintain chronological order
	std::set<int> affectedTracks;
	for (const auto& noteInfo : mNotesToMove)
	{
		affectedTracks.insert(noteInfo.trackIndex);
	}

	for (int trackIndex : affectedTracks)
	{
		Track& track = mTrackSet.GetTrack(trackIndex);
		TrackSet::SortTrack(track);
	}
}

void MoveMultipleNotesCommand::Undo()
{
	// Restore original positions for all moved notes
	for (const auto& noteInfo : mNotesToMove)
	{
		Track& track = mTrackSet.GetTrack(noteInfo.trackIndex);

		// Calculate where the notes ended up after the move
		int64_t newTickSigned = static_cast<int64_t>(noteInfo.originalStartTick) + mTickDelta;
		uint64_t newTick = (newTickSigned < 0) ? 0 : static_cast<uint64_t>(newTickSigned);

		int newPitchSigned = static_cast<int>(noteInfo.originalPitch) + mPitchDelta;
		ubyte newPitch = std::clamp(newPitchSigned, 0, MidiConstants::MAX_MIDI_NOTE);

		// Search for note-on with new position and restore original values
		for (size_t i = 0; i < track.size(); i++)
		{
			if (track[i].tick == newTick &&
				track[i].mm.isNoteOn() &&
				track[i].mm.getPitch() == newPitch)
			{
				track[i].tick = noteInfo.originalStartTick;
				track[i].mm.mData[1] = noteInfo.originalPitch;
				break;
			}
		}

		// Search for note-off with new position and restore original values
		uint64_t newEndTick = newTick + noteInfo.duration;
		for (size_t i = 0; i < track.size(); i++)
		{
			if (track[i].tick == newEndTick &&
				!track[i].mm.isNoteOn() &&
				track[i].mm.getPitch() == newPitch)
			{
				track[i].tick = noteInfo.originalStartTick + noteInfo.duration;
				track[i].mm.mData[1] = noteInfo.originalPitch;
				break;
			}
		}
	}

	// Re-sort all affected tracks to maintain chronological order
	std::set<int> affectedTracks;
	for (const auto& noteInfo : mNotesToMove)
	{
		affectedTracks.insert(noteInfo.trackIndex);
	}

	for (int trackIndex : affectedTracks)
	{
		Track& track = mTrackSet.GetTrack(trackIndex);
		TrackSet::SortTrack(track);
	}
}

std::string MoveMultipleNotesCommand::GetDescription() const
{
	int noteCount = static_cast<int>(mNotesToMove.size());
	return "Move " + std::to_string(noteCount) + " note" + (noteCount != 1 ? "s" : "");
}
