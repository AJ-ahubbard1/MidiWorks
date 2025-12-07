// AppModel.h
#pragma once
#include <unordered_map>
#include <memory>
#include <chrono>
#include <vector>
#include <functional>
#include "SoundBank.h"
#include "Transport.h"
#include "TrackSet.h"
#include "Commands/Command.h"

class AppModel
{

public:
	AppModel();
	void InitializeMetronome();
	// Called inside of MainFrame::OnTimer event
	void Update();

	/*  Checks for Messages from Midi In device
		If a SoundBank Channel is active, the message will playback.
		If a SoundBank Channel is set to record: the message will be pushed to the mRecordingBuffer.
		This buffer is used to temporarily store the midi messages during recording,
		when finished recording, the buffer is added to the track and sorted by timestamp. */
	void CheckMidiInQueue();

    SoundBank& GetSoundBank();
	Transport& GetTransport();
	TrackSet& GetTrackSet();
	Track& GetRecordingBuffer();
	Track& GetTrack(ubyte c);

	// MIDI Input port management
	std::vector<std::string> GetMidiInputPortNames() const;
	void SetMidiInputPort(int portIndex);
	int GetCurrentMidiInputPort() const;

	// Logging system (callback pattern)
	using MidiLogCallback = std::function<void(const TimedMidiEvent&)>;
	void SetLogCallback(MidiLogCallback callback);

	// Dirty state change notification (callback pattern)
	using DirtyStateCallback = std::function<void(bool isDirty)>;
	void SetDirtyStateCallback(DirtyStateCallback callback);

	// Metronome settings
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
	static const size_t MAX_UNDO_STACK_SIZE = 50;  // Limit to last 50 actions
	std::chrono::steady_clock::time_point mLastTick = std::chrono::steady_clock::now();
	SoundBank	mSoundBank;
	Transport	mTransport;
	TrackSet	mTrackSet;
	Track		mRecordingBuffer;
	int			mRecordingBufferIterator = -1;  // Iterator for efficient recording buffer playback during loops

	// Active note tracking for loop recording (to auto-close held notes at loop end)
	struct ActiveNote {
		ubyte pitch;
		ubyte channel;
		uint64_t startTick;
	};
	std::vector<ActiveNote> mActiveNotes;

	// MIDI Input
	std::shared_ptr<MidiIn> mMidiIn;

	// Logging
	MidiLogCallback mMidiLogCallback;

	// Dirty state notification
	DirtyStateCallback mDirtyStateCallback;

	// Metronome
	bool mMetronomeEnabled = true;  // Metronome on by default

	// Project state
	bool mIsDirty = false;
	std::string mCurrentProjectPath;

	// Command Pattern - Undo/Redo stacks
	std::vector<std::unique_ptr<Command>> mUndoStack;
	std::vector<std::unique_ptr<Command>> mRedoStack;

	// Clipboard
	std::vector<ClipboardNote> mClipboard;

	uint64_t GetDeltaTimeMs();
	bool IsMusicalMessage(const MidiMessage& msg);
	void PlayMessages(std::vector<MidiMessage> msgs);
	void PlayMetronomeClick(bool isDownbeat);
	void SilenceAllChannels();
	void MergeOverlappingNotes(Track& buffer);
};

