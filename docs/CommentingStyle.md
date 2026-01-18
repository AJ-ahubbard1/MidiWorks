# Commenting Style Guide

## File Comments

Add the filename at the top of each file using `//`:

```cpp
// SoundBank.h
#pragma once
```

## Class Documentation

Use `///` for class documentation to enable IntelliSense tooltips. Include:
- Brief description
- Responsibilities (bullet list)
- Usage example

```cpp
/// SoundBank manages MIDI output and channel state.
///
/// Responsibilities:
/// - Manage MIDI output device
/// - Track channel settings (program, volume, mute, solo, record)
/// - Provide playback helpers for notes, messages, and metronome
///
/// Usage:
///   SoundBank soundBank;
///   soundBank.GetChannel(0).programNumber = 25;
///   soundBank.ApplyChannelSettings();
class SoundBank
{
```

## Method Grouping

Use `//` for section headers to group related methods. These are ignored by IntelliSense:

```cpp
// Channel Management

/// Apply all channel settings to MIDI device
void ApplyChannelSettings();

/// Get a channel by index (0-14)
MidiChannel& GetChannel(ubyte ch) { return mChannels[ch]; }

// MIDI Playback

/// Play a single note
void PlayNote(ubyte pitch, ubyte velocity, ubyte channel);
```

## Method Documentation

Use `///` for method documentation (enables IntelliSense):

```cpp
/// Check if playback should loop back (loop enabled and past end tick)
bool ShouldLoopBack(uint64_t currentTick) const;
```

### @param and @return

Only include `@param` and `@return` when the meaning isn't obvious from the method name.

**Skip when obvious:**
```cpp
/// Check if currently playing
bool IsPlaying() const;  // No @return needed - obvious from name

/// Get current tick position
uint64_t GetCurrentTick() const;  // No @return needed

/// Set the velocity for preview notes
void SetPreviewVelocity(ubyte velocity);  // No @param needed
```

**Include when it adds value:**
```cpp
/// Find all notes overlapping a region
/// @param trackIndex Specific track to search, or -1 for all tracks
std::vector<NoteLocation> FindNotesInRegion(..., int trackIndex = -1) const;

/// Trigger drum pad via keyboard
/// @return Column index if pad enabled during loop playback, -1 otherwise
int TriggerDrumPad(int rowIndex);

/// Check if a channel should play based on mute/solo state
/// @param checkRecord If true, also requires record to be enabled
bool ShouldChannelPlay(const MidiChannel& channel, bool checkRecord = false) const;
```

## Code Organization

### Single-Line Methods
Place single-line methods in the header file, even if the line is long:

```cpp
Track& GetTrack(ubyte channelNumber) { return mTracks[channelNumber]; }
bool IsTrackEmpty(ubyte channelNumber) { return GetTrack(channelNumber).empty(); }
```

### Method Visibility
If a method has no references outside the class, move it to private.

### Definition Order
Keep method definitions in the `.cpp` file in the same order as their declarations in the header.

### Small Classes
Small classes with only simple inline methods don't require a separate `.cpp` file.
