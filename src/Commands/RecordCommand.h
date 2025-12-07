#pragma once
#include "Command.h"
#include "AppModel/TrackSet.h"
#include <algorithm>
#include <vector>
using namespace MidiInterface;

/// <summary>
/// Command to handle recording MIDI data.
/// Stores all recorded notes and enables undo/redo of entire recording takes.
///
/// When recording stops, all notes in the recording buffer become a single
/// undoable operation. This allows users to quickly undo bad takes and try again.
///
/// Example workflow:
/// 1. Record phrase
/// 2. Stop and listen
/// 3. If bad: Ctrl+Z removes entire recording
/// 4. Record again
/// 5. If still bad: Ctrl+Z again
/// 6. Keep iterating until satisfied
/// </summary>
class RecordCommand : public Command
{
public:
	/// <summary>
	/// Construct a record command with all recorded notes.
	/// </summary>
	/// <param name="trackSet">Reference to the TrackSet containing all tracks</param>
	/// <param name="recordedNotes">All notes from the recording buffer</param>
	RecordCommand(TrackSet& trackSet, const Track& recordedNotes)
		: mTrackSet(trackSet)
		, mRecordedNotes(recordedNotes)
	{
	}

	void Execute() override
	{
		// Add all recorded notes to their respective tracks
		for (const auto& event : mRecordedNotes)
		{
			ubyte channel = event.mm.getChannel();
			mTrackSet.GetTrack(channel).push_back(event);
		}

		// Sort all affected tracks by tick to maintain chronological order
		for (int i = 0; i < 15; i++)
		{
			auto& track = mTrackSet.GetTrack(i);
			std::sort(track.begin(), track.end(),
				[](const TimedMidiEvent& a, const TimedMidiEvent& b) {
					return a.tick < b.tick;
				});
		}
	}

	void Undo() override
	{
		// Remove all recorded notes from their respective tracks
		for (const auto& event : mRecordedNotes)
		{
			ubyte channel = event.mm.getChannel();
			auto& track = mTrackSet.GetTrack(channel);

			// Find and remove this event by matching tick and MIDI data
			auto it = std::remove_if(track.begin(), track.end(),
				[&](const TimedMidiEvent& e) {
					return e.tick == event.tick &&
						   e.mm.mData[0] == event.mm.mData[0] &&
						   e.mm.mData[1] == event.mm.mData[1] &&
						   e.mm.mData[2] == event.mm.mData[2];
				});

			track.erase(it, track.end());
		}
	}

	std::string GetDescription() const override
	{
		// Count note-on events (each note-on represents one note played)
		int noteCount = 0;
		for (const auto& event : mRecordedNotes)
		{
			// Note-on messages have status byte 144-159 (0x90-0x9F)
			ubyte status = event.mm.mData[0] & 0xF0;
			if (status == 0x90 && event.mm.mData[2] > 0) // Note-on with velocity > 0
			{
				noteCount++;
			}
		}

		return "Record " + std::to_string(noteCount) + " note" + (noteCount != 1 ? "s" : "");
	}

private:
	TrackSet& mTrackSet;
	Track mRecordedNotes;  // Copy of all recorded notes for undo purposes
};
