// RecordingSession.cpp
#include "RecordingSession.h"
#include <algorithm>

RecordingSession::RecordingSession()
{
}

Track& RecordingSession::GetBuffer()
{
	return mBuffer;
}

const Track& RecordingSession::GetBuffer() const
{
	return mBuffer;
}

void RecordingSession::Clear()
{
	mBuffer.clear();
	mBufferIterator = -1;
	mActiveNotes.clear();
}

bool RecordingSession::IsEmpty() const
{
	return mBuffer.empty();
}

void RecordingSession::AddEvent(const TimedMidiEvent& event)
{
	mBuffer.push_back(event);
}

void RecordingSession::RecordEvent(const MidiMessage& msg, uint64_t currentTick)
{
	// Add to recording buffer
	TimedMidiEvent recordedEvent{msg, currentTick};
	AddEvent(recordedEvent);
	ubyte velocity = msg.getVelocity();

	// Track active notes for loop recording
	if (msg.isNoteOn() && velocity > 0)  // NoteOn with velocity > 0
	{
		StartNote(recordedEvent);
	}
	else if (msg.isNoteOff() || (msg.isNoteOn() && velocity == 0))  // NoteOff
	{
		StopNote(msg.getChannel(), msg.getPitch());
	}
}

void RecordingSession::StartNote(const TimedMidiEvent & note)
{
	mActiveNotes.push_back(note);
}

void RecordingSession::StopNote(ubyte channel, ubyte pitch)
{
	auto it = std::remove_if(mActiveNotes.begin(), mActiveNotes.end(),
		[channel, pitch](const TimedMidiEvent& note) 
		{
			return note.mm.getPitch() == pitch && note.mm.getChannel() == channel;
		});
	mActiveNotes.erase(it, mActiveNotes.end());
}

void RecordingSession::WrapActiveNotesAtLoop(uint64_t endTick, uint64_t loopStartTick)
{
	for (auto& note : mActiveNotes)  // Note: not const, we modify tick 
	{
		// Close the note at loop end
		MidiMessage noteOff = MidiMessage::NoteOff(note.mm.getPitch(), note.mm.getChannel());
		mBuffer.push_back({noteOff, endTick});

		// Reopen the note at loop start (user still holding key)
		// note.mm already IS the Note On message - just reuse it!
		mBuffer.push_back({note.mm, loopStartTick});

		// Update active note's start tick for eventual release
		note.tick = loopStartTick;
	}
	// Don't clear mActiveNotes - notes are still physically held!
}

void RecordingSession::CloseAllActiveNotes(uint64_t endTick)
{
	// Close all active notes at the specified tick (for stopping recording)
	for (const auto& note : mActiveNotes)
	{
		MidiMessage noteOff = MidiMessage::NoteOff(note.mm.getPitch(), note.mm.getChannel());
		mBuffer.push_back({noteOff, endTick});
	}
	// Clear active notes - they're now closed
	mActiveNotes.clear();
}

bool RecordingSession::HasActiveNotes() const
{
	return !mActiveNotes.empty();
}

void RecordingSession::ResetLoopPlayback(uint64_t loopStartTick)
{
	// Handle case where user enables loop recording but hasn't played anything yet
	// (e.g., waiting through one or more loop iterations before adding notes)
	if (mBuffer.empty())
	{
		return;  // Iterator will remain -1 from previous Clear() or construction
	}

	// Find first event at or after loop start position
	int i = 0;
	while (i < mBuffer.size() && mBuffer[i].tick < loopStartTick)
	{
		i++;
	}
	mBufferIterator = (i < mBuffer.size()) ? i : -1;
}

// We only listen to recording buffer messages after they've been added to buffer 
// and we've looped back to them,
// Midi In Playback handles hearing the notes the first time
std::vector<MidiMessage> RecordingSession::GetLoopPlaybackMessages(uint64_t currentTick)
{
	std::vector<MidiMessage> messages;

	// Process all events at or before currentTick
	while (mBufferIterator != -1 &&
	       mBufferIterator < mBuffer.size() &&
	       mBuffer[mBufferIterator].tick <= currentTick)
	{
		messages.push_back(mBuffer[mBufferIterator].mm);
		mBufferIterator++;
		if (mBufferIterator >= mBuffer.size())
		{
			mBufferIterator = -1;
			break;
		}
	}

	return messages;
}
