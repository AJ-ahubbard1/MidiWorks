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

void RecordingSession::InitializeLoopPlayback(uint64_t startTick)
{
	// Find first event >= current position
	if (mBuffer.empty())
	{
		mBufferIterator = -1;
		return;
	}

	int i = 0;
	while (i < mBuffer.size() && mBuffer[i].tick < startTick)
	{
		i++;
	}
	mBufferIterator = (i < mBuffer.size()) ? i : -1;
}

void RecordingSession::ResetLoopPlayback(uint64_t loopStartTick)
{
	if (mBuffer.empty())
	{
		mBufferIterator = -1;
		return;
	}

	int i = 0;
	while (i < mBuffer.size() && mBuffer[i].tick < loopStartTick)
	{
		i++;
	}
	mBufferIterator = (i < mBuffer.size()) ? i : -1;
}

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
