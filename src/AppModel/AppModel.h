// AppModel.h
#pragma once
#include <unordered_map>
#include <memory>
#include <chrono>
#include <vector>
#include <functional>
#include <optional>
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
#include "DrumMachine/DrumMachine.h"
#include "MidiConstants.h"

class AppModel
{
public:
	AppModel();
	void Update();
	
	// Component Classes
	SoundBank& GetSoundBank() { return mSoundBank; }
	Transport& GetTransport() { return mTransport; }
	TrackSet& GetTrackSet() { return mTrackSet; }
	RecordingSession& GetRecordingSession() { return mRecordingSession; }
	ProjectManager& GetProjectManager() { return mProjectManager; }
	Clipboard& GetClipboard() { return mClipboard; }
	UndoRedoManager& GetUndoRedoManager() { return mUndoRedoManager; }
	MidiInputManager& GetMidiInputManager() { return mMidiInputManager; }
	MetronomeService& GetMetronomeService() { return mMetronomeService; }
	DrumMachine& GetDrumMachine() { return mDrumMachine; }

	// Change transport state to stop
	void StopPlaybackIfActive();

	// Track operations
	void QuantizeAllTracks(uint64_t gridSize);
	void AddNoteToRecordChannels(ubyte pitch, uint64_t startTick, uint64_t duration);

	// Note Editing - Deletion
	void DeleteNote(const NoteLocation& note);
	void DeleteNotes(const std::vector<NoteLocation>& notes);
	void ClearTrack(ubyte trackNumber);

	// Note Editing - Move and Resize
	void MoveNote(const NoteLocation& note, uint64_t newStartTick, ubyte newPitch);
	void MoveMultipleNotes(const std::vector<NoteLocation>& notes, int64_t tickDelta, int pitchDelta);
	void ResizeNote(const NoteLocation& note, uint64_t newDuration);

	// Note Edit Preview (for drag operations)
	void SetNoteMovePreview(const NoteLocation& note, uint64_t newStartTick, ubyte newPitch);
	void SetMultipleNotesMovePreview(const std::vector<NoteLocation>& notes, int64_t tickDelta, int pitchDelta);
	void SetNoteResizePreview(const NoteLocation& note, uint64_t newEndTick);
	void ClearNoteEditPreview();
	const NoteEditor::NoteEditPreview& GetNoteEditPreview() const;
	const NoteEditor::MultiNoteEditPreview& GetMultiNoteEditPreview() const;
	bool HasNoteEditPreview() const;
	bool HasMultiNoteEditPreview() const;

	// Note Add Preview (for mouse-based note creation)
	void SetNoteAddPreview(ubyte pitch, uint64_t tick, uint64_t snappedTick, uint64_t duration);
	void ClearNoteAddPreview();
	const NoteEditor::NoteAddPreview& GetNoteAddPreview() const;
	bool HasNoteAddPreview() const;

	// Clipboard for copy/paste
	void CopyNotesToClipboard(const std::vector<NoteLocation>& notes);
	void PasteNotes(std::optional<uint64_t> pasteTick = std::nullopt);
	void PasteNotesToRecordTracks(std::optional<uint64_t> pasteTick = std::nullopt);

	// Drum Machine
	void RecordDrumPatternToTrack();
	int TriggerDrumPad(int rowIndex);  // Trigger drum pad via keyboard (returns column index if pad enabled, -1 otherwise)
	void ReleaseDrumPad(int rowIndex);
	
	// Collision detection helpers
	bool IsRegionCollisionFree(uint64_t startTick, uint64_t endTick, ubyte pitch, int channel,
	                           const NoteLocation* excludeNote = nullptr) const;
	bool IsRegionCollisionFree(uint64_t startTick, uint64_t endTick, ubyte pitch, int channel,
	                           const std::vector<NoteLocation>& excludeNotes) const;


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
	DrumMachine								mDrumMachine;

	uint64_t GetDeltaTimeMs();
	bool IsMusicalMessage(const MidiMessage& msg);
	void PlayMessages(std::vector<MidiMessage> msgs);
	void RouteAndPlayMessage(const MidiMessage& mm, uint64_t currentTick);
	void HandleIncomingMidi();
	std::vector<MidiMessage> PlayDrumMachinePattern(uint64_t lastTick, uint64_t currentTick);

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

