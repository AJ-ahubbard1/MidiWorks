#pragma once
#include "../RtMidi/RtMidi.h"

namespace MidiInterface
{
    static void midiErrorCallback(RtMidiError::Type type, const std::string& errorText, void* userData)
    {
        std::cerr << "[RtMidi Error] Type: " << static_cast<int>(type)
            << " | Message: " << errorText << std::endl;

        // Optional: handle specific error types
        if (type == RtMidiError::WARNING)
        {
            std::cerr << "Warning only — continuing...\n";
        }
        else if (type == RtMidiError::DEBUG_WARNING)
        {
            std::cerr << "Debug warning — might be ALSA latency or port mismatch.\n";
        }
        else
        {
            std::cerr << "Fatal error — consider exiting or recovering.\n";
            exit(EXIT_FAILURE);
        }
    }
}
