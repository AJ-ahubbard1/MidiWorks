// PreviewManager.cpp
#include "PreviewManager.h"
#include "Commands/NoteEditCommands.h"
#include "Commands/MultiNoteCommands.h"
#include "Commands/TrackCommands.h"
#include "MidiConstants.h"


void PreviewManager::SetNoteMovePreview(const NoteLocation& note, uint64_t newStartTick, ubyte newPitch)
{
	mNoteEditPreview.isActive = true;
	mNoteEditPreview.originalNote = note;
	mNoteEditPreview.previewStartTick = newStartTick;
	mNoteEditPreview.previewEndTick = newStartTick + (note.endTick - note.startTick);
	mNoteEditPreview.previewPitch = newPitch;
}

void PreviewManager::SetNoteResizePreview(const NoteLocation& note, uint64_t newEndTick)
{
	mNoteEditPreview.isActive = true;
	mNoteEditPreview.originalNote = note;
	mNoteEditPreview.previewStartTick = note.startTick;
	mNoteEditPreview.previewEndTick = newEndTick;
	mNoteEditPreview.previewPitch = note.pitch;
}

void PreviewManager::ClearNoteEditPreview()
{
	mNoteEditPreview.isActive = false;
	mMultiNoteEditPreview.isActive = false;
}

const PreviewManager::NoteEditPreview& PreviewManager::GetNoteEditPreview() const
{
	return mNoteEditPreview;
}

bool PreviewManager::HasNoteEditPreview() const
{
	return mNoteEditPreview.isActive;
}

void PreviewManager::SetMultipleNotesMovePreview(const std::vector<NoteLocation>& notes, int64_t tickDelta, int pitchDelta)
{
	mMultiNoteEditPreview.isActive = true;
	mMultiNoteEditPreview.originalNotes = notes;
	mMultiNoteEditPreview.tickDelta = tickDelta;
	mMultiNoteEditPreview.pitchDelta = pitchDelta;
}

const PreviewManager::MultiNoteEditPreview& PreviewManager::GetMultiNoteEditPreview() const
{
	return mMultiNoteEditPreview;
}

bool PreviewManager::HasMultiNoteEditPreview() const
{
	return mMultiNoteEditPreview.isActive;
}

void PreviewManager::SetNoteAddPreview(ubyte pitch, uint64_t tick, uint64_t snappedTick, uint64_t duration)
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

void PreviewManager::ClearNoteAddPreview()
{
	if (mNoteAddPreview.isActive)
	{
		mSoundBank.StopPreviewNote();
		mNoteAddPreview.isActive = false;
	}
}

const PreviewManager::NoteAddPreview& PreviewManager::GetNoteAddPreview() const
{
	return mNoteAddPreview;
}

bool PreviewManager::HasNoteAddPreview() const
{
	return mNoteAddPreview.isActive;
}
