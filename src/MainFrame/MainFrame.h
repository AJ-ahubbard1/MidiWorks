// MainFrame.h
#pragma once
#include <wx/wx.h>
#include <wx/aui/aui.h>
#include <memory>
#include "AppModel/AppModel.h"
#include "Panels/Panels.h"
#include "PaneInfo.h"


class MainFrame : public wxFrame {
public:
    MainFrame();

private:
    std::shared_ptr<AppModel> mAppModel;
    wxAuiManager mAuiManager;
    wxTimer mTimer;

    // Panel Pointers
	std::unordered_map<int, PanelInfo> mPanels;
    MidiSettingsPanel* mMidiSettingsPanel;
    SoundBankPanel* mSoundBankPanel;
    TransportPanel* mTransportPanel;
    MidiCanvasPanel* mMidiCanvasPanel;
    LogPanel* mLogPanel;
    UndoHistoryPanel* mUndoHistoryPanel;

    void CreateDockablePanes();
    void RegisterPanel(const PanelInfo& info);
    std::unordered_map<int, PanelInfo>& GetAllPanels();
    void SetPanelVisibility(int id, bool vis);
    void CreateMenuBar();
    void CreateSizer();
    void SyncMenuChecks();
    void OnTogglePane(wxCommandEvent& event);
    void OnPaneClosed(wxAuiManagerEvent& event);
    void OnTimer(wxTimerEvent&);
    void OnAuiRender(wxAuiManagerEvent& event);
    void OnUndo(wxCommandEvent& event);
    void OnRedo(wxCommandEvent& event);
    uint64_t GetDeltaTimeMs();
};
