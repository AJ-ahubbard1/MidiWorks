#include "MidiCanvas.h"
#include "MidiCanvasConstants.h"
#include <map>

using namespace MidiCanvasConstants;

MidiCanvasPanel::MidiCanvasPanel(wxWindow* parent, std::shared_ptr<AppModel> appModel, const wxString& label)
	: wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, label),
	mAppModel(appModel),
	mTransport(appModel->GetTransport()),
	mTrackSet(appModel->GetTrackSet()),
	mRecordingBuffer(appModel->GetRecordingBuffer())
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);

	// Create main vertical sizer
	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

	// Create horizontal sizer for top controls
	wxBoxSizer* controlsSizer = new wxBoxSizer(wxHORIZONTAL);

	// Grid snap checkbox
	mGridSnapCheckbox = new wxCheckBox(this, wxID_ANY, "Grid Snap");
	mGridSnapCheckbox->SetValue(true);  // Enable by default
	controlsSizer->Add(mGridSnapCheckbox, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
	controlsSizer->AddSpacer(5);

	// Duration selector
	controlsSizer->Add(new wxStaticText(this, wxID_ANY, "Duration:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
	mDurationChoice = new wxChoice(this, wxID_ANY);

	// Populate duration choices from constants
	for (int i = 0; i < NOTE_DURATIONS_COUNT; i++)
	{
		mDurationChoice->Append(NOTE_DURATIONS[i].label, (void*)(intptr_t)NOTE_DURATIONS[i].ticks);
	}
	mDurationChoice->SetSelection(DEFAULT_DURATION_INDEX);

	controlsSizer->AddSpacer(5);
	controlsSizer->Add(mDurationChoice, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
	controlsSizer->AddSpacer(5);

	// Custom tick duration input (shown when "Custom" is selected)
	controlsSizer->Add(new wxStaticText(this, wxID_ANY, "Ticks:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
	mCustomTicksCtrl = new wxSpinCtrl(this, wxID_ANY, "960", wxDefaultPosition, wxSize(80, -1));
	mCustomTicksCtrl->SetRange(1, MAX_CUSTOM_TICKS);
	mCustomTicksCtrl->SetValue(MidiConstants::TICKS_PER_QUARTER);
	mCustomTicksCtrl->Show(false);  // Hidden by default
	controlsSizer->Add(mCustomTicksCtrl, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

	// Bind choice change event to show/hide custom input
	mDurationChoice->Bind(wxEVT_CHOICE, [this](wxCommandEvent& e) {
		int selection = mDurationChoice->GetSelection();
		intptr_t duration = (intptr_t)mDurationChoice->GetClientData(selection);
		bool isCustom = (duration == 0);
		mCustomTicksCtrl->Show(isCustom);
		Layout();  // Refresh layout to show/hide control
	});

	controlsSizer->AddStretchSpacer();

	// Debug message at the end (so it doesn't cover other controls)
	mDebugMessage = new wxStaticText(this, wxID_ANY, "", wxDefaultPosition, wxSize(200, -1));
	controlsSizer->Add(mDebugMessage, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
	controlsSizer->AddSpacer(5);

	// Add controls to main sizer at the top
	mainSizer->Add(controlsSizer, 0, wxEXPAND);

	// Apply the sizer to the panel
	SetSizer(mainSizer);

	Bind(wxEVT_PAINT, &MidiCanvasPanel::Draw, this);
	Bind(wxEVT_MOUSEWHEEL, &MidiCanvasPanel::OnMouseWheel, this);
	Bind(wxEVT_LEFT_DOWN, &MidiCanvasPanel::OnLeftDown, this);
	Bind(wxEVT_LEFT_UP, &MidiCanvasPanel::OnLeftUp, this);
	Bind(wxEVT_MIDDLE_DOWN, &MidiCanvasPanel::OnMiddleDown, this);
	Bind(wxEVT_RIGHT_DOWN, &MidiCanvasPanel::OnRightDown, this);
	Bind(wxEVT_RIGHT_UP, &MidiCanvasPanel::OnRightUp, this);
	Bind(wxEVT_MOTION, &MidiCanvasPanel::OnMouseMove, this);
	Bind(wxEVT_SIZE, &MidiCanvasPanel::OnSize, this);
	Bind(wxEVT_LEAVE_WINDOW, &MidiCanvasPanel::OnMouseLeave, this);
	Bind(wxEVT_CHAR_HOOK, &MidiCanvasPanel::OnKeyDown, this);

	// Allow panel to receive keyboard focus
	SetFocus();
}

void MidiCanvasPanel::Update()
{
	uint64_t currentTick = mTransport.GetCurrentTick();

	// Auto-pan to start when playhead is reset to tick 0
	if (currentTick == 0 && mOriginOffset.x != 0)
	{
		mOriginOffset.x = 0;  // Pan back to start position
		ClampOffset();
	}
	// Auto-scroll during playback/recording to keep playhead visible
	else if (mTransport.IsPlaying() || mTransport.IsRecording())
	{
		wxSize clientSize = GetClientSize();
		int canvasWidth = clientSize.GetWidth();

		int playheadX = TickToScreenX(currentTick);

		// If playhead exceeds threshold, scroll to keep it at target position
		int scrollThreshold = canvasWidth * AUTOSCROLL_TRIGGER_THRESHOLD;
		if (playheadX > scrollThreshold)
		{
			int targetPlayheadX = canvasWidth * AUTOSCROLL_TARGET_POSITION;
			mOriginOffset.x = targetPlayheadX - (currentTick / mTicksPerPixel);
			ClampOffset();
		}
	}

	Refresh();
}

void MidiCanvasPanel::Draw(wxPaintEvent&)
{
	wxAutoBufferedPaintDC dc(this);

	// Get the area to draw in (exclude the control bar at top)
	wxSize clientSize = GetClientSize();

	// Paint background for control area (so it's not black)
	dc.SetBrush(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE)));
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.DrawRectangle(0, 0, clientSize.GetWidth(), CONTROL_BAR_HEIGHT);

	// Only clear the canvas area, not the control area
	dc.SetClippingRegion(0, CONTROL_BAR_HEIGHT, clientSize.GetWidth(), clientSize.GetHeight() - CONTROL_BAR_HEIGHT);
	dc.Clear();

	wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
	if (!gc) return;

	// Draw all canvas elements in order
	DrawGrid(gc);
	DrawLoopRegion(gc);
	DrawTrackNotes(gc);
	DrawRecordingBuffer(gc);
	DrawNoteAddPreview(gc);
	DrawNoteEditPreview(gc);
	DrawSelectedNotes(gc);
	DrawHoverBorder(gc);
	DrawSelectionRectangle(gc);
	DrawPlayhead(gc);

	delete gc;
}

// ========== Coordinate Conversion Helper Methods ==========
int MidiCanvasPanel::FlipY(int y) const
{
	return GetSize().GetHeight() - y;
}

uint64_t MidiCanvasPanel::ScreenXToTick(int screenX) const
{
	int x = screenX - mOriginOffset.x;
	return x * mTicksPerPixel;
}

ubyte MidiCanvasPanel::ScreenYToPitch(int screenY) const
{
	int y = screenY - mOriginOffset.y;
	int flippedY = FlipY(y);
	int pitch = flippedY / mNoteHeight + 1;
	return std::clamp(pitch, 0, MidiConstants::MAX_MIDI_NOTE);
}

int MidiCanvasPanel::TickToScreenX(uint64_t tick) const
{
	return tick / mTicksPerPixel + mOriginOffset.x;
}

int MidiCanvasPanel::PitchToScreenY(ubyte pitch) const
{
	return FlipY(pitch * mNoteHeight) + mOriginOffset.y;
}

int MidiCanvasPanel::TicksToWidth(uint64_t ticks) const
{
	return ticks / mTicksPerPixel;
}

uint64_t MidiCanvasPanel::GetSelectedDuration() const
{
	int selection = mDurationChoice->GetSelection();
	if (selection == wxNOT_FOUND) return MidiConstants::TICKS_PER_QUARTER;  // Default to quarter note

	intptr_t duration = (intptr_t)mDurationChoice->GetClientData(selection);

	// If "Custom" is selected (duration == 0), use the custom tick value
	if (duration == 0)
	{
		return static_cast<uint64_t>(mCustomTicksCtrl->GetValue());
	}

	return static_cast<uint64_t>(duration);
}

uint64_t MidiCanvasPanel::ApplyGridSnap(uint64_t tick) const
{
	if (!mGridSnapCheckbox->GetValue()) return tick;

	uint64_t duration = GetSelectedDuration();
	return (tick / duration) * duration;  // Round down to nearest multiple
}

NoteLocation MidiCanvasPanel::FindNoteAtPosition(int screenX, int screenY)
{
	uint64_t clickTick = ScreenXToTick(screenX);
	ubyte clickPitch = ScreenYToPitch(screenY);

	return mTrackSet.FindNoteAt(clickTick, clickPitch);
}

std::vector<NoteLocation> MidiCanvasPanel::FindNotesInRectangle(wxPoint start, wxPoint end)
{
	// Normalize rectangle (ensure min/max regardless of drag direction)
	int minX = std::min(start.x, end.x);
	int maxX = std::max(start.x, end.x);
	int minY = std::min(start.y, end.y);
	int maxY = std::max(start.y, end.y);

	// Convert screen coordinates to tick/pitch ranges
	uint64_t minTick = ScreenXToTick(minX);
	uint64_t maxTick = ScreenXToTick(maxX);
	ubyte minPitch = ScreenYToPitch(maxY);  // Y is flipped
	ubyte maxPitch = ScreenYToPitch(minY);  // Y is flipped

	// Search through all tracks 
	return mTrackSet.FindNotesInRegion(minTick, maxTick, minPitch, maxPitch);
}

void MidiCanvasPanel::ClearSelection()
{
	mSelectedNotes.clear();
}

bool MidiCanvasPanel::IsNoteSelected(const NoteLocation& note) const
{
	for (const auto& selected : mSelectedNotes)
	{
		if (selected == note)
			return true;
	}
	return false;
}

bool MidiCanvasPanel::IsOnResizeEdge(int screenX, const NoteLocation& note)
{
	if (!note.found) return false;

	int noteEndX = TickToScreenX(note.endTick);
	return (screenX >= noteEndX - NOTE_RESIZE_LEFT_PIXELS &&
	        screenX <= noteEndX + NOTE_RESIZE_RIGHT_PIXELS);
}

bool MidiCanvasPanel::IsNearLoopStart(int screenX)
{
	int loopStartX = TickToScreenX(mTransport.GetLoopStart());
	return (screenX >= loopStartX - LOOP_EDGE_DETECTION_PIXELS &&
	        screenX <= loopStartX + LOOP_EDGE_DETECTION_PIXELS);
}

bool MidiCanvasPanel::IsNearLoopEnd(int screenX)
{
	int loopEndX = TickToScreenX(mTransport.GetLoopEnd());
	return (screenX >= loopEndX - LOOP_EDGE_DETECTION_PIXELS &&
	        screenX <= loopEndX + LOOP_EDGE_DETECTION_PIXELS);
}

void MidiCanvasPanel::ClampOffset()
{
	int canvasWidth = GetSize().GetWidth();
	int canvasHeight = GetSize().GetHeight();

	// Constants
	const int MAX_MEASURES = 100;  // Allow scrolling to 100 measures
	const int TICKS_PER_MEASURE = MidiConstants::TICKS_PER_QUARTER * 4;  // Assumes 4/4 time
	const int MAX_TICK = MAX_MEASURES * TICKS_PER_MEASURE;

	// Horizontal limits (time axis)
	// When offset.x = 0, tick 0 is at left edge
	// When offset.x < 0, we've scrolled right (tick 0 is off-screen left)

	// Right boundary: can scroll right to see up to MAX_TICK
	// When MAX_TICK is at right edge: canvasWidth = MAX_TICK / ticksPerPixel + offset.x
	int minOffsetX = canvasWidth - (MAX_TICK / mTicksPerPixel);

	// Left boundary: can't scroll left past tick 0
	int maxOffsetX = 0;

	// Clamp horizontal offset
	mOriginOffset.x = std::max(minOffsetX, std::min(mOriginOffset.x, maxOffsetX));

	// Vertical limits (pitch axis)
	// Calculate total height needed for all MIDI notes
	int totalMidiHeight = MidiConstants::MIDI_NOTE_COUNT * mNoteHeight;

	// If all notes fit on screen, no vertical scrolling allowed
	if (totalMidiHeight <= canvasHeight)
	{
		mOriginOffset.y = 0;
	}
	else
	{
		// Notes don't fit - allow scrolling within range
		// Bottom boundary: note 0 should stay at bottom (offset.y = 0)
		int minOffsetY = 0;

		// Top boundary: note 127 should be visible at top
		// When showing highest notes, we need positive offset
		int maxOffsetY = MidiConstants::MAX_MIDI_NOTE * mNoteHeight - canvasHeight;

		// Clamp vertical offset
		mOriginOffset.y = std::max(minOffsetY, std::min(mOriginOffset.y, maxOffsetY));
	}
}

void MidiCanvasPanel::OnMouseWheel(wxMouseEvent& event)
{
	int rotation = event.GetWheelRotation();     // positive = scroll up, negative = scroll down
	int delta = event.GetWheelDelta();           // usually 120
	int lines = rotation / delta;

	if (event.ShiftDown())
	{
		// Shift + wheel: Vertical zoom (adjust note height)
		if (lines > 0)
			mNoteHeight = std::min(mMaxNoteHeight, mNoteHeight + lines); // zoom in (taller notes)
		else if (lines < 0)
			mNoteHeight = std::max(mMinNoteHeight, mNoteHeight + lines); // zoom out (shorter notes)

		wxString msg = wxString::Format("Note Height: %d pixels", mNoteHeight);
		mDebugMessage->SetLabelText(msg);
	}
	else
	{
		// Normal wheel: Horizontal zoom (adjust ticks per pixel)
		if (lines > 0)
			mTicksPerPixel = std::max(1, mTicksPerPixel - lines); // zoom in
		else if (lines < 0)
			mTicksPerPixel += -lines;                             // zoom out

		wxString msg = wxString::Format("Ticks Per Pixel: %d", mTicksPerPixel);
		mDebugMessage->SetLabelText(msg);
	}

	ClampOffset(); // Apply boundaries after zoom
	Refresh(); // trigger redraw
}

void MidiCanvasPanel::OnLeftDown(wxMouseEvent& event)
{
	// Ensure panel has keyboard focus for shortcuts to work
	SetFocus();

	wxPoint pos = event.GetPosition();

	// Check if we're clicking on loop edges first (priority over notes)
	if (IsNearLoopStart(pos.x))
	{
		mMouseMode = MouseMode::DraggingLoopStart;
		return;
	}
	else if (IsNearLoopEnd(pos.x))
	{
		mMouseMode = MouseMode::DraggingLoopEnd;
		return;
	}

	// Check if we clicked on an existing note
	NoteLocation clickedNote = FindNoteAtPosition(pos.x, pos.y);

	if (clickedNote.found)
	{
		// Clicked on an existing note
		mSelectedNote = clickedNote;
		mDragStartPos = pos;

		// Check if clicking on resize edge
		if (IsOnResizeEdge(pos.x, clickedNote))
		{
			// Start resizing
			mMouseMode = MouseMode::ResizingNote;
			mOriginalStartTick = clickedNote.startTick;
			mOriginalEndTick = clickedNote.endTick;
		}
		else
		{
			// Start moving
			mMouseMode = MouseMode::MovingNote;
			mOriginalStartTick = clickedNote.startTick;
			mOriginalEndTick = clickedNote.endTick;
			mOriginalPitch = clickedNote.pitch;
		}
	}
	else
	{
		// Clicked on empty space
		uint64_t tick = ScreenXToTick(pos.x);
		ubyte pitch = ScreenYToPitch(pos.y);

		// Ignore notes above editable range (extreme high range, rarely used)
		if (pitch > MAX_EDITABLE_PITCH)
		{
			// Start rectangle selection instead
			mIsSelecting = true;
			mSelectionStart = pos;
			mSelectionEnd = pos;
			ClearSelection();
			return;
		}

		// Check if Shift is held - if so, start rectangle selection
		if (event.ShiftDown())
		{
			mIsSelecting = true;
			mSelectionStart = pos;
			mSelectionEnd = pos;
			// Don't clear selection if Shift is held (additive selection)
		}
		else
		{
			// Start preview playback on record-enabled channels
			mAppModel->PlayPreviewNote(pitch);

			// Store the starting tick for note creation
			mPreviewStartTick = tick;
			mMouseMode = MouseMode::Adding;
		}
	}
}

void MidiCanvasPanel::OnLeftUp(wxMouseEvent& event)
{
	// Finalize rectangle selection
	if (mIsSelecting)
	{
		mIsSelecting = false;
		// Final selection is already in mSelectedNotes (updated during drag)
		Refresh();
		return;
	}

	if (mMouseMode == MouseMode::Adding && mAppModel->IsPreviewingNote())
	{
		// Stop the preview playback
		mAppModel->StopPreviewNote();

		// Apply grid snap to starting tick
		uint64_t snappedTick = ApplyGridSnap(mPreviewStartTick);
		uint64_t duration = GetSelectedDuration();

		// Add note to all record-enabled channels
		mAppModel->AddNoteToRecordChannels(mAppModel->GetPreviewPitch(), snappedTick, duration);
	}
	else if (mMouseMode == MouseMode::MovingNote && mSelectedNote.found)
	{
		// Finalize move using preview state (track data was never modified)
		if (mAppModel->HasNoteEditPreview())
		{
			const auto& preview = mAppModel->GetNoteEditPreview();
			mAppModel->MoveNote(mSelectedNote, preview.previewStartTick, preview.previewPitch);
			mAppModel->ClearNoteEditPreview();
		}
		mSelectedNote.found = false;
	}
	else if (mMouseMode == MouseMode::ResizingNote && mSelectedNote.found)
	{
		// Finalize resize using preview state (track data was never modified)
		if (mAppModel->HasNoteEditPreview())
		{
			const auto& preview = mAppModel->GetNoteEditPreview();
			uint64_t newDuration = preview.previewEndTick - preview.previewStartTick;
			mAppModel->ResizeNote(mSelectedNote, newDuration);
			mAppModel->ClearNoteEditPreview();
		}
		mSelectedNote.found = false;
	}

	// Reset mode and refresh
	mMouseMode = MouseMode::Idle;
	Refresh();
}

void MidiCanvasPanel::OnMiddleDown(wxMouseEvent& event)
{
	wxPoint pos = event.GetPosition();

	// Check if we clicked on a note
	NoteLocation clickedNote = FindNoteAtPosition(pos.x, pos.y);

	if (clickedNote.found)
	{
		// Delete the note
		mAppModel->DeleteNote(clickedNote);

		// Clear hover state if we deleted the hovered note
		if (mHoveredNote.found &&
		    mHoveredNote.trackIndex == clickedNote.trackIndex &&
		    mHoveredNote.noteOnIndex == clickedNote.noteOnIndex)
		{
			mHoveredNote.found = false;
		}

		Refresh();
	} 
	// If mouse isn't on a note, move playhead to mouse click
	else
	{
		// Take the mouse position and convert that to ticks 
		uint64_t newTick = ScreenXToTick(pos.x);
		// set transport to new tick
		mTransport.ShiftToTick(newTick);
	}
}

void MidiCanvasPanel::OnRightDown(wxMouseEvent& event)
{
	// Ensure panel has keyboard focus
	SetFocus();

	mIsDragging = true;
	mLastMouse = event.GetPosition();
}

void MidiCanvasPanel::OnRightUp(wxMouseEvent& event)
{
	mIsDragging = false;
}

void MidiCanvasPanel::OnMouseMove(wxMouseEvent& event)
{
	wxPoint pos = event.GetPosition();

	// Handle rectangle selection dragging
	if (mIsSelecting)
	{
		mSelectionEnd = pos;
		// Find notes in current rectangle
		mSelectedNotes = FindNotesInRectangle(mSelectionStart, mSelectionEnd);
		Refresh();
		return;
	}

	// Handle right-click panning
	if (mIsDragging)
	{
		wxPoint delta = pos - mLastMouse;
		mOriginOffset += delta;
		ClampOffset(); // Apply boundaries after panning
		mLastMouse = pos;
		Refresh();
		return;
	}

	// Handle loop edge dragging
	if (mMouseMode == MouseMode::DraggingLoopStart)
	{
		uint64_t newTick = ScreenXToTick(pos.x);
		newTick = ApplyGridSnap(newTick);
		mTransport.SetLoopStart(newTick);
		Refresh();
		return;
	}

	if (mMouseMode == MouseMode::DraggingLoopEnd)
	{
		uint64_t newTick = ScreenXToTick(pos.x);
		newTick = ApplyGridSnap(newTick);
		mTransport.SetLoopEnd(newTick);
		Refresh();
		return;
	}

	// Handle note preview while adding (left button held)
	if (mMouseMode == MouseMode::Adding && mAppModel->IsPreviewingNote())
	{
		ubyte newPitch = ScreenYToPitch(pos.y);
		uint64_t newTick = ScreenXToTick(pos.x);

		bool pitchChanged = (newPitch != mAppModel->GetPreviewPitch());
		bool timingChanged = (newTick != mPreviewStartTick);

		// If pitch changed, switch the preview note audio
		if (pitchChanged)
		{
			mAppModel->StopPreviewNote();
			mAppModel->PlayPreviewNote(newPitch);
		}

		// If pitch or timing changed, update the visual preview
		if (pitchChanged || timingChanged)
		{
			mPreviewStartTick = newTick;  // Update current position
			Refresh();  // Redraw to show updated preview note
		}
		return;
	}

	// Handle moving note
	if (mMouseMode == MouseMode::MovingNote && mSelectedNote.found)
	{
		// Calculate new position based on mouse delta
		int deltaX = pos.x - mDragStartPos.x;
		int deltaY = pos.y - mDragStartPos.y;

		uint64_t newTick = mOriginalStartTick + (deltaX * mTicksPerPixel);
		int pitchDelta = -deltaY / mNoteHeight;  // Negative because Y is flipped
		int newPitch = std::clamp(static_cast<int>(mOriginalPitch) + pitchDelta, 0, MidiConstants::MAX_MIDI_NOTE);

		// Store preview state in model (doesn't modify track data)
		mAppModel->SetNoteMovePreview(mSelectedNote, newTick, static_cast<ubyte>(newPitch));

		Refresh();
		return;
	}

	// Handle resizing note
	if (mMouseMode == MouseMode::ResizingNote && mSelectedNote.found)
	{
		// Calculate new end position based on mouse X
		uint64_t newEndTick = ScreenXToTick(pos.x);
		// Ensure minimum duration
		if (newEndTick <= mOriginalStartTick) newEndTick = mOriginalStartTick + MIN_NOTE_DURATION_TICKS;

		// Store preview state in model (doesn't modify track data)
		mAppModel->SetNoteResizePreview(mSelectedNote, newEndTick);

		Refresh();
		return;
	}

	// Update hover state when idle
	if (mMouseMode == MouseMode::Idle)
	{
		NoteLocation newHover = FindNoteAtPosition(pos.x, pos.y);
		if (newHover.found != mHoveredNote.found ||
		    (newHover.found && (newHover.trackIndex != mHoveredNote.trackIndex ||
		                        newHover.noteOnIndex != mHoveredNote.noteOnIndex)))
		{
			mHoveredNote = newHover;
			Refresh();
		}
	}

	// Update debug message with mouse position
	uint64_t tick = ScreenXToTick(pos.x);
	ubyte pitch = ScreenYToPitch(pos.y);
	wxString msg = wxString::Format("Mouse: (%d, %d) | Tick: %llu, Pitch: %d",
	                                 pos.x, pos.y, tick, pitch);
	mDebugMessage->SetLabelText(msg);
}

void MidiCanvasPanel::OnSize(wxSizeEvent& event)
{
	int canvasHeight = GetSize().GetHeight();

	// Calculate minimum note height (fully zoomed out = all notes visible)
	mMinNoteHeight = std::max(1, canvasHeight / MidiConstants::MIDI_NOTE_COUNT);

	// Initialize to minimum zoom (fully zoomed out)
	mNoteHeight = mMinNoteHeight;

	// At minimum zoom, all notes fit exactly, so no vertical offset needed
	mOriginOffset.y = 0;

	ClampOffset(); // Ensure we're within valid bounds
	event.Skip(); // Allow default handling
}

void MidiCanvasPanel::OnMouseLeave(wxMouseEvent& event)
{
	// If we're previewing a note and the mouse leaves the window, stop the preview
	// This prevents stuck notes if the mouse leaves while left button is held
	if (mAppModel->IsPreviewingNote() && mMouseMode == MouseMode::Adding)
	{
		mAppModel->StopPreviewNote();
		mMouseMode = MouseMode::Idle;
	}
}

void MidiCanvasPanel::CopySelectedNotesToClipboard()
{
	if (mSelectedNotes.empty()) return;
	mAppModel->CopyNotesToClipboard(mSelectedNotes);
}

void MidiCanvasPanel::DeleteSelectedNotes()
{
	if (mSelectedNotes.empty()) return;

	mAppModel->DeleteNotes(mSelectedNotes);
	ClearSelection();
}

void MidiCanvasPanel::OnKeyDown(wxKeyEvent& event)
{
	int keyCode = event.GetKeyCode();

	// Delete - Delete selected notes
	if (keyCode == WXK_DELETE && !mSelectedNotes.empty())
	{
		DeleteSelectedNotes();
		Refresh();
		return;
	}

	// Escape - Clear selection
	if (keyCode == WXK_ESCAPE)
	{
		ClearSelection();
		Refresh();
		return;
	}

	// Ctrl+A - Select all notes
	if (event.ControlDown() && keyCode == 'A')
	{
		ClearSelection();
		mSelectedNotes = mTrackSet.GetAllNotes();
		Refresh();
		return;
	}

	// Ctrl+C - Copy selected notes to clipboard
	if (event.ControlDown() && keyCode == 'C' && !mSelectedNotes.empty())
	{
		CopySelectedNotesToClipboard();
		return;
	}

	// Ctrl+V - Paste clipboard notes at the playhead tick
	if (event.ControlDown() && keyCode == 'V')
	{
		mAppModel->PasteNotes();
		ClearSelection();
		Refresh();
		return;
	}

	// Ctrl+X - Cut selected notes (copy + delete)
	if (event.ControlDown() && keyCode == 'X' && !mSelectedNotes.empty())
	{
		CopySelectedNotesToClipboard();
		DeleteSelectedNotes();
		Refresh();
		return;
	}

	// Allow event to propagate for other handlers
	event.Skip();
}


// ========== Drawing Helper Methods ==========
void MidiCanvasPanel::DrawNote(wxGraphicsContext* gc, const NoteLocation& note)
{
	int x = TickToScreenX(note.startTick);
	int y = PitchToScreenY(note.pitch);
	int w = TicksToWidth(note.endTick - note.startTick);
	gc->DrawRectangle(x, y, w, mNoteHeight);
}

void MidiCanvasPanel::DrawGrid(wxGraphicsContext* gc)
{
	int canvasWidth = GetSize().GetWidth();
	int canvasHeight = GetSize().GetHeight();

	// Dynamic grid based on time signature
	int ticksPerBeat = mTransport.GetTicksPerBeat();
	int ticksPerMeasure = mTransport.GetTicksPerMeasure();

	// Draw vertical lines (time grid: beats and measures)
	int startTick = -mOriginOffset.x * mTicksPerPixel;
	int endTick = startTick + (canvasWidth * mTicksPerPixel);

	// Round to nearest beat
	startTick = (startTick / ticksPerBeat) * ticksPerBeat;

	for (int tick = startTick; tick <= endTick; tick += ticksPerBeat)
	{
		if (tick < 0) continue;

		int x = TickToScreenX(tick);
		if (x < 0 || x > canvasWidth) continue;

		bool isMeasure = (tick % ticksPerMeasure) == 0;
		if (isMeasure)
		{
			gc->SetPen(wxPen(GRID_MEASURE_LINE, 2));
		}
		else
		{
			gc->SetPen(wxPen(GRID_BEAT_LINE, 1));
		}
		gc->StrokeLine(x, 0, x, canvasHeight);
	}

	// Draw horizontal lines (pitch grid: notes and octaves)
	for (int midiNote = 0; midiNote <= MidiConstants::MAX_MIDI_NOTE; midiNote++)
	{
		int y = PitchToScreenY(midiNote);
		if (y < 0 || y > canvasHeight) continue;

		bool isOctave = (midiNote % MidiConstants::NOTES_PER_OCTAVE) == 0;
		if (isOctave)
		{
			gc->SetPen(wxPen(GRID_OCTAVE_LINE, 2));
		}
		else
		{
			gc->SetPen(wxPen(GRID_NOTE_LINE, 1));
		}
		gc->StrokeLine(0, y, canvasWidth, y);
	}
}

void MidiCanvasPanel::DrawLoopRegion(wxGraphicsContext* gc)
{
	if (!mTransport.mLoopEnabled && mTransport.GetLoopStart() == 0 && mTransport.GetLoopEnd() == MidiConstants::DEFAULT_LOOP_END)
		return;

	wxSize clientSize = GetClientSize();
	int loopStartX = TickToScreenX(mTransport.GetLoopStart());
	int loopEndX = TickToScreenX(mTransport.GetLoopEnd());
	int canvasHeight = clientSize.GetHeight() - CONTROL_BAR_HEIGHT;

	// Use semi-transparent blue when enabled, dimmed gray when disabled
	wxColour loopColor = mTransport.mLoopEnabled ? LOOP_ENABLED : LOOP_DISABLED;

	gc->SetBrush(wxBrush(loopColor));
	gc->SetPen(*wxTRANSPARENT_PEN);
	gc->DrawRectangle(loopStartX, 0, loopEndX - loopStartX, canvasHeight);
}

void MidiCanvasPanel::DrawTrackNotes(wxGraphicsContext* gc)
{
	std::vector<NoteLocation> allNotes = mTrackSet.GetAllNotes();

	for (const auto& note : allNotes)
	{
		// Only draw user tracks (0-14), skip metronome channel (15)
		if (note.trackIndex >= USER_TRACK_COUNT) continue;

		// Skip drawing the note being previewed (it's drawn separately as preview)
		if (mAppModel->HasNoteEditPreview())
		{
			const auto& preview = mAppModel->GetNoteEditPreview();
			if (note.trackIndex == preview.originalNote.trackIndex &&
			    note.noteOnIndex == preview.originalNote.noteOnIndex)
			{
				continue;  // Skip this note, will draw preview instead
			}
		}
		// Set color for this track
		gc->SetBrush(wxBrush(TRACK_COLORS[note.trackIndex]));
		DrawNote(gc, note);
	}
}

void MidiCanvasPanel::DrawRecordingBuffer(wxGraphicsContext* gc)
{
	std::vector<NoteLocation> notes = TrackSet::GetNotesFromTrack(mRecordingBuffer);

	gc->SetBrush(wxBrush(RECORDING_BUFFER));

	for (const auto& note : notes)
	{
		DrawNote(gc, note);
	}
}

void MidiCanvasPanel::DrawNoteAddPreview(wxGraphicsContext* gc)
{
	if (!mAppModel->IsPreviewingNote() || mMouseMode != MouseMode::Adding)
		return;

	gc->SetBrush(wxBrush(NOTE_ADD_PREVIEW));

	// Calculate preview note position with grid snap applied
	uint64_t snappedTick = ApplyGridSnap(mPreviewStartTick);
	uint64_t duration = GetSelectedDuration();

	int x = TickToScreenX(snappedTick);
	int y = PitchToScreenY(mAppModel->GetPreviewPitch());
	int w = TicksToWidth(duration);
	gc->DrawRectangle(x, y, w, mNoteHeight);
}

void MidiCanvasPanel::DrawNoteEditPreview(wxGraphicsContext* gc)
{
	if (!mAppModel->HasNoteEditPreview()) return;

	const auto& preview = mAppModel->GetNoteEditPreview();
	const auto& originalNote = preview.originalNote;

	// Determine color based on track (match original note color with transparency)
	wxColour previewColor = TRACK_COLORS[originalNote.trackIndex];
	previewColor.Set(
		previewColor.Red(),
		previewColor.Green(),
		previewColor.Blue(),
		NOTE_EDIT_PREVIEW_ALPHA
	);

	gc->SetBrush(wxBrush(previewColor));
	gc->SetPen(wxPen(PREVIEW_BORDER, PREVIEW_BORDER_WIDTH));

	int x = TickToScreenX(preview.previewStartTick);
	int y = PitchToScreenY(preview.previewPitch);
	int w = TicksToWidth(preview.previewEndTick - preview.previewStartTick);

	gc->DrawRectangle(x, y, w, mNoteHeight);
}

void MidiCanvasPanel::DrawSelectedNotes(wxGraphicsContext* gc)
{
	if (mSelectedNotes.empty()) return;

	gc->SetBrush(*wxTRANSPARENT_BRUSH);
	gc->SetPen(wxPen(SELECTION_BORDER, SELECTION_BORDER_WIDTH));

	for (const auto& note : mSelectedNotes)
	{
		DrawNote(gc, note);
	}
}

void MidiCanvasPanel::DrawHoverBorder(wxGraphicsContext* gc)
{
	if (!mHoveredNote.found || IsNoteSelected(mHoveredNote))
		return;

	gc->SetBrush(*wxTRANSPARENT_BRUSH);
	gc->SetPen(wxPen(HOVER_BORDER, HOVER_BORDER_WIDTH));
	DrawNote(gc, mHoveredNote);
}

void MidiCanvasPanel::DrawSelectionRectangle(wxGraphicsContext* gc)
{
	if (!mIsSelecting) return;

	gc->SetBrush(wxBrush(SELECTION_RECT_FILL));
	gc->SetPen(wxPen(SELECTION_RECT_BORDER, SELECTION_RECT_BORDER_WIDTH));

	int x = std::min(mSelectionStart.x, mSelectionEnd.x);
	int y = std::min(mSelectionStart.y, mSelectionEnd.y);
	int w = std::abs(mSelectionEnd.x - mSelectionStart.x);
	int h = std::abs(mSelectionEnd.y - mSelectionStart.y);

	gc->DrawRectangle(x, y, w, h);
}

void MidiCanvasPanel::DrawPlayhead(wxGraphicsContext* gc)
{
	int playheadX = TickToScreenX(mTransport.GetCurrentTick());
	int canvasHeight = GetSize().GetHeight();

	gc->SetPen(wxPen(PLAYHEAD, PLAYHEAD_WIDTH));
	gc->StrokeLine(playheadX, 0, playheadX, canvasHeight);
}
