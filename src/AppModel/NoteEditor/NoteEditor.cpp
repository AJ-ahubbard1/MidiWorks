// NoteEditor.cpp
#include "NoteEditor.h"
#include "Commands/NoteEditCommands.h"
#include "Commands/DeleteMultipleNotesCommand.h"
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

	// For now, we'll add to the first record-enabled channel
	// TODO: Create a compound command to handle multiple channels
	const MidiChannel* channel = channels[0];

	// Create note-on and note-off events
	MidiMessage noteOn = MidiMessage::NoteOn(pitch, velocity, channel->channelNumber);
	MidiMessage noteOff = MidiMessage::NoteOff(pitch, channel->channelNumber);

	TimedMidiEvent timedNoteOn{noteOn, startTick};
	TimedMidiEvent timedNoteOff{noteOff, startTick + duration - MidiConstants::NOTE_SEPARATION_TICKS};

	// Get the track and create the command
	Track& track = mTrackSet.GetTrack(channel->channelNumber);
	return std::make_unique<AddNoteCommand>(track, timedNoteOn, timedNoteOff);
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
