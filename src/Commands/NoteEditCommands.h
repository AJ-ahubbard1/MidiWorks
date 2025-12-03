#pragma once
#include "Command.h"
#include "AppModel/TrackSet.h"
#include <algorithm>
using namespace MidiInterface;

/// <summary>
/// Command to add a note to a track.
/// Stores the note data and track reference to enable undo.
/// </summary>
class AddNoteCommand : public Command
{
public:
	AddNoteCommand(Track& track, TimedMidiEvent noteOn, TimedMidiEvent noteOff)
		: mTrack(track), mNoteOn(noteOn), mNoteOff(noteOff)
	{
	}

	void Execute() override
	{
		// Add both note-on and note-off events to the track
		mTrack.push_back(mNoteOn);
		mTrack.push_back(mNoteOff);

		// Sort track by tick to maintain chronological order
		std::sort(mTrack.begin(), mTrack.end(), [](const TimedMidiEvent& a, const TimedMidiEvent& b) {
			return a.tick < b.tick;
		});

		// Store indices after sorting for undo
		FindNoteIndices();
	}

	void Undo() override
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

	std::string GetDescription() const override
	{
		return "Add Note (Pitch: " + std::to_string(mNoteOn.mm.mData[1]) +
		       ", Tick: " + std::to_string(mNoteOn.tick) + ")";
	}

private:
	Track& mTrack;
	TimedMidiEvent mNoteOn;
	TimedMidiEvent mNoteOff;
	size_t mNoteOnIndex = 0;
	size_t mNoteOffIndex = 0;

	// Find where the notes ended up after sorting
	void FindNoteIndices()
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
};

/// <summary>
/// Command to delete a note from a track.
/// Stores the deleted note data to enable undo (re-adding the note).
/// </summary>
class DeleteNoteCommand : public Command
{
public:
	DeleteNoteCommand(Track& track, size_t noteOnIndex, size_t noteOffIndex)
		: mTrack(track), mNoteOnIndex(noteOnIndex), mNoteOffIndex(noteOffIndex)
	{
		// Store the events before deleting
		mNoteOn = mTrack[noteOnIndex];
		mNoteOff = mTrack[noteOffIndex];
	}

	void Execute() override
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

	void Undo() override
	{
		// Re-add the deleted notes
		mTrack.push_back(mNoteOn);
		mTrack.push_back(mNoteOff);

		// Re-sort to maintain chronological order
		std::sort(mTrack.begin(), mTrack.end(), [](const TimedMidiEvent& a, const TimedMidiEvent& b) {
			return a.tick < b.tick;
		});
	}

	std::string GetDescription() const override
	{
		return "Delete Note (Pitch: " + std::to_string(mNoteOn.mm.mData[1]) +
		       ", Tick: " + std::to_string(mNoteOn.tick) + ")";
	}

private:
	Track& mTrack;
	size_t mNoteOnIndex;
	size_t mNoteOffIndex;
	TimedMidiEvent mNoteOn;
	TimedMidiEvent mNoteOff;
};

/// <summary>
/// Command to move a note to a different tick/pitch position.
/// Stores both old and new positions to enable undo/redo.
/// </summary>
class MoveNoteCommand : public Command
{
public:
	MoveNoteCommand(Track& track, size_t noteOnIndex, size_t noteOffIndex,
		uint64_t newTick, uint8_t newPitch)
		: mTrack(track), mNoteOnIndex(noteOnIndex), mNoteOffIndex(noteOffIndex),
		mNewTick(newTick), mNewPitch(newPitch)
	{
		// Store original values
		mOldTick = mTrack[noteOnIndex].tick;
		mOldPitch = mTrack[noteOnIndex].mm.mData[1];
		mNoteDuration = mTrack[noteOffIndex].tick - mTrack[noteOnIndex].tick;
	}

	void Execute() override
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
		std::sort(mTrack.begin(), mTrack.end(), [](const TimedMidiEvent& a, const TimedMidiEvent& b) {
			return a.tick < b.tick;
		});
	}

	void Undo() override
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
		std::sort(mTrack.begin(), mTrack.end(), [](const TimedMidiEvent& a, const TimedMidiEvent& b) {
			return a.tick < b.tick;
		});
	}

	std::string GetDescription() const override
	{
		return "Move Note (From Pitch: " + std::to_string(mOldPitch) +
		       " to " + std::to_string(mNewPitch) + ")";
	}

private:
	Track& mTrack;
	size_t mNoteOnIndex;
	size_t mNoteOffIndex;
	uint64_t mOldTick;
	uint64_t mNewTick;
	uint8_t mOldPitch;
	uint8_t mNewPitch;
	uint64_t mNoteDuration;

	size_t FindNoteIndex(uint64_t tick, uint8_t pitch, MidiEvent eventType)
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
};

/// <summary>
/// Command to resize a note (change its duration).
/// Stores both old and new durations to enable undo/redo.
/// </summary>
class ResizeNoteCommand : public Command
{
public:
	ResizeNoteCommand(Track& track, size_t noteOnIndex, size_t noteOffIndex, uint64_t newDuration)
		: mTrack(track), mNoteOnIndex(noteOnIndex), mNoteOffIndex(noteOffIndex),
		mNewDuration(newDuration)
	{
		// Store original duration
		mOldDuration = mTrack[noteOffIndex].tick - mTrack[noteOnIndex].tick;
		mNoteOnTick = mTrack[noteOnIndex].tick;
		mPitch = mTrack[noteOnIndex].mm.mData[1];
	}

	void Execute() override
	{
		// Update note-off tick to reflect new duration
		if (mNoteOffIndex < mTrack.size())
		{
			mTrack[mNoteOffIndex].tick = mNoteOnTick + mNewDuration;
		}

		// Re-sort track (note-off might have moved)
		std::sort(mTrack.begin(), mTrack.end(), [](const TimedMidiEvent& a, const TimedMidiEvent& b) {
			return a.tick < b.tick;
		});
	}

	void Undo() override
	{
		// Find note-off again (index may have changed)
		size_t noteOffIdx = FindNoteIndex(mNoteOnTick + mNewDuration, mPitch, MidiEvent::NOTE_OFF);

		// Restore original duration
		if (noteOffIdx < mTrack.size())
		{
			mTrack[noteOffIdx].tick = mNoteOnTick + mOldDuration;
		}

		// Re-sort track
		std::sort(mTrack.begin(), mTrack.end(), [](const TimedMidiEvent& a, const TimedMidiEvent& b) {
			return a.tick < b.tick;
		});
	}

	std::string GetDescription() const override
	{
		return "Resize Note (Pitch: " + std::to_string(mPitch) +
		       ", Duration: " + std::to_string(mOldDuration) + " -> " + std::to_string(mNewDuration) + ")";
	}

private:
	Track& mTrack;
	size_t mNoteOnIndex;
	size_t mNoteOffIndex;
	uint64_t mOldDuration;
	uint64_t mNewDuration;
	uint64_t mNoteOnTick;
	uint8_t mPitch;

	size_t FindNoteIndex(uint64_t tick, uint8_t pitch, MidiEvent eventType)
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
};
