// NoteEditCommands.h
#pragma once
#include "Command.h"
#include "AppModel/TrackSet/TrackSet.h"
#include <algorithm>
#include <cstdint>
using namespace MidiInterface;

// Single-Note Edit Commands

/// Adds a note to one or more tracks.
///
/// Responsibilities:
/// - Add note-on and note-off events to target tracks
/// - Store note indices per track for efficient undo
/// - Support multi-track layering (same note to multiple tracks)
///
/// Usage:
///   auto cmd = std::make_unique<AddNoteCommand>(trackSet, targetTracks, pitch, velocity, startTick, duration);
///   appModel.ExecuteCommand(std::move(cmd));
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
	TrackSet& mTrackSet;
	std::vector<int> mTargetTracks;
	ubyte mPitch;
	ubyte mVelocity;
	uint64_t mStartTick;
	uint64_t mDuration;
	std::vector<NoteLocation> mAddedNotes;  // Notes added to each track
};

/// Deletes a note from a track.
///
/// Responsibilities:
/// - Remove note-on and note-off events from track
/// - Store deleted note data for undo
///
/// Usage:
///   auto cmd = std::make_unique<DeleteNoteCommand>(track, noteOnIndex, noteOffIndex);
///   appModel.ExecuteCommand(std::move(cmd));
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

/// Moves a note to a different tick/pitch position.
///
/// Responsibilities:
/// - Change note position (tick and pitch)
/// - Store old and new positions for undo/redo
/// - Maintain note duration
///
/// Usage:
///   auto cmd = std::make_unique<MoveNoteCommand>(track, noteOnIndex, noteOffIndex, newTick, newPitch);
///   appModel.ExecuteCommand(std::move(cmd));
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

/// Resizes a note (changes its duration).
///
/// Responsibilities:
/// - Change note duration by moving note-off event
/// - Store old and new durations for undo/redo
///
/// Usage:
///   auto cmd = std::make_unique<ResizeNoteCommand>(track, noteOnIndex, noteOffIndex, newDuration);
///   appModel.ExecuteCommand(std::move(cmd));
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

/// Edits a note's velocity.
///
/// Responsibilities:
/// - Change note-on velocity
/// - Store old and new velocities for undo/redo
///
/// Usage:
///   auto cmd = std::make_unique<EditNoteVelocityCommand>(track, noteOnIndex, newVelocity);
///   appModel.ExecuteCommand(std::move(cmd));
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
