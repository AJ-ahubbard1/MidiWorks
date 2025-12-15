#pragma once
#include "wx/string.h"
#include "MidiConstants.h"

class Transport
{
public:
	// StopRecording, StopPlaying, ClickedPlay, and ClickedRecord are Transition States:
	// used to setup their corresponding states.
	enum class	State { Stopped, StopRecording, StopPlaying, Playing, ClickedPlay,
						Recording, ClickedRecord, Rewinding, FastForwarding };

	struct BeatInfo
	{
		bool beatOccurred = false;
		bool isDownbeat = false;  // First beat of measure
	};

	double		mTempo = MidiConstants::DEFAULT_TEMPO;
	int			mTimeSignatureNumerator = MidiConstants::DEFAULT_TIME_SIGNATURE_NUMERATOR;
	int			mTimeSignatureDenominator = MidiConstants::DEFAULT_TIME_SIGNATURE_DENOMINATOR;

	// Loop settings
	bool		mLoopEnabled = false;
	uint64_t	mLoopStartTick = 0;
	uint64_t	mLoopEndTick = MidiConstants::DEFAULT_LOOP_END;  // 4 bars in 4/4 time

	Transport() { }

	// State management
	State GetState() const { return mState; }
	void SetState(State state) { mState = state; }

	// State queries
	bool IsPlaying() const { return mState == State::Playing; }
	bool IsRecording() const { return mState == State::Recording; }
	bool IsStopped() const { return mState == State::Stopped; }

	// State transitions
	void TogglePlay()
	{
		switch (mState)
		{
		case State::Stopped:
			mState = State::ClickedPlay;
			break;
		case State::Playing:
			mState = State::StopPlaying;
			break;
		case State::Recording:
			mState = State::StopRecording;
			break;
		default:
			break;
		}
	}

	void ToggleRecord()
	{
		if (mState == State::Stopped)
		{
			mState = State::ClickedRecord;
		}
		else if (mState == State::Recording)
		{
			mState = State::StopRecording;
		}
	}

	// Loop control
	void SetLoopStart(uint64_t tick)
	{
		if (tick < mLoopEndTick)
		{
			mLoopStartTick = tick;
		}
	}

	void SetLoopEnd(uint64_t tick)
	{
		if (tick > mLoopStartTick)
		{
			mLoopEndTick = tick;
		}
	}

	uint64_t GetLoopStart() const { return mLoopStartTick; }
	uint64_t GetLoopEnd() const { return mLoopEndTick; }

	uint64_t StartPlayBack()
	{
		mStartPlayBackTick = GetCurrentTick();
		return mStartPlayBackTick;
	}

	void UpdatePlayBack(uint64_t deltaMs)
	{
		mCurrentTimeMs += deltaMs;
		// Convert ms to ticks based on tempo
		double beats = (mCurrentTimeMs / 60000.0) * mTempo;
		mCurrentTick = static_cast<uint64_t>(beats * mTicksPerQuarter);
	}
 
	uint64_t GetCurrentTick() const { return mCurrentTick; }

	uint64_t GetStartPlayBackTick() const { return mStartPlayBackTick; }

	void ShiftCurrentTime()
	{
		mShiftSpeed *= mShiftAccel;
		int shift = (mState == State::FastForwarding) ? mShiftSpeed : -mShiftSpeed;
		if (shift < 0 && -shift > mCurrentTimeMs)
		{
			mCurrentTimeMs = 0;
		}
		else
		{
			mCurrentTimeMs += shift;
		}
		double beats = (mCurrentTimeMs / 60000.0) * mTempo;
		mCurrentTick = static_cast<uint64_t>(beats * mTicksPerQuarter);

	}
	
	void ShiftToTick(uint64_t newTick)
	{
		mCurrentTick = newTick;
		mCurrentTimeMs = (1.0f * newTick / mTicksPerQuarter) * (60000.0 / mTempo);
	}

	void Stop()
	{
		mStartPlayBackTick = 0;
		mShiftSpeed = DEFAULT_SHIFT_SPEED;
	}

	void Reset()
	{
		mCurrentTimeMs = 0;
		mCurrentTick = 0;
		Stop();
	}
	
	wxString GetFormattedTime() const { return GetFormattedTime(mCurrentTimeMs); }
	wxString GetFormattedTime(uint64_t timeMs) const
	{
		return wxString::Format("%02llu:%02llu:%03llu",
			timeMs / 60000, (timeMs % 60000) / 1000, timeMs % 1000);
	}

	// Check if a beat occurred between lastTick and currentTick
	BeatInfo CheckForBeat(uint64_t lastTick, uint64_t currentTick) const
	{
		BeatInfo info;

		// Calculate ticks per beat based on time signature
		// Quarter note = mTicksPerQuarter (TICKS_PER_QUARTER)
		// Whole note = TICKS_PER_QUARTER * 4
		// Time signature bottom number determines note value (4 = quarter note)
		uint64_t ticksPerBeat = GetTicksPerBeat();

		// Which beat are we on?
		uint64_t lastBeat = lastTick / ticksPerBeat;
		uint64_t currentBeat = currentTick / ticksPerBeat;

		// Did we cross a beat boundary, or are we starting at beat 0?
		if (currentBeat > lastBeat || (lastTick == 0 && currentBeat == 0))
		{
			info.beatOccurred = true;
			// Is this the first beat of a measure?
			info.isDownbeat = (currentBeat % mTimeSignatureNumerator) == 0;
		}

		return info;
	}

	uint64_t GetTicksPerBeat() const
	{
		return mTicksPerQuarter * 4 / mTimeSignatureDenominator;
	}

	uint64_t GetTicksPerMeasure() const
	{
		return GetTicksPerBeat() * mTimeSignatureNumerator;
	}

private:
	State			mState = State::Stopped;
	uint64_t		mCurrentTimeMs = 0;
	uint64_t		mStartPlayBackTick = 0;
	uint64_t		mCurrentTick = 0;
	int				mTicksPerQuarter = MidiConstants::TICKS_PER_QUARTER;
	const double	DEFAULT_SHIFT_SPEED = 5.0f;
	double			mShiftSpeed = DEFAULT_SHIFT_SPEED;
	double			mShiftAccel = 1.01f;
};

