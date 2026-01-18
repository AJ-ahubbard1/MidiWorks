# Error Handling Implementation Guide

This guide walks you through implementing comprehensive error handling for MidiWorks.

**Phases:**
- **Phase 1:** File I/O Errors (ProjectManager) - ~75 lines
- **Phase 2:** MIDI Device Errors (SoundBank/MidiOut) - ~100 lines
- **Phase 3:** Optional Enhancements (Logging, Bounds Checking) - ~60 lines

---

# Phase 1: File I/O Error Handling

This phase handles errors from file operations (save/load/import/export).

---

## Overview

**Goal:** Show user-friendly error dialogs instead of silent failures when file operations fail.

**Architecture:**
```
MainFrame ──sets──> AppModel.ErrorCallback ──displays──> wxMessageBox
                         │
                    wires in constructor
                         │
                         v
              ProjectManager.ErrorCallback ──reports──> SaveProject/LoadProject errors
```

---

## Step 1: Add ErrorCallback to AppModel

### File: `src/AppModel/AppModel.h`

Add these after the existing includes (around line 10):

```cpp
// Error handling callback types
enum class ErrorLevel { Info, Warning, Error };
using ErrorCallback = std::function<void(const std::string& title,
                                          const std::string& message,
                                          ErrorLevel level)>;
```

Add these public methods (in the public section):

```cpp
// Error Handling
void SetErrorCallback(ErrorCallback callback);
void ReportError(const std::string& title, const std::string& msg, ErrorLevel level);
```

Add this private member (in the private section with other members):

```cpp
ErrorCallback mErrorCallback;
```

---

### File: `src/AppModel/AppModel.cpp`

Add these implementations at the end of the file:

```cpp
void AppModel::SetErrorCallback(ErrorCallback callback)
{
    mErrorCallback = callback;
}

void AppModel::ReportError(const std::string& title, const std::string& msg, ErrorLevel level)
{
    if (mErrorCallback)
    {
        mErrorCallback(title, msg, level);
    }
}
```

---

## Step 2: Add ErrorCallback to ProjectManager

### File: `src/AppModel/ProjectManager/ProjectManager.h`

Add this callback type definition (around line 137, after DirtyStateCallback):

```cpp
/**
 * Callback signature for error reporting
 * @param title Error dialog title
 * @param message Error message to display
 */
using ErrorCallback = std::function<void(const std::string& title, const std::string& message)>;

/**
 * Set callback to report errors to UI
 * @param callback Function to call when an error occurs
 */
void SetErrorCallback(ErrorCallback callback);
```

Add this private member (around line 186, with other callbacks):

```cpp
ErrorCallback mErrorCallback;
```

---

### File: `src/AppModel/ProjectManager/ProjectManager.cpp`

Add this implementation (around line 302, after other setters):

```cpp
void ProjectManager::SetErrorCallback(ErrorCallback callback)
{
    mErrorCallback = callback;
}
```

---

## Step 3: Wire ProjectManager Errors to AppModel

### File: `src/AppModel/AppModel.cpp`

In the constructor, add this after the existing callback wiring (around line 19):

```cpp
// Wire ProjectManager errors to central error callback
mProjectManager.SetErrorCallback([this](const std::string& title, const std::string& msg) {
    ReportError(title, msg, ErrorLevel::Error);
});
```

---

## Step 4: Update ProjectManager to Report Errors

### File: `src/AppModel/ProjectManager/ProjectManager.cpp`

#### SaveProject() - Update lines 94-110:

Replace:
```cpp
if (!file.is_open()) {
    return false;
}
```

With:
```cpp
if (!file.is_open()) {
    if (mErrorCallback) {
        mErrorCallback("Save Failed", "Could not open file for writing: " + filepath);
    }
    return false;
}
```

Replace the catch block:
```cpp
catch (const std::exception& e) {
    // Log error or show dialog
    return false;
}
```

With:
```cpp
catch (const std::exception& e) {
    if (mErrorCallback) {
        mErrorCallback("Save Failed", std::string("Error saving project: ") + e.what());
    }
    return false;
}
```

---

#### LoadProject() - Update lines 117-213:

Replace:
```cpp
if (!file.is_open()) {
    return false;
}
```

With:
```cpp
if (!file.is_open()) {
    if (mErrorCallback) {
        mErrorCallback("Load Failed", "Could not open file: " + filepath);
    }
    return false;
}
```

Replace the catch blocks (lines 198-213):
```cpp
catch (json::parse_error& e) {
    if (mErrorCallback) {
        mErrorCallback("Load Failed", "Project file is corrupted or not valid JSON.");
    }
    return false;
}
catch (json::type_error& e) {
    if (mErrorCallback) {
        mErrorCallback("Load Failed", "Project file has invalid data format.");
    }
    return false;
}
catch (json::out_of_range& e) {
    if (mErrorCallback) {
        mErrorCallback("Load Failed", "Project file is missing required data.");
    }
    return false;
}
catch (const std::exception& e) {
    if (mErrorCallback) {
        mErrorCallback("Load Failed", std::string("Error loading project: ") + e.what());
    }
    return false;
}
```

---

#### ExportMIDI() - Update lines 344-348:

Replace:
```cpp
catch (const std::exception& e)
{
    // Log error or show dialog
    return false;
}
```

With:
```cpp
catch (const std::exception& e)
{
    if (mErrorCallback) {
        mErrorCallback("Export Failed", std::string("Error exporting MIDI file: ") + e.what());
    }
    return false;
}
```

Also add file open check after `midifile.write()` fails (optional - midifile library may throw):
```cpp
// After line 341
midifile.write(filepath);
// Note: midifile.write() may throw on failure, caught by exception handler
```

---

#### ImportMIDI() - Update lines 356-359 and 521-525:

Replace file read failure:
```cpp
if (!midifile.read(filepath))
{
    return false;
}
```

With:
```cpp
if (!midifile.read(filepath))
{
    if (mErrorCallback) {
        mErrorCallback("Import Failed", "Could not read MIDI file: " + filepath);
    }
    return false;
}
```

Replace catch block:
```cpp
catch (const std::exception& e)
{
    // Log error or show dialog
    return false;
}
```

With:
```cpp
catch (const std::exception& e)
{
    if (mErrorCallback) {
        mErrorCallback("Import Failed", std::string("Error importing MIDI file: ") + e.what());
    }
    return false;
}
```

---

## Step 5: Wire AppModel Error Callback in MainFrame

### File: `src/MainFrame/MainFrame.cpp`

In `CreateCallbackFunctions()` (around line 100, after other callbacks):

```cpp
// Error handling callback - displays errors to user
mAppModel->SetErrorCallback([this](const std::string& title,
                                    const std::string& msg,
                                    ErrorLevel level) {
    long style = wxOK;
    switch (level) {
        case ErrorLevel::Info:    style |= wxICON_INFORMATION; break;
        case ErrorLevel::Warning: style |= wxICON_WARNING; break;
        case ErrorLevel::Error:   style |= wxICON_ERROR; break;
    }
    wxMessageBox(msg, title, style);
});
```

---

## Testing Checklist

After implementing, test these scenarios:

- [ ] **Save to read-only location** (e.g., `C:\Windows\test.mwp`) - should show "Save Failed" dialog
- [ ] **Load non-existent file** - should show "Load Failed" dialog
- [ ] **Load corrupted JSON** - create a .mwp file with invalid JSON, should show "corrupted" message
- [ ] **Load wrong format** - rename a .txt file to .mwp and load it
- [ ] **Export MIDI to read-only location** - should show "Export Failed" dialog
- [ ] **Import invalid MIDI file** - rename a .txt file to .mid and import it

All dialogs should show user-friendly messages, not raw exception text.

---

## Summary of Changes

| File | Lines Changed | What |
|------|---------------|------|
| `AppModel.h` | +10 | ErrorLevel enum, ErrorCallback type, methods, member |
| `AppModel.cpp` | +15 | SetErrorCallback(), ReportError(), constructor wiring |
| `ProjectManager.h` | +8 | ErrorCallback type, setter, member |
| `ProjectManager.cpp` | +30 | SetErrorCallback(), error messages in all catch blocks |
| `MainFrame.cpp` | +12 | Wire error callback to wxMessageBox |

**Total:** ~75 lines of code for complete file I/O error handling.

---

# Phase 2: MIDI Device Error Handling

This phase handles MIDI device disconnection and startup without devices.

**Goal:** App continues running gracefully when MIDI devices fail or are missing.

**Current Problem:** The error handler in `MidiError.h` calls `exit(EXIT_FAILURE)` on fatal errors, causing immediate crash with no user feedback.

---

## Step 1: Replace Fatal Exit with Error State

### File: `src/RtMidiWrapper/MidiDevice/MidiError.h`

Replace the entire error callback (lines 6-26):

```cpp
#pragma once
#include <iostream>
#include <string>
#include <functional>
#include "RtMidi/RtMidi.h"

// Global error state (simple approach)
inline bool g_midiErrorOccurred = false;
inline std::string g_lastMidiError = "";

// Error callback for UI notification
using MidiErrorCallback = std::function<void(const std::string& message)>;
inline MidiErrorCallback g_midiErrorCallback = nullptr;

inline void SetMidiErrorCallback(MidiErrorCallback callback)
{
    g_midiErrorCallback = callback;
}

static void midiErrorCallback(RtMidiError::Type type, const std::string& errorText, void* userData)
{
    std::cerr << "[RtMidi Error] Type: " << static_cast<int>(type)
              << " | Message: " << errorText << std::endl;

    g_lastMidiError = errorText;

    if (type == RtMidiError::WARNING || type == RtMidiError::DEBUG_WARNING)
    {
        // Warnings - continue but log
        std::cerr << "Warning only - continuing...\n";
    }
    else
    {
        // Fatal error - set error state, notify UI, but DON'T exit
        g_midiErrorOccurred = true;

        if (g_midiErrorCallback)
        {
            g_midiErrorCallback("MIDI device error: " + errorText);
        }

        // Previously: exit(EXIT_FAILURE);
        // Now: gracefully continue without MIDI
    }
}

inline void ClearMidiError()
{
    g_midiErrorOccurred = false;
    g_lastMidiError = "";
}

inline bool HasMidiError()
{
    return g_midiErrorOccurred;
}
```

---

## Step 2: Add Error Callback to SoundBank

### File: `src/AppModel/SoundBank/SoundBank.h`

Add callback type and setter (in public section):

```cpp
// Error callback for MIDI device issues
using ErrorCallback = std::function<void(const std::string& title, const std::string& message)>;
void SetErrorCallback(ErrorCallback callback);
```

Add private member:

```cpp
ErrorCallback mErrorCallback;
bool mDeviceErrorShown = false;  // Prevent spam - show error only once
```

---

### File: `src/AppModel/SoundBank/SoundBank.cpp`

Add setter implementation:

```cpp
void SoundBank::SetErrorCallback(ErrorCallback callback)
{
    mErrorCallback = callback;
}
```

Add helper method to report device errors (only once per session):

```cpp
void SoundBank::ReportDeviceError(const std::string& message)
{
    if (!mDeviceErrorShown && mErrorCallback)
    {
        mErrorCallback("MIDI Device Error", message);
        mDeviceErrorShown = true;
    }
}

void SoundBank::ClearDeviceError()
{
    mDeviceErrorShown = false;
}
```

---

## Step 3: Wrap MIDI Send Operations

### File: `src/AppModel/SoundBank/SoundBank.cpp`

Update `PlayNote()` (around line 120):

```cpp
void SoundBank::PlayNote(ubyte pitch, ubyte velocity, ubyte channel)
{
    if (!mMidiOut) return;

    // Check if device is still valid
    if (!mMidiOut->isPortOpen())
    {
        ReportDeviceError("MIDI output device disconnected. Reconnect device and restart MidiWorks.");
        return;
    }

    try
    {
        mMidiOut->sendMessage(MidiMessage::NoteOn(pitch, velocity, channel));
    }
    catch (const RtMidiError& e)
    {
        ReportDeviceError("Failed to send MIDI message: " + std::string(e.what()));
    }
}
```

Update `StopNote()` similarly (around line 126):

```cpp
void SoundBank::StopNote(ubyte pitch, ubyte channel)
{
    if (!mMidiOut) return;

    if (!mMidiOut->isPortOpen())
    {
        return;  // Silent fail - error already shown
    }

    try
    {
        mMidiOut->sendMessage(MidiMessage::NoteOff(pitch, 0, channel));
    }
    catch (const RtMidiError& e)
    {
        ReportDeviceError("Failed to send MIDI message: " + std::string(e.what()));
    }
}
```

Update `PlayMessages()` (around line 104):

```cpp
void SoundBank::PlayMessages(std::vector<MidiMessage> msgs)
{
    if (!mMidiOut) return;

    if (!mMidiOut->isPortOpen())
    {
        ReportDeviceError("MIDI output device disconnected.");
        return;
    }

    try
    {
        for (auto& m : msgs)
        {
            if (ShouldChannelPlay(m.getChannel()))
            {
                mMidiOut->sendMessage(m);
            }
        }
    }
    catch (const RtMidiError& e)
    {
        ReportDeviceError("Failed to send MIDI messages: " + std::string(e.what()));
    }
}
```

---

## Step 4: Add isPortOpen() to MidiOut

### File: `src/RtMidiWrapper/MidiDevice/MidiOut.h`

Add this method to check if device is still connected:

```cpp
bool isPortOpen() const
{
    return mPlayer && mPlayer->isPortOpen();
}
```

---

## Step 5: Handle No MIDI Devices on Startup

### File: `src/AppModel/SoundBank/SoundBank.cpp`

Update constructor to handle no devices gracefully:

```cpp
SoundBank::SoundBank()
{
    // ... existing initialization ...

    mMidiOut = std::make_shared<MidiOut>();

    // Check if we have any real MIDI devices
    if (mMidiOut->getPortCount() == 0)
    {
        // No physical MIDI devices - will use virtual port
        // Flag this so we can notify user on first playback attempt
        mNoPhysicalDevices = true;
    }
}
```

Add member to track this:

```cpp
// In SoundBank.h private section:
bool mNoPhysicalDevices = false;
```

Notify user on first playback if no devices:

```cpp
void SoundBank::PlayNote(ubyte pitch, ubyte velocity, ubyte channel)
{
    if (mNoPhysicalDevices && !mDeviceErrorShown)
    {
        if (mErrorCallback)
        {
            mErrorCallback("No MIDI Devices",
                "No MIDI output devices found. You can still compose and save projects, "
                "but you won't hear audio until a MIDI device is connected.");
        }
        mDeviceErrorShown = true;
    }

    // ... rest of PlayNote ...
}
```

---

## Step 6: Wire SoundBank Errors to AppModel

### File: `src/AppModel/AppModel.cpp`

In constructor, add after ProjectManager wiring:

```cpp
// Wire SoundBank errors to central error callback
mSoundBank.SetErrorCallback([this](const std::string& title, const std::string& msg) {
    ReportError(title, msg, ErrorLevel::Warning);  // Warning, not Error - app still works
});
```

---

## Phase 2 Testing Checklist

- [ ] **Unplug MIDI device during playback** - should show warning dialog once, playback stops gracefully
- [ ] **Start app with no MIDI devices** - should show info dialog on first play attempt
- [ ] **Reconnect device** - after restart, should work again
- [ ] **App continues running** - can still save/load projects, edit notes, just no sound

---

## Phase 2 Summary of Changes

| File | Lines Changed | What |
|------|---------------|------|
| `MidiError.h` | ~50 (rewrite) | Replace exit() with error state + callback |
| `SoundBank.h` | +10 | ErrorCallback type, setter, error state flags |
| `SoundBank.cpp` | +50 | SetErrorCallback(), ReportDeviceError(), try-catch wrappers |
| `MidiOut.h` | +5 | isPortOpen() method |
| `AppModel.cpp` | +5 | Wire SoundBank errors to central callback |

**Total:** ~100 lines of code for MIDI device error handling.

---

# Phase 3: Optional Enhancements

These are nice-to-have improvements for debugging and robustness.

---

## Enhancement 1: Basic Logging System

Create a simple logger to help debug user-reported issues.

### File: `src/Logger/Logger.h` (new file)

```cpp
#pragma once
#include <fstream>
#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

class Logger
{
public:
    static void Log(const std::string& message)
    {
        std::ofstream log("MidiWorks.log", std::ios::app);
        if (log.is_open())
        {
            log << GetTimestamp() << " [INFO] " << message << "\n";
        }
    }

    static void Warning(const std::string& message)
    {
        std::ofstream log("MidiWorks.log", std::ios::app);
        if (log.is_open())
        {
            log << GetTimestamp() << " [WARNING] " << message << "\n";
        }
    }

    static void Error(const std::string& message)
    {
        std::ofstream log("MidiWorks.log", std::ios::app);
        if (log.is_open())
        {
            log << GetTimestamp() << " [ERROR] " << message << "\n";
        }
    }

    static void ClearLog()
    {
        std::ofstream log("MidiWorks.log", std::ios::trunc);
    }

private:
    static std::string GetTimestamp()
    {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
};
```

### Usage in Error Callbacks

Update MainFrame's error callback to also log:

```cpp
#include "Logger/Logger.h"

// In CreateCallbackFunctions():
mAppModel->SetErrorCallback([this](const std::string& title,
                                    const std::string& msg,
                                    ErrorLevel level) {
    // Log to file
    switch (level) {
        case ErrorLevel::Info:    Logger::Log(title + ": " + msg); break;
        case ErrorLevel::Warning: Logger::Warning(title + ": " + msg); break;
        case ErrorLevel::Error:   Logger::Error(title + ": " + msg); break;
    }

    // Show to user
    long style = wxOK;
    switch (level) {
        case ErrorLevel::Info:    style |= wxICON_INFORMATION; break;
        case ErrorLevel::Warning: style |= wxICON_WARNING; break;
        case ErrorLevel::Error:   style |= wxICON_ERROR; break;
    }
    wxMessageBox(msg, title, style);
});
```

### Clear Log on Startup

In `App.cpp` or `MainFrame.cpp` initialization:

```cpp
#include "Logger/Logger.h"

// At app start:
Logger::ClearLog();
Logger::Log("MidiWorks started");
```

---

## Enhancement 2: Track Index Bounds Checking

Prevent crashes from invalid track indices.

### File: `src/AppModel/TrackSet/TrackSet.h`

Update `GetTrack()` to validate bounds:

```cpp
Track& GetTrack(int index)
{
    if (index < 0 || index >= CHANNEL_COUNT)
    {
        // Log the error
        Logger::Error("Invalid track index: " + std::to_string(index) +
                      " (valid range: 0-" + std::to_string(CHANNEL_COUNT - 1) + ")");

        // Return first track as safe fallback
        return mTracks[0];
    }
    return mTracks[index];
}
```

Or use `.at()` for automatic bounds checking with exceptions:

```cpp
Track& GetTrack(int index)
{
    try
    {
        return mTracks.at(index);
    }
    catch (const std::out_of_range& e)
    {
        Logger::Error("Track index out of range: " + std::to_string(index));
        return mTracks.at(0);  // Safe fallback
    }
}
```

---

## Enhancement 3: Startup Validation

Add a startup check that logs system info for debugging.

### File: `src/MainFrame/MainFrame.cpp`

In constructor or OnInit:

```cpp
void MainFrame::LogStartupInfo()
{
    Logger::Log("=== MidiWorks Startup ===");
    Logger::Log("Version: 1.1");

    // Log MIDI device info
    auto midiOut = mAppModel->GetSoundBank().GetMidiOutDevice();
    int portCount = midiOut->getPortCount();
    Logger::Log("MIDI output ports found: " + std::to_string(portCount));

    for (int i = 0; i < portCount; i++)
    {
        Logger::Log("  Port " + std::to_string(i) + ": " + midiOut->getPortName(i));
    }

    Logger::Log("=========================");
}
```

---

## Phase 3 Summary of Changes

| File | Lines Changed | What |
|------|---------------|------|
| `Logger/Logger.h` | ~50 (new) | Simple file logger |
| `MainFrame.cpp` | +15 | Log errors + startup info |
| `TrackSet.h` | +10 | Bounds checking on GetTrack() |

**Total:** ~60 lines of code for logging and bounds checking.

---

# Complete Implementation Summary

| Phase | Focus | Lines | Priority |
|-------|-------|-------|----------|
| Phase 1 | File I/O Errors | ~75 | **Critical** - Do first |
| Phase 2 | MIDI Device Errors | ~100 | **High** - Do second |
| Phase 3 | Logging & Bounds | ~60 | **Optional** - Nice to have |

**Total:** ~235 lines of code for comprehensive error handling.

**Recommended Order:**
1. Implement Phase 1, test thoroughly
2. Implement Phase 2, test with device unplugging
3. Add Phase 3 if you want debugging support

---

# Quick Reference: Files Modified

```
src/
├── AppModel/
│   ├── AppModel.h          [Phase 1] ErrorLevel, ErrorCallback, ReportError()
│   ├── AppModel.cpp        [Phase 1+2] Implementation + wiring
│   ├── ProjectManager/
│   │   ├── ProjectManager.h    [Phase 1] ErrorCallback
│   │   └── ProjectManager.cpp  [Phase 1] Error messages in catch blocks
│   ├── SoundBank/
│   │   ├── SoundBank.h     [Phase 2] ErrorCallback, error flags
│   │   └── SoundBank.cpp   [Phase 2] Try-catch wrappers, ReportDeviceError()
│   └── TrackSet/
│       └── TrackSet.h      [Phase 3] Bounds checking
├── RtMidiWrapper/
│   └── MidiDevice/
│       ├── MidiError.h     [Phase 2] Replace exit() with error state
│       └── MidiOut.h       [Phase 2] isPortOpen()
├── MainFrame/
│   └── MainFrame.cpp       [Phase 1+3] Wire callbacks, logging
└── Logger/
    └── Logger.h            [Phase 3] New file - simple logger
```
