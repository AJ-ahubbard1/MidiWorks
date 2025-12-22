#pragma once
#include "Command.h"
#include "AppModel/TrackSet/TrackSet.h"
#include <algorithm>
#include <cstdint>
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

	void Execute() override;
	void Undo() override;
	std::string GetDescription() const override;

private:
	Track& mTrack;
	TimedMidiEvent mNoteOn;
	TimedMidiEvent mNoteOff;
	size_t mNoteOnIndex = 0;
	size_t mNoteOffIndex = 0;

	void FindNoteIndices();
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
