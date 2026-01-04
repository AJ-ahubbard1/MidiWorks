#include "NoteEditCommands.h"

// AddNoteCommand implementations
void AddNoteCommand::Execute()
{
	// Clear any previous execution data (for redo)
	mAddedNotes.clear();

	// Add note to ALL target tracks
	uint64_t noteOffTick = mStartTick + mDuration - MidiConstants::NOTE_SEPARATION_TICKS;

	for (int targetTrack : mTargetTracks)
	{
		Track& track = mTrackSet.GetTrack(targetTrack);

		// Create note on event (use target track's channel)
		TimedMidiEvent noteOn;
		noteOn.tick = mStartTick;
		noteOn.mm = MidiMessage::NoteOn(mPitch, mVelocity, targetTrack);

		// Create note off event (use target track's channel)
		TimedMidiEvent noteOff;
		noteOff.tick = noteOffTick;
		noteOff.mm = MidiMessage::NoteOff(mPitch, targetTrack);

		// Add to track
		track.push_back(noteOn);
		track.push_back(noteOff);

		// Sort track to maintain chronological order
		TrackSet::SortTrack(track);

		// Find and store indices for undo using TrackSet::FindNoteInTrack
		uint64_t noteOffTick = mStartTick + mDuration - MidiConstants::NOTE_SEPARATION_TICKS;
		NoteLocation found = mTrackSet.FindNoteInTrack(targetTrack, mStartTick, noteOffTick, mPitch);
		if (found.found)
		{
			NoteIndices indices;
			indices.trackIndex = targetTrack;
			indices.noteOnIndex = found.noteOnIndex;
			indices.noteOffIndex = found.noteOffIndex;
			mAddedNotes.push_back(indices);
		}
	}
}

void AddNoteCommand::Undo()
{
	// Remove notes from each track (reverse order to handle indices correctly)
	for (auto it = mAddedNotes.rbegin(); it != mAddedNotes.rend(); ++it)
	{
		Track& track = mTrackSet.GetTrack(it->trackIndex);

		// Remove note-off first (higher index) to avoid invalidating note-on index
		if (it->noteOffIndex < track.size())
		{
			track.erase(track.begin() + it->noteOffIndex);
		}

		// Remove note-on
		if (it->noteOnIndex < track.size())
		{
			track.erase(track.begin() + it->noteOnIndex);
		}
	}
}

std::string AddNoteCommand::GetDescription() const
{
	int trackCount = static_cast<int>(mTargetTracks.size());
	std::string desc = "Add note";
	if (trackCount > 1)
	{
		desc += " to " + std::to_string(trackCount) + " tracks";
	}
	return desc;
}

// DeleteNoteCommand implementations
void DeleteNoteCommand::Execute()
{
	// Delete note-off first (higher index) to avoid invalidating note-on index
	if (mNoteOffIndex < mTrack.size())
	{
		mTrack.erase(mTrack.begin() + mNoteOffIndex);
	}

	// Delete note-on
	if (mNoteOnIndex < mTrack.size())
	{
		mTrack.erase(mTrack.begin() + mNoteOnIndex);
	}
}

void DeleteNoteCommand::Undo()
{
	// Re-add the deleted notes
	mTrack.push_back(mNoteOn);
	mTrack.push_back(mNoteOff);

	// Re-sort to maintain chronological order
	TrackSet::SortTrack(mTrack);
}

std::string DeleteNoteCommand::GetDescription() const
{
	return "Delete Note (Pitch: " + std::to_string(mNoteOn.mm.mData[1]) +
	       ", Tick: " + std::to_string(mNoteOn.tick) + ")";
}

// MoveNoteCommand implementations
void MoveNoteCommand::Execute()
{
	// Update note-on position
	if (mNoteOnIndex < mTrack.size())
	{
		mTrack[mNoteOnIndex].tick = mNewTick;
		mTrack[mNoteOnIndex].mm.mData[1] = mNewPitch;  // Pitch
	}

	// Update note-off position (maintain duration)
	if (mNoteOffIndex < mTrack.size())
	{
		mTrack[mNoteOffIndex].tick = mNewTick + mNoteDuration;
		mTrack[mNoteOffIndex].mm.mData[1] = mNewPitch;  // Pitch
	}

	// Re-sort track after moving
	TrackSet::SortTrack(mTrack);
}

void MoveNoteCommand::Undo()
{
	// Find the notes again (indices may have changed after sorting)
	size_t noteOnIdx = FindNoteIndex(mNewTick, mNewPitch, MidiEvent::NOTE_ON);
	size_t noteOffIdx = FindNoteIndex(mNewTick + mNoteDuration, mNewPitch, MidiEvent::NOTE_OFF);

	// Restore original position
	if (noteOnIdx < mTrack.size())
	{
		mTrack[noteOnIdx].tick = mOldTick;
		mTrack[noteOnIdx].mm.mData[1] = mOldPitch;
	}

	if (noteOffIdx < mTrack.size())
	{
		mTrack[noteOffIdx].tick = mOldTick + mNoteDuration;
		mTrack[noteOffIdx].mm.mData[1] = mOldPitch;
	}

	// Re-sort track
	TrackSet::SortTrack(mTrack);
}

std::string MoveNoteCommand::GetDescription() const
{
	return "Move Note (From Pitch: " + std::to_string(mOldPitch) +
	       " to " + std::to_string(mNewPitch) + ")";
}

size_t MoveNoteCommand::FindNoteIndex(uint64_t tick, uint8_t pitch, MidiEvent eventType)
{
	for (size_t i = 0; i < mTrack.size(); i++)
	{
		if (mTrack[i].tick == tick &&
			mTrack[i].mm.mData[1] == pitch &&
			mTrack[i].mm.getEventType() == eventType)
		{
			return i;
		}
	}
	return mTrack.size();  // Not found
}

// ResizeNoteCommand implementations
void ResizeNoteCommand::Execute()
{
	// Update note-off tick to reflect new duration
	if (mNoteOffIndex < mTrack.size())
	{
		mTrack[mNoteOffIndex].tick = mNoteOnTick + mNewDuration;
	}

	// Re-sort track (note-off might have moved)
	TrackSet::SortTrack(mTrack);
}

void ResizeNoteCommand::Undo()
{
	// Find note-off again (index may have changed)
	size_t noteOffIdx = FindNoteIndex(mNoteOnTick + mNewDuration, mPitch, MidiEvent::NOTE_OFF);

	// Restore original duration
	if (noteOffIdx < mTrack.size())
	{
		mTrack[noteOffIdx].tick = mNoteOnTick + mOldDuration;
	}

	// Re-sort track
	TrackSet::SortTrack(mTrack);
}

std::string ResizeNoteCommand::GetDescription() const
{
	return "Resize Note (Pitch: " + std::to_string(mPitch) +
	       ", Duration: " + std::to_string(mOldDuration) + " -> " + std::to_string(mNewDuration) + ")";
}

size_t ResizeNoteCommand::FindNoteIndex(uint64_t tick, uint8_t pitch, MidiEvent eventType)
{
	for (size_t i = 0; i < mTrack.size(); i++)
	{
		if (mTrack[i].tick == tick &&
			mTrack[i].mm.mData[1] == pitch &&
			mTrack[i].mm.getEventType() == eventType)
		{
			return i;
		}
	}
	return mTrack.size();  // Not found
}
