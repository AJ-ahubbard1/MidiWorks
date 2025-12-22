#pragma once

#include <wx/wx.h>
#include "AppModel/AppModel.h"

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

private:
	std::shared_ptr<AppModel> mAppModel;
	wxRadioBox* mInPortList;

	void CreateControls()
	{
		wxFont mainFont(wxFontInfo(wxSize(0, 12)));

		auto inPortsVec = mAppModel->GetMidiInputManager().GetPortNames();

		// Convert std::vector<std::string> to wxArrayString for cross-platform compatibility
		wxArrayString inPorts;
		for (const auto& port : inPortsVec)
		{
			inPorts.Add(port);
		}

		mInPortList = new wxRadioBox(this, wxID_ANY, wxT("Midi In Port"),
			wxDefaultPosition, wxDefaultSize, inPorts, 1, wxRA_SPECIFY_COLS);
		mInPortList->SetFont(mainFont);
	}

	void SetupSizers()
	{
		wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

		mainSizer->Add(mInPortList, wxSizerFlags().Expand());

		wxGridSizer* outerSizer = new wxGridSizer(1);
		outerSizer->Add(mainSizer, wxSizerFlags().Border(wxALL, 15).Expand());
		SetSizer(outerSizer);
		outerSizer->SetSizeHints(this);
	}

	void BindEventHandlers()
	{
		mInPortList->Bind(wxEVT_RADIOBOX, &MidiSettingsPanel::OnInPortClicked, this);
	}

	void OnInPortClicked(wxCommandEvent& evt)
	{
		unsigned int p = evt.GetSelection();
		mAppModel->GetMidiInputManager().SetInputPort(p);
	}
};
