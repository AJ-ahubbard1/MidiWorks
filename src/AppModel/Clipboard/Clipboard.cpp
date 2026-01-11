// Clipboard.cpp
#include "Clipboard.h"
#include <limits>

void Clipboard::CopyNotes(const std::vector<NoteLocation>& notes, TrackSet& trackSet)
{
	if (notes.empty()) return;

	// Find the earliest start tick to use as reference (for relative timing)
	uint64_t earliestTick = UINT64_MAX;
	for (const auto& note : notes)
	{
		if (note.startTick < earliestTick)
		{
			earliestTick = note.startTick;
		}
	}

	// Convert selected notes to clipboard format
	std::vector<ClipboardNote> clipboardNotes;
	clipboardNotes.reserve(notes.size());

	for (const auto& note : notes)
	{
		Track& track = trackSet.GetTrack(note.trackIndex);
		const TimedMidiEvent& noteOnEvent = track[note.noteOnIndex];

		ClipboardNote clipNote;
		clipNote.relativeStartTick = note.startTick - earliestTick;
		clipNote.duration = note.endTick - note.startTick;
		clipNote.pitch = note.pitch;
		clipNote.velocity = note.velocity;
		clipNote.trackIndex = note.trackIndex;

		clipboardNotes.push_back(clipNote);
	}

	// Store in clipboard
	mNotes = clipboardNotes;
}

const std::vector<Clipboard::ClipboardNote>& Clipboard::GetNotes() const
{
	return mNotes;
}

bool Clipboard::HasData() const
{
	return !mNotes.empty();
}

void Clipboard::Clear()
{
	mNotes.clear();
}
