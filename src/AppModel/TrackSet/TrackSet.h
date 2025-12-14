#pragma once
#include <array>
#include <vector>
#include "RtMidiWrapper/MidiMessage/MidiMessage.h"
#include "MidiConstants.h"
#include <algorithm>
using namespace MidiInterface;

struct TimedMidiEvent
{
	MidiMessage mm;
	uint64_t	tick = 0;
};

struct NoteLocation
{
	bool		found		 = false;
	int			trackIndex   = -1;
	size_t		noteOnIndex  = 0;
	size_t		noteOffIndex = 0;
	uint64_t	startTick    = 0;
	uint64_t	endTick      = 0;
	ubyte		pitch		 = 0;

	// Equality operator for finding notes in selection
	bool operator==(const NoteLocation& other) const {
		return trackIndex == other.trackIndex &&
		       noteOnIndex == other.noteOnIndex &&
		       noteOffIndex == other.noteOffIndex;
	}
};


using Track = std::vector<TimedMidiEvent>;
using TrackBank = std::array<Track, MidiConstants::CHANNEL_COUNT>;  // CHANNEL_COUNT tracks (channel 16 reserved for metronome)

class TrackSet
{
public:

	Track& GetTrack(ubyte channelNumber);

	std::vector<MidiMessage> PlayBack(uint64_t currentTick);

	void FindStart(uint64_t startTick);

	NoteLocation FindNoteAt(uint64_t tick, ubyte pitch);

	std::vector<NoteLocation> FindNotesInRegion(uint64_t minTick, uint64_t maxTick, 
			ubyte minPitch, ubyte maxPitch);

	std::vector<NoteLocation> GetAllNotes();

	void FinalizeRecording(Track& recordingBuffer);

private:
	TrackBank mTracks;
	int iterators[MidiConstants::CHANNEL_COUNT]{-1};

	void Sort();

};
