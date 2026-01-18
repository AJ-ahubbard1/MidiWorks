// TrackSet.h
#pragma once
#include <array>
#include <vector>
#include <algorithm>
#include <cstdint>
#include "RtMidiWrapper/MidiMessage/MidiMessage.h"
#include "MidiConstants.h"
#include "NoteTypes.h"
using namespace MidiInterface;

struct TimedMidiEvent
{
	MidiMessage mm;
	uint64_t	tick = 0;
};

using Track = std::vector<TimedMidiEvent>;
using TrackBank = std::array<Track, MidiConstants::CHANNEL_COUNT>;  // CHANNEL_COUNT tracks (channel 16 reserved for metronome)

/// TrackSet manages MIDI track data for all channels.
///
/// Responsibilities:
/// - Store MIDI events organized by channel (15 tracks)
/// - Provide playback iteration with FindStart/PlayBack
/// - Find notes by position, pitch, or region
/// - Static helpers for track operations (sort, quantize, overlap separation)
///
/// Usage:
///   TrackSet trackSet;
///   Track& track = trackSet.GetTrack(0);
///   track.push_back({midiMessage, tick});
///   trackSet.FindStart(0);
///   auto messages = trackSet.PlayBack(currentTick);
class TrackSet
{
public:
	// Track Access

	/// Get a track by channel number
	Track& GetTrack(ubyte channelNumber) { return mTracks[channelNumber]; }

	/// Check if a specific track is empty
	bool IsTrackEmpty(ubyte channelNumber) { return GetTrack(channelNumber).empty(); }

	/// Check if all tracks are empty (no notes anywhere)
	bool IsEmpty() const;

	// Playback

	/// Get messages scheduled at or before the current tick
	std::vector<MidiMessage> PlayBack(uint64_t currentTick);

	/// Set playback iterators to start from a specific tick
	void FindStart(uint64_t startTick);

	// Note Finding

	/// Find a note at a specific tick and pitch (searches all tracks)
	NoteLocation FindNoteAt(uint64_t tick, ubyte pitch) const;

	/// Find a note in a specific track by its exact boundaries
	NoteLocation FindNoteInTrack(int trackIndex, uint64_t startTick, uint64_t endTick, ubyte pitch) const;

	/// Find all notes overlapping a region
	/// @param trackIndex Specific track to search, or -1 for all tracks
	std::vector<NoteLocation> FindNotesInRegion(uint64_t minTick, uint64_t maxTick, ubyte minPitch, ubyte maxPitch, int trackIndex = -1) const;

	/// Get all notes from all tracks
	std::vector<NoteLocation> GetAllNotes() const;

	/// Get all raw MIDI events from all tracks (for debugging)
	std::vector<TimedMidiEvent> GetAllTimedMidiEvents();

	// Static Track Helpers

	/// Extract note pairs from a single track
	/// @param trackIndex Index to assign to notes (use 0 for non-trackset tracks like recording buffer)
	static std::vector<NoteLocation> GetNotesFromTrack(const Track& track, int trackIndex = 0);

	/// Sort a track by tick
	static void SortTrack(Track& track);

	/// Separate overlapping notes during loop recording.
	/// When consecutive NoteOn messages of same pitch/channel occur,
	/// shifts NoteOff of first note to keep them as separate notes.
	static void SeparateOverlappingNotes(Track& buffer);

	/// Apply duration-aware quantization to a track.
	/// Short notes (< gridSize) are extended to one grid snap.
	/// Long notes (>= gridSize) have both start and end quantized independently.
	static void QuantizeTrack(Track& track, uint64_t gridSize);

	// Recording

	/// Finalize recording by moving events from buffer to tracks (clears buffer after)
	void FinalizeRecording(Track& recordingBuffer);

private:
	TrackBank mTracks;
	int iterators[MidiConstants::CHANNEL_COUNT]{-1};

	/// Sort all tracks by tick
	void Sort();
};
