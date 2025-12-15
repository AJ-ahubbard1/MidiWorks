#include "TrackSet.h"

Track& TrackSet::GetTrack(ubyte channelNumber)
{
	return mTracks[channelNumber];
}

std::vector<MidiMessage> TrackSet::PlayBack(uint64_t currentTick)
{
	std::vector<MidiMessage> scheduledMessages;

	for (int t = 0; t < MidiConstants::CHANNEL_COUNT; t++)
	{
		if (iterators[t] == -1) continue;

		auto& track = GetTrack(t);

		// Process all events at or before currentTick
		while (iterators[t] != -1 &&
			iterators[t] < track.size() &&
			track[iterators[t]].tick <= currentTick)
		{
			scheduledMessages.push_back(track[iterators[t]].mm);
			iterators[t]++;
			if (iterators[t] >= track.size())
			{
				iterators[t] = -1;
				break;
			}
		}
	}
	return scheduledMessages;
}

void TrackSet::FindStart(uint64_t startTick)
{
	// we want to avoid TrackSet::messages with timestamp < startTick
	for (int t = 0; t < MidiConstants::CHANNEL_COUNT; t++)
	{
		auto& track = GetTrack(t);
		if (track.empty())
		{
			iterators[t] = -1;
			continue;
		}
		int i = 0;
		while (i < track.size() && track[i].tick < startTick)
		{
			i++;
		}
		iterators[t] = (i < track.size()) ? i : -1;
	}
}

NoteLocation TrackSet::FindNoteAt(uint64_t tick, ubyte pitch)
{
	auto allNotes = GetAllNotes();
	for (const NoteLocation& note : allNotes)
	{
		if (note.pitch == pitch && tick >= note.startTick && tick <= note.endTick)
		{
			return note;
		}
	}
	return NoteLocation{}; // Note not found
}

std::vector<NoteLocation> TrackSet::FindNotesInRegion(
	uint64_t minTick, uint64_t maxTick, ubyte minPitch, ubyte maxPitch)
{
	auto allNotes = GetAllNotes();
	std::vector<NoteLocation> result;

	for (const NoteLocation& note : allNotes)
	{
		// Check if note overlaps region
		bool pitchInRange = (note.pitch >= minPitch && note.pitch <= maxPitch);
		bool timeOverlaps = (note.startTick <= maxTick && note.endTick >= minTick);

		if (pitchInRange && timeOverlaps)
		{
			result.push_back(note);
		}
	}
	return result;
}

std::vector<NoteLocation> TrackSet::GetNotesFromTrack(const Track& track, int trackIndex)
{
	std::vector<NoteLocation> result;

	if (track.empty()) return result;

	size_t end = track.size();
	for (size_t i = 0; i < end; i++)
	{
		const TimedMidiEvent& noteOn = track[i];
		if (noteOn.mm.getEventType() != MidiEvent::NOTE_ON) continue;

		// Find corresponding note off
		for (size_t j = i + 1; j < end; j++)
		{
			const TimedMidiEvent& noteOff = track[j];
			if (noteOff.mm.getEventType() != MidiEvent::NOTE_OFF ||
				noteOff.mm.mData[1] != noteOn.mm.mData[1]) continue;

			NoteLocation note;
			note.found = true;
			note.trackIndex = trackIndex;
			note.noteOnIndex = i;
			note.noteOffIndex = j;
			note.startTick = noteOn.tick;
			note.endTick = noteOff.tick;
			note.pitch = noteOn.mm.mData[1];
			result.push_back(note);
			break;
		}
	}

	return result;
}

std::vector<NoteLocation> TrackSet::GetAllNotes()
{
	std::vector<NoteLocation> result;

	for (int trackIndex = 0; trackIndex < MidiConstants::CHANNEL_COUNT; trackIndex++)
	{
		Track& track = mTracks[trackIndex];
		std::vector<NoteLocation> trackNotes = GetNotesFromTrack(track, trackIndex);
		result.insert(result.end(), trackNotes.begin(), trackNotes.end());
	}

	return result;
}

void TrackSet::FinalizeRecording(Track& recordingBuffer)
{
	for (const auto& event : recordingBuffer)
	{
		ubyte channel = event.mm.getChannel();
		mTracks[channel].push_back(event);
	}
	Sort();
	recordingBuffer.clear();
}

// Private Methods
void TrackSet::Sort()
{
	for (auto& track : mTracks)
	{
		if (track.empty()) continue;

		std::sort(track.begin(), track.end(), [](const TimedMidiEvent& a, const TimedMidiEvent& b)
		{
			return a.tick < b.tick;
		});
	}
}
