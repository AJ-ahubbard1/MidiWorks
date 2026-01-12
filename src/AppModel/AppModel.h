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

	// Command Methods: creates the commands and uses UndoRedoManager to execute them, adding them to undo stack
	
	void AddNoteToRecordChannels(ubyte pitch, uint64_t startTick, uint64_t duration);
	
	void QuantizeAllTracks(uint64_t gridSize);

	// Note Editing - Deletion
	void DeleteNote(const NoteLocation& note);
	void DeleteNotes(const std::vector<NoteLocation>& notes);
	void ClearTrack(ubyte trackNumber);

	// Note Editing - Move and Resize
	void MoveNote(const NoteLocation& note, uint64_t newStartTick, ubyte newPitch);
	void MoveMultipleNotes(const std::vector<NoteLocation>& notes, int64_t tickDelta, int pitchDelta);
	void ResizeNote(const NoteLocation& note, uint64_t newDuration);
	void EditNoteVelocity(const NoteLocation& note, ubyte newVelocity);

	// Note Edit Preview (for drag operations)
	void SetNoteMovePreview(const NoteLocation& note, uint64_t newStartTick, ubyte newPitch);
	void SetMultipleNotesMovePreview(const std::vector<NoteLocation>& notes, int64_t tickDelta, int pitchDelta);
	void SetNoteResizePreview(const NoteLocation& note, uint64_t newEndTick);

	// Clipboard for copy/paste
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

	// Component Classes
	SoundBank& GetSoundBank() { return mSoundBank; }
	Transport& GetTransport() { return mTransport; }
	TrackSet& GetTrackSet() { return mTrackSet; }
	MidiInputManager& GetMidiInputManager() { return mMidiInputManager; }
	RecordingSession& GetRecordingSession() { return mRecordingSession; }
	ProjectManager& GetProjectManager() { return mProjectManager; }
	Clipboard& GetClipboard() { return mClipboard; }
	UndoRedoManager& GetUndoRedoManager() { return mUndoRedoManager; }
	MetronomeService& GetMetronomeService() { return mMetronomeService; }
	NoteEditor& GetNoteEditor() { return mNoteEditor; } 	
	DrumMachine& GetDrumMachine() { return mDrumMachine; }

private:
	SoundBank								mSoundBank;
	Transport								mTransport;
	TrackSet								mTrackSet;
	MidiInputManager						mMidiInputManager;
	RecordingSession						mRecordingSession;
	ProjectManager							mProjectManager;
	Clipboard								mClipboard;
	UndoRedoManager							mUndoRedoManager;
	MetronomeService						mMetronomeService;
	NoteEditor								mNoteEditor;
	DrumMachine								mDrumMachine;

	void HandleIncomingMidi();
	uint64_t GetDeltaTimeMs();
	void RouteAndPlayMessage(const MidiMessage& mm, uint64_t currentTick);
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

