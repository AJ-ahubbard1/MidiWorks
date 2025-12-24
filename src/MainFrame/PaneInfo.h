// PaneInfo.h
#pragma once
#include <wx/string.h>
#include <wx/window.h>
#include <wx/aui/floatpane.h>

enum class PanePosition { Left, Right, Bottom, Center, Top, Float };

struct PanelInfo
{
	wxString		name;
	wxWindow*		window = nullptr;
	PanePosition	defaultPosition;
	wxSize			minSize{-1, -1};
	bool			isVisible = true;
	bool			hasCaption = true;
	bool			hasCloseButton = true;
	int				menuId = -1;  // Moved to end for optional initialization
};

inline wxAuiPaneInfo CreatePaneInfo(const PanelInfo& info)
{
	wxAuiPaneInfo pane;
	pane.Name(info.name)
		.CloseButton(info.hasCloseButton)
		.MinSize(info.minSize)
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

