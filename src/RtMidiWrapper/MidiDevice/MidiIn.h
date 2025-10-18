#pragma once
#include "../RtMidi/RtMidi.h"
#include "../MidiMessage/MidiMessage.h"
#include "MidiError.h"

namespace MidiInterface
{
	struct Range
	{
		ubyte low, high;
	};
	class MidiIn
	{
	public:
		MidiIn()
		{
			mInstrument = new RtMidiIn();
			mInstrument->setErrorCallback(*midiErrorCallback, nullptr);
			mNumPorts = mInstrument->getPortCount();
			if (mNumPorts == 0)
			{
				std::cout << "No midiin ports.\n";
			} 
			else
			{
				fillPortNames();
				mInstrument->openPort(mPortNum);
			}
		}
		~MidiIn()
		{
			mInstrument->closePort();
			delete mInstrument;
			std::cout << "MidiIn deleted\n";
		}

		void changePort(ubyte p)
		{
			mInstrument->closePort();
			mPortNum = p;
			mInstrument->openPort(mPortNum);
		}

		unsigned int getNumPorts() const
		{
			return mNumPorts;
		}
		
		const std::vector<std::string>& getPortNames() const
		{
			return mPortNames;
		}
	
		bool checkForMessage()
		{
			static double timestamp;
			static std::vector<unsigned char> message;
			timestamp = mInstrument->getMessage(&message);
			if (message.size() > 0)
			{
				mMessage = MidiMessage(message[0], message[1], message[2]);
				mMessage.setTimestamp(timestamp);
				return true;
			}
			return false;
		}
		MidiMessage& getMessage()
		{
			return mMessage;
		}
		
		void setMidiInCallback(void (*callback)(double, std::vector<unsigned char>*, void*))
		{
			mInstrument->setCallback(callback);
		}

		void cancelCallback()
		{
			mInstrument->cancelCallback();
		}
	private:
		ubyte mPortNum{0};
		RtMidiIn* mInstrument;
		ubyte mNumPorts{0};
		Range mRange{36, 96};
		std::vector<std::string> mPortNames;
		MidiMessage mMessage;

		void fillPortNames()
		{
			for (int i = 0; i < mNumPorts; i++)
			{
				mPortNames.push_back(mInstrument->getPortName(i));
			}
		}
	};
}
