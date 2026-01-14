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
	/// Construct a delete multiple notes command.
	/// </summary>
	/// <param name="trackSet">Reference to the track set</param>
	/// <param name="notesToDelete">Vector of notes to delete</param>
	DeleteMultipleNotesCommand(TrackSet& trackSet, const std::vector<NoteLocation>& notesToDelete)
		: mTrackSet(trackSet)
		, mNotesToDelete(notesToDelete)
	{
	}

	void Execute() override;
	void Undo() override;
	std::string GetDescription() const override;

private:
	TrackSet& mTrackSet;
	std::vector<NoteLocation> mNotesToDelete;
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
	/// Construct a move multiple notes command.
	/// </summary>
	/// <param name="trackSet">Reference to the track set</param>
	/// <param name="notesToMove">Vector of notes to move</param>
	/// <param name="tickDelta">Tick offset to apply (can be negative)</param>
	/// <param name="pitchDelta">Pitch offset to apply (can be negative)</param>
	MoveMultipleNotesCommand(TrackSet& trackSet, const std::vector<NoteLocation>& notesToMove,
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
	std::vector<NoteLocation> mNotesToMove;
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
	/// Construct a quantize multiple notes command.
	/// </summary>
	/// <param name="trackSet">Reference to the track set</param>
	/// <param name="notesToQuantize">Vector of notes to quantize</param>
	/// <param name="gridSize">Grid size in ticks (e.g., 960 for quarter notes)</param>
	QuantizeMultipleNotesCommand(TrackSet& trackSet, const std::vector<NoteLocation>& notesToQuantize,
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
	std::vector<NoteLocation> mNotesToQuantize;
	uint64_t mGridSize;  // Grid size for quantization
};
