#pragma once
#include "wx/string.h"
#include "MidiConstants.h"
#include <functional>

class Transport
{
public:
	// StopRecording, StopPlaying, ClickedPlay, and ClickedRecord are Transition States,
	// used to finalize or setup the Recording and Playing states.
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

	// State management
	State GetState() const { return mState; }
	void SetState(State state) { mState = state; }

	// State queries
	bool IsPlaying() const			{ return mState == State::Playing;			}
	bool IsRecording() const		{ return mState == State::Recording;		}
	bool IsStopped() const			{ return mState == State::Stopped;			}
	bool IsFastForwarding() const	{ return mState == State::FastForwarding;	}
	bool IsRewinding() const		{ return mState == State::Rewinding;		}
	bool IsMoving() const;

	// State transitions
	void TogglePlay();	
	void ToggleRecord();
	
	// Beat settings
	BeatSettings GetBeatSettings() const { return mBeatSettings; }
	void SetBeatSettings(const BeatSettings& settings) { mBeatSettings = settings; }

	// Loop control
	LoopSettings GetLoopSettings() const { return mLoopSettings; }
	void SetLoopSettings(const LoopSettings& settings);
	void SetLoopStart(uint64_t tick);
	void SetLoopEnd(uint64_t tick);
	uint64_t GetLoopStart() const { return mLoopSettings.startTick; }
	uint64_t GetLoopEnd() const   { return mLoopSettings.endTick;   }

	// Playback Controls
	uint64_t StartPlayBack();
	void UpdatePlayBack(uint64_t deltaMs);
	void StopPlaybackIfActive();
	uint64_t GetCurrentTick() const { return mCurrentTick; }
	void ShiftCurrentTime();
	void ResetShiftRate() { mShiftSpeed = DEFAULT_SHIFT_SPEED; }
	void ShiftToTick(uint64_t newTick);
	void Reset();
	void JumpToNextMeasure();
	void JumpToPreviousMeasure();
	
	wxString GetFormattedTime() const { return GetFormattedTime(mCurrentTimeMs); }
	wxString GetFormattedTime(uint64_t timeMs) const;
	
	// Check if a beat occurred between lastTick and currentTick
	BeatInfo CheckForBeat(uint64_t lastTick, uint64_t currentTick) const;
	uint64_t GetTicksPerBeat() const;
	uint64_t GetTicksPerMeasure() const;

	// Check if playback should loop back to loop start
	bool ShouldLoopBack(uint64_t currentTick) const;

	// Callback for loop settings changes
	using LoopChangedCallback = std::function<void()>;
	void SetLoopChangedCallback(LoopChangedCallback callback) { mLoopChangedCallback = callback; }

private:
	State			mState = State::Stopped;
	BeatSettings	mBeatSettings;
	LoopSettings	mLoopSettings;
	uint64_t		mCurrentTimeMs = 0;
	uint64_t		mStartPlayBackTick = 0;
	uint64_t		mCurrentTick = 0;
	int				mTicksPerQuarter = MidiConstants::TICKS_PER_QUARTER;
	const double	DEFAULT_SHIFT_SPEED = 50.0;
	const double	MAX_SHIFT_SPEED = 1000.0;
	double			mShiftSpeed = DEFAULT_SHIFT_SPEED;
	double			mShiftAccel = 1.025;

	LoopChangedCallback mLoopChangedCallback;
};

