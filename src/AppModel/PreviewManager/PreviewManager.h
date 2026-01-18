// PreviewManager.h
#pragma once
#include <memory>
#include <vector>
#include "AppModel/TrackSet/TrackSet.h"
#include "AppModel/SoundBank/SoundBank.h"
#include "Commands/Command.h"

/// PreviewManager: handles the preview notes data before they are recorded to the TrackSet.
///
/// Responsibilities:
/// - Store the preview notes' temporary data (pitch, duration, etc)
/// - This data is copied to the trackset when the user confirms the note placement (left mouse up)
/// - Plays back audio when previewing new notes for audio feedback
/// Note: AppModel performs collision detection before calling set methods in PreviewManager 
class PreviewManager
{
public:
	PreviewManager(TrackSet& trackSet, SoundBank& soundBank)
		: mTrackSet(trackSet)
		, mSoundBank(soundBank)
	{
	}
	
	/// Note add preview (for mouse-based note creation)
	struct NoteAddPreview
	{
		bool isActive = false;
		ubyte pitch = 0;
		uint64_t tick = 0;  // Unsnapped tick for visual display
	};

	/// Note edit preview (for drag operations)
	struct NoteEditPreview
	{
		bool isActive = false;
		NoteLocation originalNote;
		uint64_t previewStartTick = 0;
		uint64_t previewEndTick = 0;
		ubyte previewPitch = 0;
	};

	/// Multi note edit preview (for drag operations)
	struct MultiNoteEditPreview
	{
		bool isActive = false;
		std::vector<NoteLocation> originalNotes;
		int64_t tickDelta = 0;
		int pitchDelta = 0;
	};

	void SetNoteAddPreview(ubyte pitch, uint64_t tick, uint64_t snappedTick, uint64_t duration);
	void ClearNoteAddPreview();

	void SetNoteMovePreview(const NoteLocation& note, uint64_t newStartTick, ubyte newPitch);
	void SetNoteResizePreview(const NoteLocation& note, uint64_t newEndTick);
	void SetMultipleNotesMovePreview(const std::vector<NoteLocation>& notes, int64_t tickDelta, int pitchDelta);
	
	/// Clears both single and multi note edit previews 
	void ClearNoteEditPreview();
	
	const NoteAddPreview& GetNoteAddPreview() const					{ return mNoteAddPreview;				 }
	bool HasNoteAddPreview() const									{ return mNoteAddPreview.isActive;		 }
	const NoteEditPreview& GetNoteEditPreview() const				{ return mNoteEditPreview;				 }
	bool HasNoteEditPreview() const									{ return mNoteEditPreview.isActive;		 }
	const MultiNoteEditPreview& GetMultiNoteEditPreview() const		{ return mMultiNoteEditPreview;			 }
	bool HasMultiNoteEditPreview() const							{ return mMultiNoteEditPreview.isActive; }

private:
	TrackSet& mTrackSet;
	SoundBank& mSoundBank;
	NoteAddPreview mNoteAddPreview;
	NoteEditPreview mNoteEditPreview;
	MultiNoteEditPreview mMultiNoteEditPreview;
};
