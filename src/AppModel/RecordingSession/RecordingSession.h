// RecordingSession.h
// Manages the temporary recording buffer and active note tracking for loop recording
#pragma once
#include "../TrackSet/TrackSet.h"
#include <vector>

class RecordingSession
{
public:
	RecordingSession() { }

	// Buffer management

	/// Get recording buffer
	Track& GetBuffer() { return mBuffer; }
	/// Get recording buffer (cannot modify)
	const Track& GetBuffer() const { return mBuffer; }
	/// Is the recording buffer empty?
	bool IsEmpty() const { return mBuffer.empty(); }
	/// clear the recording buffer and active notes list, and reset the iterator
	void Clear();

	// Recording to buffer

	/// Records midi messages to the buffer
	/// if message is a NOTE ON event, update the active notes list 
	void RecordEvent(const MidiMessage& msg, uint64_t currentTick);

	/// When we reach the end of the loop region: 
	/// Close held notes at loop end (adds NOTE_OFF events),
	/// and then wraps the active notes (adds (NOTE_ON events) to loop start
	void WrapActiveNotesAtLoop(uint64_t endTick, uint64_t loopStartTick);
	/// Closes held notes without reopening (for stop recording)
	void CloseAllActiveNotes(uint64_t endTick);  

	// Active notes

	/// Are there notes currently held down?
	bool HasActiveNotes() const { return !mActiveNotes.empty(); }
	const std::vector<TimedMidiEvent>& GetActiveNotes() const { return mActiveNotes; }
	
	// Loop recording playback (plays back previously recorded material during loop recording)

	/// Resets the iterator to the first midi event in the loop region
	void ResetLoopPlayback(uint64_t loopStartTick);
	
	/// Get the list of notes that are scheduled for playback in recording buffer	
	/// Note: We only listen to recording buffer messages after they've been added to buffer 
	/// - and we've looped back to them.  
	/// - Midi In Playback handles hearing the notes the first time (during their actual recording)
	std::vector<MidiMessage> GetLoopPlaybackMessages(uint64_t currentTick);

private:
	Track mBuffer;
	int mBufferIterator = -1;
	/// Active note tracking - displayed as light up piano keys
	/// also used for loop recording - can check active notes and prevent them from
	/// sticking at loop boundaries
	std::vector<TimedMidiEvent> mActiveNotes;
	
	/// Add a timed midi event to the recording buffer during recording
	void AddEvent(const TimedMidiEvent& event) { mBuffer.push_back(event); }
	/// Adds note to active notes vector	
	void StartNote(const TimedMidiEvent& note) { mActiveNotes.push_back(note); }
	/// Removes the active note with the given channel and pitch
	void StopNote(ubyte channel, ubyte pitch);
};
