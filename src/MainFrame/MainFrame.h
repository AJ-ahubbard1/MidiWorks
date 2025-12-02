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
    wxAuiManager mAuiManager;
    std::shared_ptr<AppModel> mAppModel;
    wxTimer mTimer;
    MidiSettingsPanel* mMidiSettingsPanel;
    SoundBankPanel* mSoundBankPanel;
    TransportPanel* mTransportPanel;
    MidiCanvasPanel* mMidiCanvasPanel;
    LogPanel* mLogPanel;
	std::unordered_map<int, PanelInfo> mPanels;

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
    uint64_t GetDeltaTimeMs();
};
