#pragma once
#include <wx/colour.h>

	// ========== Track Colors ==========
	// 15 unique colors for MIDI tracks (channel 16 reserved for metronome)
	const wxColour TRACK_COLORS[15] = {
		wxColour(255, 100, 100),  // Red
		wxColour(100, 255, 100),  // Green
		wxColour(100, 100, 255),  // Blue
		wxColour(255, 255, 100),  // Yellow
		wxColour(255, 100, 255),  // Magenta
		wxColour(100, 255, 255),  // Cyan
		wxColour(255, 150, 100),  // Orange
		wxColour(150, 100, 255),  // Purple
		wxColour(255, 200, 100),  // Light Orange
		wxColour(100, 255, 200),  // Mint
		wxColour(200, 100, 255),  // Violet
		wxColour(255, 100, 200),  // Pink
		wxColour(200, 255, 100),  // Lime
		wxColour(100, 200, 255),  // Sky Blue
		wxColour(255, 255, 200)   // Light Yellow
	};


