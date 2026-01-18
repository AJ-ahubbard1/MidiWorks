// Selection.h
#pragma once
#include "AppModel/TrackSet/TrackSet.h"
#include <vector>
#include <algorithm>

/// Selection occurs when a user Shift + Left Clicks and drags a rectangle around 1 or more notes.
/// The Selection class manages note selection state for the application.
/// Selection tracks which notes are currently selected for operations like copy, delete, quantize, edit velocity, etc.
class Selection
{
public:
	/// Select a single note. If already selected, does nothing.
	void SelectNote(const NoteLocation& note);

	/// Replace current selection entirely with the provided notes.
	void SelectNotes(const std::vector<NoteLocation>& notes) { mSelectedNotes = notes; }

	/// Deselect a specific note. If not selected, does nothing.
	void DeselectNote(const NoteLocation& note);

	/// Clear all selected notes.
	void Clear() { mSelectedNotes.clear(); }

	/// Check if a specific note is selected.
	bool Contains(const NoteLocation& note) const;

	/// Check if no notes are selected.
	bool IsEmpty() const { return mSelectedNotes.empty(); }

	/// Get read-only access to all selected notes.
	const std::vector<NoteLocation>& GetNotes() const { return mSelectedNotes; }

	/// Update the velocity of a note in selection, used when user drags velocity control
	void UpdateVelocity(ubyte trackIndex, int noteOnIndex, ubyte newVelocity);

private:
	std::vector<NoteLocation> mSelectedNotes;
};
