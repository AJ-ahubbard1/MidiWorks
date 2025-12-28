# MainFrame Architecture

**Location:** `src/MainFrame/`

## Overview

`MainFrame` is the main application window and serves as the orchestrator for the dockable panel system. It uses wxWidgets' Advanced User Interface (AUI) framework (`wxAuiManager`) to provide a flexible, user-customizable layout where panels can be dragged, docked, resized, and hidden/shown.

## File Structure

The MainFrame component is split across multiple files for separation of concerns:

- **MainFrame.h** - Class declaration, member variables, method signatures
- **MainFrame.cpp** - Constructor, panel creation, menu building, utility methods
- **MainFrameEventHandlers.cpp** - All event handler implementations (17 handlers)
- **KeyboardHandler.h** - Keyboard shortcut handler class declaration
- **KeyboardHandler.cpp** - Keyboard accelerator table setup and event binding
- **MainFrameIDs.h** - Event ID enumeration for keyboard shortcuts and panels
- **PaneInfo.h** - PanelInfo struct and helper functions
- **README.md** - This documentation

## Key Responsibilities

1. **Window Management** - Creates and manages the main application window
2. **Panel Orchestration** - Creates all dockable panels and tracks their state
3. **Layout Management** - Uses wxAuiManager for docking/floating panels
4. **Update Loop** - Runs 1ms timer that updates AppModel and refreshes panels
5. **Menu Management** - Dynamically builds View menu from registered panels
6. **Event Handling** - Responds to panel visibility toggles and close events
7. **Keyboard Handling** - Delegates to KeyboardHandler class for accelerator table setup

## Architecture

```
MainFrame (wxFrame)
  │
  ├─→ mKeyboardHandler (std::unique_ptr<KeyboardHandler>)
  │   └─→ Sets up accelerators and binds keyboard shortcuts
  │
  ├─→ wxAuiManager (docking system)
  │   ├─→ TransportPanel (docked bottom)
  │   ├─→ SoundBankPanel (docked center)
  │   ├─→ MidiSettingsPanel (docked right)
  │   ├─→ MidiCanvasPanel (docked center)
  │   └─→ LogPanel (docked bottom)
  │
  ├─→ mPanels map<int, PanelInfo> (panel registry)
  │   └─→ Tracks: name, window pointer, menuId, position, size, visibility
  │
  ├─→ mAppModel (shared_ptr<AppModel>) (application state)
  │
  └─→ mTimer (wxTimer) (1ms update loop)
```

## Core Components

### PanelInfo Struct (`PaneInfo.h`)

Metadata describing each panel:

```cpp
struct PanelInfo
{
    wxString name;               // Display name in View menu
    wxWindow* window;            // Pointer to panel (wxPanel subclass)
    PanePosition defaultPosition; // Dock position enum (Left/Right/Top/Bottom/Center/Float)
    wxSize minSize;              // Minimum panel size
    bool isVisible = true;       // Initially visible? (optional, defaults to true)
    bool hasCaption = true;      // Show title bar? (optional, defaults to true)
    bool hasCloseButton = true;  // Show [X] button? (optional, defaults to true)
    int menuId = -1;             // Auto-assigned by RegisterPanel()
};
```

**Key Features:**
- Simple initializer syntax with sensible defaults
- Auto-assigned menuId (no manual ID management)
- PanePosition enum for type-safe dock positions

### KeyboardHandler Class (`KeyboardHandler.h/cpp`)

Manages all keyboard shortcuts for MainFrame.

**Purpose:** Separation of keyboard shortcut management from MainFrame initialization code.

**Implementation:**
```cpp
class KeyboardHandler
{
public:
    KeyboardHandler(MainFrame* mainFrame, std::shared_ptr<AppModel> appModel);
    void Initialize();

private:
    MainFrame* mMainFrame;
    std::shared_ptr<AppModel> mAppModel;
    void SetupShortCuts();
};
```

**How it works:**
1. Constructor takes MainFrame* and AppModel references
2. `Initialize()` creates wxAcceleratorTable with keyboard shortcuts
3. `SetupShortCuts()` binds events to MainFrame's private event handlers
4. Friend class relationship allows access to MainFrame's private Bind methods

**Current shortcuts:**
- **Space** - Toggle Play/Pause
- **R** - Toggle Record
- **Q** - Quantize to Grid
- **Left Arrow** - Previous Measure
- **Right Arrow** - Next Measure

**Friend Relationship:**
KeyboardHandler is declared as a friend class in MainFrame.h to access private event handlers for binding.

### mPanels Map

```cpp
std::unordered_map<int, PanelInfo> mPanels;
```

Maps `menuId` → `PanelInfo`. This is the central registry of all panels in the application.

**Why a map?**
- Allows quick lookup by menuId when View menu is clicked
- Stores all panel metadata in one place
- Easy iteration for building View menu

### wxAuiManager

The wxWidgets docking framework that handles:
- Drag-and-drop panel repositioning
- Resize handles
- Floating panels (undock from main window)
- Automatic layout persistence
- Show/hide animations

---

## Adding New Keyboard Shortcuts

To add a new keyboard shortcut to MainFrame:

### Step 1: Add Event ID

Add a new ID to the enum in `MainFrameIDs.h`:

```cpp
enum MainFrameIDs
{
    ID_KEYBOARD_TOGGLE_PLAY = wxID_HIGHEST + 1000,
    ID_KEYBOARD_RECORD,
    ID_KEYBOARD_QUANTIZE,
    ID_KEYBOARD_PREVIOUS_MEASURE,
    ID_KEYBOARD_NEXT_MEASURE,
    ID_KEYBOARD_MY_NEW_SHORTCUT,  // ← Add this

    ID_PANELS_BEGIN
};
```

### Step 2: Add Event Handler

Declare the event handler in `MainFrame.h` (private section):

```cpp
private:
    // Transport Control Events
    void OnTogglePlay(wxCommandEvent& event);
    void OnStartRecord(wxCommandEvent& event);
    void OnMyNewShortcut(wxCommandEvent& event);  // ← Add declaration
```

Implement the handler in `MainFrameEventHandlers.cpp`:

```cpp
// My New Shortcut (Ctrl+M)
void MainFrame::OnMyNewShortcut(wxCommandEvent& event)
{
    // Your implementation here
    mAppModel->DoSomething();
}
```

### Step 3: Update KeyboardHandler

Add the accelerator entry in `KeyboardHandler.cpp::Initialize()`:

```cpp
void KeyboardHandler::Initialize()
{
    wxAcceleratorEntry entries[6];  // ← Increase array size
    entries[0].Set(wxACCEL_NORMAL, WXK_SPACE, ID_KEYBOARD_TOGGLE_PLAY);
    // ... existing entries ...
    entries[5].Set(wxACCEL_CTRL, 'M', ID_KEYBOARD_MY_NEW_SHORTCUT);  // ← Add this

    wxAcceleratorTable accelTable(6, entries);  // ← Update count
    mMainFrame->SetAcceleratorTable(accelTable);

    SetupShortCuts();
}
```

### Step 4: Bind the Event

Add the binding in `KeyboardHandler.cpp::SetupShortCuts()`:

```cpp
void KeyboardHandler::SetupShortCuts()
{
    // Transport
    mMainFrame->Bind(wxEVT_MENU, &MainFrame::OnTogglePlay, mMainFrame, ID_KEYBOARD_TOGGLE_PLAY);
    // ... existing bindings ...
    mMainFrame->Bind(wxEVT_MENU, &MainFrame::OnMyNewShortcut, mMainFrame, ID_KEYBOARD_MY_NEW_SHORTCUT);  // ← Add this
}
```

**Modifier keys:**
- `wxACCEL_NORMAL` - No modifier
- `wxACCEL_CTRL` - Ctrl key
- `wxACCEL_SHIFT` - Shift key
- `wxACCEL_ALT` - Alt key
- Can combine: `wxACCEL_CTRL | wxACCEL_SHIFT`

---

## Adding a New Panel - Complete Guide

### Step 1: Create Panel Class

Create a new header file in `src/Panels/`:

```cpp
// src/Panels/MyNewPanel.h
#pragma once
#include <wx/wx.h>
#include <memory>
#include "AppModel/AppModel.h"

class MyNewPanel : public wxPanel
{
public:
    MyNewPanel(wxWindow* parent, std::shared_ptr<AppModel> model)
        : wxPanel(parent), mModel(model)
    {
        // Create UI controls
        auto* sizer = new wxBoxSizer(wxVERTICAL);

        mLabel = new wxStaticText(this, wxID_ANY, "My Panel");
        sizer->Add(mLabel, 0, wxALL, 5);

        SetSizer(sizer);

        // Bind events if needed
        // Bind(wxEVT_BUTTON, &MyNewPanel::OnButton, this, ID_BUTTON);
    }

    // Optional: Called from MainFrame::OnTimer() if panel needs updates
    void Update() {
        // Refresh display based on model state
        // Example: mLabel->SetLabel(mModel->GetSomeState());
    }

private:
    std::shared_ptr<AppModel> mModel;
    wxStaticText* mLabel;
    // ... other controls
};
```

**Panel Design Guidelines:**
- Inherit from `wxPanel` (or `wxScrolledWindow` if needs scrolling)
- Accept `wxWindow* parent` and `std::shared_ptr<AppModel> model` in constructor
- Keep panels lightweight - mostly header-only
- Store `mModel` reference to access application state
- Add `Update()` method only if panel needs timer-driven refresh

### Step 2: Add to Panels.h

```cpp
// src/Panels/Panels.h
#include "TransportPanel.h"
#include "SoundBankPanel.h"
#include "MidiSettings.h"
#include "MidiCanvas.h"
#include "Log.h"
#include "MyNewPanel.h"  // ← Add this
```

This aggregation header makes all panels available via a single include.

### Step 3: Add Panel Pointer to MainFrame.h

Add a member variable for your panel in `MainFrame.h` (private section):

```cpp
class MainFrame : public wxFrame
{
    // ...
private:
    // Panel Pointers
    MidiSettingsPanel* mMidiSettingsPanel;
    SoundBankPanel* mSoundBankPanel;
    TransportPanel* mTransportPanel;
    MidiCanvasPanel* mMidiCanvasPanel;
    LogPanel* mLogPanel;
    UndoHistoryPanel* mUndoHistoryPanel;
    ShortcutsPanel* mShortcutsPanel;
    DrumMachinePanel* mDrumMachinePanel;
    MyNewPanel* mMyNewPanel;  // ← Add this
};
```

### Step 4: Initialize and Register in MainFrame::CreateDockablePanes()

In `MainFrame::CreateDockablePanes()`, create your panel instance and register it with custom metadata:

```cpp
void MainFrame::CreateDockablePanes()
{
    // ... existing panels ...

    // Create and register your panel
    mMyNewPanel = new MyNewPanel(this, mAppModel);
    RegisterPanel({"My Panel", mMyNewPanel, PanePosition::Right, wxSize(300, 200)});

    // Optional parameters (defaults shown):
    // RegisterPanel({
    //     "My Panel",              // Name in View menu
    //     mMyNewPanel,             // Panel pointer
    //     PanePosition::Right,     // Dock position
    //     wxSize(300, 200),        // Minimum size
    //     true,                    // Initially visible (default)
    //     true,                    // Has caption/title bar (default)
    //     true                     // Has close button (default)
    // });
}
```

**What RegisterPanel() does:**
- Auto-assigns unique menuId from `mNextPanelId++`
- Adds panel to `mPanels` map for View menu
- Adds pane to `mAuiManager` for docking

**Dock Position Options:**
```cpp
PanePosition::Left     // Dock to left edge
PanePosition::Right    // Dock to right edge
PanePosition::Top      // Dock to top edge
PanePosition::Bottom   // Dock to bottom edge
PanePosition::Center   // Center area (can have tabs)
PanePosition::Float    // Floating window
```

**Size Guidelines:**
- **minSize**: Smallest usable size (prevents user from making too small)
- Use logical sizes based on content (e.g., transport bar is wide/short, settings panel is narrow/tall)

### Step 5: (Optional) Add Update Call

If your panel needs regular updates from the timer loop, add a call in `MainFrameEventHandlers.cpp`:

```cpp
// In MainFrameEventHandlers.cpp
void MainFrame::OnTimer(wxTimerEvent&)
{
    mAppModel->Update();
    mTransportPanel->UpdateDisplay();
    mMidiCanvasPanel->Update();
    mMyNewPanel->Update();  // ← Add direct call to your panel
    // Note: Logging now handled via callback - no polling needed
}
```

**When to add Update()?**
- Panel displays real-time data (transport time, MIDI log, etc.)
- Panel visualizes playback state (piano roll, waveform)

**When NOT to add Update()?**
- Panel is purely settings/configuration
- Panel only responds to user input
- Panel state is static

---

## How It Works

### Initialization Flow

```
MainFrame constructor
  │
  ├─→ Create wxAuiManager
  │
  ├─→ Create AppModel (shared_ptr)
  │
  ├─→ CreateDockablePanes()
  │   ├─→ Create panel instances
  │   ├─→ RegisterPanel() for each panel
  │   │   ├─→ Auto-assign menuId from mNextPanelId++
  │   │   ├─→ Add to mPanels map
  │   │   └─→ Add to wxAuiManager
  │   └─→ Register callbacks (log, dirty state)
  │
  ├─→ CreateMenuBar()
  │   └─→ Build View menu from mPanels map
  │
  ├─→ Bind events
  │   ├─→ wxEVT_AUI_PANE_CLOSE → OnPaneClosed
  │   ├─→ wxEVT_CLOSE_WINDOW → OnClose
  │   └─→ wxEVT_TIMER → OnTimer
  │
  ├─→ Create KeyboardHandler (NEW)
  │   └─→ KeyboardHandler::Initialize()
  │       ├─→ Build wxAcceleratorTable
  │       └─→ Bind shortcuts to MainFrame handlers
  │
  └─→ mTimer.Start(1)  // 1ms update loop
```

### Update Loop (Every 1ms)

```
mTimer fires wxEVT_TIMER
  │
  └─→ MainFrame::OnTimer() (in MainFrameEventHandlers.cpp)
      ├─→ mAppModel->Update()
      │   └─→ AppModel processes state, MIDI, playback
      │
      └─→ Update panels
          ├─→ mTransportPanel->UpdateDisplay()
          └─→ mMidiCanvasPanel->Update()
          (Note: Logging handled via callback, not polling)
```

### View Menu Click Flow

```
User clicks "View → My Panel"
  │
  └─→ wxEVT_MENU with menuId
      │
      └─→ MainFrame::OnTogglePane(menuId)
          ├─→ Lookup mPanels[menuId]
          ├─→ Toggle visibility
          └─→ m_mgr.GetPane(window).Show(!currentState)
```

### Panel Close Button Flow

```
User clicks [X] on panel
  │
  └─→ wxEVT_AUI_PANE_CLOSE
      │
      └─→ MainFrame::OnPaneClosed(event)
          ├─→ Find PanelInfo by window pointer
          ├─→ Uncheck View menu item
          └─→ Allow close (event.Skip())
```

---

## Event Handlers

**Location:** All event handler implementations are in `MainFrameEventHandlers.cpp`

Event handlers are organized by category:
- **Timer Events** - OnTimer
- **View/Panel Management** - OnTogglePane, OnPaneClosed, OnAuiRender
- **Edit Menu** - OnUndo, OnRedo, OnQuantize
- **File Menu** - OnNew, OnOpen, OnSave, OnSaveAs
- **Application Lifecycle** - OnExit, OnClose
- **Transport Control** - OnTogglePlay, OnStartRecord, OnPreviousMeasure, OnNextMeasure

### OnTogglePane(wxCommandEvent& event)

**Triggered by:** View menu item clicked

**Purpose:** Show or hide a panel

```cpp
void MainFrame::OnTogglePane(wxCommandEvent& event)
{
    int menuId = event.GetId();
    auto it = mPanels.find(menuId);
    if (it == mPanels.end()) return;

    PanelInfo& info = it->second;
    wxAuiPaneInfo& pane = m_mgr.GetPane(info.window);

    // Toggle visibility
    pane.Show(!pane.IsShown());
    m_mgr.Update();
}
```

### OnPaneClosed(wxAuiManagerEvent& event)

**Triggered by:** User clicks [X] button on panel

**Purpose:** Sync View menu checkmark when panel is closed via [X]

```cpp
void MainFrame::OnPaneClosed(wxAuiManagerEvent& event)
{
    wxWindow* closedWindow = event.GetPane()->window;

    // Find corresponding menu item and uncheck it
    for (auto& [menuId, info] : mPanels) {
        if (info.window == closedWindow) {
            GetMenuBar()->Check(menuId, false);
            break;
        }
    }

    event.Skip(); // Allow close to proceed
}
```

### OnTimer(wxTimerEvent& event)

**Location:** `MainFrameEventHandlers.cpp:9-15`

**Triggered by:** wxTimer every 1ms

**Purpose:** Update AppModel and refresh panels

```cpp
void MainFrame::OnTimer(wxTimerEvent&)
{
    mAppModel->Update();
    mTransportPanel->UpdateDisplay();
    mMidiCanvasPanel->Update();
    // Note: Logging now handled via callback - no polling needed
}
```

**Notes:**
- Timer runs at 1ms interval for responsive updates
- Only panels with dynamic/real-time data need Update() calls
- Direct panel pointer access (no dynamic_cast needed)

---

## Helper Functions

### CreatePaneInfo(const PanelInfo& info)

Converts `PanelInfo` → `wxAuiPaneInfo` for wxAuiManager:

```cpp
wxAuiPaneInfo CreatePaneInfo(const PanelInfo& info)
{
    return info.pos
        .Name(info.name)
        .Caption(info.name)
        .MinSize(info.minSize)
        .BestSize(info.bestSize)
        .CloseButton(true)
        .MaximizeButton(false)
        .PinButton(true)
        .Show(info.visible);
}
```

**wxAuiPaneInfo Options:**
- `.Name()` - Internal identifier
- `.Caption()` - Title bar text
- `.CloseButton(true)` - Show [X] button
- `.MaximizeButton(false)` - No maximize button
- `.PinButton(true)` - Show pin/unpin button
- `.Show(visible)` - Initially visible?
- `.Floatable(true)` - Can be undocked? (default)
- `.Movable(true)` - Can be repositioned? (default)
- `.Resizable(true)` - Can be resized? (default)

---

## Design Notes

### Why Auto-Increment IDs with RegisterPanel()?

**Old approach (manual ID management):**
```cpp
enum {
    ID_VIEW_TRANSPORT = wxID_HIGHEST + 1,
    ID_VIEW_SOUNDBANK,
    ID_VIEW_MIDI_SETTINGS,
    // ... manually define every ID
};

PanelInfo info{"Transport", panel, ID_VIEW_TRANSPORT, ...};
mPanels[info.menuId] = info;
m_mgr.AddPane(panel, CreatePaneInfo(info));
```

**New approach (RegisterPanel with auto-increment):**
```cpp
mTransportPanel = new TransportPanel(this, mAppModel);
RegisterPanel({"Transport", mTransportPanel, PanePosition::Top, wxSize(-1, -1)});
// menuId auto-assigned from mNextPanelId++ inside RegisterPanel()
```

**Benefits:**
- ✅ No enum maintenance - just add panels
- ✅ IDs auto-generated by RegisterPanel()
- ✅ No risk of duplicate IDs
- ✅ Single line per panel (was 3 lines)
- ✅ Less boilerplate code

### Why PanelInfo Struct?

Groups all panel metadata in one place:
- Easier to pass around (single struct vs 7 parameters)
- Centralized registry in `mPanels` map
- Helper function `CreatePaneInfo()` converts to wxAuiPaneInfo
- Future-proof: easy to add new metadata fields

### Why Timer-Based Updates?

Alternative would be event-driven (callbacks, signals/slots). Timer approach is simpler:
- ✅ Predictable update order
- ✅ Easy to debug (single update function)
- ✅ No threading complexity
- ✅ Centralized update logic

Trade-off: Slightly higher CPU usage, but negligible at 1ms (1000Hz).

### Why Separate KeyboardHandler?

Extracting keyboard shortcut management into a dedicated class provides several benefits:

**Separation of Concerns:**
- MainFrame focuses on window/panel orchestration
- KeyboardHandler focuses solely on keyboard shortcuts
- Cleaner, more maintainable MainFrame constructor

**Easier to Add Shortcuts:**
- All keyboard-related code in one place
- Four-step process (ID, handler, accelerator, binding)
- No hunting through MainFrame code

**Future Extensibility:**
- Could load shortcuts from config file
- Could allow user-customizable keybindings
- Could implement context-sensitive shortcuts

**Reduced Constructor Complexity:**
- Keyboard setup delegated to single `Initialize()` call
- 30+ lines of keyboard code moved out of MainFrame
- Better code organization and readability

**Friend Class Pattern:**
- KeyboardHandler needs access to MainFrame's private `Bind()` methods
- Friend declaration makes this relationship explicit
- More controlled than making event handlers public

---

## Common Patterns

### Panel Needs Access to Another Panel

**Bad:** Store panel pointers and cross-reference
```cpp
// Don't do this - tight coupling
class MyPanel {
    TransportPanel* mTransport; // Bad: direct dependency
};
```

**Good:** Access shared AppModel
```cpp
// Do this - loose coupling via model
class MyPanel {
    std::shared_ptr<AppModel> mModel; // Access shared state

    void Update() {
        auto& transport = mModel->GetTransport();
        bool isPlaying = transport.IsPlaying(); 
    }
};
```

### Panel Needs to Modify Application State

**Pattern:** Call AppModel methods, don't mutate directly (when possible)

```cpp
// In panel event handler
void MyPanel::OnButtonClick(wxCommandEvent& event) {
    // Option 1: Call AppModel method (preferred)
    mModel->StartPlayback();

    // Option 2: Mutate state directly (acceptable for now)
    mModel->GetTransport().mState = Transport::State::ClickedPlay;
}
```

### Panel Needs Custom Event Handling

```cpp
class MyPanel : public wxPanel {
public:
    MyPanel(wxWindow* parent, std::shared_ptr<AppModel> model)
        : wxPanel(parent), mModel(model)
    {
        mButton = new wxButton(this, wxID_ANY, "Click Me");

        // Bind event
        mButton->Bind(wxEVT_BUTTON, &MyPanel::OnButtonClick, this);
    }

private:
    void OnButtonClick(wxCommandEvent& event) {
        // Handle button click
    }

    std::shared_ptr<AppModel> mModel;
    wxButton* mButton;
};
```

---

## Summary: Adding a Panel Checklist

- [ ] **Step 1:** Create panel class in `src/Panels/MyPanel.h`
  - Inherit from `wxPanel`
  - Accept `wxWindow* parent, std::shared_ptr<AppModel> model`
  - Create UI controls in constructor
  - Add `Update()` method if needed
- [ ] **Step 2:** Add include to `src/Panels/Panels.h`
  - `#include "MyPanel.h"`
- [ ] **Step 3:** Add panel pointer to `MainFrame.h`
  - `MyPanel* mMyPanel;` in private section
- [ ] **Step 4:** Initialize and register in `MainFrame::CreateDockablePanes()`
  - Create panel instance: `mMyPanel = new MyPanel(this, mAppModel);`
  - Register: `RegisterPanel({"My Panel", mMyPanel, PanePosition::Right, wxSize(300, 200)});`
- [ ] **Step 5:** (Optional) Add update call in `MainFrame::OnTimer()` (MainFrameEventHandlers.cpp)
  - `mMyPanel->Update();`
- [ ] Build and test
  - Panel appears in View menu
  - Panel can be shown/hidden
  - Panel can be docked/undocked/resized
  - Panel [X] button syncs with View menu

---

## Related Files

- `MainFrame.h` - Class declaration, member variables, method signatures
- `MainFrame.cpp` - Constructor, panel creation, menu building, utility methods
- `MainFrameEventHandlers.cpp` - All event handler implementations
- `KeyboardHandler.h/cpp` - Keyboard shortcut management
- `MainFrameIDs.h` - Event ID definitions
- `PaneInfo.h` - PanelInfo struct and helper functions
- `src/Panels/` - All panel implementations
- `AppModel/AppModel.h` - Shared application state

## wxWidgets Documentation

- [wxAuiManager](https://docs.wxwidgets.org/stable/classwx_aui_manager.html) - Docking framework
- [wxAuiPaneInfo](https://docs.wxwidgets.org/stable/classwx_aui_pane_info.html) - Pane configuration
- [wxPanel](https://docs.wxwidgets.org/stable/classwx_panel.html) - Base panel class
- [wxTimer](https://docs.wxwidgets.org/stable/classwx_timer.html) - Timer events
