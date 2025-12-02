# MainFrame Architecture

**Location:** `src/MainFrame/`

## Overview

`MainFrame` is the main application window and serves as the orchestrator for the dockable panel system. It uses wxWidgets' Advanced User Interface (AUI) framework (`wxAuiManager`) to provide a flexible, user-customizable layout where panels can be dragged, docked, resized, and hidden/shown.

## Key Responsibilities

1. **Window Management** - Creates and manages the main application window
2. **Panel Orchestration** - Creates all dockable panels and tracks their state
3. **Layout Management** - Uses wxAuiManager for docking/floating panels
4. **Update Loop** - Runs 10ms timer that updates AppModel and refreshes panels
5. **Menu Management** - Dynamically builds View menu from registered panels
6. **Event Handling** - Responds to panel visibility toggles and close events

## Architecture

```
MainFrame (wxFrame)
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
  ├─→ mModel (shared_ptr<AppModel>) (application state)
  │
  └─→ mTimer (wxTimer) (10ms update loop)
```

## Core Components

### PanelInfo Struct (`PaneInfo.h`)

Metadata describing each panel:

```cpp
struct PanelInfo
{
    wxString name;              // Display name in View menu
    wxWindow* window;           // Pointer to panel (wxPanel subclass)
    int menuId;                 // Auto-generated ID (idBase++)
    wxAuiPaneInfo::Position pos; // Dock position (Top/Bottom/Left/Right/Center)
    wxSize minSize;             // Minimum panel size
    wxSize bestSize;            // Preferred panel size
    bool visible;               // Initially visible?
};
```

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

### Step 3: Register in MainFrame::CreateDockablePanes()

```cpp
void MainFrame::CreateDockablePanes()
{
    int idBase = wxID_HIGHEST + 1;

    // ... existing panels ...

    // Create your panel
    auto* myPanel = new MyNewPanel(this, mModel);

    // Create PanelInfo
    PanelInfo myInfo{
        "My Panel",                  // Name shown in View menu
        myPanel,                     // Panel instance
        idBase++,                    // Auto-increment menu ID
        wxAuiPaneInfo::Right(),      // Dock position (see options below)
        wxSize(300, 200),            // Minimum size
        wxSize(400, 300),            // Best/preferred size
        true                         // Initially visible?
    };

    // Add to panel registry
    mPanels[myInfo.menuId] = myInfo;

    // Add to AUI manager
    m_mgr.AddPane(myPanel, CreatePaneInfo(myInfo));

    // ... more panels ...

    // Apply layout
    m_mgr.Update();
}
```

**Dock Position Options:**
```cpp
wxAuiPaneInfo::Top()      // Dock to top edge
wxAuiPaneInfo::Bottom()   // Dock to bottom edge
wxAuiPaneInfo::Left()     // Dock to left edge
wxAuiPaneInfo::Right()    // Dock to right edge
wxAuiPaneInfo::Center()   // Center area (can have tabs)
```

**Size Guidelines:**
- **minSize**: Smallest usable size (prevents user from making too small)
- **bestSize**: Default size when first docked
- Use logical sizes based on content (e.g., transport bar is wide/short, settings panel is narrow/tall)

### Step 4: (Optional) Add Update Call

If your panel needs regular updates from the timer loop:

```cpp
void MainFrame::OnTimer(wxTimerEvent& event)
{
    mModel->Update();

    // Update panels that need refresh
    auto* transport = dynamic_cast<TransportPanel*>(mPanels[transportId].window);
    if (transport) transport->UpdateDisplay();

    auto* myPanel = dynamic_cast<MyNewPanel*>(mPanels[myPanelId].window);
    if (myPanel) myPanel->Update();  // ← Add this
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
  │   ├─→ Build PanelInfo structs with idBase++
  │   ├─→ Add to mPanels map
  │   └─→ Add to wxAuiManager
  │
  ├─→ CreateMenuBar()
  │   └─→ Build View menu from mPanels map
  │
  ├─→ Bind events
  │   ├─→ wxEVT_MENU → OnTogglePane
  │   └─→ wxEVT_AUI_PANE_CLOSE → OnPaneClosed
  │
  └─→ mTimer.Start(10)  // 10ms update loop
```

### Update Loop (Every 10ms)

```
mTimer fires wxEVT_TIMER
  │
  └─→ MainFrame::OnTimer()
      ├─→ mModel->Update()
      │   └─→ AppModel processes state, MIDI, playback
      │
      └─→ Update panels
          ├─→ TransportPanel::UpdateDisplay()
          ├─→ MidiCanvasPanel::Update()
          └─→ LogPanel::LogMidiEvent() (if flagged)
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

**Triggered by:** wxTimer every 10ms

**Purpose:** Update AppModel and refresh panels

```cpp
void MainFrame::OnTimer(wxTimerEvent& event)
{
    // Update model (handles MIDI, playback, recording)
    mModel->Update();

    // Update panels that need refresh
    // (Only panels with dynamic/real-time data)
}
```

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

### Why Auto-Increment IDs (idBase++)?

**Old approach (manual enums):**
```cpp
enum {
    ID_VIEW_TRANSPORT = wxID_HIGHEST + 1,
    ID_VIEW_SOUNDBANK,
    ID_VIEW_MIDI_SETTINGS,
    // ... manually define every ID
};
```

**New approach (auto-increment):**
```cpp
int idBase = wxID_HIGHEST + 1;
PanelInfo info1{"Transport", panel1, idBase++, ...};
PanelInfo info2{"SoundBank", panel2, idBase++, ...};
```

**Benefits:**
- ✅ No enum maintenance - just add panels
- ✅ IDs auto-generated in order
- ✅ No risk of duplicate IDs
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

Trade-off: Slightly higher CPU usage, but negligible at 10ms (100Hz).

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
        bool isPlaying = (transport.mState == Transport::State::Playing);
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

- [ ] Create panel class in `src/Panels/MyPanel.h`
  - Inherit from `wxPanel`
  - Accept `wxWindow* parent, std::shared_ptr<AppModel> model`
  - Create UI controls in constructor
  - Add `Update()` method if needed
- [ ] Add include to `src/Panels/Panels.h`
- [ ] Register in `MainFrame::CreateDockablePanes()`
  - Create panel instance
  - Create `PanelInfo` with `idBase++`
  - Add to `mPanels` map
  - Add to `m_mgr` via `CreatePaneInfo()`
- [ ] (Optional) Add update call in `MainFrame::OnTimer()`
- [ ] Build and test
  - Panel appears in View menu
  - Panel can be shown/hidden
  - Panel can be docked/undocked/resized
  - Panel [X] button syncs with View menu

---

## Related Files

- `MainFrame.h/cpp` - Main window and panel orchestration
- `PaneInfo.h` - PanelInfo struct definition
- `src/Panels/` - All panel implementations
- `AppModel/AppModel.h` - Shared application state

## wxWidgets Documentation

- [wxAuiManager](https://docs.wxwidgets.org/stable/classwx_aui_manager.html) - Docking framework
- [wxAuiPaneInfo](https://docs.wxwidgets.org/stable/classwx_aui_pane_info.html) - Pane configuration
- [wxPanel](https://docs.wxwidgets.org/stable/classwx_panel.html) - Base panel class
- [wxTimer](https://docs.wxwidgets.org/stable/classwx_timer.html) - Timer events
