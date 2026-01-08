#include "TrackSet.h"
#include <set>

Track& TrackSet::GetTrack(ubyte channelNumber)
{
	return mTracks[channelNumber];
}

bool TrackSet::IsTrackEmpty(ubyte channelNumber)
{
	return GetTrack(channelNumber).empty();
}

std::vector<MidiMessage> TrackSet::PlayBack(uint64_t currentTick)
{
	std::vector<MidiMessage> scheduledMessages;

	for (int t = 0; t < MidiConstants::CHANNEL_COUNT; t++)
	{
		// Track is empty or at the end
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

NoteLocation TrackSet::FindNoteAt(uint64_t tick, ubyte pitch) const
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

NoteLocation TrackSet::FindNoteInTrack(int trackIndex, uint64_t startTick, uint64_t endTick, ubyte pitch) const
{
	const Track& track = mTracks[trackIndex];
	auto trackNotes = GetNotesFromTrack(track, trackIndex);

	for (const NoteLocation& note : trackNotes)
	{
		if (note.pitch == pitch && note.startTick == startTick && note.endTick == endTick)
		{
			return note;
		}
	}
	return NoteLocation{}; // Note not found
}

std::vector<NoteLocation> TrackSet::FindNotesInRegion(
	uint64_t minTick, uint64_t maxTick, ubyte minPitch, ubyte maxPitch, int trackIndex) const
{
	std::vector<NoteLocation> allNotes;

	// If trackIndex specified, only search that track. Otherwise search all tracks.
	if (trackIndex >= 0 && trackIndex < MidiConstants::CHANNEL_COUNT)
	{
		allNotes = GetNotesFromTrack(mTracks[trackIndex], trackIndex);
	}
	else
	{
		allNotes = GetAllNotes();
	}

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
			// Skip if not the corresponding note off
			if (!noteOff.mm.isNoteOff() || noteOff.mm.getPitch() != noteOn.mm.getPitch()) continue;

			NoteLocation note;
			note.found = true;
			note.trackIndex = trackIndex;
			note.noteOnIndex = i;
			note.noteOffIndex = j;
			note.startTick = noteOn.tick;
			note.endTick = noteOff.tick;
			note.pitch = noteOn.mm.getPitch();
			note.velocity = noteOn.mm.getVelocity();
			result.push_back(note);
			break;
		}
	}

	return result;
}

std::vector<NoteLocation> TrackSet::GetAllNotes() const
{
	std::vector<NoteLocation> result;

	for (int trackIndex = 0; trackIndex < MidiConstants::CHANNEL_COUNT; trackIndex++)
	{
		const Track& track = mTracks[trackIndex];
		std::vector<NoteLocation> trackNotes = GetNotesFromTrack(track, trackIndex);
		result.insert(result.end(), trackNotes.begin(), trackNotes.end());
	}

	return result;
}

std::vector<TimedMidiEvent> TrackSet::GetAllTimedMidiEvents()
{
	std::vector<TimedMidiEvent> result;

	for (int trackIndex = 0; trackIndex < MidiConstants::CHANNEL_COUNT; trackIndex++)
	{
		const Track& track = mTracks[trackIndex];
		result.insert(result.end(), track.begin(), track.end());
	}

	return result;
}

void TrackSet::SortTrack(Track& track)
{
	if (track.empty()) return;

	std::sort(track.begin(), track.end(), [](const TimedMidiEvent& a, const TimedMidiEvent& b)
	{
		return a.tick < b.tick;
	});
}

void TrackSet::SeparateOverlappingNotes(Track& buffer)
{
	if (buffer.size() < 2) return;

	// Sort by tick to ensure chronological order
	SortTrack(buffer);

	std::set<size_t> movedIndices; // Track which NoteOffs we've already moved

	// Process each NoteOn to find bad pairs
	for (size_t i = 0; i < buffer.size(); i++)
	{
		auto& eventI = buffer[i].mm;
		if (!eventI.isNoteOn()) continue;

		// Look for the next event with the same channel and pitch
		for (size_t j = i + 1; j < buffer.size(); j++)
		{
			auto& eventJ = buffer[j].mm;
			if (eventI.getPitch() != eventJ.getPitch() ||
				eventI.getChannel() != eventJ.getChannel()) continue;

			// Found next event with same pitch
			if (eventJ.isNoteOn())
			{
				// Bad pair detected! Two NoteOns in a row
				// Find the first NoteOff with this pitch after j that hasn't been moved yet
				for (size_t k = j + 1; k < buffer.size(); k++)
				{
					auto& eventK = buffer[k].mm;
					if (eventK.isNoteOff() && eventK.getPitch() == eventI.getPitch() && 
						movedIndices.find(k) == movedIndices.end())
					{
						// Move this NoteOff to prevent overlap with the second NoteOn
						buffer[k].tick = buffer[j].tick - MidiConstants::NOTE_SEPARATION_TICKS;
						movedIndices.insert(k);
						break;
					}
				}
			}
			// Whether it's a NoteOn or NoteOff, we've found the next event for this pitch
			break;
		}
	}

	// Re-sort after moving NoteOffs
	SortTrack(buffer);
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
		SortTrack(track);
	}
}
