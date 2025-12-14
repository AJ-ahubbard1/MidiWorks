#pragma once
#include "Command.h"
#include "AppModel/TrackSet/TrackSet.h"
#include "AppModel/AppModel.h"
#include <algorithm>
#include <vector>
using namespace MidiInterface;

/// <summary>
/// Command to paste clipboard notes into tracks.
/// Takes clipboard notes (with relative timing) and pastes them at a target tick position.
/// Stores pasted note indices for undo.
/// </summary>
class PasteCommand : public Command
{
public:
	/// <summary>
	/// Construct a paste command.
	/// </summary>
	/// <param name="trackSet">Reference to the track set</param>
	/// <param name="clipboardNotes">Notes to paste (from AppModel clipboard)</param>
	/// <param name="pasteTick">Tick position where notes should be pasted</param>
	PasteCommand(TrackSet& trackSet, const std::vector<AppModel::ClipboardNote>& clipboardNotes, uint64_t pasteTick)
		: mTrackSet(trackSet)
		, mClipboardNotes(clipboardNotes)
		, mPasteTick(pasteTick)
	{
	}

	void Execute() override
	{
		// Clear any previous paste data (for redo)
		mPastedNotes.clear();

		// Add each clipboard note to its track
		for (const auto& clipNote : mClipboardNotes)
		{
			Track& track = mTrackSet.GetTrack(clipNote.trackIndex);

			// Calculate absolute tick positions
			uint64_t noteOnTick = mPasteTick + clipNote.relativeStartTick;
			uint64_t noteOffTick = noteOnTick + clipNote.duration;

			// Create note on event
			TimedMidiEvent noteOn;
			noteOn.tick = noteOnTick;
			noteOn.mm = MidiMessage::NoteOn(clipNote.pitch, clipNote.velocity, clipNote.trackIndex);

			// Create note off event
			TimedMidiEvent noteOff;
			noteOff.tick = noteOffTick;
			noteOff.mm = MidiMessage::NoteOff(clipNote.pitch, clipNote.trackIndex);

			// Add to track
			track.push_back(noteOn);
			track.push_back(noteOff);

			// Re-sort track by tick to maintain chronological order
			std::sort(track.begin(), track.end(),
				[](const TimedMidiEvent& a, const TimedMidiEvent& b) {
					return a.tick < b.tick;
				});

			// Find indices of the added notes (for undo)
			size_t noteOnIndex = 0;
			size_t noteOffIndex = 0;
			for (size_t i = 0; i < track.size(); i++)
			{
				const auto& event = track[i];
				if (event.tick == noteOnTick && event.mm.isNoteOn() && event.mm.getPitch() == clipNote.pitch)
				{
					noteOnIndex = i;
				}
				if (event.tick == noteOffTick && event.mm.isNoteOff() && event.mm.getPitch() == clipNote.pitch)
				{
					noteOffIndex = i;
					break;
				}
			}

			// Store pasted note info for undo
			PastedNoteInfo pastedInfo;
			pastedInfo.trackIndex = clipNote.trackIndex;
			pastedInfo.noteOnIndex = noteOnIndex;
			pastedInfo.noteOffIndex = noteOffIndex;
			pastedInfo.noteOn = noteOn;
			pastedInfo.noteOff = noteOff;
			mPastedNotes.push_back(pastedInfo);
		}
	}

	void Undo() override
	{
		// Remove pasted notes in reverse order (to maintain indices)
		// Group by track and sort descending
		std::vector<PastedNoteInfo> sortedNotes = mPastedNotes;
		std::sort(sortedNotes.begin(), sortedNotes.end(),
			[](const PastedNoteInfo& a, const PastedNoteInfo& b) {
				if (a.trackIndex != b.trackIndex)
					return a.trackIndex > b.trackIndex;
				return a.noteOnIndex > b.noteOnIndex;
			});

		for (const auto& pastedInfo : sortedNotes)
		{
			Track& track = mTrackSet.GetTrack(pastedInfo.trackIndex);

			// Delete in reverse order (noteOff first, then noteOn) to maintain indices
			if (pastedInfo.noteOffIndex < track.size())
			{
				track.erase(track.begin() + pastedInfo.noteOffIndex);
			}
			if (pastedInfo.noteOnIndex < track.size())
			{
				track.erase(track.begin() + pastedInfo.noteOnIndex);
			}
		}
	}

	std::string GetDescription() const override
	{
		int noteCount = static_cast<int>(mClipboardNotes.size());
		return "Paste " + std::to_string(noteCount) + " note" + (noteCount != 1 ? "s" : "");
	}

private:
	struct PastedNoteInfo {
		int trackIndex = 0;
		size_t noteOnIndex = 0;
		size_t noteOffIndex = 0;
		TimedMidiEvent noteOn;
		TimedMidiEvent noteOff;
	};

	TrackSet& mTrackSet;
	std::vector<AppModel::ClipboardNote> mClipboardNotes;
	uint64_t mPasteTick;
	std::vector<PastedNoteInfo> mPastedNotes;  // Indices of pasted notes for undo
};
