# AppModel Encapsulation Refactoring - Option B

## Overview
This refactoring improves encapsulation by hiding implementation details and providing clean, focused interfaces.

---

## 1. MidiIn Encapsulation

### Current Problem
```cpp
// External code can access entire MidiIn interface
mAppModel->mMidiIn->getPortNames();
mAppModel->mMidiIn->changePort(p);
```

### Proposed Solution
Expose only the needed operations through AppModel's interface:

```cpp
// AppModel.h - Public interface
class AppModel {
public:
    // MIDI Input port management
    std::vector<std::string> GetMidiInputPortNames() const;
    void SetMidiInputPort(int portIndex);
    int GetCurrentMidiInputPort() const;

private:
    std::shared_ptr<MidiIn> mMidiIn;  // Now private!
};
```

```cpp
// AppModel.cpp - Implementation
std::vector<std::string> AppModel::GetMidiInputPortNames() const
{
    return mMidiIn->getPortNames();
}

void AppModel::SetMidiInputPort(int portIndex)
{
    mMidiIn->changePort(portIndex);
}

int AppModel::GetCurrentMidiInputPort() const
{
    return mMidiIn->getCurrentPort();  // Assuming this method exists
}
```

### Updated Call Sites
```cpp
// MidiSettings.h - BEFORE
auto& inPorts = mAppModel->mMidiIn->getPortNames();
mAppModel->mMidiIn->changePort(p);

// MidiSettings.h - AFTER
auto inPorts = mAppModel->GetMidiInputPortNames();
mAppModel->SetMidiInputPort(p);
```

**Benefits:**
- MidiIn is now an implementation detail
- Can swap MIDI library without changing external code
- Clear, documented interface for MIDI operations
- No accidental access to internal MidiIn methods

---

## 2. Logging System Redesign

### Current Problem
```cpp
// Tight coupling - MainFrame polls for log messages
if (mAppModel->mUpdateLog) {
    mLogPanel->LogMidiEvent(mAppModel->mLogMessage);
    mAppModel->mUpdateLog = false;
}
```

### Proposed Solution A: Callback Pattern (Recommended)
```cpp
// AppModel.h
class AppModel {
public:
    // Callback signature for log messages
    using LogCallback = std::function<void(const TimedMidiEvent&)>;

    // Register a callback to be notified of new MIDI events
    void SetLogCallback(LogCallback callback);

private:
    LogCallback mLogCallback;
    // Remove: mLogMessage, mUpdateLog
};
```

```cpp
// AppModel.cpp - When a MIDI message arrives
void AppModel::CheckMidiInQueue()
{
    if (!mMidiIn->checkForMessage()) return;

    MidiMessage mm = mMidiIn->getMessage();
    uint64_t currentTick = mTransport.GetCurrentTick();

    // Notify callback instead of setting flags
    if (mLogCallback) {
        mLogCallback({mm, currentTick});
    }

    // ... rest of processing
}
```

```cpp
// MainFrame.cpp - Register callback during initialization
void MainFrame::CreateDockablePanes()
{
    // ... create panels

    // Register log callback
    mAppModel->SetLogCallback([this](const TimedMidiEvent& event) {
        if (mLogPanel) {
            mLogPanel->LogMidiEvent(event);
        }
    });
}
```

```cpp
// MainFrame.cpp - OnTimer() is now simpler
void MainFrame::OnTimer(wxTimerEvent& event)
{
    mAppModel->Update();
    mTransportPanel->UpdateDisplay();
    mMidiCanvasPanel->Update();
    // No more log polling needed!
}
```

**Benefits:**
- Removes polling logic from MainFrame
- AppModel doesn't need to know about MainFrame or logging
- Cleaner separation of concerns
- Log events are delivered immediately, not on next timer tick
- Extensible: can add multiple log listeners

---

### Proposed Solution B: Pull-Based with Better API
If you prefer to keep the polling pattern:

```cpp
// AppModel.h
class AppModel {
public:
    // Returns true if a new log message is available
    bool PollLogMessage(TimedMidiEvent& outMessage);

private:
    TimedMidiEvent mLogMessage;
    bool mHasNewLogMessage = false;
};
```

```cpp
// AppModel.cpp
bool AppModel::PollLogMessage(TimedMidiEvent& outMessage)
{
    if (!mHasNewLogMessage) {
        return false;
    }

    outMessage = mLogMessage;
    mHasNewLogMessage = false;
    return true;
}
```

```cpp
// MainFrame.cpp
void MainFrame::OnTimer(wxTimerEvent& event)
{
    mAppModel->Update();

    TimedMidiEvent logEvent;
    if (mAppModel->PollLogMessage(logEvent)) {
        mLogPanel->LogMidiEvent(logEvent);
    }

    mTransportPanel->UpdateDisplay();
    mMidiCanvasPanel->Update();
}
```

**Benefits:**
- Encapsulates the flag pattern
- Atomic read-and-clear operation
- Still maintains current architecture flow

---

## 3. Metronome Setting

### Current Problem
```cpp
// TransportPanel directly mutates AppModel state
mModel->mMetronomeEnabled = mMetronomeCheckBox->GetValue();
if (mModel->mMetronomeEnabled) { /* ... */ }
```

### Proposed Solution
Simple getter/setter pattern (this is appropriate here):

```cpp
// AppModel.h
class AppModel {
public:
    bool IsMetronomeEnabled() const;
    void SetMetronomeEnabled(bool enabled);

private:
    bool mMetronomeEnabled = true;
};
```

```cpp
// AppModel.cpp
bool AppModel::IsMetronomeEnabled() const
{
    return mMetronomeEnabled;
}

void AppModel::SetMetronomeEnabled(bool enabled)
{
    mMetronomeEnabled = enabled;
    // Future: Could add logic here like "if changing from on to off, stop current click"
}
```

```cpp
// TransportPanel.h - Updated usage
mMetronomeCheckBox->SetValue(mModel->IsMetronomeEnabled());
mModel->SetMetronomeEnabled(mMetronomeCheckBox->GetValue());
```

**Benefits:**
- Future-proof: can add validation or side effects later
- Consistent with other getters/setters in AppModel
- Self-documenting API

---

## Summary of Changes

### Before
```cpp
class AppModel {
public:
    std::shared_ptr<MidiIn> mMidiIn;           // Full exposure
    TimedMidiEvent mLogMessage;                 // Direct access
    bool mUpdateLog = false;                    // Flag exposed
    bool mMetronomeEnabled = true;              // Direct mutation
};
```

### After (Recommended - Callback Pattern)
```cpp
class AppModel {
public:
    // MIDI Input
    std::vector<std::string> GetMidiInputPortNames() const;
    void SetMidiInputPort(int portIndex);
    int GetCurrentMidiInputPort() const;

    // Logging (push-based)
    using LogCallback = std::function<void(const TimedMidiEvent&)>;
    void SetLogCallback(LogCallback callback);

    // Metronome
    bool IsMetronomeEnabled() const;
    void SetMetronomeEnabled(bool enabled);

private:
    std::shared_ptr<MidiIn> mMidiIn;
    LogCallback mLogCallback;
    bool mMetronomeEnabled = true;
};
```

### Alternative After (Pull-Based Pattern)
```cpp
class AppModel {
public:
    // MIDI Input
    std::vector<std::string> GetMidiInputPortNames() const;
    void SetMidiInputPort(int portIndex);

    // Logging (pull-based)
    bool PollLogMessage(TimedMidiEvent& outMessage);

    // Metronome
    bool IsMetronomeEnabled() const;
    void SetMetronomeEnabled(bool enabled);

private:
    std::shared_ptr<MidiIn> mMidiIn;
    TimedMidiEvent mLogMessage;
    bool mHasNewLogMessage = false;
    bool mMetronomeEnabled = true;
};
```

---

## Files That Need Updates

### Core Changes
- `src/AppModel/AppModel.h` - Add new public methods, move members to private
- `src/AppModel/AppModel.cpp` - Implement new methods

### Call Site Updates
- `src/Panels/MidiSettings.h` - Update MidiIn access (2 locations)
- `src/Panels/TransportPanel.h` - Update metronome access (2 locations)
- `src/MainFrame/MainFrame.cpp` - Update logging system (2-3 locations)

### Total Impact
- **Files modified:** 5
- **Breaking changes:** None (only internal AppModel users affected)
- **Estimated effort:** 30-45 minutes

---

## Recommendation

I recommend the **Callback Pattern** for logging because:
1. More event-driven and reactive
2. Removes polling logic
3. Better separation of concerns
4. Easier to extend (multiple log destinations)
5. More modern C++ style

The other changes (MidiIn encapsulation and metronome getter/setter) are straightforward improvements with clear benefits.
