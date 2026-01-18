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
	void SelectNote(const NoteLocation& note)
	{
		// Don't add if already selected (avoid duplicates)
		if (Contains(note)) return;

		mSelectedNotes.push_back(note);
	}

	/// Replace current selection entirely with the provided notes.
	void SelectNotes(const std::vector<NoteLocation>& notes) { mSelectedNotes = notes; }

	/// Deselect a specific note. If not selected, does nothing.
	void DeselectNote(const NoteLocation& note)
	{
		// Remove the note from selection if present
		auto it = std::find_if(mSelectedNotes.begin(), mSelectedNotes.end(),
			[&note](const NoteLocation& selected) {
				return selected == note;
			});

		if (it != mSelectedNotes.end())
		{
			mSelectedNotes.erase(it);
		}
	}

	/// Clear all selected notes.
	void Clear() { mSelectedNotes.clear(); }

	/// Check if a specific note is selected.
	bool Contains(const NoteLocation& note) const
	{
		for (const auto& selected : mSelectedNotes)
		{
			if (selected == note)
				return true;
		}
		return false;
	}

	/// Check if no notes are selected.
	bool IsEmpty() const { return mSelectedNotes.empty(); }

	/// Get read-only access to all selected notes.
	const std::vector<NoteLocation>& GetNotes() const { return mSelectedNotes; }

	/// Update the velocity of a note in selection, used when user drags velocity control
	void UpdateVelocity(ubyte trackIndex, int noteOnIndex, ubyte newVelocity)
	{
		for (auto& note : mSelectedNotes)
		{
			if (note.trackIndex == trackIndex && note.noteOnIndex == noteOnIndex)
			{
				note.velocity = newVelocity;
				break;
			}
		}
	}

private:
	std::vector<NoteLocation> mSelectedNotes;
};

