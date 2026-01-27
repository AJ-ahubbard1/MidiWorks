#pragma once
#include <iostream>
#include <string>
#include <functional>
#include "../RtMidi/RtMidi.h"

namespace MidiInterface
{
    /// Global MIDI error callback
	/// @param isWarning, true for warnings, false for errors
    using MidiErrorCallback = std::function<void(const std::string& message, bool isWarning)>;
    inline MidiErrorCallback g_midiErrorCallback = nullptr;

    inline void SetMidiErrorCallback(MidiErrorCallback callback)
    {
        g_midiErrorCallback = callback;
    }

    static void midiErrorCallback(RtMidiError::Type type, const std::string& errorText, void* userData)
    {
        // Keep std::cerr for developer debugging
        std::cerr << "[RtMidi Error] Type: " << static_cast<int>(type) << " | Message: " << errorText << std::endl;
               
		// Route to UI callback
        if (g_midiErrorCallback)
        {
            bool isWarning = (type == RtMidiError::WARNING || type == RtMidiError::DEBUG_WARNING);
            g_midiErrorCallback(errorText, isWarning);
        }
    }
}
