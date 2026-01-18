// MultiNoteCommands.h
#pragma once
#include "Command.h"
#include "AppModel/TrackSet/TrackSet.h"
#include <algorithm>
#include <vector>
#include <map>
#include <set>
using namespace MidiInterface;

// Multi-Note Batch Commands

/// Deletes multiple notes across multiple tracks.
///
/// Responsibilities:
/// - Handle batch deletion efficiently
/// - Maintain undo capability for all deleted notes
///
/// Usage:
///   auto cmd = std::make_unique<DeleteMultipleNotesCommand>(trackSet, notesToDelete);
///   appModel.ExecuteCommand(std::move(cmd));
class DeleteMultipleNotesCommand : public Command
{
public:
	/// Construct a delete multiple notes command
	/// @param notesToDelete Vector of notes to delete
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

/// Moves multiple notes by applying a tick and pitch delta.
///
/// Responsibilities:
/// - Apply tick and pitch deltas to multiple notes
/// - Store original positions for undo
///
/// Usage:
///   auto cmd = std::make_unique<MoveMultipleNotesCommand>(trackSet, notesToMove, tickDelta, pitchDelta);
///   appModel.ExecuteCommand(std::move(cmd));
class MoveMultipleNotesCommand : public Command
{
public:
	/// Construct a move multiple notes command
	/// @param notesToMove Vector of notes to move
	/// @param tickDelta Tick offset to apply (can be negative)
	/// @param pitchDelta Pitch offset to apply (can be negative)
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

/// Quantizes a specific set of notes (e.g., selected notes only).
///
/// Responsibilities:
/// - Quantize specific notes to grid
/// - Use duration-aware quantization algorithm
/// - Provide surgical control without affecting other notes
///
/// Usage:
///   auto cmd = std::make_unique<QuantizeMultipleNotesCommand>(trackSet, notesToQuantize, gridSize);
///   appModel.ExecuteCommand(std::move(cmd));
class QuantizeMultipleNotesCommand : public Command
{
public:
	/// Construct a quantize multiple notes command
	/// @param notesToQuantize Vector of notes to quantize
	/// @param gridSize Grid size in ticks (e.g., 960 for quarter notes)
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
