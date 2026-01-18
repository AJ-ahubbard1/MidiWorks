// ShortcutsPanel.h
#pragma once
#include <wx/wx.h>
#include <wx/scrolwin.h>

/// Panel displaying all keyboard shortcuts and mouse interactions.
///
/// Responsibilities:
/// - Display complete keyboard shortcut reference
/// - Show mouse interaction guide
/// - Provide tips for effective usage
/// - Enable scrolling for long content
class ShortcutsPanel : public wxScrolledWindow
{
public:
	ShortcutsPanel(wxWindow* parent, const wxColour& bgColor, const wxString& label)
		: wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL, label)
	{
		SetBackgroundColour(bgColor);
		CreateControls();
		SetupSizers();

		// Enable scrolling
		SetScrollRate(0, 10);
	}

private:
	wxBoxSizer* mMainSizer;

	void CreateControls()
	{
		mMainSizer = new wxBoxSizer(wxVERTICAL);

		// Title
		auto* title = new wxStaticText(this, wxID_ANY, "Keyboard & Mouse Reference");
		wxFont titleFont = title->GetFont();
		titleFont.SetPointSize(12);
		titleFont.SetWeight(wxFONTWEIGHT_BOLD);
		title->SetFont(titleFont);
		mMainSizer->Add(title, wxSizerFlags().Border(wxALL, 10).Center());

		// Separator
		mMainSizer->Add(new wxStaticLine(this), wxSizerFlags().Expand().Border(wxLEFT | wxRIGHT, 10));

		// File Operations Section
		AddSection("File Operations");
		AddShortcut("Ctrl+N", "New Project");
		AddShortcut("Ctrl+O", "Open Project");
		AddShortcut("Ctrl+S", "Save Project");
		AddShortcut("Ctrl+Shift+S", "Save As");
		AddShortcut("Alt+F4", "Exit MidiWorks");

		mMainSizer->AddSpacer(10);

		// Edit Operations Section
		AddSection("Edit Operations");
		AddShortcut("Ctrl+Z", "Undo");
		AddShortcut("Ctrl+Y", "Redo");
		AddShortcut("Q", "Quantize notes to grid");
		AddShortcut("Ctrl+C", "Copy selected notes");
		AddShortcut("Ctrl+V", "Paste notes at playhead");
		AddShortcut("Ctrl+Shift+V", "Paste notes to record-enabled tracks");
		AddShortcut("Ctrl+X", "Cut selected notes");

		mMainSizer->AddSpacer(10);

		// Piano Roll - Selection Section
		AddSection("Piano Roll - Selection");
		AddShortcut("Shift+Drag", "Rectangle selection");
		AddShortcut("Ctrl+A", "Select all notes");
		AddShortcut("Escape", "Clear selection");
		AddShortcut("Delete", "Delete selected notes");
		AddInfo("Selected notes have yellow borders");

		mMainSizer->AddSpacer(10);

		// Piano Roll - Mouse Interactions Section
		AddSection("Piano Roll - Mouse");
		AddShortcut("Left Click", "Add note (on empty space)");
		AddShortcut("Left Drag", "Move note (click and drag note)");
		AddShortcut("Left Drag", "Resize note (drag note edge)");
		AddShortcut("Middle Click", "Delete note");
		AddShortcut("Right Drag", "Pan view");
		AddShortcut("Mouse Wheel", "Zoom vertically (pitch)");
		AddShortcut("Shift+Wheel", "Zoom horizontally (time)");

		mMainSizer->AddSpacer(10);

		// Transport Controls Section
		AddSection("Transport Controls");
		AddShortcut("Spacebar", "Toggle Play/Stop");
		AddShortcut("R", "Toggle Record");
		AddShortcut("Left Arrow", "Jump to previous measure");
		AddShortcut("Right Arrow", "Jump to next measure");
		AddInfo("Transport buttons:");
		AddShortcut("Play Button", "Start playback");
		AddShortcut("Stop Button", "Stop playback/recording");
		AddShortcut("Record Button", "Start recording");
		AddShortcut("Rewind/FF", "Hold to seek (release to stop)");
		AddShortcut("Reset Button", "Jump to start (tick 0)");

		mMainSizer->AddSpacer(10);

		// Channel Controls Section
		AddSection("Channel Controls");
		AddShortcut("Program Dropdown", "Change instrument sound");
		AddShortcut("Volume Slider", "Adjust channel volume");
		AddShortcut("M Button", "Mute channel");
		AddShortcut("S Button", "Solo channel");
		AddShortcut("R Button", "Enable recording on channel");
		AddShortcut("Click Checkbox", "Enable/disable metronome");

		mMainSizer->AddSpacer(10);

		// Grid & Snap Section
		AddSection("Grid & Snap");
		AddShortcut("Snap Checkbox", "Toggle snap to grid");
		AddShortcut("Duration Dropdown", "Set grid size (whole/half/quarter/etc.)");
		AddInfo("When snap is enabled, notes align to grid");

		mMainSizer->AddSpacer(10);

		// Tips Section
		AddSection("Tips");
		AddTip("Recording is undoable! Press Ctrl+Z to remove a bad take.");
		AddTip("All edits are undoable with full history (up to 50 actions).");
		AddTip("Quantize (Q) snaps all notes to the selected grid size.");
		AddTip("Use Shift+Drag to select multiple notes, then copy/paste them!");
		AddTip("Solo mode: When any channel is solo'd, only solo channels play.");
		AddTip("Metronome uses channel 16 and plays woodblock sound.");
		AddTip("Grid lines: Light gray = beats, darker gray = measures.");
		AddTip("Your work is saved in .mwp files (JSON format).");

		mMainSizer->AddSpacer(20);
	}

	void SetupSizers()
	{
		wxBoxSizer* outerSizer = new wxBoxSizer(wxVERTICAL);
		outerSizer->Add(mMainSizer, wxSizerFlags(1).Expand().Border(wxALL, 5));
		SetSizer(outerSizer);
		FitInside(); // Important for scrolling
	}

	void AddSection(const wxString& sectionName)
	{
		auto* sectionLabel = new wxStaticText(this, wxID_ANY, sectionName);
		wxFont sectionFont = sectionLabel->GetFont();
		sectionFont.SetPointSize(10);
		sectionFont.SetWeight(wxFONTWEIGHT_BOLD);
		sectionLabel->SetFont(sectionFont);
		mMainSizer->Add(sectionLabel, wxSizerFlags().Border(wxLEFT | wxTOP, 10));
		mMainSizer->Add(new wxStaticLine(this), wxSizerFlags().Expand().Border(wxLEFT | wxRIGHT, 10));
	}

	void AddShortcut(const wxString& key, const wxString& description)
	{
		auto* rowSizer = new wxBoxSizer(wxHORIZONTAL);

		// Key/Button (bold)
		auto* keyLabel = new wxStaticText(this, wxID_ANY, key);
		wxFont keyFont = keyLabel->GetFont();
		keyFont.SetWeight(wxFONTWEIGHT_BOLD);
		keyLabel->SetFont(keyFont);
		rowSizer->Add(keyLabel, wxSizerFlags().Border(wxLEFT, 20));

		// Spacer
		rowSizer->AddSpacer(10);

		// Description
		auto* descLabel = new wxStaticText(this, wxID_ANY, "- " + description);
		rowSizer->Add(descLabel, wxSizerFlags(1));

		mMainSizer->Add(rowSizer, wxSizerFlags().Expand().Border(wxTOP, 3));
	}

	void AddInfo(const wxString& info)
	{
		auto* infoLabel = new wxStaticText(this, wxID_ANY, info);
		wxFont infoFont = infoLabel->GetFont();
		infoFont.SetStyle(wxFONTSTYLE_ITALIC);
		infoLabel->SetFont(infoFont);
		mMainSizer->Add(infoLabel, wxSizerFlags().Border(wxLEFT | wxTOP, 20));
	}

	void AddTip(const wxString& tip)
	{
		auto* tipLabel = new wxStaticText(this, wxID_ANY, "• " + tip);
		mMainSizer->Add(tipLabel, wxSizerFlags().Border(wxLEFT | wxTOP, 20).Proportion(0));
		tipLabel->Wrap(300); // Wrap text at 300 pixels
	}
};
