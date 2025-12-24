// MidiInputManager.cpp
#include "MidiInputManager.h"

MidiInputManager::MidiInputManager()
{
	mMidiIn = std::make_shared<MidiIn>();
}

std::vector<std::string> MidiInputManager::GetPortNames() const
{
	return mMidiIn->getPortNames();
}

void MidiInputManager::SetInputPort(int portIndex)
{
	mMidiIn->changePort(portIndex);
}

MidiIn& MidiInputManager::GetDevice()
{
	return *mMidiIn;
}

void MidiInputManager::SetLogCallback(MidiLogCallback callback)
{
	mLogCallback = callback;
}

const MidiInputManager::MidiLogCallback& MidiInputManager::GetLogCallback() const
{
	return mLogCallback;
}

std::optional<MidiMessage> MidiInputManager::PollAndNotify(uint64_t currentTick)
{
	if (!mMidiIn->checkForMessage()) return std::nullopt;

	MidiMessage mm = mMidiIn->getMessage();

	if (mLogCallback)
	{
		mLogCallback({mm, currentTick});
	}

	return mm;
}
