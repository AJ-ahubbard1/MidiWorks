// Clipboard.h
#pragma once
#include <vector>
#include <cstdint>
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

	// Copy notes to clipboard (converts NoteLocation to ClipboardNote)
	void CopyNotes(const std::vector<NoteLocation>& notes, TrackSet& trackSet);

	// Access clipboard data
	const std::vector<ClipboardNote>& GetNotes() const;
	bool HasData() const;
	void Clear();

private:
	std::vector<ClipboardNote> mNotes;
};
