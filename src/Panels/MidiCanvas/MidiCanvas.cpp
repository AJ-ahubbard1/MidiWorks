#include "MidiCanvas.h"
#include <map>
#include <cmath>

MidiCanvasPanel::MidiCanvasPanel(wxWindow* parent, std::shared_ptr<AppModel> appModel, const wxString& label)
	: wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, label),
	mAppModel(appModel),
	mTransport(appModel->GetTransport()),
	mTrackSet(appModel->GetTrackSet()),
	mRecordingBuffer(appModel->GetRecordingSession().GetBuffer()),
	mPreviewManager(appModel->GetPreviewManager())
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

	// Bind all Event Handlers
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
	// Initialize to UINT64_MAX to force offset update on first call
	static uint64_t lastTick = UINT64_MAX;
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
	DrawVelocityEditor(gc);
	DrawPianoKeyboard(gc);  // Draw last so it appears on top of scrolling notes

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

	// Prevent negative coordinates from causing uint64_t underflow
	if (x < 0) return 0;

	uint64_t tick = static_cast<uint64_t>(x) * mTicksPerPixel;

	// Clamp to MAX_TICK_VALUE to prevent overflow (also catches any underflow edge cases)
	if (tick > MidiConstants::MAX_TICK_VALUE) return 0;

	return tick;
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
	// @TODO: Always rounding down, might consider using round function to allow rounding up when greater than half of the duration
	return (tick / duration) * duration;  // Round down to nearest multiple
}

std::vector<NoteLocation> MidiCanvasPanel::FindNotesInRegionWithSoloFilter(
	uint64_t minTick, uint64_t maxTick,
	ubyte minPitch, ubyte maxPitch)
{
	auto& soundBank = mAppModel->GetSoundBank();

	// If solos are active, only search solo'd channels
	if (soundBank.SolosFound())
	{
		std::vector<NoteLocation> results;
		auto soloChannels = soundBank.GetSoloChannels();
		for (MidiChannel* channel : soloChannels)
		{
			std::vector<NoteLocation> channelNotes = mTrackSet.FindNotesInRegion(
				minTick, maxTick,
				minPitch, maxPitch,
				channel->channelNumber
			);
			results.insert(results.end(), channelNotes.begin(), channelNotes.end());
		}
		return results;
	}
	else
	{
		// Normal behavior - search all tracks
		return mTrackSet.FindNotesInRegion(minTick, maxTick, minPitch, maxPitch);
	}
}

NoteLocation MidiCanvasPanel::FindNoteAtPosition(int screenX, int screenY)
{
	uint64_t clickTick = ScreenXToTick(screenX);
	ubyte clickPitch = ScreenYToPitch(screenY);

	auto notes = FindNotesInRegionWithSoloFilter(
		clickTick, clickTick + 1,
		clickPitch, clickPitch
	);

	return notes.empty() ? NoteLocation{} : notes[0];
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
	
	if (minTick == maxTick)
	{
		return std::vector<NoteLocation>{};
	}

	return FindNotesInRegionWithSoloFilter(minTick, maxTick, minPitch, maxPitch);
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

NoteLocation MidiCanvasPanel::FindVelocityControlAtPosition(int screenX, int screenY)
{
	// Check if mouse is within the velocity editor region (bottom 25% of canvas)
	int canvasHeight = GetSize().GetHeight();
	double velocityEditorTop = 0.75 * canvasHeight;

	if (screenY < velocityEditorTop)
		return NoteLocation{};  // Not in velocity editor region

	// Check each selected note's velocity control
	int controlsPadding = 10;
	int controlsTop = velocityEditorTop + controlsPadding;
	int controlsHeight = canvasHeight - controlsPadding - controlsTop;
	int controlsRadius = 8;

	for (const auto& note : mSelectedNotes)
	{
		int startNoteX = TickToScreenX(note.startTick);
		int velocityControlY = controlsTop + (127 - note.velocity) * controlsHeight / 127;

		// Check if click is within radius of this control rectangle
		int dx = screenX - startNoteX;
		int dy = screenY - velocityControlY;
		int distance = std::sqrt(dx * dx + dy * dy);

		if (distance <= controlsRadius + 5)  // Add 5px tolerance for easier clicking
		{
			return note;
		}
	}

	return NoteLocation{};  // No control found
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

	// Horizontal limits: ticks [0, MAX_TICK_VALUE] gets mapped to pixels [0, minOffsetX]
	// When offset.x = 0, tick 0 is at left edge
	// When offset.x < 0, we've scrolled right (tick 0 is off-screen left)

	// Right boundary: can scroll right to see up to MAX_TICK_VALUE
	// When maxTick is at right edge: canvasWidth = MAX_TICK_VALUE / ticksPerPixel + offset.x
	int minOffsetX = canvasWidth - (MidiConstants::MAX_TICK_VALUE / mTicksPerPixel);

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
	wxColour loopColor = mTransport.GetLoopSettings().enabled ? LOOP_ENABLED : LOOP_DISABLED;

	gc->SetBrush(wxBrush(loopColor));
	gc->SetPen(*wxTRANSPARENT_PEN);
	gc->DrawRectangle(loopStartX, 0, loopEndX - loopStartX, canvasHeight);
}

void MidiCanvasPanel::DrawTrackNotes(wxGraphicsContext* gc)
{
	wxSize clientSize = GetClientSize();

	// Calculate visible viewport bounds for culling
	uint64_t visibleStartTick = ScreenXToTick(0);
	uint64_t visibleEndTick = ScreenXToTick(clientSize.GetWidth());
	ubyte visibleMinPitch = ScreenYToPitch(clientSize.GetHeight());
	ubyte visibleMaxPitch = ScreenYToPitch(0);

	// Only get notes within visible viewport (viewport culling optimization)
	std::vector<NoteLocation> visibleNotes = FindNotesInRegionWithSoloFilter(
		visibleStartTick,
		visibleEndTick,
		visibleMinPitch,
		visibleMaxPitch
	);

	for (const auto& note : visibleNotes)
	{
		// Only draw user tracks (0-14), skip metronome channel (15)
		if (note.trackIndex >= USER_TRACK_COUNT) continue;

		// Skip drawing the note being previewed (it's drawn separately as preview)
		if (mPreviewManager.HasNoteEditPreview())
		{
			const auto& preview = mPreviewManager.GetNoteEditPreview();
			if (note.trackIndex == preview.originalNote.trackIndex &&
			    note.noteOnIndex == preview.originalNote.noteOnIndex)
			{
				continue;  // Skip this note, will draw preview instead
			}
		}
		// Set color for this track
		auto trackColor = mAppModel->GetSoundBank().GetChannelColor(note.trackIndex);
		gc->SetBrush(wxBrush(trackColor));
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
	if (!mPreviewManager.HasNoteAddPreview() || mMouseMode != MouseMode::Adding)
		return;

	gc->SetBrush(wxBrush(NOTE_ADD_PREVIEW));

	// Get preview state from model
	const auto& preview = mPreviewManager.GetNoteAddPreview();
	uint64_t snappedTick = ApplyGridSnap(preview.tick);
	uint64_t duration = GetSelectedDuration();

	int x = TickToScreenX(snappedTick);
	int y = PitchToScreenY(preview.pitch);
	int w = TicksToWidth(duration);
	gc->DrawRectangle(x, y, w, mNoteHeight);
}

void MidiCanvasPanel::DrawNoteEditPreview(wxGraphicsContext* gc)
{
	// Draw single note preview
	if (mPreviewManager.HasNoteEditPreview())
	{
		const auto& preview = mPreviewManager.GetNoteEditPreview();
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

	// Draw multi-note preview
	if (mPreviewManager.HasMultiNoteEditPreview())
	{
		const auto& preview = mPreviewManager.GetMultiNoteEditPreview();
		for (const auto& originalNote : preview.originalNotes)
		{
			// Calculate new position with delta
			int64_t newTickSigned = static_cast<int64_t>(originalNote.startTick) + preview.tickDelta;
			uint64_t newTick = (newTickSigned < 0) ? 0 : static_cast<uint64_t>(newTickSigned);

			int newPitchSigned = static_cast<int>(originalNote.pitch) + preview.pitchDelta;
			ubyte newPitch = std::clamp(newPitchSigned, 0, MidiConstants::MAX_MIDI_NOTE);

			uint64_t duration = originalNote.endTick - originalNote.startTick;

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

			int x = TickToScreenX(newTick);
			int y = PitchToScreenY(newPitch);
			int w = TicksToWidth(duration);

			gc->DrawRectangle(x, y, w, mNoteHeight);
		}
	}
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

		if (event.mm.isNoteOn())
		{
			color = MIDI_EVENT_NOTE_ON;
		}
		else if (event.mm.isNoteOff())
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
		mDebugEvents.push_back({event, screenX, screenY});
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
	auto& midiMsg = event.timedEvent.mm;
	ubyte velocity = midiMsg.getVelocity();
	std::string eventType;
	if (midiMsg.isNoteOn() && velocity > 0)
	{
		eventType = "Note On";
	}
	else if (midiMsg.isNoteOff() || (midiMsg.isNoteOn() && velocity == 0))
	{
		eventType = "Note Off";
	}
	else
	{
		eventType = "Other";
	}

	// Format tooltip text
	std::string text = std::format("{}:{}:{}:{}, {}",
		eventType, midiMsg.getChannel(), midiMsg.getPitch(), velocity, event.timedEvent.tick);

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

void MidiCanvasPanel::DrawVelocityEditor(wxGraphicsContext* gc)
{
	// Only draw velocity editor if notes are selected
	if (mSelectedNotes.empty())
		return;

	int canvasWidth = GetSize().GetWidth();
	int canvasHeight = GetSize().GetHeight();
	// editor dimensions
	double x = 0;
	double y = 0.75 * canvasHeight;
	double w = canvasWidth;
	double h = canvasHeight - y;
	// Draw background of editor
	gc->SetBrush(wxBrush(wxColour(45, 45, 48)));  // Dark charcoal gray (similar to Ableton/FL Studio)
	gc->SetPen(*wxTRANSPARENT_PEN);
	gc->DrawRectangle(x, y, w, h);

	int controlsPadding = 10;
	int controlsTop = y + controlsPadding;
	int controlsHeight = canvasHeight - controlsPadding - controlsTop;
	int controlsRadius = 8;

	for (const NoteLocation& note : mSelectedNotes)
	{
		int startNoteX = TickToScreenX(note.startTick);

		// Check if this is the control being edited
		bool isBeingEdited = (mMouseMode == MouseMode::EditingVelocity &&
		                      mVelocityEditNote.found &&
		                      mVelocityEditNote.trackIndex == note.trackIndex &&
		                      mVelocityEditNote.noteOnIndex == note.noteOnIndex);

		// Use temporary velocity value if being edited, otherwise use note's velocity
		ubyte displayVelocity = isBeingEdited ? mVelocityEditNote.velocity : note.velocity;
		int velocityControlY = controlsTop + (127 - displayVelocity) * controlsHeight / 127;

		// Set colors based on edit state
		if (isBeingEdited)
		{
			gc->SetBrush(wxBrush(wxColour(255, 200, 100)));  // Orange for active editing
			gc->SetPen(wxPen(wxColour(255, 150, 50), 2));  // Bright orange for vertical line
		}
		else
		{
			gc->SetBrush(wxBrush(wxColour(120, 180, 255)));  // Light blue for slider handles
			gc->SetPen(wxPen(wxColour(120, 120, 125), 1));  // Light gray for vertical lines
		}

		// Draw vertical line
		gc->StrokeLine(startNoteX, controlsTop, startNoteX, controlsTop + controlsHeight);

		// Draw control rectangle
		gc->DrawRectangle(startNoteX - controlsRadius,
		                velocityControlY - controlsRadius,
		                controlsRadius * 2,
		                controlsRadius * 2);

		// Draw velocity value text when editing
		if (isBeingEdited)
		{
			std::string velocityText = std::to_string(displayVelocity);
			gc->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD), *wxWHITE);
			gc->DrawText(velocityText, startNoteX + controlsRadius + 5, velocityControlY - 5);
		}
	}
}

void MidiCanvasPanel::DrawPianoKeyboard(wxGraphicsContext* gc)
{
	int canvasWidth = GetSize().GetWidth();
	int canvasHeight = GetSize().GetHeight();
	int keyboardWidth = static_cast<int>(canvasWidth * 0.15);  // 15% of canvas width (leaves 5% gap to playhead at 20%)

	// Draw background for entire keyboard area (light gray)
	gc->SetBrush(wxBrush(wxColour(240, 240, 240)));
	gc->SetPen(*wxTRANSPARENT_PEN);
	gc->DrawRectangle(0, CONTROL_BAR_HEIGHT, keyboardWidth, canvasHeight - CONTROL_BAR_HEIGHT);

	// First pass: Draw white keys
	for (int pitch = 0; pitch <= MidiConstants::MAX_MIDI_NOTE; pitch++)
	{
		int y = PitchToScreenY(pitch);

		// Skip if outside visible area
		if (y < CONTROL_BAR_HEIGHT || y >= canvasHeight) continue;

		// Determine if this is a white or black key
		int noteInOctave = pitch % MidiConstants::NOTES_PER_OCTAVE;  // 0=C, 1=C#, 2=D, etc.
		bool isBlackKey = (noteInOctave == 1 || noteInOctave == 3 ||
		                  noteInOctave == 6 || noteInOctave == 8 || noteInOctave == 10);

		if (!isBlackKey)
		{
			// White key
			gc->SetBrush(wxBrush(*wxWHITE));
			gc->SetPen(wxPen(wxColour(180, 180, 180), 1));
			gc->DrawRectangle(0, y, keyboardWidth, mNoteHeight);

			// Add octave labels on C notes
			if (noteInOctave == 0)
			{
				int octave = pitch / MidiConstants::NOTES_PER_OCTAVE;
				std::string label = "C" + std::to_string(octave);
				gc->SetFont(wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL), *wxBLACK);
				gc->DrawText(label, 2, y + 2);
			}
		}
	}

	// Second pass: Draw black keys (on top of white keys)
	for (int pitch = 0; pitch <= MidiConstants::MAX_MIDI_NOTE; pitch++)
	{
		int y = PitchToScreenY(pitch);

		// Skip if outside visible area
		if (y < CONTROL_BAR_HEIGHT || y >= canvasHeight) continue;

		int noteInOctave = pitch % MidiConstants::NOTES_PER_OCTAVE;
		bool isBlackKey = (noteInOctave == 1 || noteInOctave == 3 ||
		                  noteInOctave == 6 || noteInOctave == 8 || noteInOctave == 10);

		if (isBlackKey)
		{
			// Black key: smaller width (60% of keyboard width)
			int blackKeyWidth = static_cast<int>(keyboardWidth * 0.6);
			gc->SetBrush(wxBrush(*wxBLACK));
			gc->SetPen(wxPen(wxColour(50, 50, 50), 1));
			gc->DrawRectangle(0, y, blackKeyWidth, mNoteHeight);
		}
	}

	// Third pass: Highlight active notes (drawn on top)
	// Check for preview note (during note add)
	if (mPreviewManager.HasNoteAddPreview())
	{
		const auto& preview = mPreviewManager.GetNoteAddPreview();
		int y = PitchToScreenY(preview.pitch);

		if (y >= CONTROL_BAR_HEIGHT && y < canvasHeight)
		{
			// Green highlight for preview note
			gc->SetBrush(wxBrush(wxColour(100, 255, 100, 180)));
			gc->SetPen(*wxTRANSPARENT_PEN);
			gc->DrawRectangle(0, y, keyboardWidth, mNoteHeight);
		}
	}

	// Check for active recording notes
	if (mTransport.IsRecording())
	{
		const auto& activeNotes = mAppModel->GetRecordingSession().GetActiveNotes();
		for (const auto& activeNote : activeNotes)
		{
			int y = PitchToScreenY(activeNote.mm.getPitch());

			if (y >= CONTROL_BAR_HEIGHT && y < canvasHeight)
			{
				// Red-orange highlight for recording notes
				gc->SetBrush(wxBrush(wxColour(255, 150, 100, 180)));
				gc->SetPen(*wxTRANSPARENT_PEN);
				gc->DrawRectangle(0, y, keyboardWidth, mNoteHeight);
			}
		}
	}

	// Draw separator line at keyboard edge
	gc->SetPen(wxPen(*wxBLACK, 2));
	gc->StrokeLine(keyboardWidth, CONTROL_BAR_HEIGHT, keyboardWidth, canvasHeight);
}
