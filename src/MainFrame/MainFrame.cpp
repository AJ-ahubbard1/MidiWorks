// MainFrame.cpp
#include "MainFrame.h"
#include <wx/string.h>
#include "Commands/QuantizeCommand.h"


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
	Bind(wxEVT_CLOSE_WINDOW, &MainFrame::OnClose, this);
	mTimer.Bind(wxEVT_TIMER, &MainFrame::OnTimer, this);

	// Set up keyboard shortcuts for transport control
	wxAcceleratorEntry entries[5];
	entries[0].Set(wxACCEL_NORMAL, WXK_SPACE, ID_KEYBOARD_TOGGLE_PLAY);		// Spacebar = Toggle Play
	entries[1].Set(wxACCEL_NORMAL, 'R', ID_KEYBOARD_RECORD);				// R = Record
	entries[2].Set(wxACCEL_NORMAL, 'Q', ID_KEYBOARD_QUANTIZE);				// Q = Quantize
	entries[3].Set(wxACCEL_NORMAL, WXK_LEFT, ID_KEYBOARD_PREVIOUS_MEASURE); // LEFT Arrow = Previous Measure
	entries[4].Set(wxACCEL_NORMAL, WXK_RIGHT, ID_KEYBOARD_NEXT_MEASURE);	// RIGHT Arrow = Next Measure
	wxAcceleratorTable accelTable(5, entries);
	SetAcceleratorTable(accelTable);

	Bind(wxEVT_MENU, &MainFrame::OnTogglePlay, this, ID_KEYBOARD_TOGGLE_PLAY);
	Bind(wxEVT_MENU, &MainFrame::OnStartRecord, this, ID_KEYBOARD_RECORD);
	Bind(wxEVT_MENU, &MainFrame::OnPreviousMeasure, this, ID_KEYBOARD_PREVIOUS_MEASURE);
	Bind(wxEVT_MENU, &MainFrame::OnNextMeasure, this, ID_KEYBOARD_NEXT_MEASURE);

	mAuiManager.Update();
	mTimer.Start(1);
	Bind(wxEVT_AUI_RENDER, &MainFrame::OnAuiRender, this);

	CreateStatusBar();
	SetStatusText("Thanks for using MidiWorks");
}

// Instantiate panels, define layout metadata, and register each panel (IDs auto-assigned)
void MainFrame::CreateDockablePanes()
{
	auto& soundBank = mAppModel->GetSoundBank();

	mSoundBankPanel = new SoundBankPanel(this, soundBank);
	RegisterPanel({"Sound Bank", mSoundBankPanel, PanePosition::Left, wxSize(247, 636)});

	mMidiSettingsPanel = new MidiSettingsPanel(this, mAppModel, *wxLIGHT_GREY, "Midi Settings");
	RegisterPanel({"Midi Settings", mMidiSettingsPanel, PanePosition::Left, wxSize(247, 253)});

	mTransportPanel = new TransportPanel(this, mAppModel, *wxLIGHT_GREY, "Transport");
	RegisterPanel({"Transport", mTransportPanel, PanePosition::Top, wxSize(-1, -1), true, false, false});

	mMidiCanvasPanel = new MidiCanvasPanel(this, mAppModel, "Canvas");
	RegisterPanel({"Midi Canvas", mMidiCanvasPanel, PanePosition::Center});

	mLogPanel = new LogPanel(this);
	RegisterPanel({"Midi Log", mLogPanel, PanePosition::Float, wxSize(247, 300), false});

	mUndoHistoryPanel = new UndoHistoryPanel(this, mAppModel);
	RegisterPanel({"Undo History", mUndoHistoryPanel, PanePosition::Float, wxSize(247, 300), false});

	mShortcutsPanel = new ShortcutsPanel(this, *wxLIGHT_GREY, "Shortcuts");
	RegisterPanel({"Shortcuts", mShortcutsPanel, PanePosition::Float, wxSize(347, 500), false});

	// Register log callback for MIDI event logging
	mAppModel->GetMidiInputManager().SetLogCallback([this](const TimedMidiEvent& event)
	{
		if (mLogPanel)
		{
			mLogPanel->LogMidiEvent(event);
		}
	});

	// Register dirty state callback for title bar updates
	mAppModel->SetDirtyStateCallback([this](bool isDirty) 
	{
		UpdateTitle();
	});
}

// Add Panels to map, used to toggle visibility
void MainFrame::RegisterPanel(PanelInfo info)
{
	// Auto-assign unique menu ID for this panel
	info.menuId = mNextPanelId++;

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

	// File Menu
	auto* fileMenu = new wxMenu();
	// Menu Append params: event id, label string, help string
	fileMenu->Append(wxID_NEW, "&New Project\tCtrl+N", "Create a new project");
	fileMenu->Append(wxID_OPEN, "&Open...\tCtrl+O", "Open a project");
	fileMenu->Append(wxID_SAVE, "&Save\tCtrl+S", "Save the current project");
	fileMenu->Append(wxID_SAVEAS, "Save &As...\tCtrl+Shift+S", "Save project with a new name");
	fileMenu->AppendSeparator();
	fileMenu->Append(wxID_EXIT, "E&xit\tAlt+F4", "Exit MidiWorks");

	// Use event ids and bind them to event handlers
	Bind(wxEVT_MENU, &MainFrame::OnNew, this, wxID_NEW);
	Bind(wxEVT_MENU, &MainFrame::OnOpen, this, wxID_OPEN);
	Bind(wxEVT_MENU, &MainFrame::OnSave, this, wxID_SAVE);
	Bind(wxEVT_MENU, &MainFrame::OnSaveAs, this, wxID_SAVEAS);
	Bind(wxEVT_MENU, &MainFrame::OnExit, this, wxID_EXIT);
	menuBar->Append(fileMenu, "&File");

	// Edit Menu - Undo/Redo
	auto* editMenu = new wxMenu();
	editMenu->Append(wxID_UNDO, "Undo\tCtrl+Z", "Undo last action");
	editMenu->Append(wxID_REDO, "Redo\tCtrl+Y", "Redo last undone action");
	editMenu->AppendSeparator();
	editMenu->Append(ID_KEYBOARD_QUANTIZE, "Quantize to Grid\tQ", "Snap all notes to nearest grid division");
	Bind(wxEVT_MENU, &MainFrame::OnUndo, this, wxID_UNDO);
	Bind(wxEVT_MENU, &MainFrame::OnRedo, this, wxID_REDO);
	Bind(wxEVT_MENU, &MainFrame::OnQuantize, this, ID_KEYBOARD_QUANTIZE);
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

// Update the App Model then necessary panels
void MainFrame::OnTimer(wxTimerEvent&)
{
	mAppModel->Update();
	mTransportPanel->UpdateDisplay();
	mMidiCanvasPanel->Update();
	// Note: Logging now handled via callback - no polling needed
}

// After Panes are created, set the checks in the views menu to match the panes' visibilities
void MainFrame::SyncMenuChecks()
{
	for (const auto& [id, info] : GetAllPanels())
	{
		GetMenuBar()->Check(id, info.isVisible);
	}
}

void MainFrame::UpdateTitle()
{
	std::string title = "MidiWorks - ";
	std::string path = mAppModel->GetProjectManager().GetCurrentProjectPath();

	if (path.empty())
	{
		title += "Untitled";
	}
	else
	{
		// Extract filename from full path
		size_t lastSlash = path.find_last_of("/\\");
		if (lastSlash != std::string::npos)
		{
			title += path.substr(lastSlash + 1);
		}
		else
		{
			title += path;
		}
	}

	// Add asterisk if dirty
	if (mAppModel->GetProjectManager().IsProjectDirty()) 
	{
		title += " *";
	}

	SetTitle(title);
}
