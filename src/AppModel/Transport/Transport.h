#pragma once
#include "wx/string.h"
#include "MidiConstants.h"

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

	double		mTempo						= MidiConstants::DEFAULT_TEMPO;
	int			mTimeSignatureNumerator		= MidiConstants::DEFAULT_TIME_SIGNATURE_NUMERATOR;
	int			mTimeSignatureDenominator	= MidiConstants::DEFAULT_TIME_SIGNATURE_DENOMINATOR;

	// Loop settings
	bool		mLoopEnabled = false;
	uint64_t	mLoopStartTick = 0;
	uint64_t	mLoopEndTick = MidiConstants::DEFAULT_LOOP_END;  // 4 bars in 4/4 time

	Transport();

	// State management
	State GetState() const;
	void SetState(State state);

	// State queries
	bool IsPlaying() const;
	bool IsRecording() const;
	bool IsStopped() const;
	bool IsFastForwarding() const;
	bool IsRewinding() const;
	bool IsMoving() const;

	// State transitions
	void TogglePlay();	
	void ToggleRecord();
	
	// Loop control
	void SetLoopStart(uint64_t tick);
	void SetLoopEnd(uint64_t tick);
	uint64_t GetLoopStart() const;
	uint64_t GetLoopEnd() const;

	// Playback Controls
	uint64_t StartPlayBack();
	void UpdatePlayBack(uint64_t deltaMs);
	uint64_t GetCurrentTick() const;
	uint64_t GetStartPlayBackTick() const;
	void ShiftCurrentTime();
	void ResetShiftRate();
	void ShiftToTick(uint64_t newTick);
	void Stop();
	void Reset();
	
	
	wxString GetFormattedTime() const;
	wxString GetFormattedTime(uint64_t timeMs) const;
	
	// Check if a beat occurred between lastTick and currentTick
	BeatInfo CheckForBeat(uint64_t lastTick, uint64_t currentTick) const;
	uint64_t GetTicksPerBeat() const;
	uint64_t GetTicksPerMeasure() const;
	

private:
	State			mState = State::Stopped;
	uint64_t		mCurrentTimeMs = 0;
	uint64_t		mStartPlayBackTick = 0;
	uint64_t		mCurrentTick = 0;
	int				mTicksPerQuarter = MidiConstants::TICKS_PER_QUARTER;
	const double	DEFAULT_SHIFT_SPEED = 20.0;
	const double	MAX_SHIFT_SPEED = 500.0;
	double			mShiftSpeed = DEFAULT_SHIFT_SPEED;
	double			mShiftAccel = 1.01;
};

