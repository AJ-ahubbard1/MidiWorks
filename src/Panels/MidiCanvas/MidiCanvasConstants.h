#pragma once
#include <wx/wx.h>
#include <string>

namespace MidiCanvasConstants
{
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

	// ========== Grid Colors ==========
	const wxColour GRID_BEAT_LINE(220, 220, 220);      // Light gray for beats
	const wxColour GRID_MEASURE_LINE(180, 180, 180);   // Darker gray for measures
	const wxColour GRID_NOTE_LINE(240, 240, 240);      // Very light gray for notes
	const wxColour GRID_OCTAVE_LINE(200, 200, 200);    // Light gray for octaves

	// ========== Loop Region Colors ==========
	const wxColour LOOP_ENABLED(100, 150, 255, 60);    // Semi-transparent blue when enabled
	const wxColour LOOP_DISABLED(128, 128, 128, 30);   // Semi-transparent gray when disabled

	// ========== Note Colors ==========
	const wxColour RECORDING_BUFFER(255, 100, 50, 180);  // Red-orange for recording buffer
	const wxColour NOTE_ADD_PREVIEW(100, 255, 100, 180); // Green for note being added
	const int NOTE_EDIT_PREVIEW_ALPHA = 180;             // Alpha for move/resize preview

	// ========== Selection & Hover Colors ==========
	const wxColour SELECTION_BORDER(255, 255, 0);        // Yellow for selected notes
	const int SELECTION_BORDER_WIDTH = 3;
	const wxColour HOVER_BORDER(*wxWHITE);                // White for hovered notes
	const int HOVER_BORDER_WIDTH = 2;
	const wxColour PREVIEW_BORDER(*wxWHITE);              // White for preview notes
	const int PREVIEW_BORDER_WIDTH = 2;

	// ========== Selection Rectangle Colors ==========
	const wxColour SELECTION_RECT_FILL(100, 150, 255, 60);  // Semi-transparent blue fill
	const wxColour SELECTION_RECT_BORDER(100, 150, 255);    // Blue border
	const int SELECTION_RECT_BORDER_WIDTH = 2;

	// ========== Playhead Colors ==========
	const wxColour PLAYHEAD(*wxRED);
	const int PLAYHEAD_WIDTH = 2;

	// ========== UI Layout Constants ==========
	const int CONTROL_BAR_HEIGHT = 40;              // Height of control bar at top
	const int NOTE_RESIZE_LEFT_PIXELS = 5;          // Hit zone left of note edge for resize
	const int NOTE_RESIZE_RIGHT_PIXELS = 2;         // Hit zone right of note edge for resize
	const int LOOP_EDGE_DETECTION_PIXELS = 5;       // Detection zone on each side of loop edge
	const int MIN_NOTE_DURATION_TICKS = 100;        // Minimum note duration when resizing

	// ========== Auto-Scroll Constants ==========
	const double AUTOSCROLL_TRIGGER_THRESHOLD = 0.8;  // Playhead position that triggers auto-scroll (80% of screen)
	const double AUTOSCROLL_TARGET_POSITION = 0.2;    // Target playhead position after scroll (20% from left)

	// ========== Zoom Constraints ==========
	const int DEFAULT_NOTE_HEIGHT_PIXELS = 5;        // Initial note height before window resize
	const int MAX_NOTE_HEIGHT_PIXELS = 50;           // Maximum zoom: 50 pixels per note
	const int MIN_NOTE_HEIGHT_PIXELS = 1;            // Absolute minimum: 1 pixel per note

	// ========== Note Editing Constraints ==========
	const int MAX_EDITABLE_PITCH = 120;               // Notes above this pitch trigger selection mode instead
	const int USER_TRACK_COUNT = 15;                  // Number of user MIDI tracks (0-14, channel 15 reserved for metronome)

	// ========== Debug MIDI Events Display ==========
	const int MIDI_EVENT_CIRCLE_RADIUS = 4;
	const wxColour MIDI_EVENT_NOTE_ON(0, 255, 0, 200);    // Green for Note On
	const wxColour MIDI_EVENT_NOTE_OFF(255, 0, 0, 200);   // Red for Note Off
	const wxColour MIDI_EVENT_OTHER(100, 150, 255, 200);  // Blue for other MIDI messages
	const int MIDI_EVENT_HOVER_DISTANCE = 8;              // Pixels to detect hover

	// ========== Note Duration Options ==========
	const int MAX_CUSTOM_TICKS = 10000;               // Maximum value for custom tick duration

	struct NoteDuration {
		const char* label;
		uint64_t ticks;
	};

	const NoteDuration NOTE_DURATIONS[] = {
		{ "Whole Note (3840)", 3840 },
		{ "Half Note (1920)", 1920 },
		{ "Quarter Note (960)", 960 },
		{ "Quarter Triplet (640)", 640 },
		{ "Eighth Note (480)", 480 },
		{ "Eighth Triplet (320)", 320 },
		{ "Sixteenth Note (240)", 240 },
		{ "Sixteenth Triplet (160)", 160 },
		{ "Custom", 0 }  // 0 = custom, tick value from spin control
	};

	const int NOTE_DURATIONS_COUNT = sizeof(NOTE_DURATIONS) / sizeof(NOTE_DURATIONS[0]);
	const int DEFAULT_DURATION_INDEX = 2;  // Quarter note
}
