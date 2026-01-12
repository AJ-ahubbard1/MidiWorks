//==============================================================================
// NoteEditCommands.h
// Commands for single-note operations (add, delete, move, resize, velocity)
//==============================================================================

#pragma once
#include "Command.h"
#include "AppModel/TrackSet/TrackSet.h"
#include <algorithm>
#include <cstdint>
using namespace MidiInterface;

//==============================================================================
// AddNoteCommand
//==============================================================================

/// <summary>
/// Command to add a note to one or more tracks.
/// Stores note indices per track to enable efficient undo.
/// When multiple tracks are provided, adds the same note to all tracks (useful for layering).
/// </summary>
class AddNoteCommand : public Command
{
public:
	AddNoteCommand(TrackSet& trackSet, const std::vector<int>& targetTracks,
	               ubyte pitch, ubyte velocity, uint64_t startTick, uint64_t duration)
		: mTrackSet(trackSet)
		, mTargetTracks(targetTracks)
		, mPitch(pitch)
		, mVelocity(velocity)
		, mStartTick(startTick)
		, mDuration(duration)
	{
	}

	void Execute() override;
	void Undo() override;
	std::string GetDescription() const override;

private:
	struct NoteIndices {
		int trackIndex = 0;
		size_t noteOnIndex = 0;
		size_t noteOffIndex = 0;
	};

	TrackSet& mTrackSet;
	std::vector<int> mTargetTracks;
	ubyte mPitch;
	ubyte mVelocity;
	uint64_t mStartTick;
	uint64_t mDuration;
	std::vector<NoteIndices> mAddedNotes;  // Indices of notes added to each track
};

//==============================================================================
// DeleteNoteCommand
//==============================================================================

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

	void Execute() override;
	void Undo() override;
	std::string GetDescription() const override;

private:
	Track& mTrack;
	size_t mNoteOnIndex;
	size_t mNoteOffIndex;
	TimedMidiEvent mNoteOn;
	TimedMidiEvent mNoteOff;
};

//==============================================================================
// MoveNoteCommand
//==============================================================================

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

	void Execute() override;
	void Undo() override;
	std::string GetDescription() const override;

private:
	Track& mTrack;
	size_t mNoteOnIndex;
	size_t mNoteOffIndex;
	uint64_t mOldTick;
	uint64_t mNewTick;
	uint8_t mOldPitch;
	uint8_t mNewPitch;
	uint64_t mNoteDuration;

	size_t FindNoteIndex(uint64_t tick, uint8_t pitch, MidiEvent eventType);
};

//==============================================================================
// ResizeNoteCommand
//==============================================================================

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

	void Execute() override;
	void Undo() override;
	std::string GetDescription() const override;

private:
	Track& mTrack;
	size_t mNoteOnIndex;
	size_t mNoteOffIndex;
	uint64_t mOldDuration;
	uint64_t mNewDuration;
	uint64_t mNoteOnTick;
	uint8_t mPitch;

	size_t FindNoteIndex(uint64_t tick, uint8_t pitch, MidiEvent eventType);
};

//==============================================================================
// EditNoteVelocityCommand
//==============================================================================

/// <summary>
/// Command to edit a note's velocity.
/// Stores both old and new velocities to enable undo/redo.
/// </summary>
class EditNoteVelocityCommand : public Command
{
public:
	EditNoteVelocityCommand(Track& track, size_t noteOnIndex, ubyte newVelocity)
		: mTrack(track), mNoteOnIndex(noteOnIndex), mNewVelocity(newVelocity)
	{
		// Store original velocity
		if (mNoteOnIndex < mTrack.size())
		{
			mOldVelocity = mTrack[mNoteOnIndex].mm.getVelocity();
		}
	}

	void Execute() override;
	void Undo() override;
	std::string GetDescription() const override;

private:
	Track& mTrack;
	size_t mNoteOnIndex;
	ubyte mOldVelocity = 0;
	ubyte mNewVelocity = 0;
};
