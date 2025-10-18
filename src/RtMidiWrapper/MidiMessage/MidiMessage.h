#pragma once

#include "SoundMaps.h"
#include <format>

namespace MidiInterface
{
	using ubyte = unsigned char;

	enum MidiEvent
	{
		NOTE_OFF = 0x80,
		NOTE_ON = 0X90,
		AFTERTOUCH = 0XA0,
		CONTROL_CHANGE = 0XB0,
		PROGRAM_CHANGE = 0XC0,
		CHANNEL_PRESSURE = 0XD0,
		PITCH_BEND = 0XE0
	};

	enum ChannelMessage
	{
		VOLUME = 0x07,
		ALL_SOUND_OFF = 0x78,
		RESET_CONTROLLERS = 0X79,
		ALL_NOTES_OFF = 0x7B,
	};

	class MidiMessage
	{
	public:
		ubyte mData[3];
		double timestamp;

		MidiMessage()
		{
			mData[0] = 0;
			mData[1] = 0;
			mData[2] = 0;
			timestamp = 0;
		}

		MidiMessage(ubyte d0, ubyte d1, ubyte d2)
		{
			mData[0] = d0;
			mData[1] = d1;
			mData[2] = d2;
			timestamp = 0;
		}
		// Copy constructor 
		MidiMessage(const MidiMessage& other)
		{
			std::copy(other.mData, other.mData + 3, mData);
			timestamp = other.timestamp;
		}

		// Copy assignment operator
		MidiMessage& operator=(const MidiMessage& other)
		{
			if (this != &other)
			{
				std::copy(other.mData, other.mData + 3, mData);
				timestamp = other.timestamp;
			}
			return *this;
		}

		void setChannel(ubyte channel)
		{
			ubyte event = mData[0] >> 4;        // strip off channel bits
			mData[0] = (event << 4) | channel;  // shift evt back to front 4 + new channel
		}

		void setTimestamp(double ts)
		{
			timestamp = ts;
		}

		static MidiMessage NoteOn(ubyte keyNumber, ubyte velocity, ubyte channel = 0)
		{
			return MidiMessage(MidiEvent::NOTE_ON | channel, keyNumber, velocity);
		}

		static MidiMessage NoteOff(ubyte keyNumber, ubyte channel = 0)
		{
			return MidiMessage(MidiEvent::NOTE_OFF | channel, keyNumber, 0);
		}

		static MidiMessage NoteOff(const MidiMessage& other)
		{
			return MidiMessage(MidiEvent::NOTE_OFF | (other.mData[0] % 16), other.mData[1], 0);
		}
		
		static MidiMessage ProgramChange(SoundSet patch, ubyte channel = 0)
		{
			return MidiMessage(MidiEvent::PROGRAM_CHANGE | channel, ubyte(patch), 0);
		}

		static MidiMessage ProgramChange(ubyte patchNumber, ubyte channel = 0)
		{
			return MidiMessage(MidiEvent::PROGRAM_CHANGE | channel, patchNumber, 0);
		}

		static MidiMessage ControlChange(ubyte ctrlNum, ubyte ctrlVal, ubyte channel = 0)
		{
			return MidiMessage(MidiEvent::CONTROL_CHANGE | channel, ctrlNum, ctrlVal);
		}

		static MidiMessage AllNotesOff(ubyte channel = 0)
		{
			return MidiMessage(MidiEvent::CONTROL_CHANGE | channel, ChannelMessage::ALL_NOTES_OFF, 0);
		}
	
		static const std::string& getSoundName(ubyte ss)
		{
			return SoundNames[ss];
		}
	
		std::string getString()
		{
			return std::format("[{:#0X},{},{}]", mData[0], mData[1], mData[2]);
		}

		MidiEvent getEventType() const
		{
			return static_cast<MidiEvent>(mData[0] & 0xF0);
		}

		ubyte getChannel() const
		{
			return static_cast<ubyte>(mData[0] & 0x0F);
		}
	};
}
