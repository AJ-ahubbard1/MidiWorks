// Selection.cpp
#include "Selection.h"

void Selection::SelectNote(const NoteLocation& note)
{
	// Don't add if already selected (avoid duplicates)
	if (Contains(note)) return;

	mSelectedNotes.push_back(note);
}

void Selection::SelectNotes(const std::vector<NoteLocation>& notes)
{
	// Replace current selection entirely
	mSelectedNotes = notes;
}

void Selection::DeselectNote(const NoteLocation& note)
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

void Selection::Clear()
{
	mSelectedNotes.clear();
}

bool Selection::Contains(const NoteLocation& note) const
{
	for (const auto& selected : mSelectedNotes)
	{
		if (selected == note)
			return true;
	}
	return false;
}

bool Selection::IsEmpty() const
{
	return mSelectedNotes.empty();
}

const std::vector<NoteLocation>& Selection::GetNotes() const
{
	return mSelectedNotes;
}

void Selection::UpdateVelocity(ubyte trackIndex, int noteOnIndex, ubyte newVelocity)
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
