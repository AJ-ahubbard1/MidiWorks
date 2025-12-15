#pragma once
#include <wx/wx.h>
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/spinctrl.h>
#include "RtMidiWrapper/RtMidiWrapper.h"
#include "AppModel/Transport.h"
#include "AppModel/TrackSet/TrackSet.h"
#include "AppModel/AppModel.h"
#include "Commands/NoteEditCommands.h"
#include "Commands/PasteCommand.h"
#include "MidiConstants.h"

using namespace MidiInterface;

class MidiCanvasPanel : public wxPanel
{
public:
	MidiCanvasPanel(wxWindow* parent, std::shared_ptr<AppModel> appModel, const wxString& label);

	void Update();
	uint64_t GetGridSize() const { return GetSelectedDuration(); }

private:
	// ========== Core Model References ==========
	std::shared_ptr<AppModel> mAppModel;
	Transport& mTransport;
	TrackSet& mTrackSet;
	Track& mRecordingBuffer;

	// ========== UI Controls ==========
	wxStaticText* mDebugMessage;
	wxCheckBox* mGridSnapCheckbox;
	wxChoice* mDurationChoice;
	wxSpinCtrl* mCustomTicksCtrl;

	// ========== View State (Zoom & Pan) ==========
	int mNoteHeight = 5;         // Current note height in pixels (pixels per MIDI note)
	int mMinNoteHeight = 1;      // Minimum zoom: canvasHeight / MIDI_NOTE_COUNT (all notes visible)
	int mMaxNoteHeight = 50;     // Maximum zoom: 50 pixels per note
	int mTicksPerPixel = 30;     // Horizontal zoom level
	wxPoint mOriginOffset;       // Pan offset for scrolling

	// ========== Mouse/Interaction State ==========
	enum class MouseMode {
		Idle,
		Adding,
		MovingNote,
		ResizingNote,
		DraggingLoopStart,
		DraggingLoopEnd
	};
	MouseMode mMouseMode = MouseMode::Idle;
	bool mIsDragging = false;    // Right-click dragging (panning)
	wxPoint mLastMouse;          // Last mouse position for drag operations

	// ========== Note Selection State ==========
	NoteLocation mHoveredNote;              // Note currently under mouse cursor
	NoteLocation mSelectedNote;             // Single selected note (for single-note operations)
	std::vector<NoteLocation> mSelectedNotes;  // Multiple selected notes
	bool mIsSelecting = false;              // Currently dragging selection rectangle
	wxPoint mSelectionStart;                // Where selection drag started
	wxPoint mSelectionEnd;                  // Current mouse position during drag

	// ========== Note Editing State ==========
	int mCurrentEditTrack = 0;   // Which track/channel we're editing (0-14)
	uint64_t mOriginalStartTick = 0;  // Original values for move/resize operations
	uint64_t mOriginalEndTick = 0;
	ubyte mOriginalPitch = 0;
	wxPoint mDragStartPos;       // Mouse position when drag started
	uint64_t mPreviewStartTick = 0;  // Note creation preview (UI state for visual feedback during drag)

	// Drawing
	void Draw(wxPaintEvent&);

	// Coordinate conversion helpers
	int FlipY(int y) const;
	uint64_t ScreenXToTick(int screenX) const;
	ubyte ScreenYToPitch(int screenY) const;
	int TickToScreenX(uint64_t tick) const;
	int PitchToScreenY(ubyte pitch) const;
	int TicksToWidth(uint64_t ticks) const;

	// UI helpers
	uint64_t GetSelectedDuration() const;
	uint64_t ApplyGridSnap(uint64_t tick) const;

	// Note finding
	NoteLocation FindNoteAtPosition(int screenX, int screenY);
	bool IsOnResizeEdge(int screenX, const NoteLocation& note);

	// Multi-selection
	std::vector<NoteLocation> FindNotesInRectangle(wxPoint start, wxPoint end);
	void ClearSelection();
	bool IsNoteSelected(const NoteLocation& note) const;

	// Helper methods to eliminate duplication
	void CopySelectedNotesToClipboard();
	void DeleteSelectedNotes();

	// Loop edge detection
	bool IsNearLoopStart(int screenX);
	bool IsNearLoopEnd(int screenX);

	// View management
	void ClampOffset();

	// Drawing helpers
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

	// Event handlers
	void OnMouseWheel(wxMouseEvent& event);
	void OnLeftDown(wxMouseEvent& event);
	void OnLeftUp(wxMouseEvent& event);
	void OnMiddleDown(wxMouseEvent& event);
	void OnRightDown(wxMouseEvent& event);
	void OnRightUp(wxMouseEvent& event);
	void OnMouseMove(wxMouseEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnMouseLeave(wxMouseEvent& event);
	void OnKeyDown(wxKeyEvent& event);
};
