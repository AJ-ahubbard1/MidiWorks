// PaneInfo.h
#pragma once
#include <wx/string.h>
#include <wx/window.h>
#include <wx/aui/floatpane.h>

enum class PanePosition { Left, Right, Bottom, Center, Top, Float };

enum class PanelID
{
	MidiSettings,
	SoundBank,
	ChannelControls,
	TrackList,
	OnScreenKeyboard,
	Transport,
	MidiCanvas,
	Log
	// Add Panels as needed
};

struct PanelInfo
{
	wxString		name;
	wxWindow*		window = nullptr;
	int				menuId = -1;
	PanePosition	defaultPosition;
	wxSize			minSize{-1, -1};
	wxSize			bestSize{-1, -1};
	bool			hasCaption = true;
	bool			hasCloseButton = true;
	bool			isVisible = true;
};

inline wxAuiPaneInfo CreatePaneInfo(const PanelInfo& info)
{
	wxAuiPaneInfo pane;
	pane.Name(info.name)
		.CloseButton(info.hasCloseButton)
		.MinSize(info.minSize)
		.BestSize(info.bestSize)
		.Show(info.isVisible)
		.Caption(info.name)
		.CaptionVisible(info.hasCaption);

	switch (info.defaultPosition)
	{
		case PanePosition::Left:   pane.Left();   break;
		case PanePosition::Right:  pane.Right();  break;
		case PanePosition::Bottom: pane.Bottom(); break;
		case PanePosition::Center: pane.Center(); break;
		case PanePosition::Top:    pane.Top();    break;
		case PanePosition::Float:  pane.Float();  break;
		
	} 

	return pane;
}
