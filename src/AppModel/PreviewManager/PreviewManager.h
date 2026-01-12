// PreviewManager.h
#pragma once
#include <memory>
#include <vector>
#include "AppModel/TrackSet/TrackSet.h"
#include "AppModel/SoundBank/SoundBank.h"
#include "Commands/Command.h"

class PreviewManager
{
public:
	PreviewManager(TrackSet& trackSet, SoundBank& soundBank)
		: mTrackSet(trackSet)
		, mSoundBank(soundBank)
	{
	}

	// Note edit preview (for drag operations)
	struct NoteEditPreview
	{
		bool isActive = false;
		NoteLocation originalNote;
		uint64_t previewStartTick = 0;
		uint64_t previewEndTick = 0;
		ubyte previewPitch = 0;
	};

	struct MultiNoteEditPreview
	{
		bool isActive = false;
		std::vector<NoteLocation> originalNotes;
		int64_t tickDelta = 0;
		int pitchDelta = 0;
	};

	void SetNoteMovePreview(const NoteLocation& note, uint64_t newStartTick, ubyte newPitch);
	void SetNoteResizePreview(const NoteLocation& note, uint64_t newEndTick);
	void SetMultipleNotesMovePreview(const std::vector<NoteLocation>& notes, int64_t tickDelta, int pitchDelta);
	void ClearNoteEditPreview();
	const NoteEditPreview& GetNoteEditPreview() const;
	const MultiNoteEditPreview& GetMultiNoteEditPreview() const;
	bool HasNoteEditPreview() const;
	bool HasMultiNoteEditPreview() const;

	// Note add preview (for mouse-based note creation)
	struct NoteAddPreview
	{
		bool isActive = false;
		ubyte pitch = 0;
		uint64_t tick = 0;  // Unsnapped tick for visual display
	};

	void SetNoteAddPreview(ubyte pitch, uint64_t tick, uint64_t snappedTick, uint64_t duration);
	void ClearNoteAddPreview();
	const NoteAddPreview& GetNoteAddPreview() const;
	bool HasNoteAddPreview() const;

private:
	TrackSet& mTrackSet;
	SoundBank& mSoundBank;
	NoteEditPreview mNoteEditPreview;
	MultiNoteEditPreview mMultiNoteEditPreview;
	NoteAddPreview mNoteAddPreview;
};
