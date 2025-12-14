// AppModel.h
#pragma once
#include <unordered_map>
#include <memory>
#include <chrono>
#include <vector>
#include <functional>
#include "SoundBank/SoundBank.h"
#include "Transport.h"
#include "TrackSet/TrackSet.h"
#include "Commands/Command.h"
#include "MidiConstants.h"

class AppModel
{
public:
	AppModel();
	void Update();
	void CheckMidiInQueue();
	
	SoundBank& GetSoundBank();
	Transport& GetTransport();
	TrackSet& GetTrackSet();
	Track& GetRecordingBuffer();
	Track& GetTrack(ubyte c);

	// Playback Helpers
	void PlayNote(ubyte pitch, ubyte velocity, ubyte channel);
	void StopNote(ubyte pitch, ubyte channel);

	// Note preview for UI (keyboard/mouse hover)
	void PlayPreviewNote(ubyte pitch);
	void StopPreviewNote();
	bool IsPreviewingNote() const { return mIsPreviewingNote; }
	ubyte GetPreviewPitch() const { return mPreviewPitch; }
	ubyte GetPreviewVelocity() const { return mPreviewVelocity; }
	void SetPreviewVelocity(ubyte velocity) { mPreviewVelocity = velocity; }

	// Change transport state to stop
	void StopPlaybackIfActive();

	// Track operations
	void QuantizeAllTracks(uint64_t gridSize);
	void AddNoteToRecordChannels(ubyte pitch, uint64_t startTick, uint64_t duration);

	// MIDI Input port management
	std::vector<std::string> GetMidiInputPortNames() const;
	void SetMidiInputPort(int portIndex);

	// Callbacks
	using MidiLogCallback = std::function<void(const TimedMidiEvent&)>;
	void SetLogCallback(MidiLogCallback callback);
	using DirtyStateCallback = std::function<void(bool isDirty)>;
	void SetDirtyStateCallback(DirtyStateCallback callback);

	// Metronome settings
	void InitializeMetronome();
	bool IsMetronomeEnabled() const;
	void SetMetronomeEnabled(bool enabled);

	// Save/Load Project
	bool SaveProject(const std::string& filepath);
	bool LoadProject(const std::string& filepath);
	void ClearProject();

	// Project state
	bool IsProjectDirty() const;
	void MarkDirty();
	void MarkClean();
	const std::string& GetCurrentProjectPath() const;

	// Command Pattern - Undo/Redo System
	void ExecuteCommand(std::unique_ptr<Command> cmd);
	void Undo();
	void Redo();
	bool CanUndo() const;
	bool CanRedo() const;
	const std::vector<std::unique_ptr<Command>>& GetUndoStack() const;
	const std::vector<std::unique_ptr<Command>>& GetRedoStack() const;
	void ClearUndoHistory();

	// Clipboard for copy/paste
	struct ClipboardNote {
		uint64_t relativeStartTick;  // Relative to first note in selection
		uint64_t duration;
		uint8_t pitch;
		uint8_t velocity;
		int trackIndex;  // Which track it came from
	};
	void CopyToClipboard(const std::vector<ClipboardNote>& notes);
	const std::vector<ClipboardNote>& GetClipboard() const;
	bool HasClipboardData() const;
	void ClearClipboard();

private:
	std::shared_ptr<MidiIn> mMidiIn;
	std::chrono::steady_clock::time_point mLastTick; 
	SoundBank	mSoundBank;
	Transport	mTransport;
	TrackSet	mTrackSet;
	Track		mRecordingBuffer;
	int			mRecordingBufferIterator = -1;  // Iterator for efficient recording buffer playback during loops
	static const size_t MAX_UNDO_STACK_SIZE = 50;  // Limit to last 50 actions
	std::vector<std::unique_ptr<Command>> mUndoStack;
	std::vector<std::unique_ptr<Command>> mRedoStack;
	std::vector<ClipboardNote> mClipboard;
	
	// Active note tracking for loop recording (to auto-close held notes at loop end)
	struct ActiveNote {
		ubyte pitch;
		ubyte channel;
		uint64_t startTick;
	};
	std::vector<ActiveNote> mActiveNotes;
	MidiLogCallback mMidiLogCallback;
	DirtyStateCallback mDirtyStateCallback;
	bool mMetronomeEnabled = true;  // Metronome on by default
	bool mIsDirty = false;
	std::string mCurrentProjectPath;

	// Preview note state
	ubyte mPreviewVelocity = MidiConstants::DEFAULT_VELOCITY;
	bool mIsPreviewingNote = false;
	ubyte mPreviewPitch = 0;
	std::vector<ubyte> mPreviewChannels;

	uint64_t GetDeltaTimeMs();
	bool IsMusicalMessage(const MidiMessage& msg);
	void PlayMessages(std::vector<MidiMessage> msgs);
	void PlayMetronomeClick(bool isDownbeat);
	void SilenceAllChannels();
	void MergeOverlappingNotes(Track& buffer);
};

