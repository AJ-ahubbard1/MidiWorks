// NoteEditor.cpp
#include "NoteEditor.h"
#include "Commands/NoteEditCommands.h"
#include "Commands/DeleteMultipleNotesCommand.h"
#include "Commands/MoveMultipleNotesCommand.h"
#include "Commands/QuantizeCommand.h"
#include "MidiConstants.h"

NoteEditor::NoteEditor(TrackSet& trackSet, SoundBank& soundBank)
	: mTrackSet(trackSet)
	, mSoundBank(soundBank)
{
}

std::unique_ptr<Command> NoteEditor::CreateAddNoteToRecordChannels(
	ubyte pitch,
	uint64_t startTick,
	uint64_t duration)
{
	auto channels = mSoundBank.GetRecordEnabledChannels();
	if (channels.empty()) return nullptr;

	// @TODO velocity should be based on recording settings, separate from preview settings
	ubyte velocity = mSoundBank.GetPreviewVelocity();

	// Get all record-enabled channels and convert to track indices
	std::vector<int> targetTracks;
	for (const MidiChannel* channel : channels)
	{
		targetTracks.push_back(static_cast<int>(channel->channelNumber));
	}

	// Create command to add note to all record-enabled tracks
	return std::make_unique<AddNoteCommand>(mTrackSet, targetTracks, pitch, velocity, startTick, duration);
}

std::unique_ptr<Command> NoteEditor::CreateDeleteNote(const NoteLocation& note)
{
	if (!note.found) return nullptr;

	Track& track = mTrackSet.GetTrack(note.trackIndex);
	return std::make_unique<DeleteNoteCommand>(
		track,
		note.noteOnIndex,
		note.noteOffIndex
	);
}

std::unique_ptr<Command> NoteEditor::CreateDeleteNotes(const std::vector<NoteLocation>& notes)
{
	if (notes.empty()) return nullptr;

	// Build deletion list
	std::vector<DeleteMultipleNotesCommand::NoteToDelete> notesToDelete;
	notesToDelete.reserve(notes.size());

	for (const auto& note : notes)
	{
		Track& track = mTrackSet.GetTrack(note.trackIndex);

		DeleteMultipleNotesCommand::NoteToDelete noteData;
		noteData.trackIndex = note.trackIndex;
		noteData.noteOnIndex = note.noteOnIndex;
		noteData.noteOffIndex = note.noteOffIndex;
		noteData.noteOn = track[note.noteOnIndex];
		noteData.noteOff = track[note.noteOffIndex];

		notesToDelete.push_back(noteData);
	}

	// Create single batch command
	return std::make_unique<DeleteMultipleNotesCommand>(mTrackSet, notesToDelete);
}

std::unique_ptr<Command> NoteEditor::CreateMoveNote(
	const NoteLocation& note,
	uint64_t newStartTick,
	ubyte newPitch)
{
	if (!note.found) return nullptr;

	// Only create command if position actually changed
	if (newStartTick == note.startTick && newPitch == note.pitch)
		return nullptr;

	Track& track = mTrackSet.GetTrack(note.trackIndex);

	return std::make_unique<MoveNoteCommand>(
		track,
		note.noteOnIndex,
		note.noteOffIndex,
		newStartTick,
		newPitch
	);
}

std::unique_ptr<Command> NoteEditor::CreateResizeNote(
	const NoteLocation& note,
	uint64_t newDuration)
{
	if (!note.found) return nullptr;

	uint64_t oldDuration = note.endTick - note.startTick;

	// Only create command if duration actually changed
	if (newDuration == oldDuration)
		return nullptr;

	Track& track = mTrackSet.GetTrack(note.trackIndex);

	return std::make_unique<ResizeNoteCommand>(
		track,
		note.noteOnIndex,
		note.noteOffIndex,
		newDuration
	);
}

std::unique_ptr<Command> NoteEditor::CreateEditNoteVelocity(
	const NoteLocation& note,
	ubyte newVelocity)
{
	if (!note.found) return nullptr;

	// Only create command if velocity actually changed
	if (newVelocity == note.velocity)
		return nullptr;

	Track& track = mTrackSet.GetTrack(note.trackIndex);

	return std::make_unique<EditNoteVelocityCommand>(
		track,
		note.noteOnIndex,
		newVelocity
	);
}

void NoteEditor::SetNoteMovePreview(const NoteLocation& note, uint64_t newStartTick, ubyte newPitch)
{
	mNoteEditPreview.isActive = true;
	mNoteEditPreview.originalNote = note;
	mNoteEditPreview.previewStartTick = newStartTick;
	mNoteEditPreview.previewEndTick = newStartTick + (note.endTick - note.startTick);
	mNoteEditPreview.previewPitch = newPitch;
}

void NoteEditor::SetNoteResizePreview(const NoteLocation& note, uint64_t newEndTick)
{
	mNoteEditPreview.isActive = true;
	mNoteEditPreview.originalNote = note;
	mNoteEditPreview.previewStartTick = note.startTick;
	mNoteEditPreview.previewEndTick = newEndTick;
	mNoteEditPreview.previewPitch = note.pitch;
}

void NoteEditor::ClearNoteEditPreview()
{
	mNoteEditPreview.isActive = false;
	mMultiNoteEditPreview.isActive = false;
}

const NoteEditor::NoteEditPreview& NoteEditor::GetNoteEditPreview() const
{
	return mNoteEditPreview;
}

bool NoteEditor::HasNoteEditPreview() const
{
	return mNoteEditPreview.isActive;
}

std::vector<std::unique_ptr<Command>> NoteEditor::CreateQuantizeAllTracks(uint64_t gridSize)
{
	std::vector<std::unique_ptr<Command>> commands;

	for (int i = 0; i < MidiConstants::CHANNEL_COUNT; i++)
	{
		Track& track = mTrackSet.GetTrack(i);
		if (!track.empty())
		{
			commands.push_back(std::make_unique<QuantizeCommand>(track, gridSize));
		}
	}

	return commands;
}

std::unique_ptr<Command> NoteEditor::CreateMoveMultipleNotes(
	const std::vector<NoteLocation>& notes,
	int64_t tickDelta,
	int pitchDelta)
{
	if (notes.empty()) return nullptr;

	// Don't create command if no movement
	if (tickDelta == 0 && pitchDelta == 0)
		return nullptr;

	// Build move list
	std::vector<MoveMultipleNotesCommand::NoteToMove> notesToMove;
	notesToMove.reserve(notes.size());

	for (const auto& note : notes)
	{
		Track& track = mTrackSet.GetTrack(note.trackIndex);

		MoveMultipleNotesCommand::NoteToMove noteData;
		noteData.trackIndex = note.trackIndex;
		noteData.noteOnIndex = note.noteOnIndex;
		noteData.noteOffIndex = note.noteOffIndex;
		noteData.originalStartTick = note.startTick;
		noteData.originalPitch = note.pitch;
		noteData.duration = note.endTick - note.startTick;

		notesToMove.push_back(noteData);
	}

	// Create single batch command
	return std::make_unique<MoveMultipleNotesCommand>(mTrackSet, notesToMove, tickDelta, pitchDelta);
}

void NoteEditor::SetMultipleNotesMovePreview(const std::vector<NoteLocation>& notes, int64_t tickDelta, int pitchDelta)
{
	mMultiNoteEditPreview.isActive = true;
	mMultiNoteEditPreview.originalNotes = notes;
	mMultiNoteEditPreview.tickDelta = tickDelta;
	mMultiNoteEditPreview.pitchDelta = pitchDelta;
}

const NoteEditor::MultiNoteEditPreview& NoteEditor::GetMultiNoteEditPreview() const
{
	return mMultiNoteEditPreview;
}

bool NoteEditor::HasMultiNoteEditPreview() const
{
	return mMultiNoteEditPreview.isActive;
}

void NoteEditor::SetNoteAddPreview(ubyte pitch, uint64_t tick, uint64_t snappedTick, uint64_t duration)
{
	uint64_t endTick = snappedTick + duration - MidiConstants::NOTE_SEPARATION_TICKS;

	// Collision check: Check for conflicts in ALL record-enabled channels
	// (since note will be added to all of them)
	auto channels = mSoundBank.GetRecordEnabledChannels();
	for (const MidiChannel* channel : channels)
	{
		int trackIndex = static_cast<int>(channel->channelNumber);
		auto conflicts = mTrackSet.FindNotesInRegion(snappedTick, endTick, pitch, pitch, trackIndex);
		if (!conflicts.empty())
		{
			return;  // Collision detected in this channel, don't update preview
		}
	}

	// Handle audio preview - switch pitch if changed
	bool pitchChanged = !mNoteAddPreview.isActive || (pitch != mNoteAddPreview.pitch);
	if (pitchChanged)
	{
		if (mNoteAddPreview.isActive)
		{
			mSoundBank.StopPreviewNote();
		}
		mSoundBank.PlayPreviewNote(pitch);
	}

	// Store preview state (using unsnapped tick for visual display)
	mNoteAddPreview.isActive = true;
	mNoteAddPreview.pitch = pitch;
	mNoteAddPreview.tick = tick;
}

void NoteEditor::ClearNoteAddPreview()
{
	if (mNoteAddPreview.isActive)
	{
		mSoundBank.StopPreviewNote();
		mNoteAddPreview.isActive = false;
	}
}

const NoteEditor::NoteAddPreview& NoteEditor::GetNoteAddPreview() const
{
	return mNoteAddPreview;
}

bool NoteEditor::HasNoteAddPreview() const
{
	return mNoteAddPreview.isActive;
}
