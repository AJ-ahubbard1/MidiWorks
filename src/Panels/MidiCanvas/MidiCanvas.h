#pragma once
#include <wx/wx.h>
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/spinctrl.h>
#include "RtMidiWrapper/RtMidiWrapper.h"
#include "AppModel/Transport/Transport.h"
#include "AppModel/TrackSet/TrackSet.h"
#include "AppModel/AppModel.h"
#include "Commands/NoteEditCommands.h"
#include "Commands/ClipboardCommands.h"
#include "MidiConstants.h"
#include "MidiCanvasConstants.h"

using namespace MidiInterface;
using namespace MidiCanvasConstants;

class MidiCanvasPanel : public wxPanel
{
public:
	MidiCanvasPanel(wxWindow* parent, std::shared_ptr<AppModel> appModel, const wxString& label);

	void Update();

	// Value used for quantizing notes
	uint64_t GetGridSize() const { return GetSelectedDuration(); }

private:
	// ========== Core Model References ==========
	std::shared_ptr<AppModel> mAppModel;
	Transport& mTransport;
	TrackSet& mTrackSet;
	Track& mRecordingBuffer;
	PreviewManager& mPreviewManager;
	Selection& mSelection;

	// ========== UI Controls ==========
	wxStaticText* mDebugMessage;
	wxCheckBox* mGridSnapCheckbox;
	wxChoice* mDurationChoice;
	wxSpinCtrl* mCustomTicksCtrl;
	wxCheckBox* mShowMidiEventsCheckbox;

	// ========== View State (Zoom & Pan) ==========
	int mNoteHeight = DEFAULT_NOTE_HEIGHT_PIXELS;  // Current note height in pixels
	int mMinNoteHeight = 0;		 // Minimum zoom: dynamically calculated as canvasHeight / MIDI_NOTE_COUNT
	int mTicksPerPixel = 30;     // Horizontal zoom level
	wxPoint mOriginOffset;       // Pan offset for scrolling

	// ========== Mouse/Interaction State ==========
	enum class MouseMode
	{
		Idle,
		Adding,
		MovingNote,
		MovingMultipleNotes,
		ResizingNote,
		DraggingLoopStart,
		DraggingLoopEnd,
		EditingVelocity
	};
	MouseMode mMouseMode = MouseMode::Idle;
	bool mIsDragging = false;           // Right-click dragging (panning)
	wxPoint mLastMouse;                 // Last mouse position for drag operations

	// ========== Note Selection State ==========
	NoteLocation mHoveredNote;              // Note currently under mouse cursor
	NoteLocation mSelectedNote;             // Single selected note (for single-note operations)
	bool mIsSelecting = false;              // Currently dragging selection rectangle
	wxPoint mSelectionStart;                // Where selection drag started
	wxPoint mSelectionEnd;                  // Current mouse position during drag

	// ========== Note Editing State ==========
	int mCurrentEditTrack = 0;   // Which track/channel we're editing (0-14)
	uint64_t mOriginalStartTick = 0;  // Original values for move/resize operations
	uint64_t mOriginalEndTick = 0;
	ubyte mOriginalPitch = 0;
	ubyte mOriginalVelocity = 0;  // Original velocity for velocity editing
	wxPoint mDragStartPos;       // Mouse position when drag started
	std::vector<NoteLocation> mOriginalSelectedNotes;  // Original positions for multi-note move
	NoteLocation mVelocityEditNote;  // Note whose velocity is being edited

	// ========== Debug MIDI Events State ==========
	struct MidiEventDebugInfo 
	{
		TimedMidiEvent timedEvent;
		int screenX;
		int screenY;
	};
	std::vector<MidiEventDebugInfo> mDebugEvents;  // Cache for hover detection
	int mHoveredEventIndex = -1;  // Index of currently hovered event

	// ========================================================================
	// METHODS - Implemented in MidiCanvas.cpp
	// ========================================================================

	// Core Update & Drawing
	void Draw(wxPaintEvent&);

	// Coordinate Conversion Helpers
	int FlipY(int y) const;
	uint64_t ScreenXToTick(int screenX) const;
	ubyte ScreenYToPitch(int screenY) const;
	int TickToScreenX(uint64_t tick) const;
	int PitchToScreenY(ubyte pitch) const;
	int TicksToWidth(uint64_t ticks) const;

	// UI Helpers
	uint64_t GetSelectedDuration() const;
	uint64_t ApplyGridSnap(uint64_t tick) const;

	// Note Finding & Selection
	NoteLocation FindNoteAtPosition(int screenX, int screenY);
	bool IsOnResizeEdge(int screenX, const NoteLocation& note);
	std::vector<NoteLocation> FindNotesInRectangle(wxPoint start, wxPoint end);
	NoteLocation FindVelocityControlAtPosition(int screenX, int screenY);

	// Solo filtering helper
	std::vector<NoteLocation> FindNotesInRegionWithSoloFilter(
		uint64_t minTick, uint64_t maxTick,
		ubyte minPitch, ubyte maxPitch);

	// Loop Edge Detection
	bool IsNearLoopStart(int screenX);
	bool IsNearLoopEnd(int screenX);

	// View Management
	void ClampOffset();

	// Drawing Helpers
	void DrawNote(wxGraphicsContext* gc, const NoteLocation& note);
	void DrawGrid(wxGraphicsContext* gc);
	void DrawLoopRegion(wxGraphicsContext* gc);
	void DrawTrackNotes(wxGraphicsContext* gc);
	void DrawRecordingBuffer(wxGraphicsContext* gc);
	void DrawNoteAddPreview(wxGraphicsContext* gc);
	void DrawNoteEditPreview(wxGraphicsContext* gc);
	void DrawSelectedNotes(wxGraphicsContext* gc);
	void DrawHoverBorder(wxGraphicsContext* gc);
	void DrawSelectionRectangle(wxGraphicsContext* gc);
	void DrawPlayhead(wxGraphicsContext* gc);
	void DrawMidiEventsDebug(wxGraphicsContext* gc);
	void DrawMidiEventTooltip(wxGraphicsContext* gc, const MidiEventDebugInfo& event);
	void DrawVelocityEditor(wxGraphicsContext* gc);
	void DrawPianoKeyboard(wxGraphicsContext* gc);

	// ========================================================================
	// EVENT HANDLERS - Implemented in MidiCanvasEventHandlers.cpp
	// ========================================================================

	// Mouse 
	void OnMouseWheel(wxMouseEvent& event);			// Zooming
	void OnLeftDown(wxMouseEvent& event);			// Note Add/Move/Resize
	void OnLeftUp(wxMouseEvent& event);
	void OnMiddleDown(wxMouseEvent& event);			// Delete Note / Move Playhead
	void OnRightDown(wxMouseEvent& event);			// Panning
	void OnRightUp(wxMouseEvent& event);
	void OnMouseMove(wxMouseEvent& event);			// Drag Operations

	// Window Events
	void OnSize(wxSizeEvent& event);
	void OnMouseLeave(wxMouseEvent& event);

	// Keyboard Events
	void OnKeyDown(wxKeyEvent& event);

	// Helper Methods (for keyboard shortcuts)
	void CopySelectedNotesToClipboard();
	void DeleteSelectedNotes();
};
