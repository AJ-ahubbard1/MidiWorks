#include "MidiCanvas.h"
#include <map>

MidiCanvasPanel::MidiCanvasPanel(wxWindow* parent, std::shared_ptr<AppModel> appModel, const wxString& label)
	: wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, label),
	mAppModel(appModel),
	mTransport(appModel->GetTransport()),
	mTrackSet(appModel->GetTrackSet()),
	mRecordingBuffer(appModel->GetRecordingSession().GetBuffer())
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

	// Show MIDI Events checkbox (debug)
	mShowMidiEventsCheckbox = new wxCheckBox(this, wxID_ANY, "Show MIDI Events");
	mShowMidiEventsCheckbox->SetValue(false);  // Off by default
	controlsSizer->Add(mShowMidiEventsCheckbox, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
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
	controlsSizer->AddSpacer(30);

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
	wxSize clientSize = GetClientSize();
	int canvasWidth = clientSize.GetWidth();
	int targetPlayheadX = canvasWidth * AUTOSCROLL_TARGET_POSITION;

	// Track tick changes to detect Reset button clicks while stopped
	static uint64_t lastTick = 0;
	bool tickChanged = (currentTick != lastTick);
	lastTick = currentTick;

	// Fixed playhead auto-scroll: playhead locked at target position, content scrolls
	// Update when: moving OR tick changed while stopped (e.g., Reset button)
	if (mTransport.IsMoving() || tickChanged)
	{
		// Lock playhead at target position (e.g., 20% from left edge)
		mOriginOffset.x = targetPlayheadX - (currentTick / mTicksPerPixel);
		ClampOffset();
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
	DrawMidiEventsDebug(gc);  // Debug: MIDI event circles
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

	// Horizontal limits (time axis)
	// When offset.x = 0, tick 0 is at left edge
	// When offset.x < 0, we've scrolled right (tick 0 is off-screen left)

	// Right boundary: can scroll right to see up to MAX_TICK_VALUE
	// When maxTick is at right edge: canvasWidth = MAX_TICK_VALUE / ticksPerPixel + offset.x
	int minOffsetX = canvasWidth - (MAX_TICK_VALUE / mTicksPerPixel);

	// Left boundary: allow scrolling left to position tick 0 at the target playhead position
	// This enables fixed playhead scrolling where tick 0 appears at the target position
	int targetPlayheadX = canvasWidth * AUTOSCROLL_TARGET_POSITION;
	int maxOffsetX = targetPlayheadX;

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
	wxSize clientSize = GetClientSize();
	int loopStartX = TickToScreenX(mTransport.GetLoopStart());
	int loopEndX = TickToScreenX(mTransport.GetLoopEnd());
	int canvasHeight = clientSize.GetHeight(); 

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
	if (!mAppModel->GetSoundBank().IsPreviewingNote() || mMouseMode != MouseMode::Adding)
		return;

	gc->SetBrush(wxBrush(NOTE_ADD_PREVIEW));

	// Calculate preview note position with grid snap applied
	uint64_t snappedTick = ApplyGridSnap(mPreviewStartTick);
	uint64_t duration = GetSelectedDuration();

	int x = TickToScreenX(snappedTick);
	int y = PitchToScreenY(mAppModel->GetSoundBank().GetPreviewPitch());
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

void MidiCanvasPanel::DrawMidiEventsDebug(wxGraphicsContext* gc)
{
	if (!mShowMidiEventsCheckbox->GetValue()) return;

	// Clear previous debug events cache
	mDebugEvents.clear();

	// Get all raw MIDI events from TrackSet
	std::vector<TimedMidiEvent> allEvents = mTrackSet.GetAllTimedMidiEvents();

	// Draw each MIDI event as a colored circle
	for (const auto& event : allEvents)
	{
		// Calculate screen position based on tick and pitch
		int screenX = TickToScreenX(event.tick);
		int screenY = PitchToScreenY(event.mm.getPitch());

		// Determine color based on event type
		wxColour color;
		MidiEvent eventType = event.mm.getEventType();
		bool isNoteOn = (eventType == MidiEvent::NOTE_ON);
		bool isNoteOff = (eventType == MidiEvent::NOTE_OFF);

		if (isNoteOn)
		{
			color = MIDI_EVENT_NOTE_ON;
		}
		else if (isNoteOff)
		{
			color = MIDI_EVENT_NOTE_OFF;
		}
		else
		{
			color = MIDI_EVENT_OTHER;
		}

		// Draw circle
		gc->SetBrush(wxBrush(color));
		gc->SetPen(*wxTRANSPARENT_PEN);
		gc->DrawEllipse(screenX - MIDI_EVENT_CIRCLE_RADIUS,
		                screenY - MIDI_EVENT_CIRCLE_RADIUS,
		                MIDI_EVENT_CIRCLE_RADIUS * 2,
		                MIDI_EVENT_CIRCLE_RADIUS * 2);

		// Cache event info for hover detection
		MidiEventDebugInfo debugInfo;
		debugInfo.tick = event.tick;
		debugInfo.pitch = event.mm.getPitch();
		debugInfo.velocity = event.mm.mData[2];
		debugInfo.trackIndex = event.mm.getChannel();
		debugInfo.isNoteOn = isNoteOn;
		debugInfo.screenX = screenX;
		debugInfo.screenY = screenY;
		mDebugEvents.push_back(debugInfo);
	}

	// Draw tooltip for hovered event
	if (mHoveredEventIndex >= 0 && mHoveredEventIndex < mDebugEvents.size())
	{
		DrawMidiEventTooltip(gc, mDebugEvents[mHoveredEventIndex]);
	}
}

void MidiCanvasPanel::DrawMidiEventTooltip(wxGraphicsContext* gc, const MidiEventDebugInfo& event)
{
	// Determine event type string
	std::string eventType;
	if (event.isNoteOn)
	{
		eventType = "Note On";
	}
	else if (event.velocity == 0 && !event.isNoteOn)
	{
		eventType = "Note Off";
	}
	else
	{
		eventType = "Other";
	}

	// Format tooltip text
	std::string text = std::format("{}:{}:{}:{}, {}",
	                               eventType,
	                               event.trackIndex,
	                               event.pitch,
	                               event.velocity,
	                               event.tick);

	// Measure text size
	double width, height;
	gc->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL), *wxBLACK);
	gc->GetTextExtent(text, &width, &height);

	// Draw background rectangle
	int padding = 4;
	int rectX = event.screenX + 10;
	int rectY = event.screenY - 10 - height - padding * 2;

	gc->SetBrush(wxBrush(wxColour(255, 255, 200, 230)));  // Light yellow background
	gc->SetPen(wxPen(*wxBLACK, 1));
	gc->DrawRectangle(rectX, rectY, width + padding * 2, height + padding * 2);

	// Draw text
	gc->DrawText(text, rectX + padding, rectY + padding);
}
