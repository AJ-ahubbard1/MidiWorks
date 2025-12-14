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
#include "Commands/DeleteMultipleNotesCommand.h"
#include "MidiConstants.h"

using namespace MidiInterface;

class MidiCanvasPanel : public wxPanel
{
public:
	MidiCanvasPanel(wxWindow* parent, std::shared_ptr<AppModel> appModel, const wxString& label);

	void Update();
	uint64_t GetGridSize() const { return GetSelectedDuration(); }

private:
	std::shared_ptr<AppModel> mAppModel;
	Transport& mTransport;
	TrackSet& mTrackSet;
	Track& mRecordingBuffer;
	wxStaticText* mDebugMessage;
	wxCheckBox* mGridSnapCheckbox;
	wxChoice* mDurationChoice;
	wxSpinCtrl* mCustomTicksCtrl;
	int mNoteHeight = 5;  // Current note height in pixels (pixels per MIDI note)
	int mMinNoteHeight = 1;  // Minimum zoom: canvasHeight / MIDI_NOTE_COUNT (all notes visible)
	int mMaxNoteHeight = 50;  // Maximum zoom: 50 pixels per note
	int mTicksPerPixel = 30;
	bool mIsDragging = false;  // Right-click dragging (panning)
	wxPoint mLastMouse;
	wxPoint mOriginOffset;

	// Note editing state
	int mCurrentEditTrack = 0;  // Which track/channel we're editing (0-14)
	enum class MouseMode {
		Idle,
		Adding,
		MovingNote,
		ResizingNote,
		DraggingLoopStart,
		DraggingLoopEnd
	};
	MouseMode mMouseMode = MouseMode::Idle;
	size_t mSelectedNoteOnIndex = 0;
	size_t mSelectedNoteOffIndex = 0;

	// Note selection/hover state
	NoteLocation mHoveredNote;
	NoteLocation mSelectedNote;

	// Multi-selection state
	std::vector<NoteLocation> mSelectedNotes;  // Multiple selected notes
	bool mIsSelecting = false;             // Currently dragging selection rectangle
	wxPoint mSelectionStart;               // Where selection drag started
	wxPoint mSelectionEnd;                 // Current mouse position during drag

	// For move/resize operations
	uint64_t mOriginalStartTick = 0;
	uint64_t mOriginalEndTick = 0;
	ubyte mOriginalPitch = 0;
	wxPoint mDragStartPos;

	// Note creation preview (UI state for visual feedback during drag)
	uint64_t mPreviewStartTick = 0;

	// Drawing
	void Draw(wxPaintEvent&);
	void DrawGrid(wxGraphicsContext* gc);
	int Flip(int y);

	// Coordinate conversion helpers
	uint64_t ScreenXToTick(int screenX);
	ubyte ScreenYToPitch(int screenY);
	int TickToScreenX(uint64_t tick);
	int PitchToScreenY(ubyte pitch);

	// Audio preview
	void PlayPreviewNote(ubyte pitch);
	void StopPreviewNote();

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
