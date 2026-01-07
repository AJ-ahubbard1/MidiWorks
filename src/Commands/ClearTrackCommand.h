#pragma once
#include "Command.h"
#include "AppModel/TrackSet/TrackSet.h"
#include <algorithm>
#include <vector>
#include <map>
using namespace MidiInterface;

/// <summary>
/// Command to clear notes from 1 track.
/// Handles batch deletion efficiently while maintaining undo capability.
/// </summary>
class ClearTrackCommand : public Command
{
public:

	ClearTrackCommand(Track& track, int trackNumber)
		: mTrack(track)
		, mTrackNumber(trackNumber)
	{
	}

	void Execute() override
	{
		// Backup all events before clearing
		mBackup = mTrack;

		// Clear the track
		mTrack.clear();
	}

	void Undo() override
	{
		// Restore backed-up events
		mTrack = mBackup;
	}

	std::string GetDescription() const override
	{
		return "Clear Track " + std::to_string(mTrackNumber + 1);
	}

private:
	Track& mTrack;
	int mTrackNumber;
	Track mBackup;  // Backup storage for undo
};
