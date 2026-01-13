//==============================================================================
// MultiNoteCommands.h
// Commands for batch operations on multiple notes
//==============================================================================

#pragma once
#include "Command.h"
#include "AppModel/TrackSet/TrackSet.h"
#include <algorithm>
#include <vector>
#include <map>
#include <set>
using namespace MidiInterface;

//==============================================================================
// DeleteMultipleNotesCommand
//==============================================================================

/// <summary>
/// Command to delete multiple notes across multiple tracks.
/// Handles batch deletion efficiently while maintaining undo capability.
/// </summary>
class DeleteMultipleNotesCommand : public Command
{
public:
	/// <summary>
	/// Represents a note to be deleted with its location and event data.
	/// </summary>
	struct NoteToDelete 
	{
		int trackIndex = 0;          // Which track the note is in
		size_t noteOnIndex = 0;      // Index of note-on event
		size_t noteOffIndex = 0;     // Index of note-off event
		TimedMidiEvent noteOn;       // Stored for undo
		TimedMidiEvent noteOff;      // Stored for undo
	};

	/// <summary>
	/// Construct a delete multiple notes command.
	/// </summary>
	/// <param name="trackSet">Reference to the track set</param>
	/// <param name="notesToDelete">Vector of notes to delete</param>
	DeleteMultipleNotesCommand(TrackSet& trackSet, const std::vector<NoteToDelete>& notesToDelete)
		: mTrackSet(trackSet)
		, mNotesToDelete(notesToDelete)
	{
	}

	void Execute() override;
	void Undo() override;
	std::string GetDescription() const override;

private:
	TrackSet& mTrackSet;
	std::vector<NoteToDelete> mNotesToDelete;
};

//==============================================================================
// MoveMultipleNotesCommand
//==============================================================================

/// <summary>
/// Command to move multiple notes by applying a tick and pitch delta.
/// Stores original positions to enable undo.
/// </summary>
class MoveMultipleNotesCommand : public Command
{
public:
	/// <summary>
	/// Represents a note to be moved with its location and original properties.
	/// </summary>
	struct NoteToMove 
	{
		int trackIndex = 0;              // Which track the note is in
		size_t noteOnIndex = 0;          // Index of note-on event
		size_t noteOffIndex = 0;         // Index of note-off event
		uint64_t originalStartTick = 0;  // Original start position (for undo)
		ubyte originalPitch = 0;         // Original pitch (for undo)
		uint64_t duration = 0;           // Note duration (preserved during move)
	};

	/// <summary>
	/// Construct a move multiple notes command.
	/// </summary>
	/// <param name="trackSet">Reference to the track set</param>
	/// <param name="notesToMove">Vector of notes to move</param>
	/// <param name="tickDelta">Tick offset to apply (can be negative)</param>
	/// <param name="pitchDelta">Pitch offset to apply (can be negative)</param>
	MoveMultipleNotesCommand(TrackSet& trackSet, const std::vector<NoteToMove>& notesToMove,
		int64_t tickDelta, int pitchDelta)
		: mTrackSet(trackSet)
		, mNotesToMove(notesToMove)
		, mTickDelta(tickDelta)
		, mPitchDelta(pitchDelta)
	{
	}

	void Execute() override;
	void Undo() override;
	std::string GetDescription() const override;

private:
	TrackSet& mTrackSet;
	std::vector<NoteToMove> mNotesToMove;
	int64_t mTickDelta;  // Tick offset to apply to all notes
	int mPitchDelta;     // Pitch offset to apply to all notes
};

//==============================================================================
// QuantizeMultipleNotesCommand
//==============================================================================

/// <summary>
/// Command to quantize a specific set of notes (e.g., selected notes only).
/// Uses the same duration-aware quantization algorithm as QuantizeCommand.
/// Provides surgical control over which notes to quantize without affecting others.
/// </summary>
class QuantizeMultipleNotesCommand : public Command
{
public:
	/// <summary>
	/// Represents a note to be quantized with its location and original properties.
	/// </summary>
	struct NoteToQuantize 
	{
		int trackIndex = 0;              // Which track the note is in
		size_t noteOnIndex = 0;          // Index of note-on event
		size_t noteOffIndex = 0;         // Index of note-off event
		uint64_t originalStartTick = 0;  // Original start position (for undo)
		uint64_t originalEndTick = 0;    // Original end position (for undo)
		ubyte pitch = 0;                 // Pitch (for identification)
	};

	/// <summary>
	/// Construct a quantize multiple notes command.
	/// </summary>
	/// <param name="trackSet">Reference to the track set</param>
	/// <param name="notesToQuantize">Vector of notes to quantize</param>
	/// <param name="gridSize">Grid size in ticks (e.g., 960 for quarter notes)</param>
	QuantizeMultipleNotesCommand(TrackSet& trackSet, const std::vector<NoteToQuantize>& notesToQuantize,
		uint64_t gridSize)
		: mTrackSet(trackSet)
		, mNotesToQuantize(notesToQuantize)
		, mGridSize(gridSize)
	{
	}

	void Execute() override;
	void Undo() override;
	std::string GetDescription() const override;

private:
	TrackSet& mTrackSet;
	std::vector<NoteToQuantize> mNotesToQuantize;
	uint64_t mGridSize;  // Grid size for quantization
};
