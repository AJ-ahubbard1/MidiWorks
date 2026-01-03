#pragma once
#include "Command.h"
#include "AppModel/TrackSet/TrackSet.h"
#include <algorithm>
#include <vector>
#include <set>
using namespace MidiInterface;

/// <summary>
/// Command to move multiple notes by applying a tick and pitch delta.
/// Stores original positions to enable undo.
/// </summary>
class MoveMultipleNotesCommand : public Command
{
public:
	struct NoteToMove {
		int trackIndex = 0;
		size_t noteOnIndex = 0;
		size_t noteOffIndex = 0;
		uint64_t originalStartTick = 0;
		ubyte originalPitch = 0;
		uint64_t duration = 0;
	};

	MoveMultipleNotesCommand(TrackSet& trackSet, const std::vector<NoteToMove>& notesToMove,
		int64_t tickDelta, int pitchDelta)
		: mTrackSet(trackSet)
		, mNotesToMove(notesToMove)
		, mTickDelta(tickDelta)
		, mPitchDelta(pitchDelta)
	{
	}

	void Execute() override
	{
		for (const auto& noteInfo : mNotesToMove)
		{
			Track& track = mTrackSet.GetTrack(noteInfo.trackIndex);

			// Calculate new position with delta
			int64_t newTickSigned = static_cast<int64_t>(noteInfo.originalStartTick) + mTickDelta;
			uint64_t newTick = (newTickSigned < 0) ? 0 : static_cast<uint64_t>(newTickSigned);

			int newPitchSigned = static_cast<int>(noteInfo.originalPitch) + mPitchDelta;
			ubyte newPitch = std::clamp(newPitchSigned, 0, MidiConstants::MAX_MIDI_NOTE);

			// Update note-on event
			if (noteInfo.noteOnIndex < track.size())
			{
				track[noteInfo.noteOnIndex].tick = newTick;
				track[noteInfo.noteOnIndex].mm.mData[1] = newPitch;  // Set pitch
			}

			// Update note-off event (maintain duration)
			if (noteInfo.noteOffIndex < track.size())
			{
				track[noteInfo.noteOffIndex].tick = newTick + noteInfo.duration;
				track[noteInfo.noteOffIndex].mm.mData[1] = newPitch;  // Set pitch
			}
		}

		// Re-sort all affected tracks
		std::set<int> affectedTracks;
		for (const auto& noteInfo : mNotesToMove)
		{
			affectedTracks.insert(noteInfo.trackIndex);
		}

		for (int trackIndex : affectedTracks)
		{
			Track& track = mTrackSet.GetTrack(trackIndex);
			TrackSet::SortTrack(track);
		}
	}

	void Undo() override
	{
		// Restore original positions
		for (const auto& noteInfo : mNotesToMove)
		{
			Track& track = mTrackSet.GetTrack(noteInfo.trackIndex);

			// Find the moved note events (they may have moved in the sorted array)
			// We need to search for them by their new position
			int64_t newTickSigned = static_cast<int64_t>(noteInfo.originalStartTick) + mTickDelta;
			uint64_t newTick = (newTickSigned < 0) ? 0 : static_cast<uint64_t>(newTickSigned);

			int newPitchSigned = static_cast<int>(noteInfo.originalPitch) + mPitchDelta;
			ubyte newPitch = std::clamp(newPitchSigned, 0, MidiConstants::MAX_MIDI_NOTE);

			// Search for note-on with new position
			for (size_t i = 0; i < track.size(); i++)
			{
				if (track[i].tick == newTick &&
					track[i].mm.isNoteOn() &&
					track[i].mm.getPitch() == newPitch)
				{
					track[i].tick = noteInfo.originalStartTick;
					track[i].mm.mData[1] = noteInfo.originalPitch;
					break;
				}
			}

			// Search for note-off with new position
			uint64_t newEndTick = newTick + noteInfo.duration;
			for (size_t i = 0; i < track.size(); i++)
			{
				if (track[i].tick == newEndTick &&
					!track[i].mm.isNoteOn() &&
					track[i].mm.getPitch() == newPitch)
				{
					track[i].tick = noteInfo.originalStartTick + noteInfo.duration;
					track[i].mm.mData[1] = noteInfo.originalPitch;
					break;
				}
			}
		}

		// Re-sort all affected tracks
		std::set<int> affectedTracks;
		for (const auto& noteInfo : mNotesToMove)
		{
			affectedTracks.insert(noteInfo.trackIndex);
		}

		for (int trackIndex : affectedTracks)
		{
			Track& track = mTrackSet.GetTrack(trackIndex);
			TrackSet::SortTrack(track);
		}
	}

	std::string GetDescription() const override
	{
		int noteCount = static_cast<int>(mNotesToMove.size());
		return "Move " + std::to_string(noteCount) + " note" + (noteCount != 1 ? "s" : "");
	}

private:
	TrackSet& mTrackSet;
	std::vector<NoteToMove> mNotesToMove;
	int64_t mTickDelta;
	int mPitchDelta;
};
