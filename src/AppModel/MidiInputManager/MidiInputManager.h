// MidiInputManager.h
#pragma once
#include <vector>
#include <string>
#include <memory>
#include <functional>
#include "RtMidiWrapper/MidiDevice/MidiIn.h"
#include "AppModel/TrackSet/TrackSet.h"

/**
 * MidiInputManager handles MIDI input device management.
 *
 * Responsibilities:
 * - Manage MIDI input device
 * - Provide port selection
 * - Provide access to device for polling
 * - Manage MIDI event logging callback
 *
 * Note: CheckMidiInQueue() remains in AppModel as it's orchestration logic
 * that coordinates SoundBank, Transport, and RecordingSession.
 *
 * Usage:
 *   MidiInputManager inputManager;
 *   auto ports = inputManager.GetPortNames();
 *   inputManager.SetInputPort(0);
 *   inputManager.SetLogCallback([](const TimedMidiEvent& event) {
 *       LogPanel::Display(event);
 *   });
 */
class MidiInputManager
{
public:
	MidiInputManager();

	/**
	 * Get list of available MIDI input port names
	 * @return Vector of port names
	 */
	std::vector<std::string> GetPortNames() const;

	/**
	 * Set active MIDI input port
	 * @param portIndex Index of port to use (from GetPortNames)
	 */
	void SetInputPort(int portIndex);

	/**
	 * Get reference to MIDI input device
	 * Used by AppModel::CheckMidiInQueue() to poll for messages
	 * @return Reference to MidiIn device
	 */
	MidiIn& GetDevice();

	/**
	 * Callback signature for MIDI event logging
	 * @param event The MIDI event with timestamp
	 */
	using MidiLogCallback = std::function<void(const TimedMidiEvent&)>;

	/**
	 * Set callback for MIDI event logging
	 * Called when MIDI messages are received
	 * @param callback Function to call for each MIDI event
	 */
	void SetLogCallback(MidiLogCallback callback);

	/**
	 * Get current log callback
	 * Used by CheckMidiInQueue to log events
	 * @return Current log callback (may be null)
	 */
	const MidiLogCallback& GetLogCallback() const;

private:
	std::shared_ptr<MidiIn> mMidiIn;
	MidiLogCallback mLogCallback;
};
