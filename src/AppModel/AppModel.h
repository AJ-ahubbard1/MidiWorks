// AppModel.h
#pragma once
#include <unordered_map>
#include <memory>
#include <chrono>
#include <vector>
#include <functional>
#include "SoundBank/SoundBank.h"
#include "Transport/Transport.h"
#include "TrackSet/TrackSet.h"
#include "RecordingSession/RecordingSession.h"
#include "Clipboard/Clipboard.h"
#include "NoteEditor/NoteEditor.h"
#include "ProjectManager/ProjectManager.h"
#include "UndoRedoManager/UndoRedoManager.h"
#include "MidiInputManager/MidiInputManager.h"
#include "MetronomeService/MetronomeService.h"
#include "MidiConstants.h"

class AppModel
{
public:
	AppModel();
	void Update();

	SoundBank& GetSoundBank();
	Transport& GetTransport();
	TrackSet& GetTrackSet();
	RecordingSession& GetRecordingSession();
	ProjectManager& GetProjectManager();
	Clipboard& GetClipboard();
	UndoRedoManager& GetUndoRedoManager();
	MidiInputManager& GetMidiInputManager();
	MetronomeService& GetMetronomeService();

	// Change transport state to stop
	void StopPlaybackIfActive();

	// Track operations
	void QuantizeAllTracks(uint64_t gridSize);
	void AddNoteToRecordChannels(ubyte pitch, uint64_t startTick, uint64_t duration);

	// Note Editing - Deletion
	void DeleteNote(const NoteLocation& note);
	void DeleteNotes(const std::vector<NoteLocation>& notes);

	// Note Editing - Move and Resize
	void MoveNote(const NoteLocation& note, uint64_t newStartTick, ubyte newPitch);
	void ResizeNote(const NoteLocation& note, uint64_t newDuration);

	// Note Edit Preview (for drag operations)
	void SetNoteMovePreview(const NoteLocation& note, uint64_t newStartTick, ubyte newPitch);
	void SetNoteResizePreview(const NoteLocation& note, uint64_t newEndTick);
	void ClearNoteEditPreview();
	const NoteEditor::NoteEditPreview& GetNoteEditPreview() const;
	bool HasNoteEditPreview() const;

	// Callbacks
	using DirtyStateCallback = std::function<void(bool isDirty)>;
	void SetDirtyStateCallback(DirtyStateCallback callback);

	// Clipboard for copy/paste
	void CopyNotesToClipboard(const std::vector<NoteLocation>& notes);
	void PasteNotes(uint64_t pasteTick = UINT64_MAX);


private:
	std::chrono::steady_clock::time_point	mLastTick;
	SoundBank								mSoundBank;
	Transport								mTransport;
	TrackSet								mTrackSet;
	RecordingSession						mRecordingSession;
	Clipboard								mClipboard;
	NoteEditor								mNoteEditor;
	ProjectManager							mProjectManager;
	UndoRedoManager							mUndoRedoManager;
	MidiInputManager						mMidiInputManager;
	MetronomeService						mMetronomeService;
	DirtyStateCallback						mDirtyStateCallback;

	uint64_t GetDeltaTimeMs();
	bool IsMusicalMessage(const MidiMessage& msg);
	void PlayMessages(std::vector<MidiMessage> msgs);
	void RouteAndPlayMessage(const MidiMessage& mm, uint64_t currentTick);
	void HandleIncomingMidi();

	// Transport state handlers
	void HandleStopRecording();
	void HandleStopPlaying();
	void HandleClickedPlay();
	void HandlePlaying();
	void HandleClickedRecord();
	void HandleRecording();
	void HandleFastForwardRewind();
	void HandlePlaybackCore(bool isRecording);
};

