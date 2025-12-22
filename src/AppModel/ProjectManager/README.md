# ProjectManager

**Location:** `src/AppModel/ProjectManager/`

## Overview

`ProjectManager` handles all project persistence and dirty state tracking for MidiWorks. This class was extracted from `AppModel` to create better separation of concerns.

## Responsibilities

1. **Project File I/O**
   - Save projects to JSON format (`.mwp` files)
   - Load projects from JSON files
   - Clear/reset project to default state

2. **Dirty State Tracking**
   - Track whether project has unsaved changes
   - Notify UI when dirty state changes (for window title updates)

3. **Project State Management**
   - Track current project file path
   - Coordinate with other components during save/load/clear

## Architecture

### Dependencies

ProjectManager requires references to these core components:
- **Transport** - tempo, time signature, current playback position
- **SoundBank** - channel settings (program, volume, mute, solo, record)
- **TrackSet** - all track data (timed MIDI events)
- **RecordingSession** - recording buffer (cleared on project reset)

### Callbacks

ProjectManager uses callbacks to coordinate with AppModel:

1. **DirtyStateCallback** - Called when dirty state changes
   - Used by UI to update window title (e.g., `MidiWorks - myproject.mwp*`)

2. **ClearUndoHistoryCallback** - Called when undo/redo should be cleared
   - Invoked during `LoadProject()` and `ClearProject()`
   - Undo/redo history is not serialized to project files

## Usage Example

```cpp
// In AppModel constructor
ProjectManager mProjectManager(mTransport, mSoundBank, mTrackSet, mRecordingSession);

// Setup callbacks
mProjectManager.SetDirtyStateCallback([this](bool dirty) {
    // Update window title with asterisk if dirty
    UpdateWindowTitle(dirty);
});

mProjectManager.SetClearUndoHistoryCallback([this]() {
    ClearUndoHistory();
});

// Save/Load operations
if (mProjectManager.SaveProject("myproject.mwp")) {
    // Save successful, project is now clean
}

if (mProjectManager.LoadProject("myproject.mwp")) {
    // Load successful, undo history cleared
}

// Mark as dirty when edits occur
mProjectManager.MarkDirty();

// Check if save needed
if (mProjectManager.IsProjectDirty()) {
    // Prompt user to save
}
```

## Project File Format

Projects are saved as JSON (`.mwp` extension) with the following structure:

```json
{
  "version": "1.0",
  "appVersion": "0.3",
  "transport": {
    "tempo": 120,
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
    // ... 14 more channels
  ],
  "tracks": [
    {
      "channel": 0,
      "events": [
        {
          "tick": 0,
          "midiData": [144, 60, 100]
        },
        // ... more events
      ]
    },
    // ... 14 more tracks
  ]
}
```

## Design Rationale

### Why Extract from AppModel?

Before extraction, AppModel contained 200+ lines of JSON serialization code mixed with other responsibilities. This made AppModel harder to understand and test.

**Benefits of extraction:**
- **Single Responsibility** - ProjectManager focuses solely on persistence
- **Easier Testing** - Can test save/load in isolation
- **Better Organization** - File I/O code grouped together
- **Reduced AppModel Size** - AppModel now delegates instead of implementing

### Why Not Serialize Undo/Redo?

Undo/redo history is intentionally **not** saved to project files:
- Undo stacks can be large and complex
- Commands contain references to app state that may change
- Loading a project is a clean slate - old edit history isn't relevant
- Keeps project files simple and portable

### Why Use Callbacks?

ProjectManager needs to notify other components during operations:
- **DirtyStateCallback** - UI needs to update immediately when state changes
- **ClearUndoHistoryCallback** - AppModel owns the undo system, ProjectManager just signals when to clear

This is a lightweight alternative to a full event system while maintaining loose coupling.

## Integration with AppModel

ProjectManager is a member of AppModel:

```cpp
// AppModel.h
class AppModel {
private:
    Transport mTransport;
    SoundBank mSoundBank;
    TrackSet mTrackSet;
    RecordingSession mRecordingSession;
    ProjectManager mProjectManager;  // Initialized after dependencies
};
```

AppModel's public interface delegates to ProjectManager:

```cpp
// AppModel delegates to ProjectManager
bool AppModel::SaveProject(const std::string& filepath) {
    return mProjectManager.SaveProject(filepath);
}

bool AppModel::IsProjectDirty() const {
    return mProjectManager.IsProjectDirty();
}
```

## Future Enhancements

Potential improvements:
- Support for different project file versions
- Auto-save functionality
- Recent files list management
- Project file compression
- Export to MIDI file format
- Cloud storage integration

## Related Files

- `AppModel/AppModel.h` - Owns ProjectManager instance
- `Transport/Transport.h` - Provides tempo/timing data
- `SoundBank/SoundBank.h` - Provides channel settings
- `TrackSet/TrackSet.h` - Provides track data
- `RecordingSession/RecordingSession.h` - Recording buffer
