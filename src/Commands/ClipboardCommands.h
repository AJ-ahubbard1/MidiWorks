//==============================================================================
// ClipboardCommands.h
// Commands for clipboard operations (paste, paste to tracks, etc.)
//==============================================================================

#pragma once
#include "Command.h"
#include "AppModel/TrackSet/TrackSet.h"
#include "AppModel/Clipboard/Clipboard.h"
#include <algorithm>
#include <vector>
#include <set>
using namespace MidiInterface;

//==============================================================================
// PasteCommand
//==============================================================================

/// <summary>
/// Command to paste clipboard notes into tracks with overdub behavior.
/// Takes clipboard notes (with relative timing) and pastes them at a target tick position.
/// Uses SeparateOverlappingNotes to handle collisions (like loop recording overdub).
/// Stores complete track snapshots for undo to handle all modifications.
/// </summary>
class PasteCommand : public Command
{
public:
	/// <summary>
	/// Construct a paste command.
	/// </summary>
	/// <param name="trackSet">Reference to the track set</param>
	/// <param name="clipboardNotes">Notes to paste (from clipboard)</param>
	/// <param name="pasteTick">Tick position where notes should be pasted</param>
	PasteCommand(TrackSet& trackSet, const std::vector<Clipboard::ClipboardNote>& clipboardNotes, uint64_t pasteTick)
		: mTrackSet(trackSet)
		, mClipboardNotes(clipboardNotes)
		, mPasteTick(pasteTick)
	{
	}

	void Execute() override
	{
		// Clear any previous paste data (for redo)
		mTrackSnapshots.clear();

		// Determine which tracks will be affected
		std::set<int> affectedTracks;
		for (const auto& clipNote : mClipboardNotes)
		{
			affectedTracks.insert(clipNote.trackIndex);
		}

		// Store complete snapshot of each affected track BEFORE pasting
		for (int trackIndex : affectedTracks)
		{
			TrackSnapshot snapshot;
			snapshot.trackIndex = trackIndex;
			snapshot.originalTrack = mTrackSet.GetTrack(trackIndex);  // Copy entire track
			mTrackSnapshots.push_back(snapshot);
		}

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
		}

		// Separate overlapping notes (like loop recording overdub)
		for (int trackIndex : affectedTracks)
		{
			Track& track = mTrackSet.GetTrack(trackIndex);
			TrackSet::SeparateOverlappingNotes(track);
		}
	}

	void Undo() override
	{
		// Restore each affected track to its pre-paste state
		for (const auto& snapshot : mTrackSnapshots)
		{
			mTrackSet.GetTrack(snapshot.trackIndex) = snapshot.originalTrack;
		}
	}

	std::string GetDescription() const override
	{
		int noteCount = static_cast<int>(mClipboardNotes.size());
		return "Paste " + std::to_string(noteCount) + " note" + (noteCount != 1 ? "s" : "");
	}

private:
	/// <summary>
	/// Stores a complete snapshot of a track for undo purposes.
	/// </summary>
	struct TrackSnapshot {
		int trackIndex = 0;     // Which track this snapshot is for
		Track originalTrack;    // Complete copy of track before paste
	};

	TrackSet& mTrackSet;
	std::vector<Clipboard::ClipboardNote> mClipboardNotes;
	uint64_t mPasteTick;
	std::vector<TrackSnapshot> mTrackSnapshots;  // Track snapshots for undo
};

//==============================================================================
// PasteToTracksCommand
//==============================================================================

/// <summary>
/// Command to paste clipboard notes to specific target tracks (instead of original tracks).
/// Used for cross-track pasting, typically to record-enabled channels.
/// Each clipboard note is pasted to ALL target tracks, allowing easy layering/doubling.
/// Uses SeparateOverlappingNotes to handle collisions (like loop recording overdub).
/// </summary>
class PasteToTracksCommand : public Command
{
public:
	/// <summary>
	/// Construct a paste-to-tracks command.
	/// </summary>
	/// <param name="trackSet">Reference to the track set</param>
	/// <param name="clipboardNotes">Notes to paste (from clipboard)</param>
	/// <param name="pasteTick">Tick position where notes should be pasted</param>
	/// <param name="targetTracks">Track indices to paste to (overrides clipboard trackIndex)</param>
	PasteToTracksCommand(
		TrackSet& trackSet,
		const std::vector<Clipboard::ClipboardNote>& clipboardNotes,
		uint64_t pasteTick,
		const std::vector<int>& targetTracks)
		: mTrackSet(trackSet)
		, mClipboardNotes(clipboardNotes)
		, mPasteTick(pasteTick)
		, mTargetTracks(targetTracks)
	{
	}

	void Execute() override
	{
		// Clear any previous paste data (for redo)
		mTrackSnapshots.clear();

		// Store complete snapshot of each target track BEFORE pasting
		for (int trackIndex : mTargetTracks)
		{
			TrackSnapshot snapshot;
			snapshot.trackIndex = trackIndex;
			snapshot.originalTrack = mTrackSet.GetTrack(trackIndex);  // Copy entire track
			mTrackSnapshots.push_back(snapshot);
		}

		// Paste each clipboard note to ALL target tracks
		for (int targetTrack : mTargetTracks)
		{
			for (const auto& clipNote : mClipboardNotes)
			{
				Track& track = mTrackSet.GetTrack(targetTrack);

				// Calculate absolute tick positions
				uint64_t noteOnTick = mPasteTick + clipNote.relativeStartTick;
				uint64_t noteOffTick = noteOnTick + clipNote.duration;

				// Create note on event (use target track's channel)
				TimedMidiEvent noteOn;
				noteOn.tick = noteOnTick;
				noteOn.mm = MidiMessage::NoteOn(clipNote.pitch, clipNote.velocity, targetTrack);

				// Create note off event (use target track's channel)
				TimedMidiEvent noteOff;
				noteOff.tick = noteOffTick;
				noteOff.mm = MidiMessage::NoteOff(clipNote.pitch, targetTrack);

				// Add to track
				track.push_back(noteOn);
				track.push_back(noteOff);
			}
		}

		// Separate overlapping notes on each target track (like loop recording overdub)
		for (int trackIndex : mTargetTracks)
		{
			Track& track = mTrackSet.GetTrack(trackIndex);
			TrackSet::SeparateOverlappingNotes(track);
		}
	}

	void Undo() override
	{
		// Restore each affected track to its pre-paste state
		for (const auto& snapshot : mTrackSnapshots)
		{
			mTrackSet.GetTrack(snapshot.trackIndex) = snapshot.originalTrack;
		}
	}

	std::string GetDescription() const override
	{
		int noteCount = static_cast<int>(mClipboardNotes.size());
		int trackCount = static_cast<int>(mTargetTracks.size());

		std::string desc = "Paste " + std::to_string(noteCount) + " note" + (noteCount != 1 ? "s" : "");
		desc += " to " + std::to_string(trackCount) + " track" + (trackCount != 1 ? "s" : "");
		return desc;
	}

private:
	/// <summary>
	/// Stores a complete snapshot of a track for undo purposes.
	/// </summary>
	struct TrackSnapshot {
		int trackIndex = 0;     // Which track this snapshot is for
		Track originalTrack;    // Complete copy of track before paste
	};

	TrackSet& mTrackSet;
	std::vector<Clipboard::ClipboardNote> mClipboardNotes;
	uint64_t mPasteTick;
	std::vector<int> mTargetTracks;              // Target tracks to paste to
	std::vector<TrackSnapshot> mTrackSnapshots;  // Track snapshots for undo
};
