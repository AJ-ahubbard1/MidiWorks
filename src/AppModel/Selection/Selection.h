#pragma once
#include "AppModel/TrackSet/TrackSet.h"
#include <vector>
#include <algorithm>

/// <summary>
/// Manages note selection state for the application.
/// Tracks which notes are currently selected for operations like copy, delete, quantize, etc.
/// </summary>
class Selection
{
public:
	/// Select a single note. If already selected, does nothing.
	void SelectNote(const NoteLocation& note);

	/// Replace current selection with the provided notes.
	void SelectNotes(const std::vector<NoteLocation>& notes);

	/// Deselect a specific note. If not selected, does nothing.
	void DeselectNote(const NoteLocation& note);

	/// Clear all selected notes.
	void Clear();

	/// Check if a specific note is selected.
	bool Contains(const NoteLocation& note) const;

	/// Check if no notes are selected.
	bool IsEmpty() const;

	/// Get read-only access to all selected notes.
	const std::vector<NoteLocation>& GetNotes() const;

	/// Update Velocity of a note in selection
	void UpdateVelocity(ubyte trackIndex, int noteOnIndex, ubyte newVelocity);


private:
	std::vector<NoteLocation> mSelectedNotes;
};
