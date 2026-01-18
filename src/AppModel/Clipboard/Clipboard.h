// Clipboard.h
#pragma once
#include <vector>
#include <cstdint>
#include <limits>
#include "AppModel/TrackSet/TrackSet.h"

class Clipboard
{
public:
	struct ClipboardNote
	{
		uint64_t relativeStartTick;  // Relative to first note in selection
		uint64_t duration;
		uint8_t pitch;
		uint8_t velocity;
		int trackIndex;  // Which track it came from
	};

	/// Copy notes to clipboard (converts NoteLocation to ClipboardNote)
	void CopyNotes(const std::vector<NoteLocation>& notes, TrackSet& trackSet)
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

	/// Access clipboard data
	/// @return the vector of clipboard notes
	const std::vector<ClipboardNote>& GetNotes() const { return mNotes; }
	
	/// returns true if the clipboard has notes (is not empty) 	
	bool HasData() const { return !mNotes.empty(); }

	/// clear the clipboard notes vector 
	void Clear() { mNotes.clear(); }

private:
	std::vector<ClipboardNote> mNotes;
};
