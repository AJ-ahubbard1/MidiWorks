#pragma once
#include <array>
#include <vector>
#include "RtMidiWrapper/MidiMessage/MidiMessage.h"
#include <algorithm>
using namespace MidiInterface;

struct TimedMidiEvent
{
	MidiMessage mm;
	uint64_t	tick;
};

using Track = std::vector<TimedMidiEvent>;

using TrackBank = std::array<Track, 15>;  // 15 tracks (channel 16 reserved for metronome)

class TrackSet
{
public:
	Track& GetTrack(ubyte channelNumber)
	{
		return mTracks[channelNumber];
	}

	/*
	void Record(TimedMidiEvent midiEvent, ubyte channel)
	{
		mTracks[channel].push_back(midiEvent);
	}
	*/

	std::vector<MidiMessage> PlayBack(uint64_t currentTick)
	{
		std::vector<MidiMessage> scheduledMessages;

		for (int t = 0; t < 15; t++)
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

	void FindStart(uint64_t startTick)
	{
		// we want to avoid messages with timestamp < startTick
		for (int t = 0; t < 15; t++)
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

	void FinalizeRecording(Track& recordingBuffer)
	{
		for (const auto& event : recordingBuffer)
		{
			ubyte channel = event.mm.getChannel();
			mTracks[channel].push_back(event);
		}
		Sort();
		recordingBuffer.clear();
	}

private:
	TrackBank mTracks;
	int iterators[15]{-1};

	void Sort()
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

};
