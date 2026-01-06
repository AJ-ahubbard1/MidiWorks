// ProjectManager.h
#pragma once
#include <string>
#include <functional>

// Forward declarations
class Transport;
class SoundBank;
class TrackSet;
class RecordingSession;

/**
 * ProjectManager handles project persistence and state tracking.
 *
 * Responsibilities:
 * - Save/Load project files (JSON format)
 * - Clear/reset project to default state
 * - Track dirty state (unsaved changes)
 * - Track current project file path
 * - Notify UI of dirty state changes
 *
 * Dependencies:
 * - Transport: for tempo, time signature, current tick
 * - SoundBank: for channel settings (program, volume, mute, solo, record)
 * - TrackSet: for all track data (timed MIDI events)
 * - RecordingSession: for clearing recording buffer
 *
 * Usage:
 *   ProjectManager pm(transport, soundBank, trackSet, recordingSession);
 *   pm.SetDirtyStateCallback([](bool dirty) { UpdateWindowTitle(dirty); });
 *   pm.SetClearUndoHistoryCallback([&]() { undoManager.ClearHistory(); });
 *
 *   if (pm.SaveProject("myproject.mwp")) {
 *       // Save successful
 *   }
 */
class ProjectManager
{
public:
	/**
	 * Constructor
	 * @param transport Reference to Transport for tempo/time signature
	 * @param soundBank Reference to SoundBank for channel settings
	 * @param trackSet Reference to TrackSet for track data
	 * @param recordingSession Reference to RecordingSession for buffer clearing
	 */
	ProjectManager(
		Transport& transport,
		SoundBank& soundBank,
		TrackSet& trackSet,
		RecordingSession& recordingSession
	);

	// ============================================================
	// Save/Load Operations
	// ============================================================

	/**
	 * Save current project to a file
	 * @param filepath Path to save file (e.g., "myproject.mwp")
	 * @return true if save successful, false on error
	 *
	 * On success:
	 * - Updates current project path
	 * - Marks project as clean (not dirty)
	 * - Notifies dirty state callback
	 */
	bool SaveProject(const std::string& filepath);

	/**
	 * Load project from a file
	 * @param filepath Path to project file
	 * @return true if load successful, false on error
	 *
	 * On success:
	 * - Restores all Transport settings
	 * - Restores all SoundBank channels and applies settings to MIDI device
	 * - Restores all TrackSet data
	 * - Updates current project path
	 * - Marks project as clean
	 * - Calls ClearUndoHistoryCallback (undo/redo not preserved across loads)
	 * - Notifies dirty state callback
	 */
	bool LoadProject(const std::string& filepath);

	/**
	 * Clear/reset project to default state
	 *
	 * Actions:
	 * - Stops Transport and resets to default tempo
	 * - Clears all Tracks
	 * - Clears RecordingSession buffer
	 * - Resets all SoundBank channels to defaults
	 * - Silences all MIDI channels
	 * - Calls ClearUndoHistoryCallback
	 * - Clears project path
	 * - Marks project as clean
	 */
	void ClearProject();

	// ============================================================
	// Project State
	// ============================================================

	/**
	 * Check if project has unsaved changes
	 * @return true if project has been modified since last save
	 */
	bool IsProjectDirty() const;

	/**
	 * Mark project as having unsaved changes
	 * Notifies dirty state callback if state changed
	 */
	void MarkDirty();

	/**
	 * Mark project as clean (no unsaved changes)
	 * Notifies dirty state callback if state changed
	 */
	void MarkClean();

	/**
	 * Get current project file path
	 * @return Path to current project file, or empty string if no file
	 */
	const std::string& GetCurrentProjectPath() const;

	// ============================================================
	// Callbacks
	// ============================================================

	/**
	 * Callback signature for dirty state changes
	 * @param isDirty true if project now has unsaved changes
	 */
	using DirtyStateCallback = std::function<void(bool isDirty)>;

	/**
	 * Set callback to be notified of dirty state changes
	 * Used by UI to update window title (e.g., "MidiWorks - myproject.mwp*")
	 * @param callback Function to call when dirty state changes
	 */
	void SetDirtyStateCallback(DirtyStateCallback callback);

	/**
	 * Callback signature for clearing undo history
	 */
	using ClearUndoHistoryCallback = std::function<void()>;

	/**
	 * Set callback to clear undo/redo history
	 * Called during LoadProject() and ClearProject() since undo/redo
	 * is not serialized and should be reset when project changes
	 * @param callback Function to call to clear undo/redo stacks
	 */
	void SetClearUndoHistoryCallback(ClearUndoHistoryCallback callback);

	/**
     * Export Project Midi data to a midifile
	 * @param filepath is the output midifile
	 * @return true if export successful, false on error
	*/
	bool ExportMIDI(const std::string& filepath);

	/**
	 * Import MIDI data from a midi file into the project
	 * @param filepath is the input midi file
	 * @return true if import successful, false on error
	 */
	bool ImportMIDI(const std::string& filepath);

private:
	// References to app model components
	Transport& mTransport;
	SoundBank& mSoundBank;
	TrackSet& mTrackSet;
	RecordingSession& mRecordingSession;

	// Project state
	bool mIsDirty = false;
	std::string mCurrentProjectPath;

	// Callbacks
	DirtyStateCallback mDirtyStateCallback;
	ClearUndoHistoryCallback mClearUndoHistoryCallback;
};
