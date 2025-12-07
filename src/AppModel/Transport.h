#pragma once
#include "wx/string.h"
// Channel 16 (index 15) is reserved for metronome
static constexpr unsigned char METRONOME_CHANNEL = 15;

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

	State		mState = State::Stopped;
	double		mTempo = 120.0;
	int			mTimeSignatureNumerator = 4;    // Top number (beats per measure)
	int			mTimeSignatureDenominator = 4;  // Bottom number (note value)

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
		// Quarter note = mTicksPerQuarter (960)
		// Whole note = 960 * 4 = 3840
		// Time signature bottom number determines note value (4 = quarter note)
		uint64_t ticksPerBeat = (mTicksPerQuarter * 4) / mTimeSignatureDenominator;

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

private:
	uint64_t		mCurrentTimeMs = 0;
	uint64_t		mStartPlayBackTick = 0;
	uint64_t		mCurrentTick = 0;
	int				mTicksPerQuarter = 960;
	const double	DEFAULT_SHIFT_SPEED = 5.0f;
	double			mShiftSpeed = DEFAULT_SHIFT_SPEED;
	double			mShiftAccel = 1.01f;
};

