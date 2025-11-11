// MainFrame.cpp
#include "MainFrame.h"
#include <wx/string.h>


MainFrame::MainFrame()
	: wxFrame(nullptr, wxID_ANY, "MidiWorks")
{
	mAppModel = std::make_shared<AppModel>();
	mAuiManager.SetManagedWindow(this);

	CreateDockablePanes();
	CreateMenuBar();
	SyncMenuChecks();
	CreateSizer();

	Bind(wxEVT_AUI_PANE_CLOSE, &MainFrame::OnPaneClosed, this);
	mTimer.Bind(wxEVT_TIMER, &MainFrame::OnTimer, this);

	mAuiManager.Update();
	mTimer.Start(10);
	Bind(wxEVT_AUI_RENDER, &MainFrame::OnAuiRender, this);
	CreateStatusBar();
	SetStatusText("Thanks for using MidiWorks");
}

// Instantiate panels and register them in AppModel
void MainFrame::CreateDockablePanes() 
{
	int idBase = wxID_HIGHEST + 1;
	auto& soundBank = mAppModel->GetSoundBank();
	auto& transport = mAppModel->GetTransport();
	mMidiSettingsPanel = new MidiSettingsPanel(this, mAppModel, *wxLIGHT_GREY, "Midi Settings");
	mSoundBankPanel = new SoundBankPanel(this, soundBank);
	mTransportPanel = new TransportPanel(this, transport, *wxLIGHT_GREY, "Transport");
	mMidiCanvasPanel = new MidiCanvasPanel(this, transport, mAppModel->GetTrack(0), "Canvas");
	mLogPanel = new LogPanel(this);

	// Define layout metadata and register each panel
	PanelInfo midiSettingsPanelInfo
	{
		"Midi Settings", mMidiSettingsPanel, idBase++,
		PanePosition::Float, wxSize(169, -1), wxSize(-1, -1)	
	};
	PanelInfo soundBankInfo
	{
		"Sound Bank", mSoundBankPanel, idBase++, 
		PanePosition::Left, wxSize(247, -1)
	};
	PanelInfo transportPanelInfo
	{
		"Transport", mTransportPanel, idBase++,
		PanePosition::Top, wxSize(-1, -1), wxSize(-1, -1), false, false
	};
	PanelInfo midiCanvasInfo
	{
		"Midi Canvas", mMidiCanvasPanel, idBase++, PanePosition::Center
	};
	PanelInfo logPanelInfo
	{
		"Midi Log", mLogPanel, idBase++, PanePosition::Right
	};

	// Add PanelIDs as needed inside of AppModel/PanelInfo.h
	mAppModel->RegisterPanel(PanelID::MidiSettings, midiSettingsPanelInfo);
	mAppModel->RegisterPanel(PanelID::SoundBank, soundBankInfo);
	mAppModel->RegisterPanel(PanelID::Transport, transportPanelInfo);
	mAppModel->RegisterPanel(PanelID::MidiCanvas, midiCanvasInfo);
	mAppModel->RegisterPanel(PanelID::Log, logPanelInfo);

	// CreatePaneInfo defined inside AppModel.h, returns type wxAuiPaneInfo
	mAuiManager.AddPane(mMidiSettingsPanel, CreatePaneInfo(midiSettingsPanelInfo));
	mAuiManager.AddPane(mSoundBankPanel, CreatePaneInfo(soundBankInfo));
	mAuiManager.AddPane(mTransportPanel, CreatePaneInfo(transportPanelInfo));
	mAuiManager.AddPane(mMidiCanvasPanel, CreatePaneInfo(midiCanvasInfo));
	mAuiManager.AddPane(mLogPanel, CreatePaneInfo(logPanelInfo));
}

// Builds View Menu Dynamically from AppModel
void MainFrame::CreateMenuBar()
{
	auto* menuBar = new wxMenuBar();
	auto* viewMenu = new wxMenu();

	for (const auto& [id, info] : mAppModel->GetAllPanels())
	{
		viewMenu->AppendCheckItem(info.menuId, "Show " + info.name);
		Bind(wxEVT_MENU, &MainFrame::OnTogglePane, this, info.menuId);
	}
	menuBar->Append(viewMenu, "View");
	SetMenuBar(menuBar);
}

// Apply vertical sizer to the frame 
void MainFrame::CreateSizer()
{
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(this, 1, wxEXPAND);
	SetSizer(sizer);
}

// Toggle visibility of panes associated with clicked menu items
void MainFrame::OnTogglePane(wxCommandEvent& event)
{
	for (const auto& [id, info] : mAppModel->GetAllPanels())
	{
		// find the correct pane and flip visibility
		// new state needs to be passed to AUI manager, menu, and AppModel
		if (event.GetId() == info.menuId)
		{
			auto& pane = mAuiManager.GetPane(info.name);
			bool newState = !pane.IsShown();
			pane.Show(newState);
			mAuiManager.Update();

			GetMenuBar()->Check(info.menuId, newState);
			mAppModel->SetPanelVisibility(id, newState);
			break;
		}
	}
}

// Update AppModel and view menu when a pane's [x] close button is clicked
void MainFrame::OnPaneClosed(wxAuiManagerEvent& event)
{
	wxString name = event.GetPane()->name;

	for (const auto& [id, info] : mAppModel->GetAllPanels())
	{
		if (info.name == name)
		{
			GetMenuBar()->Check(info.menuId, false);
			mAppModel->SetPanelVisibility(id, false);
			break;
		}
	}
}

void MainFrame::OnTimer(wxTimerEvent&)
{
	mAppModel->Update();
	// @TODO: If needed later on, make a MainFrame::Update function
	// for multiple panes
	mTransportPanel->UpdateDisplay();
	mMidiCanvasPanel->Update();
	if (mAppModel->mUpdateLog)
	{
		mLogPanel->PrependMessage(mAppModel->mLogMessage);
		mAppModel->mUpdateLog = false;
	}
}

void MainFrame::OnAuiRender(wxAuiManagerEvent& event)
{
	wxString msg = "Layout changed";
	for (const auto& pane : mAuiManager.GetAllPanes())
	{
		wxSize sz = pane.window->GetSize();
		msg += wxString::Format("| %s: %d x %d", pane.name, sz.GetWidth(), sz.GetHeight());
	}
	SetStatusText(msg);
	event.Skip();
}

// After Panes are created, set views menu checks to match visibilities in AppModel
void MainFrame::SyncMenuChecks()
{
	for (const auto& [id, info] : mAppModel->GetAllPanels())
	{
		if (info.menuId != -1)
		{
			GetMenuBar()->Check(info.menuId, info.isVisible);
		}
	}
}
