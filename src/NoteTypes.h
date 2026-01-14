#pragma once
#include <cstdint>
#include "RtMidiWrapper/MidiMessage/MidiMessage.h"

using MidiInterface::ubyte;

struct NoteLocation
{
	bool		found		 = false;
	int			trackIndex   = -1;
	size_t		noteOnIndex  = 0;
	size_t		noteOffIndex = 0;
	uint64_t	startTick    = 0;
	uint64_t	endTick      = 0;
	ubyte		pitch		 = 0;
	ubyte		velocity     = 0;

	// Equality operator for finding notes in selection
	bool operator==(const NoteLocation& other) const 
	{
		return trackIndex == other.trackIndex &&
		       noteOnIndex == other.noteOnIndex &&
		       noteOffIndex == other.noteOffIndex;
	}

	uint64_t GetDuration() const { return endTick - startTick; }
};

