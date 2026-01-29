// MidiSettings.h
#pragma once
#include <wx/wx.h>
#include <wx/listbox.h>
#include "AppModel/AppModel.h"

/// Panel for MIDI input port selection.
///
/// Responsibilities:
/// - Display available MIDI input ports
/// - Allow user to select active input port
class MidiSettingsPanel : public wxPanel
{
public:
	MidiSettingsPanel(wxWindow* parent, std::shared_ptr<AppModel> appModel, const wxColour& bgColor, const wxString& label)
		: wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, label),
		mAppModel(std::move(appModel))
	{
		SetBackgroundColour(bgColor);
		CreateControls();
		SetupSizers();
		BindEventHandlers();
	}

	void UpdateList() 
	{
		auto& midiIn = mAppModel->GetMidiInputManager();
		wxString label = wxString::Format("Midi In Ports: %d", midiIn.GetDevice().getNumPorts());
		mTitle->SetLabelText(label);
		mInPortList->Clear();
		// Convert std::vector<std::string> to wxArrayString for cross-platform compatibility
		auto portNames = midiIn.GetPortNames();
		wxArrayString inPorts;
		for (const auto& port : portNames)
		{
			inPorts.Add(port);
		}
		mInPortList->Append(inPorts);
		Layout();
	}

private:
	std::shared_ptr<AppModel> mAppModel;
	wxStaticText* mTitle;
	wxListBox* mInPortList;

	void CreateControls()
	{
		auto& midiIn = mAppModel->GetMidiInputManager();
		wxFont mainFont(wxFontInfo(wxSize(0, 12)));
		
		mTitle = new wxStaticText(this, wxID_ANY, "");
		wxString label = wxString::Format("Midi In Ports: %d", midiIn.GetDevice().getNumPorts());
		mTitle->SetLabelText(label);
		// Convert std::vector<std::string> to wxArrayString for cross-platform compatibility
		auto portNames = midiIn.GetPortNames();
		wxArrayString inPorts;
		for (const auto& port : portNames)
		{
			inPorts.Add(port);
		}
		mInPortList = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, inPorts, wxLB_SINGLE);
		mInPortList->SetFont(mainFont);
		mInPortList->SetSelection(0);
	}

	void SetupSizers()
	{
		wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

		mainSizer->Add(mTitle, wxSizerFlags().Expand());
		
		mainSizer->Add(mInPortList, wxSizerFlags().Expand().Border(wxALL, 5));

		wxGridSizer* outerSizer = new wxGridSizer(1);
		outerSizer->Add(mainSizer, wxSizerFlags().Border(wxALL, 15).Expand());
		SetSizer(outerSizer);
		outerSizer->SetSizeHints(this);
	}

	void BindEventHandlers()
	{
		mInPortList->Bind(wxEVT_LISTBOX, &MidiSettingsPanel::OnInPortClicked, this);
	}

	void OnInPortClicked(wxCommandEvent& evt)
	{
		unsigned int p = evt.GetSelection();
		mAppModel->GetMidiInputManager().SetInputPort(p);   
	}
};
