// AppModel.h
#pragma once
#include <unordered_map>
#include <memory>
#include <chrono>
#include <vector>
#include "SoundBank.h"
#include "Transport.h"
#include "TrackSet.h"
#include "Commands/Command.h"

class AppModel
{

public:
	std::shared_ptr<MidiIn> mMidiIn;
	TimedMidiEvent			mLogMessage{MidiMessage::NoteOff(0), 0};
	bool					mUpdateLog = false;
	bool					mMetronomeEnabled = true;  // Metronome on by default

	AppModel();
	void InitializeMetronome();
	// Called inside of MainFrame::OnTimer event
	void Update();
	/*  Checks for Midi In Messages
		If a SoundBank Channel is active, the message will playback.
		If a SoundBank Channel is set to record: the message will be pushed to the mRecordingBuffer.
		This buffer is used to temporarily store the midi messages during recording, 
		when finished recording, the buffer is added to the track and sorted by timestamp. 
	*/
	void CheckMidiInQueue();
	
    SoundBank& GetSoundBank(); 
	Transport& GetTransport(); 
	TrackSet& GetTrackSet(); 
	Track& GetRecordingBuffer(); 
	Track& GetTrack(ubyte c); 

	// Command Pattern - Undo/Redo System
	void ExecuteCommand(std::unique_ptr<Command> cmd);
	void Undo();
	void Redo();
	bool CanUndo() const; 
	bool CanRedo() const; 
	const std::vector<std::unique_ptr<Command>>& GetUndoStack() const; 
	const std::vector<std::unique_ptr<Command>>& GetRedoStack() const; 
	void ClearUndoHistory();

private:
	static const size_t MAX_UNDO_STACK_SIZE = 50;  // Limit to last 50 actions
	std::chrono::steady_clock::time_point mLastTick = std::chrono::steady_clock::now();
	SoundBank	mSoundBank;
	Transport	mTransport;
	TrackSet	mTrackSet;
	Track		mRecordingBuffer;

	// Command Pattern - Undo/Redo stacks
	std::vector<std::unique_ptr<Command>> mUndoStack;
	std::vector<std::unique_ptr<Command>> mRedoStack;

	uint64_t GetDeltaTimeMs();
	bool IsMusicalMessage(const MidiMessage& msg);
	void PlayMessages(std::vector<MidiMessage> msgs);
	void PlayMetronomeClick(bool isDownbeat);
	void SilenceAllChannels();
};

