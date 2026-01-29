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
			mInstrument = nullptr;
			std::cout << "MidiIn deleted\n";
		}

		ubyte getCurrentPort()
		{
			return mPortNum;
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
		/// identifies what ports have been added/removed 	
		void findDifference(std::vector<std::string>& biggerVec, std::vector<std::string>& smallerVec)
		{
			if (biggerVec.size() < smallerVec.size()) return findDifference(smallerVec, biggerVec);
			
			mChangedPorts.clear();
			for (auto& str1 : biggerVec)
			{
				if (std::find(smallerVec.begin(), smallerVec.end(), str1) == smallerVec.end())
				{
					mChangedPorts.push_back(str1);
				}
			}
		}

		/// verify that number of ports hasn't changed
		/// @returns the difference in port count
	    int detectChange()
		{
			auto portCount = mInstrument->getPortCount();
		
			// if count hasn't changed
			if (portCount == mNumPorts) return 0;

			int difference = portCount - mNumPorts;
			mNumPorts = portCount;
			auto oldPorts = mPortNames;
			mPortNames.clear();
			fillPortNames();
			findDifference(oldPorts, mPortNames);
			return difference;
		}

		const std::vector<std::string>& getChangedPorts() const { return mChangedPorts; }

	private:
		ubyte mPortNum{0};
		RtMidiIn* mInstrument;
		ubyte mNumPorts{0};
		Range mRange{36, 96};
		std::vector<std::string> mPortNames;
		std::vector<std::string> mChangedPorts;
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
