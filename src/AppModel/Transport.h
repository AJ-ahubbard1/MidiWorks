#pragma once

class Transport
{
public:
	// StopRecording, StopPlaying, ClickedPlay, and ClickedRecord are Transition States: 
	// used to setup their corresponding states.
	enum class	State { Stopped, StopRecording, StopPlaying, Playing, ClickedPlay, 
						Recording, ClickedRecord, Rewinding, FastForwarding };

	State		mState = State::Stopped;
	double		mTempo = 120.0;

	Transport() { }

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
	}

	void Stop()
	{
		mStartPlayBackTick = 0;
		mShiftSpeed = DEFAULT_SHIFT_SPEED;
	}

	void Reset()
	{
		mCurrentTimeMs = 0;
		Stop();
	}
	
	wxString GetFormattedTime() const { return GetFormattedTime(mCurrentTimeMs); }
	wxString GetFormattedTime(uint64_t timeMs) const
	{
		return wxString::Format("%02llu:%02llu:%03llu",
			timeMs / 60000, (timeMs % 60000) / 1000, timeMs % 1000);
	}

private:
	uint64_t		mCurrentTimeMs = 0;
	uint64_t		mStartPlayBackTick = 0;
	uint64_t		mCurrentTick = 0;
	int				mTicksPerQuarter = 960;
	const double	DEFAULT_SHIFT_SPEED = 5.0f;
	double			mShiftSpeed = DEFAULT_SHIFT_SPEED;
	float			mShiftAccel = 1.01;
};

