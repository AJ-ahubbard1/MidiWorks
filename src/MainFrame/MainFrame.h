// MainFrame.h
#pragma once
#include <wx/wx.h>
#include <wx/aui/aui.h>
#include <memory>
#include "AppModel/AppModel.h"
#include "Panels/Panels.h"
#include "PaneInfo.h"
#include "MainFrameIDs.h"
#include "KeyboardHandler.h"

class MainFrame : public wxFrame 
{
    friend class KeyboardHandler;

public:
    MainFrame();

private:
    // MEMBER VARIABLES
    std::shared_ptr<AppModel> mAppModel;  // Contains all app data & business logic 
    wxAuiManager mAuiManager;			  // Advanced UI, enables dockable panes
    wxTimer mTimer;						  // Triggers Update method every 1ms 
    std::unique_ptr<KeyboardHandler> mKeyboardHandler;
    int mNextPanelId = ID_PANELS_BEGIN;   // Auto-incrementing panel ID counter

    // Panel Pointers
	std::unordered_map<int, PanelInfo> mPanels; // Map used to toggle visibility
    MidiSettingsPanel* mMidiSettingsPanel;
    SoundBankPanel* mSoundBankPanel;
    TransportPanel* mTransportPanel;
    MidiCanvasPanel* mMidiCanvasPanel;
    LogPanel* mLogPanel;
    UndoHistoryPanel* mUndoHistoryPanel;
    ShortcutsPanel* mShortcutsPanel;
    DrumMachinePanel* mDrumMachinePanel;

    // METHODS - Implemented in MainFrame.cpp
    void CreateDockablePanes();
    void RegisterPanel(PanelInfo info);
    std::unordered_map<int, PanelInfo>& GetAllPanels();
    void SetPanelVisibility(int id, bool vis);
    void CreateMenuBar();
    void CreateSizer();
    void OnTimer(wxTimerEvent&);
    void SyncMenuChecks();
    void UpdateTitle();
    uint64_t GetDeltaTimeMs();
    
	// EVENT HANDLERS - Implemented in MainFrameEventHandlers.cpp
    // View / Panel Management Events
    void OnTogglePane(wxCommandEvent& event);
    void OnPaneClosed(wxAuiManagerEvent& event);
    void OnAuiRender(wxAuiManagerEvent& event);

    // Edit Menu Events
    void OnUndo(wxCommandEvent& event);
    void OnRedo(wxCommandEvent& event);
    void OnQuantize(wxCommandEvent& event);

    // File Menu Events
	// Helper for unsaved changes prompt 
    enum class UnsavedChangesAction { Continue, Cancel };
    UnsavedChangesAction PromptForUnsavedChanges();
    void OnNew(wxCommandEvent& event);
    void OnOpen(wxCommandEvent& event);
    void OnSave(wxCommandEvent& event);
    void OnSaveAs(wxCommandEvent& event);

    // Application Lifecycle Events
    void OnExit(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);

    // Transport Control Events
    void OnTogglePlay(wxCommandEvent& event);
    void OnStartRecord(wxCommandEvent& event);
    void OnPreviousMeasure(wxCommandEvent& event);
    void OnNextMeasure(wxCommandEvent& event);
};
