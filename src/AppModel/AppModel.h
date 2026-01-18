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
#include "PreviewManager/PreviewManager.h"
#include "ProjectManager/ProjectManager.h"
#include "UndoRedoManager/UndoRedoManager.h"
#include "MidiInputManager/MidiInputManager.h"
#include "MetronomeService/MetronomeService.h"
#include "DrumMachine/DrumMachine.h"
#include "Selection/Selection.h"
#include "MidiConstants.h"

// Error Handling callback types
enum class ErrorLevel { Info, Warning, Error };
using ErrorCallback = std::function<void(const std::string& title, const std::string& msg, ErrorLevel level)>;

/// AppModel is the central coordinator for the MIDI application.
///
/// Responsibilities:
/// - Coordinate all subsystems (Transport, TrackSet, SoundBank, etc.)
/// - Handle the main update loop for playback and recording
/// - Execute commands through UndoRedoManager for undo/redo support
/// - Provide collision detection for note editing
/// - Route MIDI input to appropriate channels
///
/// Usage:
///   AppModel model;
///   model.Update();  // Call from timer
///   model.AddNoteToRecordChannels(60, 0, 960);  // Add middle C
class AppModel
{
public:
	AppModel();

	/// Main update loop - call from timer event.
	/// Handles transport state machine and MIDI input.
	void Update();

	// Note Edit Preview

	/// Set preview for moving a single note (with collision detection)
	void SetNoteMovePreview(const NoteLocation& note, uint64_t newStartTick, ubyte newPitch);

	/// Set preview for moving multiple notes (with collision detection)
	void SetMultipleNotesMovePreview(const std::vector<NoteLocation>& notes, int64_t tickDelta, int pitchDelta);

	/// Set preview for resizing a note (with collision detection)
	void SetNoteResizePreview(const NoteLocation& note, uint64_t newEndTick);

	// Command Methods (creates commands and executes via UndoRedoManager)

	/// Add a note to all record-enabled channels
	void AddNoteToRecordChannels(ubyte pitch, uint64_t startTick, uint64_t duration);

	/// Delete a single note
	void DeleteNote(const NoteLocation& note);

	/// Delete multiple notes
	void DeleteNotes(const std::vector<NoteLocation>& notes);

	/// Clear all events from a track
	void ClearTrack(ubyte trackNumber);

	/// Move a note to a new position and/or pitch
	void MoveNote(const NoteLocation& note, uint64_t newStartTick, ubyte newPitch);

	/// Move multiple notes by a delta
	void MoveMultipleNotes(const std::vector<NoteLocation>& notes, int64_t tickDelta, int pitchDelta);

	/// Resize a note to a new duration
	void ResizeNote(const NoteLocation& note, uint64_t newDuration);

	/// Edit a note's velocity
	void EditNoteVelocity(const NoteLocation& note, ubyte newVelocity);

	/// Quantize notes based on context (selection, solo tracks, or all)
	void Quantize(uint64_t gridSize);

	// Clipboard Operations

	/// Paste notes from clipboard at given tick (default: current playhead)
	void PasteNotes(std::optional<uint64_t> pasteTick = std::nullopt);

	/// Paste notes from clipboard to record-enabled tracks only
	void PasteNotesToRecordTracks(std::optional<uint64_t> pasteTick = std::nullopt);

	// Drum Machine

	/// Record current drum pattern to TrackSet within loop region
	void RecordDrumPatternToTrack();

	/// Trigger drum pad via keyboard
	/// @return Column index if pad enabled during loop playback, -1 otherwise
	int TriggerDrumPad(int rowIndex);

	/// Release drum pad (send NoteOff)
	void ReleaseDrumPad(int rowIndex);

	// Collision Detection

	/// Check if a region is free of notes (single note exclusion)
	bool IsRegionCollisionFree(uint64_t startTick, uint64_t endTick, ubyte pitch, int channel,
	                           const NoteLocation* excludeNote = nullptr) const;

	/// Check if a region is free of notes (multiple note exclusion)
	bool IsRegionCollisionFree(uint64_t startTick, uint64_t endTick, ubyte pitch, int channel,
	                           const std::vector<NoteLocation>& excludeNotes) const;

	// Error Handling

	/// Set callback for error reporting to UI
	void SetErrorCallback(ErrorCallback callback) { mErrorCallback = callback; }

	/// Report an error through the callback
	void ReportError(const std::string& title, const std::string& msg, ErrorLevel level);

	// Component Accessors

	SoundBank& GetSoundBank() { return mSoundBank; }
	Transport& GetTransport() { return mTransport; }
	TrackSet& GetTrackSet() { return mTrackSet; }
	MidiInputManager& GetMidiInputManager() { return mMidiInputManager; }
	RecordingSession& GetRecordingSession() { return mRecordingSession; }
	ProjectManager& GetProjectManager() { return mProjectManager; }
	Clipboard& GetClipboard() { return mClipboard; }
	UndoRedoManager& GetUndoRedoManager() { return mUndoRedoManager; }
	MetronomeService& GetMetronomeService() { return mMetronomeService; }
	PreviewManager& GetPreviewManager() { return mPreviewManager; }
	DrumMachine& GetDrumMachine() { return mDrumMachine; }
	Selection& GetSelection() { return mSelection; }

private:
	std::chrono::steady_clock::time_point mLastTick;
	SoundBank mSoundBank;
	Transport mTransport;
	TrackSet mTrackSet;
	MidiInputManager mMidiInputManager;
	RecordingSession mRecordingSession;
	ProjectManager mProjectManager;
	Clipboard mClipboard;
	UndoRedoManager mUndoRedoManager;
	MetronomeService mMetronomeService;
	PreviewManager mPreviewManager;
	DrumMachine mDrumMachine;
	Selection mSelection;
	ErrorCallback mErrorCallback;

	/// Handle incoming MIDI messages from input device
	void HandleIncomingMidi();

	/// Get elapsed time since last call
	uint64_t GetDeltaTimeMs();

	/// Route a MIDI message to appropriate channels and record if needed
	void RouteAndPlayMessage(const MidiMessage& mm, uint64_t currentTick);

	/// Get drum machine messages for current tick range
	std::vector<MidiMessage> PlayDrumMachinePattern(uint64_t lastTick, uint64_t currentTick);

	// Transport State Handlers

	void HandleStopRecording();
	void HandleStopPlaying();
	void HandleClickedPlay();
	void HandlePlaying();
	void HandleClickedRecord();
	void HandleRecording();
	void HandleFastForwardRewind();
	void HandlePlaybackCore(bool isRecording);
};
