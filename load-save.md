# Save/Load Implementation Guide

This document provides a complete reference for implementing save/load functionality in MidiWorks using nlohmann/json.

---

## Table of Contents
1. [Overview](#overview)
2. [Setup](#setup)
3. [Implementation Checklist](#implementation-checklist)
4. [Code Examples](#code-examples)
5. [Tips & Best Practices](#tips--best-practices)
6. [Testing](#testing)

---

## Overview

### What is nlohmann/json?

**nlohmann/json** (JSON for Modern C++) is the most popular JSON library for C++.

**Key Features:**
- 🌟 42,000+ stars on GitHub
- 📦 Single header file (zero dependencies)
- 🎯 Intuitive STL-like API
- 📘 Excellent documentation
- ✅ MIT License (free for commercial use)

**Links:**
- GitHub: https://github.com/nlohmann/json
- Documentation: https://json.nlohmann.me/
- Download: https://github.com/nlohmann/json/releases

### Why JSON for MidiWorks?

- ✅ Human-readable (easy debugging)
- ✅ Easy to extend format later
- ✅ Built-in support for nested structures
- ✅ Simple to implement
- ✅ Files will be small (< 1MB typically)

---

## Setup

### Installation Steps

1. **Download the header file:**
   - Go to: https://github.com/nlohmann/json/releases
   - Download `json.hpp` (single header version)

2. **Add to project:**
   ```
   MidiWorks/
   └── src/
       └── external/
           └── json.hpp    <- Place here
   ```

3. **Update Visual Studio project:**
   - Right-click project → Properties
   - C/C++ → General → Additional Include Directories
   - Add: `$(ProjectDir)src\external`

4. **Include in your files:**
   ```cpp
   #include "external/json.hpp"
   using json = nlohmann::json;
   ```

---

## Implementation Checklist

### Phase 1: Setup & Basic Structure
- [ ] Download and add `json.hpp` to project
- [ ] Create `AppModel::SaveProject(const std::string& filepath)`
- [ ] Create `AppModel::LoadProject(const std::string& filepath)`
- [ ] Add dirty flag: `bool mIsDirty` to AppModel
- [ ] Add current project path: `std::string mCurrentProjectPath`

### Phase 2: Data Serialization

#### Transport Data
- [ ] Serialize `mTempo` (double)
- [ ] Serialize `mTimeSignatureNumerator` (int)
- [ ] Serialize `mTimeSignatureDenominator` (int)
- [ ] Optional: Serialize `mCurrentTick` (for restore position)

#### SoundBank Data (15 channels)
- [ ] Serialize channel array with:
  - [ ] `channelNumber` (ubyte)
  - [ ] `programNumber` (ubyte)
  - [ ] `volume` (ubyte)
  - [ ] `mute` (bool)
  - [ ] `solo` (bool)
  - [ ] `record` (bool)

#### TrackSet Data (15 tracks)
- [ ] Serialize each track's TimedMidiEvent vector:
  - [ ] `tick` (uint64_t)
  - [ ] `mData[0]` (ubyte) - MIDI status byte
  - [ ] `mData[1]` (ubyte) - MIDI data byte 1
  - [ ] `mData[2]` (ubyte) - MIDI data byte 2

#### Metadata
- [ ] Add project version number ("1.0")
- [ ] Add app version ("0.3")
- [ ] Optional: Created/modified timestamp

### Phase 3: User Interface
- [ ] Add File menu to MainFrame
  - [ ] New Project (Ctrl+N)
  - [ ] Open (Ctrl+O)
  - [ ] Save (Ctrl+S)
  - [ ] Save As (Ctrl+Shift+S)
- [ ] Add dirty flag tracking:
  - [ ] Mark dirty in `ExecuteCommand()`
  - [ ] Show asterisk (*) in title bar when dirty
- [ ] Add "Save changes?" dialog on window close
- [ ] Optional: Recent files list

### Phase 4: Error Handling
- [ ] Wrap `json::parse()` in try-catch
- [ ] Handle file not found
- [ ] Handle corrupt JSON
- [ ] Handle missing required fields
- [ ] Show error dialogs to user

### Phase 5: Testing
- [ ] Test save → verify JSON is readable
- [ ] Test load → verify all data restored correctly
- [ ] Test with empty tracks
- [ ] Test with complex multi-track project
- [ ] Test undo/redo cleared after load
- [ ] Test `ApplyChannelSettings()` called after load
- [ ] Test error cases (corrupt JSON, missing file)

---

## Code Examples

### Basic JSON Usage

#### Creating JSON Objects
```cpp
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// Simple object
json j = {
    {"tempo", 120.0},
    {"timeSignature", {4, 4}},
    {"metronomeEnabled", true}
};

// Arrays
json channels = json::array();
channels.push_back({"channelNumber", 0});
channels.push_back({"channelNumber", 1});

// Nested structures
json project = {
    {"version", "1.0"},
    {"transport", {
        {"tempo", 120.0}
    }},
    {"tracks", json::array()}
};
```

#### Reading JSON
```cpp
// Access values
double tempo = j["tempo"];  // 120.0
int numerator = j["timeSignature"][0];  // 4

// Safe access with default
double tempo = j.value("tempo", 120.0);

// Check if key exists
if (j.contains("tracks")) {
    // Process tracks
}

// Iterate arrays
for (const auto& channel : j["channels"]) {
    int num = channel["channelNumber"];
}
```

#### File I/O
```cpp
#include <fstream>

// Write to file (pretty-printed)
std::ofstream file("project.mwp");
file << j.dump(4);  // 4 = indent level

// Read from file
std::ifstream infile("project.mwp");
json loaded = json::parse(infile);
```

### Complete SaveProject Implementation

```cpp
bool AppModel::SaveProject(const std::string& filepath) {
    try {
        json project;

        // Metadata
        project["version"] = "1.0";
        project["appVersion"] = "0.3";

        // 1. Transport
        project["transport"] = {
            {"tempo", mTransport.mTempo},
            {"timeSignature", {
                mTransport.mTimeSignatureNumerator,
                mTransport.mTimeSignatureDenominator
            }},
            {"currentTick", mTransport.GetCurrentTick()}
        };

        // 2. Channels
        project["channels"] = json::array();
        auto channels = mSoundBank.GetAllChannels();
        for (const auto& ch : channels) {
            project["channels"].push_back({
                {"channelNumber", ch.channelNumber},
                {"programNumber", ch.programNumber},
                {"volume", ch.volume},
                {"mute", ch.mute},
                {"solo", ch.solo},
                {"record", ch.record}
            });
        }

        // 3. Tracks
        project["tracks"] = json::array();
        for (int i = 0; i < 15; i++) {
            Track& track = mTrackSet.GetTrack(i);
            json trackJson;
            trackJson["channel"] = i;
            trackJson["events"] = json::array();

            for (const auto& event : track) {
                trackJson["events"].push_back({
                    {"tick", event.tick},
                    {"midiData", {
                        event.mm.mData[0],
                        event.mm.mData[1],
                        event.mm.mData[2]
                    }}
                });
            }

            project["tracks"].push_back(trackJson);
        }

        // Write to file (pretty-printed with 4-space indent)
        std::ofstream file(filepath);
        if (!file.is_open()) {
            return false;
        }

        file << project.dump(4);
        file.close();

        // Update state
        mCurrentProjectPath = filepath;
        mIsDirty = false;

        return true;
    }
    catch (const std::exception& e) {
        // Log error or show dialog
        // std::cerr << "Save error: " << e.what() << '\n';
        return false;
    }
}
```

### Complete LoadProject Implementation

```cpp
bool AppModel::LoadProject(const std::string& filepath) {
    try {
        // Read file
        std::ifstream file(filepath);
        if (!file.is_open()) {
            return false;
        }

        json project = json::parse(file);
        file.close();

        // Check version compatibility
        std::string version = project.value("version", "1.0");
        // TODO: Handle different versions if needed

        // 1. Transport
        mTransport.mTempo = project["transport"]["tempo"];
        mTransport.mTimeSignatureNumerator = project["transport"]["timeSignature"][0];
        mTransport.mTimeSignatureDenominator = project["transport"]["timeSignature"][1];

        // Optional: Restore playback position
        if (project["transport"].contains("currentTick")) {
            // You'll need to add a setter for this
            // mTransport.SetCurrentTick(project["transport"]["currentTick"]);
        }

        // 2. Channels
        int channelIdx = 0;
        for (const auto& chJson : project["channels"]) {
            auto& ch = mSoundBank.GetChannel(channelIdx++);
            ch.programNumber = chJson["programNumber"];
            ch.volume = chJson["volume"];
            ch.mute = chJson["mute"];
            ch.solo = chJson["solo"];
            ch.record = chJson["record"];
        }

        // IMPORTANT: Apply channel settings to MIDI device
        mSoundBank.ApplyChannelSettings();

        // 3. Tracks
        for (const auto& trackJson : project["tracks"]) {
            int channel = trackJson["channel"];
            Track& track = mTrackSet.GetTrack(channel);
            track.clear();

            for (const auto& eventJson : trackJson["events"]) {
                TimedMidiEvent event;
                event.tick = eventJson["tick"];
                event.mm.mData[0] = eventJson["midiData"][0];
                event.mm.mData[1] = eventJson["midiData"][1];
                event.mm.mData[2] = eventJson["midiData"][2];
                track.push_back(event);
            }
        }

        // Clear undo/redo history (don't restore edit history)
        ClearUndoHistory();

        // Update state
        mCurrentProjectPath = filepath;
        mIsDirty = false;

        return true;
    }
    catch (json::parse_error& e) {
        // Invalid JSON
        // std::cerr << "Parse error: " << e.what() << '\n';
        return false;
    }
    catch (json::type_error& e) {
        // Wrong data type (e.g., expected number, got string)
        // std::cerr << "Type error: " << e.what() << '\n';
        return false;
    }
    catch (const std::exception& e) {
        // Other errors
        // std::cerr << "Load error: " << e.what() << '\n';
        return false;
    }
}
```

### Example JSON Output

Here's what a saved project file would look like:

```json
{
    "version": "1.0",
    "appVersion": "0.3",
    "transport": {
        "tempo": 120.0,
        "timeSignature": [4, 4],
        "currentTick": 0
    },
    "channels": [
        {
            "channelNumber": 0,
            "programNumber": 0,
            "volume": 100,
            "mute": false,
            "solo": false,
            "record": false
        },
        {
            "channelNumber": 1,
            "programNumber": 8,
            "volume": 100,
            "mute": false,
            "solo": false,
            "record": true
        }
        // ... 13 more channels
    ],
    "tracks": [
        {
            "channel": 0,
            "events": [
                {
                    "tick": 0,
                    "midiData": [144, 60, 100]
                },
                {
                    "tick": 480,
                    "midiData": [128, 60, 0]
                }
            ]
        },
        {
            "channel": 1,
            "events": []
        }
        // ... 13 more tracks
    ]
}
```

### Dirty Flag Implementation

```cpp
// In AppModel.h
class AppModel {
public:
    bool IsProjectDirty() const { return mIsDirty; }
    void MarkDirty() { mIsDirty = true; }
    void MarkClean() { mIsDirty = false; }

private:
    bool mIsDirty = false;
    std::string mCurrentProjectPath;
};

// Mark dirty when data changes
void AppModel::ExecuteCommand(std::unique_ptr<Command> cmd) {
    cmd->Execute();
    mUndoStack.push_back(std::move(cmd));
    mRedoStack.clear();

    // Mark project as dirty
    MarkDirty();

    if (mUndoStack.size() > MAX_UNDO_STACK_SIZE) {
        mUndoStack.erase(mUndoStack.begin());
    }
}
```

### File Menu Integration

```cpp
// In MainFrame.cpp - CreateMenuBar()
auto* fileMenu = new wxMenu();
fileMenu->Append(wxID_NEW, "&New Project\tCtrl+N", "Create a new project");
fileMenu->Append(wxID_OPEN, "&Open...\tCtrl+O", "Open a project");
fileMenu->Append(wxID_SAVE, "&Save\tCtrl+S", "Save the current project");
fileMenu->Append(wxID_SAVEAS, "Save &As...\tCtrl+Shift+S", "Save project with a new name");
fileMenu->AppendSeparator();
fileMenu->Append(wxID_EXIT, "E&xit\tAlt+F4", "Exit MidiWorks");

Bind(wxEVT_MENU, &MainFrame::OnNew, this, wxID_NEW);
Bind(wxEVT_MENU, &MainFrame::OnOpen, this, wxID_OPEN);
Bind(wxEVT_MENU, &MainFrame::OnSave, this, wxID_SAVE);
Bind(wxEVT_MENU, &MainFrame::OnSaveAs, this, wxID_SAVEAS);
Bind(wxEVT_MENU, &MainFrame::OnExit, this, wxID_EXIT);

menuBar->Append(fileMenu, "&File");
```

### Save/Open Dialogs

```cpp
// In MainFrame.cpp

void MainFrame::OnSave(wxCommandEvent& event) {
    if (mAppModel->GetCurrentProjectPath().empty()) {
        // No path yet, use Save As
        OnSaveAs(event);
        return;
    }

    if (mAppModel->SaveProject(mAppModel->GetCurrentProjectPath())) {
        SetTitle("MidiWorks - " + mAppModel->GetCurrentProjectPath());
    } else {
        wxMessageBox("Failed to save project", "Error", wxOK | wxICON_ERROR);
    }
}

void MainFrame::OnSaveAs(wxCommandEvent& event) {
    wxFileDialog saveDialog(this,
        "Save MidiWorks Project",
        wxEmptyString,
        wxEmptyString,
        "MidiWorks Projects (*.mwp)|*.mwp",
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    if (saveDialog.ShowModal() == wxID_CANCEL) {
        return;
    }

    std::string path = saveDialog.GetPath().ToStdString();
    if (mAppModel->SaveProject(path)) {
        SetTitle("MidiWorks - " + path);
    } else {
        wxMessageBox("Failed to save project", "Error", wxOK | wxICON_ERROR);
    }
}

void MainFrame::OnOpen(wxCommandEvent& event) {
    // Check for unsaved changes
    if (mAppModel->IsProjectDirty()) {
        int result = wxMessageBox(
            "Do you want to save changes to the current project?",
            "Unsaved Changes",
            wxYES_NO | wxCANCEL | wxICON_QUESTION);

        if (result == wxYES) {
            OnSave(event);
        } else if (result == wxCANCEL) {
            return;
        }
    }

    wxFileDialog openDialog(this,
        "Open MidiWorks Project",
        wxEmptyString,
        wxEmptyString,
        "MidiWorks Projects (*.mwp)|*.mwp",
        wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (openDialog.ShowModal() == wxID_CANCEL) {
        return;
    }

    std::string path = openDialog.GetPath().ToStdString();
    if (mAppModel->LoadProject(path)) {
        SetTitle("MidiWorks - " + path);
        Refresh();  // Redraw canvas with loaded data
    } else {
        wxMessageBox("Failed to load project", "Error", wxOK | wxICON_ERROR);
    }
}

void MainFrame::OnNew(wxCommandEvent& event) {
    // Check for unsaved changes
    if (mAppModel->IsProjectDirty()) {
        int result = wxMessageBox(
            "Do you want to save changes to the current project?",
            "Unsaved Changes",
            wxYES_NO | wxCANCEL | wxICON_QUESTION);

        if (result == wxYES) {
            OnSave(event);
        } else if (result == wxCANCEL) {
            return;
        }
    }

    // Clear all data
    // You'll need to add a ClearProject() method to AppModel
    mAppModel->ClearProject();
    SetTitle("MidiWorks - Untitled");
    Refresh();
}
```

---

## Tips & Best Practices

### Development Tips

1. **Start Simple**
   - Get Transport data saving/loading first
   - Then add channels
   - Finally add tracks
   - Test after each step!

2. **Use Pretty Print**
   ```cpp
   file << project.dump(4);  // 4-space indent
   ```
   This makes debugging much easier.

3. **Test Incrementally**
   - Save after adding each section
   - Open the JSON file in a text editor
   - Verify the structure looks correct

4. **Check the Examples**
   The nlohmann/json README has excellent examples:
   https://github.com/nlohmann/json#examples

### Common Pitfalls

1. **Forgetting to call `ApplyChannelSettings()`**
   - After loading channels, you MUST call this
   - Otherwise MIDI devices won't have correct programs/volumes

2. **Not clearing undo/redo stacks**
   - Call `ClearUndoHistory()` after loading
   - Otherwise users can undo into undefined state

3. **Not handling missing fields**
   - Use `.value("key", default)` for optional fields
   - Use `.contains("key")` to check existence

4. **Not closing files**
   - Always close file streams
   - Or use RAII pattern with scope

5. **Not updating window title**
   - Show project name in title bar
   - Show asterisk (*) when dirty

### Error Handling

Always wrap JSON operations in try-catch:

```cpp
try {
    json j = json::parse(file);
}
catch (json::parse_error& e) {
    // Syntax error in JSON
}
catch (json::type_error& e) {
    // Wrong data type
}
catch (json::out_of_range& e) {
    // Missing required field
}
catch (const std::exception& e) {
    // Other errors
}
```

---

## Testing

### Test Cases

#### 1. Basic Round-Trip
```
1. Create a simple project with a few notes
2. Save to file
3. Close and reopen application
4. Load the file
5. Verify all notes are present
```

#### 2. Empty Project
```
1. Save project with no notes
2. Load it back
3. Should work without errors
```

#### 3. Complex Project
```
1. Use all 15 channels
2. Add 100+ notes across multiple tracks
3. Save and load
4. Verify all data intact
```

#### 4. Channel Settings
```
1. Change programs, volumes, mute/solo
2. Save and load
3. Verify all settings restored
4. Verify MIDI devices have correct programs
```

#### 5. Error Cases
```
1. Try loading a non-existent file
2. Try loading a corrupt JSON file
3. Try loading a file with missing fields
4. Verify error messages shown to user
```

### Verification Script

After loading, verify:
- [ ] Transport tempo matches
- [ ] Time signature matches
- [ ] All 15 channels have correct settings
- [ ] All tracks have correct number of events
- [ ] Each event has correct tick and MIDI data
- [ ] Undo/redo stacks are empty
- [ ] Dirty flag is false
- [ ] Window title shows project name

---

## What NOT to Save

These should NOT be saved to file:

- ❌ Undo/Redo stacks (reset on load)
- ❌ Recording buffer (should be empty)
- ❌ MIDI device connections (user selects on startup)
- ❌ Iterator state (recalculated on playback)
- ❌ Panel layout (not project data)
- ❌ Window size/position (use wxWidgets config instead)

---

## File Format Details

### File Extension
`.mwp` (MidiWorks Project)

### Default Save Location
```
Windows: C:\Users\<username>\Documents\MidiWorks\
Linux:   ~/MidiWorks/
Mac:     ~/Documents/MidiWorks/
```

### Version Compatibility
Include a `"version"` field in JSON:
```json
{
    "version": "1.0",
    ...
}
```

Later, you can handle different versions:
```cpp
std::string version = project.value("version", "1.0");
if (version == "1.0") {
    // Handle v1.0 format
} else if (version == "2.0") {
    // Handle v2.0 format
}
```

---

## Additional Resources

### Documentation
- **nlohmann/json GitHub**: https://github.com/nlohmann/json
- **API Documentation**: https://json.nlohmann.me/api/basic_json/
- **Examples**: https://github.com/nlohmann/json#examples

### wxWidgets File Dialogs
- **wxFileDialog**: https://docs.wxwidgets.org/3.0/classwx_file_dialog.html
- **wxMessageBox**: https://docs.wxwidgets.org/3.0/group__group__funcmacro__dialog.html

### C++ File I/O
- **std::ofstream**: https://en.cppreference.com/w/cpp/io/basic_ofstream
- **std::ifstream**: https://en.cppreference.com/w/cpp/io/basic_ifstream

---

## Next Steps After Implementation

Once save/load is working:

1. **Auto-save** (optional)
   - Save to temp file every 5 minutes
   - Restore on crash

2. **Recent Files** (optional)
   - Track recently opened projects
   - Add to File menu

3. **Backup** (optional)
   - Create .bak file before overwriting
   - Helps recover from corruption

4. **Export to MIDI** (later)
   - Export to standard .mid format
   - Share with other DAWs

---

## Good Luck!

This is a major milestone for MidiWorks. Once you have save/load working, you'll have a real DAW where users can preserve their work!

Remember:
- Start simple (Transport first)
- Test incrementally
- Use pretty-print to debug
- Don't forget ApplyChannelSettings()!

You've got this! 🎵
