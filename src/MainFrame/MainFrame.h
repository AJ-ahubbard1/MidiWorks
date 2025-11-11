// MainFrame.h
#pragma once
#include <wx/wx.h>
#include <wx/aui/aui.h>
#include <memory>
#include "AppModel/AppModel.h"
#include "Panels/Panels.h"

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

    void CreateDockablePanes();
    void CreateMenuBar();
    void CreateSizer();
    void SyncMenuChecks();
    void OnTogglePane(wxCommandEvent& event);
    void OnPaneClosed(wxAuiManagerEvent& event);
    void OnTimer(wxTimerEvent&);
    void OnAuiRender(wxAuiManagerEvent& event);
    uint64_t GetDeltaTimeMs();
};
