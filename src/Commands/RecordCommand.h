// RecordCommand.h
#pragma once
#include "Command.h"
#include "AppModel/TrackSet/TrackSet.h"
#include <algorithm>
#include <vector>
using namespace MidiInterface;

/// Handles MIDI recording with undo/redo support.
///
/// Responsibilities:
/// - Store all recorded notes from recording buffer
/// - Enable undo/redo of entire recording takes as single operation
/// - Distribute recorded events to appropriate tracks by channel
///
/// When recording stops, all notes become a single undoable operation.
/// This allows quick iteration: record, listen, Ctrl+Z if bad, try again.
///
/// Usage:
///   auto cmd = std::make_unique<RecordCommand>(trackSet, recordingBuffer);
///   appModel.ExecuteCommand(std::move(cmd));
class RecordCommand : public Command
{
public:
	/// Construct a record command with all recorded notes
	/// @param recordedNotes All notes from the recording buffer
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
			TrackSet::SortTrack(track);
		}
	}

	void Undo() override
	{
		// Remove all recorded notes from their respective tracks
		for (const auto& event : mRecordedNotes)
		{
			ubyte channel = event.mm.getChannel();

			// Skip if channel is out of bounds (should not happen, but safety check)
			if (channel >= 15) continue;

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
