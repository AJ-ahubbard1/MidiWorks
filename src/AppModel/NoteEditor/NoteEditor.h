// NoteEditor.h
#pragma once
#include <memory>
#include <vector>
#include "AppModel/TrackSet/TrackSet.h"
#include "AppModel/SoundBank/SoundBank.h"
#include "Commands/Command.h"

class NoteEditor
{
public:
	NoteEditor(TrackSet& trackSet, SoundBank& soundBank);

	// Note creation - handles business logic, returns command to execute
	std::unique_ptr<Command> CreateAddNoteToRecordChannels(
		ubyte pitch,
		uint64_t startTick,
		uint64_t duration
	);

	// Note deletion
	std::unique_ptr<Command> CreateDeleteNote(const NoteLocation& note);
	std::unique_ptr<Command> CreateDeleteNotes(const std::vector<NoteLocation>& notes);

	// Note modification
	std::unique_ptr<Command> CreateMoveNote(
		const NoteLocation& note,
		uint64_t newStartTick,
		ubyte newPitch
	);
	std::unique_ptr<Command> CreateResizeNote(
		const NoteLocation& note,
		uint64_t newDuration
	);

	// Quantization
	std::vector<std::unique_ptr<Command>> CreateQuantizeAllTracks(uint64_t gridSize);

	// Note edit preview (for drag operations)
	struct NoteEditPreview
	{
		bool isActive = false;
		NoteLocation originalNote;
		uint64_t previewStartTick = 0;
		uint64_t previewEndTick = 0;
		ubyte previewPitch = 0;
	};

	void SetNoteMovePreview(const NoteLocation& note, uint64_t newStartTick, ubyte newPitch);
	void SetNoteResizePreview(const NoteLocation& note, uint64_t newEndTick);
	void ClearNoteEditPreview();
	const NoteEditPreview& GetNoteEditPreview() const;
	bool HasNoteEditPreview() const;

private:
	TrackSet& mTrackSet;
	SoundBank& mSoundBank;
	NoteEditPreview mNoteEditPreview;
};
