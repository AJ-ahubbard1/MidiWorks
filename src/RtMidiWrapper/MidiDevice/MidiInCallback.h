#pragma once
#include <iostream>
#include <vector>
#include "../MidiMessage/MidiMessage.h"

static std::vector<MidiInterface::MidiMessage> MidiInQueue;

/// Only called when midi data is received from instrument
static void midiInCallback(double deltaTime, std::vector<unsigned char>* message, void* userData)
{
	unsigned int nBytes = message->size();
	for (unsigned int i = 0; i < nBytes; i++)
	{
		std::cout << "Byte " << i << " = " << (int)message->at(i) << ", ";
	}
	if (nBytes > 0)
	{
		std::cout << "stamp = " << deltaTime << std::endl;
	}
}
