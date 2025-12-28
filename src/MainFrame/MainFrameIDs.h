#pragma once
#include <wx/wx.h>

// Custom wxWidgets IDs for MainFrame
enum MainFrameIDs
{
	// Keyboard shortcuts
	ID_KEYBOARD_TOGGLE_PLAY = wxID_HIGHEST + 1000,
	ID_KEYBOARD_RECORD,
	ID_KEYBOARD_QUANTIZE,
    ID_KEYBOARD_PREVIOUS_MEASURE,
    ID_KEYBOARD_NEXT_MEASURE,

	// Panel menu IDs (dynamically assigned starting here)
	ID_PANELS_BEGIN
};


