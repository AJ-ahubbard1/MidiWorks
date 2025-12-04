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
	mTimer.Start(2);
	Bind(wxEVT_AUI_RENDER, &MainFrame::OnAuiRender, this);

	CreateStatusBar();
	SetStatusText("Thanks for using MidiWorks");
}

// Instantiate panels, define layout metadata, and add panel into mPanels map
void MainFrame::CreateDockablePanes()
{
	int idBase = wxID_HIGHEST + 1;
	auto& soundBank = mAppModel->GetSoundBank();
	mMidiSettingsPanel = new MidiSettingsPanel(this, mAppModel, *wxLIGHT_GREY, "Midi Settings");
	mSoundBankPanel = new SoundBankPanel(this, soundBank);
	mTransportPanel = new TransportPanel(this, mAppModel, *wxLIGHT_GREY, "Transport");
	mMidiCanvasPanel = new MidiCanvasPanel(this, mAppModel, "Canvas");
	mLogPanel = new LogPanel(this);
	mUndoHistoryPanel = new UndoHistoryPanel(this, mAppModel);

	// Define layout metadata and register each panel
	PanelInfo soundBankInfo
	{
		"Sound Bank", mSoundBankPanel, idBase++,
		PanePosition::Left, wxSize(247, 636)
	};
	PanelInfo midiSettingsPanelInfo
	{
		"Midi Settings", mMidiSettingsPanel, idBase++,
		PanePosition::Left, wxSize(247, 253), wxSize(-1, -1)
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
		"Midi Log", mLogPanel, idBase++,
		PanePosition::Right, wxSize(247, -1)
	};
	PanelInfo undoHistoryPanelInfo
	{
		"Undo History", mUndoHistoryPanel, idBase++,
		PanePosition::Right, wxSize(247, -1)
	};

	RegisterPanel(soundBankInfo);
	RegisterPanel(midiSettingsPanelInfo);
	RegisterPanel(transportPanelInfo);
	RegisterPanel(midiCanvasInfo);
	RegisterPanel(logPanelInfo);
	RegisterPanel(undoHistoryPanelInfo);
}

// Add Panels to map, used to toggle visibility
void MainFrame::RegisterPanel(const PanelInfo& info)
{
	// The IDs are incremented inside of CreateDockablePanes to insure they are unique
	mPanels.insert({info.menuId, info});
	mAuiManager.AddPane(info.window, CreatePaneInfo(info));
}

std::unordered_map<int, PanelInfo>& MainFrame::GetAllPanels() 
{ 
	return mPanels; 
}

void MainFrame::SetPanelVisibility(int id, bool vis) 
{ 
	mPanels.at(id).isVisible = vis; 
}

// Builds View Menu Dynamically from AppModel
void MainFrame::CreateMenuBar()
{
	auto* menuBar = new wxMenuBar();

	// Edit Menu - Undo/Redo
	auto* editMenu = new wxMenu();
	editMenu->Append(wxID_UNDO, "Undo\tCtrl+Z", "Undo last action");
	editMenu->Append(wxID_REDO, "Redo\tCtrl+Y", "Redo last undone action");
	Bind(wxEVT_MENU, &MainFrame::OnUndo, this, wxID_UNDO);
	Bind(wxEVT_MENU, &MainFrame::OnRedo, this, wxID_REDO);
	menuBar->Append(editMenu, "Edit");

	// View Menu - Dockable Panels
	auto* viewMenu = new wxMenu();
	for (const auto& [id, info] : GetAllPanels())
	{
		viewMenu->AppendCheckItem(id, "Show " + info.name);
		Bind(wxEVT_MENU, &MainFrame::OnTogglePane, this, id);
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

// Toggle visibility of panes associated with clicked panes in view menu
void MainFrame::OnTogglePane(wxCommandEvent& event)
{
	for (const auto& [id, info] : GetAllPanels())
	{
		// find the correct pane and flip visibility
		// new state needs to be passed to AUI manager, menu, and AppModel
		if (event.GetId() == id)
		{
			auto& pane = mAuiManager.GetPane(info.name);
			bool newState = !pane.IsShown();
			pane.Show(newState);
			mAuiManager.Update();

			GetMenuBar()->Check(id, newState);
			SetPanelVisibility(id, newState);
			break;
		}
	}
}

// Update AppModel and view menu when a pane's [x] close button is clicked
void MainFrame::OnPaneClosed(wxAuiManagerEvent& event)
{
	wxString name = event.GetPane()->name;

	for (const auto& [id, info] : GetAllPanels())
	{
		if (info.name == name)
		{
			GetMenuBar()->Check(id, false);
			SetPanelVisibility(id, false);
			break;
		}
	}
}

void MainFrame::OnTimer(wxTimerEvent&)
{
	mAppModel->Update();
	// @TODO: If needed later on, make a MainFrame::Update function for the multiple panes
	mTransportPanel->UpdateDisplay();
	mMidiCanvasPanel->Update();
	if (mAppModel->mUpdateLog)
	{
		mLogPanel->LogMidiEvent(mAppModel->mLogMessage);
		mAppModel->mUpdateLog = false;
	}
}

// Debug Tool for layout, when docked panes are resized, the dimensions are shown in controlbar
void MainFrame::OnAuiRender(wxAuiManagerEvent& event)
{
	wxString msg = "Layout changed";
	for (const auto& pane : mAuiManager.GetAllPanes())
	{
		wxSize sz = pane.window->GetSize();
		msg += wxString::Format("|%s: %d x %d", pane.name, sz.GetWidth(), sz.GetHeight());
	}
	SetStatusText(msg);
	event.Skip();
}

// After Panes are created, set the checks in the views menu to match the panes' visibilities
void MainFrame::SyncMenuChecks()
{
	for (const auto& [id, info] : GetAllPanels())
	{
		GetMenuBar()->Check(id, info.isVisible);
	}
}

// Undo last action (Ctrl+Z)
void MainFrame::OnUndo(wxCommandEvent& event)
{
	mAppModel->Undo();
	mUndoHistoryPanel->UpdateDisplay();  // Update command history display
	Refresh();  // Redraw canvas to show changes
}

// Redo last undone action (Ctrl+Y)
void MainFrame::OnRedo(wxCommandEvent& event)
{
	mAppModel->Redo();
	mUndoHistoryPanel->UpdateDisplay();  // Update command history display
	Refresh();  // Redraw canvas to show changes
}
