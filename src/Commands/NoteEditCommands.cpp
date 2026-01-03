#include "NoteEditCommands.h"

// AddNoteCommand implementations
void AddNoteCommand::Execute()
{
	// Add both note-on and note-off events to the track
	mTrack.push_back(mNoteOn);
	mTrack.push_back(mNoteOff);

	// Sort track by tick to maintain chronological order
	TrackSet::SortTrack(mTrack);

	// Store indices after sorting for undo
	FindNoteIndices();
}

void AddNoteCommand::Undo()
{
	// Remove note-off first (higher index) to avoid invalidating note-on index
	if (mNoteOffIndex < mTrack.size() && mTrack[mNoteOffIndex].tick == mNoteOff.tick)
	{
		mTrack.erase(mTrack.begin() + mNoteOffIndex);
	}

	// Remove note-on
	if (mNoteOnIndex < mTrack.size() && mTrack[mNoteOnIndex].tick == mNoteOn.tick)
	{
		mTrack.erase(mTrack.begin() + mNoteOnIndex);
	}
}

std::string AddNoteCommand::GetDescription() const
{
	return "Add Note (Pitch: " + std::to_string(mNoteOn.mm.mData[1]) +
	       ", Tick: " + std::to_string(mNoteOn.tick) + ")";
}

void AddNoteCommand::FindNoteIndices()
{
	for (size_t i = 0; i < mTrack.size(); i++)
	{
		if (mTrack[i].tick == mNoteOn.tick &&
			mTrack[i].mm.mData[1] == mNoteOn.mm.mData[1] &&
			mTrack[i].mm.getEventType() == MidiEvent::NOTE_ON)
		{
			mNoteOnIndex = i;
		}
		if (mTrack[i].tick == mNoteOff.tick &&
			mTrack[i].mm.mData[1] == mNoteOff.mm.mData[1] &&
			mTrack[i].mm.getEventType() == MidiEvent::NOTE_OFF)
		{
			mNoteOffIndex = i;
		}
	}
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
