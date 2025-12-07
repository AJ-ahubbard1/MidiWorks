#pragma once
#include <wx/wx.h>
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/spinctrl.h>
#include "RtMidiWrapper/RtMidiWrapper.h"
#include "AppModel/Transport.h"
#include "AppModel/TrackSet.h"
#include "AppModel/AppModel.h"
#include "Commands/NoteEditCommands.h"
#include "Commands/PasteCommand.h"
#include "Commands/DeleteMultipleNotesCommand.h"

using namespace MidiInterface;

class MidiCanvasPanel : public wxPanel
{
public:
	MidiCanvasPanel(wxWindow* parent, std::shared_ptr<AppModel> appModel, const wxString& label);

	void Update();

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
	int mMinNoteHeight = 1;  // Minimum zoom: canvasHeight / 128 (all notes visible)
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

	// Preview note state (for pitch auditioning)
	bool mIsPreviewingNote = false;
	uint8_t mPreviewPitch = 0;
	uint64_t mPreviewStartTick = 0;
	std::vector<uint8_t> mPreviewChannels;  // Channels currently playing preview

	// Note selection/hover state
	struct NoteInfo {
		int trackIndex = -1;
		size_t noteOnIndex = 0;
		size_t noteOffIndex = 0;
		uint64_t startTick = 0;
		uint64_t endTick = 0;
		uint8_t pitch = 0;
		bool valid = false;

		// Equality operator for finding notes in selection
		bool operator==(const NoteInfo& other) const {
			return trackIndex == other.trackIndex &&
			       noteOnIndex == other.noteOnIndex &&
			       noteOffIndex == other.noteOffIndex;
		}
	};
	NoteInfo mHoveredNote;
	NoteInfo mSelectedNote;

	// Multi-selection state
	std::vector<NoteInfo> mSelectedNotes;  // Multiple selected notes
	bool mIsSelecting = false;             // Currently dragging selection rectangle
	wxPoint mSelectionStart;               // Where selection drag started
	wxPoint mSelectionEnd;                 // Current mouse position during drag

	// For move/resize operations
	uint64_t mOriginalStartTick = 0;
	uint64_t mOriginalEndTick = 0;
	uint8_t mOriginalPitch = 0;
	wxPoint mDragStartPos;

	// Drawing
	void Draw(wxPaintEvent&);
	void DrawGrid(wxGraphicsContext* gc);
	int Flip(int y);

	// Coordinate conversion helpers
	uint64_t ScreenXToTick(int screenX);
	uint8_t ScreenYToPitch(int screenY);
	int TickToScreenX(uint64_t tick);
	int PitchToScreenY(uint8_t pitch);

	// Audio preview
	void PlayPreviewNote(uint8_t pitch);
	void StopPreviewNote();

	// UI helpers
	uint64_t GetSelectedDuration() const;
	uint64_t ApplyGridSnap(uint64_t tick) const;

public:
	// Public accessor for MainFrame to get grid size for quantize
	uint64_t GetGridSize() const { return GetSelectedDuration(); }

	// Note finding
	NoteInfo FindNoteAtPosition(int screenX, int screenY);
	bool IsOnResizeEdge(int screenX, const NoteInfo& note);

	// Multi-selection
	std::vector<NoteInfo> FindNotesInRectangle(wxPoint start, wxPoint end);
	void ClearSelection();
	bool IsNoteSelected(const NoteInfo& note) const;

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
