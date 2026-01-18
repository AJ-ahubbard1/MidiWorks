// MainFrameEventHandlers.cpp
// Event handler implementations for MainFrame
#include "MainFrame.h"
#include "Commands/TrackCommands.h"

// TIMER EVENTS

/// Update the App Model, then necessary panels
/// Updates here should be reserved for real-time state changes
/// Opt for event-driven callbacks for discrete state changes
/// Every millisecond counts!
void MainFrame::OnTimer(wxTimerEvent&)
{
	mAppModel->Update();

	mTransportPanel->Update(); // Update the tick display 
	mMidiCanvasPanel->Update();
	// Note: Logging and drum machine updates now handled via callbacks
	// no polling needed, see MainFrame::CreateCallbackFunctions()
}

// VIEW / PANEL MANAGEMENT EVENTS

/// Toggle visibility of panes associated with clicked panes in view menu
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

/// Update AppModel and view menu when a pane's [x] close button is clicked
void MainFrame::OnPaneClosed(wxAuiManagerEvent& event)
{
	wxString name = event.GetPane()->name;
	ClosePane(name);
}

void MainFrame::ClosePane(const wxString& paneName)
{
	for (const auto& [id, info] : GetAllPanels())
	{
		if (info.name == paneName)
		{
			GetMenuBar()->Check(id, false);
			SetPanelVisibility(id, false);
			break;
		}
	}
}

/// Debug display for layout
/// when docked panes are resized, the dimensions are shown in controlbar
void MainFrame::OnAuiRender(wxAuiManagerEvent& event)
{
	wxString msg = "Layout changed";
	wxAuiPaneInfoArray& panes = mAuiManager.GetAllPanes();
	for (size_t i = 0; i < panes.GetCount(); i++)
	{
		wxAuiPaneInfo& pane = panes.Item(i);
		wxSize sz = pane.window->GetSize();
		msg += wxString::Format(" |%s: %d x %d", pane.name, sz.GetWidth(), sz.GetHeight());
	}
	SetStatusText(msg);
	event.Skip();
}

// EDIT MENU EVENTS

/// Undo last action (Ctrl+Z)
void MainFrame::OnUndo(wxCommandEvent& event)
{
	// Stop playback/recording to prevent iterator invalidation
	// (can't modify tracks while they're being iterated during playback)
	mAppModel->GetTransport().StopPlaybackIfActive();
	mAppModel->GetUndoRedoManager().Undo();
	mUndoHistoryPanel->UpdateDisplay();  // Update command history display
	Refresh();  // Redraw canvas to show changes
}

/// Redo last undone action (Ctrl+Y)
void MainFrame::OnRedo(wxCommandEvent& event)
{
	// Stop playback/recording to prevent iterator invalidation
	// (can't modify tracks while they're being iterated during playback)
	mAppModel->GetTransport().StopPlaybackIfActive();
	mAppModel->GetUndoRedoManager().Redo();
	mUndoHistoryPanel->UpdateDisplay();  // Update command history display
	Refresh();  // Redraw canvas to show changes
}

/// Context-aware quantization to grid (Q)
void MainFrame::OnQuantize(wxCommandEvent& event)
{
	uint64_t gridSize = mMidiCanvasPanel->GetGridSize();
	mAppModel->Quantize(gridSize);
	mUndoHistoryPanel->UpdateDisplay();
	Refresh();
}

// FILE MENU EVENTS - connect to AppModel's ProjectManager 
// Error dialogs are shown via ErrorCallback with detailed message

/// HELPER METHOD for file operations 
/// Prompts user about unsaved changes and optionally saves
/// Returns: Continue (proceed with action) or Cancel (abort action)
MainFrame::UnsavedChangesAction MainFrame::PromptForUnsavedChanges()
{
	if (!mAppModel->GetProjectManager().IsProjectDirty()) 
	{
		return UnsavedChangesAction::Continue;  // No unsaved changes, proceed
	}

	int result = wxMessageBox(
		"Do you want to save changes to the current project?",
		"Unsaved Changes",
		wxYES_NO | wxCANCEL | wxICON_QUESTION);

	if (result == wxYES) 
	{
		wxCommandEvent saveEvent;
		OnSave(saveEvent);
		return UnsavedChangesAction::Continue;
	}
	else if (result == wxCANCEL) 
	{
		return UnsavedChangesAction::Cancel;
	}

	// User clicked NO - discard changes and continue
	return UnsavedChangesAction::Continue;
}

/// Create new project (Ctrl+N)
void MainFrame::OnNew(wxCommandEvent& event)
{
	if (PromptForUnsavedChanges() == UnsavedChangesAction::Cancel) 
	{
		return;
	}

	// Clear all data
	mAppModel->GetProjectManager().ClearProject();

	// Update UI controls to reflect cleared state
	mSoundBankPanel->UpdateFromModel();
	mTransportPanel->UpdateTempoDisplay();

	UpdateTitle();
	Refresh();
}

/// Open existing project (Ctrl+O)
void MainFrame::OnOpen(wxCommandEvent& event)
{
	if (PromptForUnsavedChanges() == UnsavedChangesAction::Cancel) 
	{
		return;
	}

	wxFileDialog openDialog(this,
		"Open MidiWorks Project",
		wxEmptyString,
		wxEmptyString,
		"MidiWorks Projects (*.mwp)|*.mwp",
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	if (openDialog.ShowModal() == wxID_CANCEL) 
	{
		return;
	}

	std::string path = openDialog.GetPath().ToStdString();
	if (mAppModel->GetProjectManager().LoadProject(path))
	{
		// Update UI controls to reflect loaded data
		mSoundBankPanel->UpdateFromModel();
		mTransportPanel->UpdateTempoDisplay();

		UpdateTitle();
		Refresh();  // Redraw canvas with loaded data
	}
}

/// Save current project (Ctrl+S)
void MainFrame::OnSave(wxCommandEvent& event)
{
	if (mAppModel->GetProjectManager().GetCurrentProjectPath().empty())
	{
		// No path yet, use Save As
		OnSaveAs(event);
		return;
	}

	if (mAppModel->GetProjectManager().SaveProject(mAppModel->GetProjectManager().GetCurrentProjectPath()))
	{
		UpdateTitle();  // Title updates automatically via callback, but ensure it's current
	}
}

/// Save project with new name (Ctrl+Shift+S)
void MainFrame::OnSaveAs(wxCommandEvent& event)
{
	wxFileDialog saveDialog(this,
		"Save MidiWorks Project",
		wxEmptyString,
		wxEmptyString,
		"MidiWorks Projects (*.mwp)|*.mwp",
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

	if (saveDialog.ShowModal() == wxID_CANCEL) 
	{
		return;
	}

	std::string path = saveDialog.GetPath().ToStdString();
	if (mAppModel->GetProjectManager().SaveProject(path))
	{
		UpdateTitle();  // Title updates automatically via callback, but ensure it's current
	}
}

/// Load midi (.mid) file into a new project
void MainFrame::OnImportMidiFile(wxCommandEvent& event)
{
	// Warn user that importing will clear the current project
	int response = wxMessageBox(
		"Importing a MIDI file will clear the current project.\n\nDo you want to continue?",
		"Warning",
		wxYES_NO | wxICON_WARNING,
		this
	);

	if (response != wxYES)
	{
		return;
	}

	wxFileDialog openDialog(this,
		"Import MIDI File",
		wxEmptyString,
		wxEmptyString,
		"MIDI Files (*.mid;*.midi)|*.mid;*.midi",
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	if (openDialog.ShowModal() == wxID_CANCEL)
	{
		return;
	}

	std::string path = openDialog.GetPath().ToStdString();
	if (mAppModel->GetProjectManager().ImportMIDI(path))
	{
		wxMessageBox("MIDI file imported successfully", "Import Complete", wxOK | wxICON_INFORMATION);

		// Update title to reflect dirty state
		UpdateTitle();
		// Show new tempo from midifile
		if (mTransportPanel)
		{
			mTransportPanel->UpdateTempoDisplay();
		}
		// Show patch changes from midifile
		if (mSoundBankPanel)
		{
			mSoundBankPanel->UpdateFromModel();
		}
	}
}

/// Save project as a midi file (.mid)
void MainFrame::OnExportMidiFile(wxCommandEvent& event)
{
	wxFileDialog saveDialog(this,
		"Export MIDI File",
		wxEmptyString,
		wxEmptyString,
		"MIDI Files (*.mid)|*.mid",
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

	if (saveDialog.ShowModal() == wxID_CANCEL)
	{
		return;
	}

	std::string path = saveDialog.GetPath().ToStdString();
	if (mAppModel->GetProjectManager().ExportMIDI(path))
	{
		wxMessageBox("MIDI file exported successfully", "Export Complete", wxOK | wxICON_INFORMATION);
	}
}


// APPLICATION LIFECYCLE EVENTS

/// Exit application (Alt+F4 or File > Exit)
void MainFrame::OnExit(wxCommandEvent& event)
{
	// Trigger close event which handles unsaved changes
	Close(false);
}

/// Window close event (X button or Close() call)
void MainFrame::OnClose(wxCloseEvent& event)
{
	if (PromptForUnsavedChanges() == UnsavedChangesAction::Cancel)
	{
		// Cancel the close
		event.Veto();  	
		return;
	}

	// Stop the timer before destroying panels to prevent slow shutdown
	mTimer.Stop();
	// Allow the window to close
	event.Skip(); 		
}

// TRANSPORT CONTROL EVENTS

/// Toggle play/pause (Spacebar)
void MainFrame::OnTogglePlay(wxCommandEvent& event)
{
	mAppModel->GetTransport().TogglePlay();
}

/// Toggle record (R key)
void MainFrame::OnStartRecord(wxCommandEvent& event)
{
	mAppModel->GetTransport().ToggleRecord();
}

/// Previous Measure (Left Arrow key)
void MainFrame::OnPreviousMeasure(wxCommandEvent& event)
{
	mAppModel->GetTransport().JumpToPreviousMeasure();
}

/// Next Measure (Right Arrow key)
void MainFrame::OnNextMeasure(wxCommandEvent& event)
{
	mAppModel->GetTransport().JumpToNextMeasure();
}

// DRUM PAD TRIGGER EVENTS

/// Trigger drum pad via keyboard (1-0 keys map to rows 0-9)
void MainFrame::OnDrumPad(wxCommandEvent& event)
{
	int eventId = event.GetId();
	int rowIndex = -1;

	// Map event ID to drum row index
	if (eventId >= ID_KEYBOARD_DRUM_PAD_1 && eventId <= ID_KEYBOARD_DRUM_PAD_9)
	{
		rowIndex = eventId - ID_KEYBOARD_DRUM_PAD_1;  // 1-9 = rows 0-8
	}
	else if (eventId == ID_KEYBOARD_DRUM_PAD_0)
	{
		rowIndex = 9;  // 0 = row 9
	}

	if (rowIndex >= 0)
	{
		int columnIndex = mAppModel->TriggerDrumPad(rowIndex);

		// Update pad button color if a pad was enabled during loop playback
		if (columnIndex >= 0 && mDrumMachinePanel)
		{
			mDrumMachinePanel->RefreshPadButtonColor(rowIndex, columnIndex);
		}
	}
}
