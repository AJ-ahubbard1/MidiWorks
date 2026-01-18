// MainFrame.cpp
#include "MainFrame.h"
#include <wx/string.h>
#include "Commands/TrackCommands.h"


MainFrame::MainFrame()
	: wxFrame(nullptr, wxID_ANY, "MidiWorks")
{
	mAppModel = std::make_shared<AppModel>();
	mAuiManager.SetManagedWindow(this);

	CreateDockablePanes();
	CreateCallbackFunctions();
	CreateMenuBar();
	SyncMenuChecks();
	CreateSizer();

	Bind(wxEVT_AUI_PANE_CLOSE, &MainFrame::OnPaneClosed, this);
	Bind(wxEVT_CLOSE_WINDOW, &MainFrame::OnClose, this);
	mModelTimer.Bind(wxEVT_TIMER, &MainFrame::OnModelTimer, this);
	mDisplayTimer.Bind(wxEVT_TIMER, &MainFrame::OnDisplayTimer, this);

	// Set up Keyboard Shortcuts, now delegated to KeyboardHandler
	mKeyboardHandler = std::make_unique<KeyboardHandler>(this, mAppModel);
	mKeyboardHandler->Initialize();

	// Fix Linux control sizes before AUI layout
	FixLinuxControlSizes(this);

	mAuiManager.Update();
	mModelTimer.Start(1);
	mDisplayTimer.Start(16);
	Bind(wxEVT_AUI_RENDER, &MainFrame::OnAuiRender, this);

	CreateStatusBar();
	SetStatusText("Thanks for using MidiWorks");
}

// Instantiate panels, define layout metadata, and register each panel (IDs auto-assigned)
void MainFrame::CreateDockablePanes()
{
	mSoundBankPanel = new SoundBankPanel(this, mAppModel);
	RegisterPanel({"Sound Bank", mSoundBankPanel, PanePosition::Left, wxSize(313, 636)});

	mMidiSettingsPanel = new MidiSettingsPanel(this, mAppModel, *wxLIGHT_GREY, "Midi Settings");
	RegisterPanel({"Midi Settings", mMidiSettingsPanel, PanePosition::Left, wxSize(247, 253), false});

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

	mDrumMachinePanel = new DrumMachinePanel(this, mAppModel);
	RegisterPanel({"Drum Machine", mDrumMachinePanel, PanePosition::Float, wxSize(600, 400), false});

}
// Event-driven callback functions for discrete state changes
// Add callback functions here
void MainFrame::CreateCallbackFunctions()
{
	// Register log callback for MIDI event logging
	mAppModel->GetMidiInputManager().SetLogCallback([this](const TimedMidiEvent& event)
	{
		if (mLogPanel)
		{
			mLogPanel->LogMidiEvent(event);
		}
	});

	// Register dirty state callback for title bar updates
	auto& projectManager = mAppModel->GetProjectManager();
	projectManager.SetDirtyStateCallback([this](bool isDirty)
	{
		UpdateTitle();
	});
	
	// Register loop changed callback for drum machine grid updates
	mAppModel->GetTransport().SetLoopChangedCallback([this]()
	{
		if (mDrumMachinePanel)
		{
			// Update pattern with new loop duration first
			auto loopSettings = mAppModel->GetTransport().GetLoopSettings();
			uint64_t loopDuration = loopSettings.endTick - loopSettings.startTick;
			mAppModel->GetDrumMachine().UpdatePattern(loopDuration);

			// Refresh pad colors (efficient - no widget recreation)
			mDrumMachinePanel->UpdateTicksPerColumnDisplay();
			mDrumMachinePanel->RefreshAllPadButtonColors();
		}
	});

	// Error handling callback - displays errors to user
	mAppModel->SetErrorCallback([this](const std::string& title, const std::string& msg, ErrorLevel level) 
	{
    	long style = wxOK;
    	switch (level) 
		{
        case ErrorLevel::Info:    
			style |= wxICON_INFORMATION; 
			break;
        case ErrorLevel::Warning: 
			style |= wxICON_WARNING; 
			break;
        case ErrorLevel::Error:   
			style |= wxICON_ERROR; 
			break;
    	}
    	wxMessageBox(msg, title, style);
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

bool MainFrame::CheckPanelVisibility(const wxString& panelName)
{
	for (const auto& [id, info] : GetAllPanels())
	{
		if (info.name == panelName)
		{
			return info.isVisible;
		}
	}
	return false;
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
	fileMenu->Append(ID_MENU_IMPORT_MIDIFILE, "Import Midi File...", "Import MIDI file into project");
	fileMenu->Append(ID_MENU_EXPORT_MIDIFILE, "Export Midi File...", "Export project as a midi file");
	fileMenu->AppendSeparator();
	fileMenu->Append(wxID_EXIT, "E&xit\tAlt+F4", "Exit MidiWorks");

	// Use event ids and bind them to event handlers
	Bind(wxEVT_MENU, &MainFrame::OnNew, this, wxID_NEW);
	Bind(wxEVT_MENU, &MainFrame::OnOpen, this, wxID_OPEN);
	Bind(wxEVT_MENU, &MainFrame::OnSave, this, wxID_SAVE);
	Bind(wxEVT_MENU, &MainFrame::OnSaveAs, this, wxID_SAVEAS);
	Bind(wxEVT_MENU, &MainFrame::OnImportMidiFile, this, ID_MENU_IMPORT_MIDIFILE);
	Bind(wxEVT_MENU, &MainFrame::OnExportMidiFile, this, ID_MENU_EXPORT_MIDIFILE);
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

void MainFrame::FixLinuxControlSizes(wxWindow* parent)
{
#ifdef __WXGTK__
	wxWindowList& children = parent->GetChildren();
	for (wxWindow* child : children)
	{
		wxString className = child->GetClassInfo()->GetClassName();

		if (wxButton* btn = dynamic_cast<wxButton*>(child))
		{
			// GTK3 needs 34px minimum (17px padding each side) + label width
			// Set to 60px to ensure comfortable fit for most button labels
			btn->SetMinSize(wxSize(60, 28));
			wxLogDebug("Fixed button: %s -> MinSize: 60x28", btn->GetLabel());
		}
		else if (wxSpinCtrl* spin = dynamic_cast<wxSpinCtrl*>(child))
		{
			// GTK3 needs 32px minimum for spin controls
			// Set to 70px to accommodate arrows + text entry
			spin->SetMinSize(wxSize(70, -1));
			wxLogDebug("Fixed spin control -> MinSize: 70x-1");
		}

		// Recurse into child containers
		FixLinuxControlSizes(child);
	}

	// Force layout recalculation after setting minimum sizes
	parent->Layout();
	parent->Refresh();
#endif
}
