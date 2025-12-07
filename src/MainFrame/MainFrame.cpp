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
	mTimer.Bind(wxEVT_TIMER, &MainFrame::OnTimer, this);

	// Set up keyboard shortcuts for transport control
	wxAcceleratorEntry entries[3];
	entries[0].Set(wxACCEL_NORMAL, WXK_SPACE, wxID_HIGHEST + 1000);  // Spacebar = Toggle Play
	entries[1].Set(wxACCEL_NORMAL, 'R', wxID_HIGHEST + 1001);        // R = Record
	entries[2].Set(wxACCEL_NORMAL, 'Q', wxID_HIGHEST + 1002);        // Q = Quantize
	wxAcceleratorTable accelTable(3, entries);
	SetAcceleratorTable(accelTable);

	Bind(wxEVT_MENU, &MainFrame::OnTogglePlay, this, wxID_HIGHEST + 1000);
	Bind(wxEVT_MENU, &MainFrame::OnStartRecord, this, wxID_HIGHEST + 1001);

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
	mShortcutsPanel = new ShortcutsPanel(this, *wxLIGHT_GREY, "Shortcuts");

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
	PanelInfo shortcutsPanelInfo
	{
		"Shortcuts", mShortcutsPanel, idBase++,
		PanePosition::Right, wxSize(347, -1)
	};

	RegisterPanel(soundBankInfo);
	RegisterPanel(midiSettingsPanelInfo);
	RegisterPanel(transportPanelInfo);
	RegisterPanel(midiCanvasInfo);
	RegisterPanel(logPanelInfo);
	RegisterPanel(undoHistoryPanelInfo);
	RegisterPanel(shortcutsPanelInfo);

	// Register log callback for MIDI event logging
	mAppModel->SetLogCallback([this](const TimedMidiEvent& event) {
		if (mLogPanel) {
			mLogPanel->LogMidiEvent(event);
		}
	});

	// Register dirty state callback for title bar updates
	mAppModel->SetDirtyStateCallback([this](bool isDirty) {
		UpdateTitle();
	});
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

	// File Menu
	auto* fileMenu = new wxMenu();
	fileMenu->Append(wxID_NEW, "&New Project\tCtrl+N", "Create a new project");
	fileMenu->Append(wxID_OPEN, "&Open...\tCtrl+O", "Open a project");
	fileMenu->Append(wxID_SAVE, "&Save\tCtrl+S", "Save the current project");
	fileMenu->Append(wxID_SAVEAS, "Save &As...\tCtrl+Shift+S", "Save project with a new name");
	fileMenu->AppendSeparator();
	fileMenu->Append(wxID_EXIT, "E&xit\tAlt+F4", "Exit MidiWorks");

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
	editMenu->Append(wxID_HIGHEST + 1002, "Quantize to Grid\tQ", "Snap all notes to nearest grid division");
	Bind(wxEVT_MENU, &MainFrame::OnUndo, this, wxID_UNDO);
	Bind(wxEVT_MENU, &MainFrame::OnRedo, this, wxID_REDO);
	Bind(wxEVT_MENU, &MainFrame::OnQuantize, this, wxID_HIGHEST + 1002);
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
	// Note: Logging now handled via callback - no polling needed
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
	// Stop playback/recording to prevent iterator invalidation
	// (can't modify tracks while they're being iterated during playback)
	auto& transport = mAppModel->GetTransport();
	if (transport.mState == Transport::State::Playing)
	{
		transport.mState = Transport::State::StopPlaying;
	}
	else if (transport.mState == Transport::State::Recording)
	{
		transport.mState = Transport::State::StopRecording;
	}

	mAppModel->Undo();
	mUndoHistoryPanel->UpdateDisplay();  // Update command history display
	Refresh();  // Redraw canvas to show changes
}

// Redo last undone action (Ctrl+Y)
void MainFrame::OnRedo(wxCommandEvent& event)
{
	// Stop playback/recording to prevent iterator invalidation
	// (can't modify tracks while they're being iterated during playback)
	auto& transport = mAppModel->GetTransport();
	if (transport.mState == Transport::State::Playing)
	{
		transport.mState = Transport::State::StopPlaying;
	}
	else if (transport.mState == Transport::State::Recording)
	{
		transport.mState = Transport::State::StopRecording;
	}

	mAppModel->Redo();
	mUndoHistoryPanel->UpdateDisplay();  // Update command history display
	Refresh();  // Redraw canvas to show changes
}

// Quantize all tracks to grid (Q)
void MainFrame::OnQuantize(wxCommandEvent& event)
{
	// Stop playback/recording to prevent iterator invalidation
	auto& transport = mAppModel->GetTransport();
	if (transport.mState == Transport::State::Playing)
	{
		transport.mState = Transport::State::StopPlaying;
	}
	else if (transport.mState == Transport::State::Recording)
	{
		transport.mState = Transport::State::StopRecording;
	}

	// Get grid size from MidiCanvas duration selector
	// This way, users can choose quantize resolution by changing the dropdown
	uint64_t gridSize = mMidiCanvasPanel->GetGridSize();

	// Quantize all non-empty tracks
	auto& trackSet = mAppModel->GetTrackSet();
	for (int i = 0; i < 15; i++)
	{
		Track& track = trackSet.GetTrack(i);
		if (!track.empty())
		{
			auto cmd = std::make_unique<QuantizeCommand>(track, gridSize);
			mAppModel->ExecuteCommand(std::move(cmd));
		}
	}

	mUndoHistoryPanel->UpdateDisplay();  // Update command history display
	Refresh();  // Redraw canvas to show changes
}

// File Menu Handlers
void MainFrame::OnNew(wxCommandEvent& event)
{
	// Check for unsaved changes
	if (mAppModel->IsProjectDirty()) {
		int result = wxMessageBox(
			"Do you want to save changes to the current project?",
			"Unsaved Changes",
			wxYES_NO | wxCANCEL | wxICON_QUESTION);

		if (result == wxYES) {
			OnSave(event);
		}
		else if (result == wxCANCEL) {
			return;
		}
	}

	// Clear all data
	mAppModel->ClearProject();

	// Update UI controls to reflect cleared state
	mSoundBankPanel->UpdateFromModel();
	mTransportPanel->UpdateTempoDisplay();

	UpdateTitle();
	Refresh();
}

void MainFrame::OnOpen(wxCommandEvent& event)
{
	// Check for unsaved changes
	if (mAppModel->IsProjectDirty()) {
		int result = wxMessageBox(
			"Do you want to save changes to the current project?",
			"Unsaved Changes",
			wxYES_NO | wxCANCEL | wxICON_QUESTION);

		if (result == wxYES) {
			OnSave(event);
		}
		else if (result == wxCANCEL) {
			return;
		}
	}

	wxFileDialog openDialog(this,
		"Open MidiWorks Project",
		wxEmptyString,
		wxEmptyString,
		"MidiWorks Projects (*.mwp)|*.mwp",
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	if (openDialog.ShowModal() == wxID_CANCEL) {
		return;
	}

	std::string path = openDialog.GetPath().ToStdString();
	if (mAppModel->LoadProject(path)) {
		// Update UI controls to reflect loaded data
		mSoundBankPanel->UpdateFromModel();
		mTransportPanel->UpdateTempoDisplay();

		UpdateTitle();
		Refresh();  // Redraw canvas with loaded data
	}
	else {
		wxMessageBox("Failed to load project", "Error", wxOK | wxICON_ERROR);
	}
}

void MainFrame::OnSave(wxCommandEvent& event)
{
	if (mAppModel->GetCurrentProjectPath().empty()) {
		// No path yet, use Save As
		OnSaveAs(event);
		return;
	}

	if (mAppModel->SaveProject(mAppModel->GetCurrentProjectPath())) {
		UpdateTitle();  // Title updates automatically via callback, but ensure it's current
	}
	else {
		wxMessageBox("Failed to save project", "Error", wxOK | wxICON_ERROR);
	}
}

void MainFrame::OnSaveAs(wxCommandEvent& event)
{
	wxFileDialog saveDialog(this,
		"Save MidiWorks Project",
		wxEmptyString,
		wxEmptyString,
		"MidiWorks Projects (*.mwp)|*.mwp",
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

	if (saveDialog.ShowModal() == wxID_CANCEL) {
		return;
	}

	std::string path = saveDialog.GetPath().ToStdString();
	if (mAppModel->SaveProject(path)) {
		UpdateTitle();  // Title updates automatically via callback, but ensure it's current
	}
	else {
		wxMessageBox("Failed to save project", "Error", wxOK | wxICON_ERROR);
	}
}

void MainFrame::OnExit(wxCommandEvent& event)
{
	// Check for unsaved changes
	if (mAppModel->IsProjectDirty()) {
		int result = wxMessageBox(
			"Do you want to save changes before exiting?",
			"Unsaved Changes",
			wxYES_NO | wxCANCEL | wxICON_QUESTION);

		if (result == wxYES) {
			OnSave(event);
		}
		else if (result == wxCANCEL) {
			return;
		}
	}

	Close(true);
}

void MainFrame::UpdateTitle()
{
	std::string title = "MidiWorks - ";
	std::string path = mAppModel->GetCurrentProjectPath();

	if (path.empty()) {
		title += "Untitled";
	}
	else {
		// Extract filename from full path
		size_t lastSlash = path.find_last_of("/\\");
		if (lastSlash != std::string::npos) {
			title += path.substr(lastSlash + 1);
		}
		else {
			title += path;
		}
	}

	// Add asterisk if dirty
	if (mAppModel->IsProjectDirty()) {
		title += " *";
	}

	SetTitle(title);
}

// Transport keyboard shortcuts
void MainFrame::OnTogglePlay(wxCommandEvent& event)
{
	auto& transport = mAppModel->GetTransport();

	switch (transport.mState)
	{
	case Transport::State::Stopped:
		// Start playing
		transport.mState = Transport::State::ClickedPlay;
		break;

	case Transport::State::Playing:
		// Stop playing
		transport.mState = Transport::State::StopPlaying;
		break;

	case Transport::State::Recording:
		// Stop recording
		transport.mState = Transport::State::StopRecording;
		break;

	default:
		// For other states (ClickedPlay, ClickedRecord, etc.), do nothing
		break;
	}
}

void MainFrame::OnStartRecord(wxCommandEvent& event)
{
	auto& transport = mAppModel->GetTransport();

	// Start recording if not already in a recording-related state
	if (transport.mState == Transport::State::Stopped)
	{
		transport.mState = Transport::State::ClickedRecord;
	}
	else if (transport.mState == Transport::State::Recording)
	{
		// Toggle: stop recording if already recording
		transport.mState = Transport::State::StopRecording;
	}
}
