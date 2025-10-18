// DummyPanel.h
#pragma once

#include <wx/wx.h>

class DummyPanel : public wxPanel 
{
public:
	DummyPanel(wxWindow* parent, const wxColour& bgColor, const wxString& label)
		: wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, label)
	{
		SetBackgroundColour(bgColor);

		auto* text = new wxStaticText(this, wxID_ANY, label); 
		SetSize(wxSize(250, 250));
	} 
};
