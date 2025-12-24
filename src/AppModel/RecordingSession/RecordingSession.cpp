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
	AddEvent({msg, currentTick});

	// Track active notes for loop recording
	ubyte status = msg.mData[0] & 0xF0;
	ubyte pitch = msg.getPitch();
	ubyte channel = msg.getChannel();

	if (status == 0x90 && msg.mData[2] > 0)  // NoteOn with velocity > 0
	{
		StartNote(pitch, channel, currentTick);
	}
	else if (status == 0x80 || (status == 0x90 && msg.mData[2] == 0))  // NoteOff
	{
		StopNote(pitch, channel);
	}
}

void RecordingSession::StartNote(ubyte pitch, ubyte channel, uint64_t startTick)
{
	mActiveNotes.push_back({pitch, channel, startTick});
}

void RecordingSession::StopNote(ubyte pitch, ubyte channel)
{
	auto it = std::remove_if(mActiveNotes.begin(), mActiveNotes.end(),
		[pitch, channel](const ActiveNote& note) {
			return note.pitch == pitch && note.channel == channel;
		});
	mActiveNotes.erase(it, mActiveNotes.end());
}

void RecordingSession::CloseAllActiveNotes(uint64_t endTick)
{
	// Create NoteOff messages for all active notes
	for (const auto& note : mActiveNotes)
	{
		MidiMessage noteOff = MidiMessage::NoteOff(note.pitch, note.channel);
		mBuffer.push_back({noteOff, endTick});
	}
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
