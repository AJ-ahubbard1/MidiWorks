// MidiCanvasEventHandlers.cpp
// Event handler implementations for MidiCanvasPanel
#include "MidiCanvas.h"
#include <cmath>


// ============================================================================
// MOUSE WHEEL - ZOOM
// ============================================================================

void MidiCanvasPanel::OnMouseWheel(wxMouseEvent& event)
{
	int rotation = event.GetWheelRotation();     // positive = scroll up, negative = scroll down
	int delta = event.GetWheelDelta();           // usually 120
	int lines = rotation / delta;

	if (event.ShiftDown())
	{
		// Shift + wheel: Vertical zoom (adjust note height)
		if (lines > 0)
			mNoteHeight = std::min(MAX_NOTE_HEIGHT_PIXELS, mNoteHeight + lines); // zoom in (taller notes)
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


// ============================================================================
// LEFT MOUSE BUTTON - NOTE ADD/MOVE/RESIZE START
// ============================================================================

void MidiCanvasPanel::OnLeftDown(wxMouseEvent& event)
{
	// Ensure panel has keyboard focus for shortcuts to work
	SetFocus();

	wxPoint pos = event.GetPosition();

	// Check if we're clicking on velocity controls first (only if notes are selected)
	if (!mSelection.IsEmpty())
	{
		NoteLocation velocityControl = FindVelocityControlAtPosition(pos.x, pos.y);
		if (velocityControl.found)
		{
			mMouseMode = MouseMode::EditingVelocity;
			mVelocityEditNote = velocityControl;
			mOriginalVelocity = velocityControl.velocity;
			mDragStartPos = pos;
			return;
		}
	}

	// Check if we're clicking on loop edges (priority over notes)
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
			// Start resizing (single note only, not supported for multi-select)
			mMouseMode = MouseMode::ResizingNote;
			mOriginalStartTick = clickedNote.startTick;
			mOriginalEndTick = clickedNote.endTick;
		}
		else
		{
			// Check if clicked note is part of current selection
			if (mSelection.Contains(clickedNote))
			{
				// Start multi-note move
				mMouseMode = MouseMode::MovingMultipleNotes;
				mOriginalSelectedNotes = mSelection.GetNotes();  // Store original positions
			}
			else
			{
				// Start single-note move
				mMouseMode = MouseMode::MovingNote;
				mOriginalStartTick = clickedNote.startTick;
				mOriginalEndTick = clickedNote.endTick;
				mOriginalPitch = clickedNote.pitch;
			}
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
			mSelection.Clear();
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
			// Start note add preview
			uint64_t snappedTick = ApplyGridSnap(tick);
			uint64_t duration = GetSelectedDuration();
			mPreviewManager.SetNoteAddPreview(pitch, tick, snappedTick, duration);
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
		// Final selection is already in mSelection (updated during drag)
		Refresh();
		return;
	}

	// Finalize note addition using preview state
	if (mMouseMode == MouseMode::Adding && mPreviewManager.HasNoteAddPreview())
	{
		const auto& preview = mPreviewManager.GetNoteAddPreview();
		uint64_t snappedTick = ApplyGridSnap(preview.tick);
		uint64_t duration = GetSelectedDuration();

		// Add note to all record-enabled channels
		mAppModel->AddNoteToRecordChannels(preview.pitch, snappedTick, duration);
		mPreviewManager.ClearNoteAddPreview();
	}
	// Finalize move using preview state (track data was never modified)
	else if (mMouseMode == MouseMode::MovingNote && mSelectedNote.found)
	{
		if (mPreviewManager.HasNoteEditPreview())
		{
			const auto& preview = mPreviewManager.GetNoteEditPreview();
			mAppModel->MoveNote(mSelectedNote, preview.previewStartTick, preview.previewPitch);
			mPreviewManager.ClearNoteEditPreview();
		}
		mSelectedNote.found = false;
	}
	else if (mMouseMode == MouseMode::MovingMultipleNotes && !mOriginalSelectedNotes.empty())
	{
		// Finalize multi-note move using preview state (track data was never modified)
		if (mPreviewManager.HasMultiNoteEditPreview())
		{
			const auto& preview = mPreviewManager.GetMultiNoteEditPreview();
			mAppModel->MoveMultipleNotes(preview.originalNotes, preview.tickDelta, preview.pitchDelta);
			mPreviewManager.ClearNoteEditPreview();
		}
		mOriginalSelectedNotes.clear();
		mSelection.Clear();  // Clear stale selection (old positions no longer valid)
	}
	else if (mMouseMode == MouseMode::ResizingNote && mSelectedNote.found)
	{
		// Finalize resize using preview state (track data was never modified)
		if (mPreviewManager.HasNoteEditPreview())
		{
			const auto& preview = mPreviewManager.GetNoteEditPreview();
			uint64_t newDuration = preview.previewEndTick - preview.previewStartTick;
			mAppModel->ResizeNote(mSelectedNote, newDuration);
			mPreviewManager.ClearNoteEditPreview();
		}
		mSelectedNote.found = false;
	}
	else if (mMouseMode == MouseMode::EditingVelocity && mVelocityEditNote.found)
	{
		// Finalize velocity edit
		ubyte finalVelocity = mVelocityEditNote.velocity;

		// Create note location with original velocity for the command
		NoteLocation noteWithOriginalVelocity = mVelocityEditNote;
		noteWithOriginalVelocity.velocity = mOriginalVelocity;

		// Execute command (which will modify track data and enable undo/redo)
		// Updates Selection to show the new velocity value 
		mAppModel->EditNoteVelocity(noteWithOriginalVelocity, finalVelocity);
		
		// Reset state
		mVelocityEditNote.found = false;
	}

	// Reset mode and refresh
	mMouseMode = MouseMode::Idle;
	Refresh();
}

// ============================================================================
// MIDDLE MOUSE BUTTON - DELETE NOTE / MOVE PLAYHEAD
// ============================================================================

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
		if (newTick < MidiConstants::MAX_TICK_VALUE)
		{
			mTransport.ShiftToTick(newTick);
		}
		else
		{
			mTransport.ShiftToTick(0);
		}
	}
}


// ============================================================================
// RIGHT MOUSE BUTTON - PANNING
// ============================================================================

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


// ============================================================================
// MOUSE MOVE - DRAG OPERATIONS
// ============================================================================

void MidiCanvasPanel::OnMouseMove(wxMouseEvent& event)
{
	wxPoint pos = event.GetPosition();

	// Update debug message with mouse position
	uint64_t tick = ScreenXToTick(pos.x);
	ubyte pitch = ScreenYToPitch(pos.y);
	wxString msg = wxString::Format("Mouse: (%d, %d) | Tick: %llu, Pitch: %d",
	                                 pos.x, pos.y, tick, pitch);
	mDebugMessage->SetLabelText(msg);

	bool outOfBounds = (pos.x < GetSize().GetWidth() * AUTOSCROLL_TARGET_POSITION);

	// Handle rectangle selection dragging
	if (mIsSelecting)
	{
		mSelectionEnd = pos;
		// Find notes in current rectangle
		mSelection.SelectNotes(FindNotesInRectangle(mSelectionStart, mSelectionEnd));
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
		wxString msg = wxString::Format("Offset (%d, %d)", mOriginOffset.x, mOriginOffset.y);
		mDebugMessage->SetLabelText(msg);
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
	if (mMouseMode == MouseMode::Adding && mPreviewManager.HasNoteAddPreview())
	{
		ubyte newPitch = ScreenYToPitch(pos.y);
		uint64_t newTick = ScreenXToTick(pos.x);
		uint64_t snappedTick = ApplyGridSnap(newTick);
		uint64_t duration = GetSelectedDuration();

		// Update preview (handles collision detection and audio automatically)
		mPreviewManager.SetNoteAddPreview(newPitch, newTick, snappedTick, duration);
		Refresh();
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

	// Handle moving multiple notes
	if (mMouseMode == MouseMode::MovingMultipleNotes && !mOriginalSelectedNotes.empty())
	{
		// Calculate delta from drag start
		int deltaX = pos.x - mDragStartPos.x;
		int deltaY = pos.y - mDragStartPos.y;

		int64_t tickDelta = deltaX * mTicksPerPixel;
		int pitchDelta = -deltaY / mNoteHeight;  // Negative because Y is flipped

		// Store preview state in model (doesn't modify track data)
		mAppModel->SetMultipleNotesMovePreview(mOriginalSelectedNotes, tickDelta, pitchDelta);

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

	// Handle editing velocity
	if (mMouseMode == MouseMode::EditingVelocity && mVelocityEditNote.found)
	{
		// Calculate velocity editor region bounds
		int canvasHeight = GetSize().GetHeight();
		double velocityEditorTop = 0.75 * canvasHeight;
		int controlsPadding = 10;
		int controlsTop = velocityEditorTop + controlsPadding;
		int controlsHeight = canvasHeight - controlsPadding - controlsTop;

		// Calculate new velocity based on vertical mouse position (inverted: top = 127, bottom = 0)
		int relativeY = pos.y - controlsTop;
		int newVelocity = 127 - (relativeY * 127 / controlsHeight);
		newVelocity = std::clamp(newVelocity, 1, 127);  // Velocity range 1-127 (0 would be note-off)

		// Update only the temporary velocity in mVelocityEditNote for visual feedback
		// The actual track data will be updated when the command executes on mouse up
		mVelocityEditNote.velocity = static_cast<ubyte>(newVelocity);

		Refresh();
		return;
	}

	// Update hover state when idle
	if (mMouseMode == MouseMode::Idle && !outOfBounds)
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

	if (outOfBounds)
	{
		mHoveredNote = NoteLocation{};
	}
	
	// Update MIDI event hover detection
	if (mShowMidiEventsCheckbox->GetValue())
	{
		mHoveredEventIndex = -1;
		for (size_t i = 0; i < mDebugEvents.size(); i++)
		{
			int dx = pos.x - mDebugEvents[i].screenX;
			int dy = pos.y - mDebugEvents[i].screenY;
			int distance = std::sqrt(dx * dx + dy * dy);
			if (distance <= MIDI_EVENT_HOVER_DISTANCE)
			{
				mHoveredEventIndex = static_cast<int>(i);
				Refresh();
				break;
			}
		}
	}
}


// ============================================================================
// WINDOW EVENTS
// ============================================================================
// Event triggers when window size changes
void MidiCanvasPanel::OnSize(wxSizeEvent& event)
{
	int canvasWidth = GetSize().GetWidth();
	int canvasHeight = GetSize().GetHeight();

	// Calculate minimum note height (fully zoomed out = all notes visible)
	mMinNoteHeight = std::max(MIN_NOTE_HEIGHT_PIXELS, canvasHeight / MidiConstants::MIDI_NOTE_COUNT);

	// Initialize to 3 times minimum zoom
	mNoteHeight = mMinNoteHeight * 3;

	// Force ClampOffset() to reset horizontal position to show tick 0 at playhead
	// (handles timing issues where OnSize fires before canvas width is initialized)
	mOriginOffset.x = INT32_MAX;

	// Center viewport vertically on middle octaves
	mOriginOffset.y = (MidiConstants::MAX_MIDI_NOTE * mNoteHeight - canvasHeight) * 0.5;

	ClampOffset(); // Apply offset bounds and finalize positioning
	event.Skip();
}

void MidiCanvasPanel::OnMouseLeave(wxMouseEvent& event)
{
	// If we're previewing a note and the mouse leaves the window, stop the preview
	// This prevents stuck notes if the mouse leaves while left button is held
	if (mPreviewManager.HasNoteAddPreview() && mMouseMode == MouseMode::Adding)
	{
		mPreviewManager.ClearNoteAddPreview();
		mMouseMode = MouseMode::Idle;
	}
}


// ============================================================================
// KEYBOARD EVENTS
// ============================================================================

void MidiCanvasPanel::OnKeyDown(wxKeyEvent& event)
{

	int keyCode = event.GetKeyCode();

	// Delete - Delete selected notes
	if (keyCode == WXK_DELETE && !mSelection.IsEmpty())
	{
		DeleteSelectedNotes();
		Refresh();
		return;
	}

	// Escape - Clear selection
	if (keyCode == WXK_ESCAPE)
	{
		mSelection.Clear();
		Refresh();
		return;
	}

	// Ctrl+A - Select all notes
	if (event.ControlDown() && keyCode == 'A')
	{
		mSelection.Clear();
		mSelection.SelectNotes(
			FindNotesInRegionWithSoloFilter( 
				0, MidiConstants::MAX_TICK_VALUE, 0, MidiConstants::MAX_MIDI_NOTE
			)
		);
		Refresh();
		return;
	}

	// Ctrl+C - Copy selected notes to clipboard
	if (event.ControlDown() && keyCode == 'C' && !mSelection.IsEmpty())
	{
		CopySelectedNotesToClipboard();
		return;
	}

	// Ctrl+V - Paste clipboard notes at the playhead tick
	if (event.ControlDown() && keyCode == 'V')
	{
		// Ctrl+Shift+V - Paste to record-enabled tracks
		if (event.ShiftDown())
		{
			mAppModel->PasteNotesToRecordTracks();
			mSelection.Clear();
			Refresh();
			return;
		}
		// Ctrl+V - Regular paste
		else
		{
			mAppModel->PasteNotes();
			mSelection.Clear();
			Refresh();
			return;
		}
	}

	// Ctrl+X - Cut selected notes (copy + delete)
	if (event.ControlDown() && keyCode == 'X' && !mSelection.IsEmpty())
	{
		CopySelectedNotesToClipboard();
		DeleteSelectedNotes();
		Refresh();
		return;
	}

	// Allow event to propagate for other handlers
	event.Skip();
}


// ============================================================================
// HELPER METHODS (for keyboard shortcuts)
// ============================================================================

void MidiCanvasPanel::CopySelectedNotesToClipboard()
{
	if (mSelection.IsEmpty()) return;
	mAppModel->GetClipboard().CopyNotes(mSelection.GetNotes(), mTrackSet);
}

void MidiCanvasPanel::DeleteSelectedNotes()
{
	if (mSelection.IsEmpty()) return;

	mAppModel->DeleteNotes(mSelection.GetNotes());
	mSelection.Clear();
}
