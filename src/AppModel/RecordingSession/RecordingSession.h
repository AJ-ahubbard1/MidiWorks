// RecordingSession.h
// Manages the temporary recording buffer and active note tracking for loop recording
#pragma once
#include "../TrackSet/TrackSet.h"
#include <vector>

class RecordingSession
{
public:
	RecordingSession();

	// Buffer management
	Track& GetBuffer();
	const Track& GetBuffer() const;
	void Clear();
	bool IsEmpty() const;
	void AddEvent(const TimedMidiEvent& event);

	// Recording with automatic note tracking
	void RecordEvent(const MidiMessage& msg, uint64_t currentTick);

	// Active note tracking (for loop recording - prevents stuck notes at loop boundaries)
	void StartNote(ubyte pitch, ubyte channel, uint64_t startTick);
	void StopNote(ubyte pitch, ubyte channel);
	void CloseAllActiveNotes(uint64_t endTick);
	bool HasActiveNotes() const;

	// Loop recording playback (plays back previously recorded material during loop recording)
	void ResetLoopPlayback(uint64_t loopStartTick);
	std::vector<MidiMessage> GetLoopPlaybackMessages(uint64_t currentTick);

private:
	Track mBuffer;
	int mBufferIterator = -1;

	struct ActiveNote {
		ubyte pitch;
		ubyte channel;
		uint64_t startTick;
	};
	std::vector<ActiveNote> mActiveNotes;
};
