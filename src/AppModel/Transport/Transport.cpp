#include "Transport.h"

Transport::Transport() { }

// State management
Transport::State Transport::GetState() const { return mState; }
void Transport::SetState(State state) { mState = state; }

// State queries
bool Transport::IsPlaying() const		 { return mState == State::Playing;			}
bool Transport::IsRecording() const		 { return mState == State::Recording;		}
bool Transport::IsStopped() const		 { return mState == State::Stopped;			}
bool Transport::IsFastForwarding() const { return mState == State::FastForwarding;	}
bool Transport::IsRewinding() const		 { return mState == State::Rewinding;		}
// Is the Playhead currently moving?
bool Transport::IsMoving() const 
{
	return IsPlaying() || IsRecording() || IsFastForwarding() || IsRewinding();
}

// State transitions
void Transport::TogglePlay()
{
	switch (mState)
	{
	case State::Stopped:	mState = State::ClickedPlay;	break;
	case State::Playing:	mState = State::StopPlaying;	break;
	case State::Recording:	mState = State::StopRecording;	break;
	default: break;
	}
}

void Transport::ToggleRecord()
{
	if (mState == State::Stopped) mState = State::ClickedRecord;

	else if (mState == State::Recording) mState = State::StopRecording;
}

// Beat settings
Transport::BeatSettings Transport::GetBeatSettings() const { return mBeatSettings; }
void Transport::SetBeatSettings(const BeatSettings& settings) { mBeatSettings = settings; }

// Loop control
Transport::LoopSettings Transport::GetLoopSettings() const { return mLoopSettings; }
void Transport::SetLoopSettings(const LoopSettings& settings) { mLoopSettings = settings; }

void Transport::SetLoopStart(uint64_t tick)
{
	if (tick < mLoopSettings.endTick)
	{
		mLoopSettings.startTick = tick;
	}
}

void Transport::SetLoopEnd(uint64_t tick)
{
	if (tick > mLoopSettings.startTick)
	{
		mLoopSettings.endTick = tick;
	}
}

uint64_t Transport::GetLoopStart() const { return mLoopSettings.startTick; }
uint64_t Transport::GetLoopEnd() const { return mLoopSettings.endTick; }

uint64_t Transport::StartPlayBack()
{
	mStartPlayBackTick = GetCurrentTick();
	return mStartPlayBackTick;
}

void Transport::UpdatePlayBack(uint64_t deltaMs)
{
	mCurrentTimeMs += deltaMs;
	// Convert ms to ticks based on tempo
	double beats = (mCurrentTimeMs / 60000.0) * mBeatSettings.tempo;
	mCurrentTick = static_cast<uint64_t>(beats * mTicksPerQuarter);
}

uint64_t Transport::GetCurrentTick() const { return mCurrentTick; }

uint64_t Transport::GetStartPlayBackTick() const { return mStartPlayBackTick; }

void Transport::ShiftCurrentTime()
{
	mShiftSpeed *= mShiftAccel;

	// Cap shift speed to prevent excessive acceleration
	if (mShiftSpeed > MAX_SHIFT_SPEED)
		mShiftSpeed = MAX_SHIFT_SPEED;

	int shift = IsFastForwarding() ? mShiftSpeed : -mShiftSpeed;
	if (shift < 0 && -shift > mCurrentTimeMs)
	{
		mCurrentTimeMs = 0;
	}
	else
	{
		mCurrentTimeMs += shift;
	}
	double beats = (mCurrentTimeMs / 60000.0) * mBeatSettings.tempo;
	mCurrentTick = static_cast<uint64_t>(beats * mTicksPerQuarter);

}

void Transport::ResetShiftRate()
{
	mShiftSpeed = DEFAULT_SHIFT_SPEED;
}

void Transport::ShiftToTick(uint64_t newTick)
{
	if (newTick > MidiConstants::MAX_TICK_VALUE) return;

	mCurrentTick = newTick;
	mCurrentTimeMs = (1.0f * newTick / mTicksPerQuarter) * (60000.0 / mBeatSettings.tempo);
}

void Transport::Stop()
{
	mStartPlayBackTick = 0;
}

void Transport::Reset()
{
	mCurrentTimeMs = 0;
	mCurrentTick = 0;
	Stop();
}

void Transport::JumpToNextMeasure()
{
	auto ticksPerMeasure = GetTicksPerMeasure();
	// If we are already lined up with a measure, add a full measure
	if (GetCurrentTick() % GetTicksPerMeasure() == 0)
	{
		ShiftToTick(GetCurrentTick() + ticksPerMeasure);
	}
	// If we are in the middle of a measure
	else
	{
		uint64_t measureCount = GetCurrentTick() / ticksPerMeasure;
		uint64_t newTick = (measureCount + 1) * ticksPerMeasure;
		ShiftToTick(newTick);
	}
}

void Transport::JumpToPreviousMeasure()
{
	auto ticksPerMeasure = GetTicksPerMeasure();
	uint64_t newTick;
	// If we are already lined up with a measure, subtract a full measure
	if (GetCurrentTick() % GetTicksPerMeasure() == 0)
	{
		newTick = GetCurrentTick() - ticksPerMeasure;
	}
	// If we are in the middle of a measure
	else
	{
		uint64_t measureCount = GetCurrentTick() / ticksPerMeasure;
		newTick = measureCount * ticksPerMeasure;
	}
	ShiftToTick(newTick);
}

wxString Transport::GetFormattedTime() const { return GetFormattedTime(mCurrentTimeMs); }
wxString Transport::GetFormattedTime(uint64_t timeMs) const
{
	return wxString::Format("%02llu:%02llu:%03llu",
		timeMs / 60000, (timeMs % 60000) / 1000, timeMs % 1000);
}

// Check if a beat occurred between lastTick and currentTick
Transport::BeatInfo Transport::CheckForBeat(uint64_t lastTick, uint64_t currentTick) const
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
		info.isDownbeat = (currentBeat % mBeatSettings.timeSignatureNumerator) == 0;
	}

	return info;
}

uint64_t Transport::GetTicksPerBeat() const
{
	return mTicksPerQuarter * 4 / mBeatSettings.timeSignatureDenominator;
}

uint64_t Transport::GetTicksPerMeasure() const
{
	return GetTicksPerBeat() * mBeatSettings.timeSignatureNumerator;
}
