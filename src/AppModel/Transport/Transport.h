// Transport.h
#pragma once
#include "wx/string.h"
#include "MidiConstants.h"
#include <functional>

/// Transport manages playback state, timing, and loop control.
///
/// Responsibilities:
/// - Track playback state (stopped, playing, recording, etc.)
/// - Convert between time (ms) and ticks based on tempo
/// - Manage loop region settings
/// - Provide beat/measure detection for metronome
///
/// Usage:
///   Transport transport;
///   transport.SetBeatSettings({120.0, 4, 4});  // 120 BPM, 4/4 time
///   transport.TogglePlay();
///   transport.UpdatePlayBack(deltaMs);
///   auto tick = transport.GetCurrentTick();
class Transport
{
public:
	/// StopRecording, StopPlaying, ClickedPlay, and ClickedRecord are Transition States,
	/// used to finalize or setup the Recording and Playing states.
	enum class State { Stopped, StopRecording, StopPlaying, Playing, ClickedPlay,
						Recording, ClickedRecord, Rewinding, FastForwarding };

	struct BeatInfo
	{
		bool beatOccurred = false;
		bool isDownbeat = false;  // First beat of measure
	};

	struct BeatSettings
	{
		double tempo = MidiConstants::DEFAULT_TEMPO;
		int timeSignatureNumerator = MidiConstants::DEFAULT_TIME_SIGNATURE_NUMERATOR;
		int timeSignatureDenominator = MidiConstants::DEFAULT_TIME_SIGNATURE_DENOMINATOR;
	};

	struct LoopSettings
	{
		bool enabled = false;
		uint64_t startTick = 0;
		uint64_t endTick = MidiConstants::DEFAULT_LOOP_END;  // 4 bars in 4/4 time
	};

	Transport() { }

	// State Management

	/// Get current transport state
	State GetState() const { return mState; }

	/// Set transport state directly
	void SetState(State state) { mState = state; }

	// State Queries

	/// Check if currently playing
	bool IsPlaying() const { return mState == State::Playing; }

	/// Check if currently recording
	bool IsRecording() const { return mState == State::Recording; }

	/// Check if stopped
	bool IsStopped() const { return mState == State::Stopped; }

	/// Check if fast forwarding
	bool IsFastForwarding() const { return mState == State::FastForwarding; }

	/// Check if rewinding
	bool IsRewinding() const { return mState == State::Rewinding; }

	/// Check if playhead is currently moving (playing, recording, or shuttling)
	bool IsMoving() const;

	// State Transitions

	/// Toggle between play and stop states
	void TogglePlay();

	/// Toggle between record and stop states
	void ToggleRecord();

	// Beat Settings

	/// Get current beat settings (tempo, time signature)
	BeatSettings GetBeatSettings() const { return mBeatSettings; }

	/// Set beat settings
	void SetBeatSettings(const BeatSettings& settings) { mBeatSettings = settings; }

	// Loop Control

	/// Get current loop settings
	LoopSettings GetLoopSettings() const { return mLoopSettings; }

	/// Set loop settings (notifies callback if changed)
	void SetLoopSettings(const LoopSettings& settings);

	/// Set loop start tick (must be less than end tick)
	void SetLoopStart(uint64_t tick);

	/// Set loop end tick (must be greater than start tick)
	void SetLoopEnd(uint64_t tick);

	/// Get loop start tick
	uint64_t GetLoopStart() const { return mLoopSettings.startTick; }

	/// Get loop end tick
	uint64_t GetLoopEnd() const { return mLoopSettings.endTick; }

	// Playback Control

	/// Start playback from current position, returns starting tick
	uint64_t StartPlayBack();

	/// Update playback position based on elapsed milliseconds
	void UpdatePlayBack(uint64_t deltaMs);

	/// Stop playback if currently playing or recording
	void StopPlaybackIfActive();

	/// Get current tick position
	uint64_t GetCurrentTick() const { return mCurrentTick; }

	/// Shift current time during fast forward/rewind (accelerates over time)
	void ShiftCurrentTime();

	/// Reset shift speed to default (call when FF/RW stops)
	void ResetShiftRate() { mShiftSpeed = DEFAULT_SHIFT_SPEED; }

	/// Jump directly to a specific tick
	void ShiftToTick(uint64_t newTick);

	/// Reset to tick 0
	void Reset();

	/// Jump to the start of the next measure
	void JumpToNextMeasure();

	/// Jump to the start of the previous measure
	void JumpToPreviousMeasure();

	// Time Formatting

	/// Get formatted time string for current position (MM:SS:mmm)
	wxString GetFormattedTime() const { return GetFormattedTime(mCurrentTimeMs); }

	/// Get formatted time string for given milliseconds
	wxString GetFormattedTime(uint64_t timeMs) const;

	// Beat Detection

	/// Check if a beat occurred between lastTick and currentTick
	BeatInfo CheckForBeat(uint64_t lastTick, uint64_t currentTick) const;

	/// Get ticks per beat based on time signature
	uint64_t GetTicksPerBeat() const;

	/// Get ticks per measure based on time signature
	uint64_t GetTicksPerMeasure() const;

	// Loop Detection

	/// Check if playback should loop back (loop enabled and past end tick)
	bool ShouldLoopBack(uint64_t currentTick) const;

	// Callbacks

	/// Callback signature for loop settings changes
	using LoopChangedCallback = std::function<void()>;

	/// Set callback to be notified when loop settings change
	void SetLoopChangedCallback(LoopChangedCallback callback) { mLoopChangedCallback = callback; }

private:
	State mState = State::Stopped;
	BeatSettings mBeatSettings;
	LoopSettings mLoopSettings;
	uint64_t mCurrentTimeMs = 0;
	uint64_t mStartPlayBackTick = 0;
	uint64_t mCurrentTick = 0;
	int mTicksPerQuarter = MidiConstants::TICKS_PER_QUARTER;
	const double DEFAULT_SHIFT_SPEED = 50.0;
	const double MAX_SHIFT_SPEED = 1000.0;
	double mShiftSpeed = DEFAULT_SHIFT_SPEED;
	double mShiftAccel = 1.025;

	LoopChangedCallback mLoopChangedCallback;
};
