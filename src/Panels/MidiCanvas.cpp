#include "MidiCanvas.h"

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
	mDurationChoice->Append("Whole Note (3840)", (void*)(intptr_t)3840);
	mDurationChoice->Append("Half Note (1920)", (void*)(intptr_t)1920);
	mDurationChoice->Append("Quarter Note (960)", (void*)(intptr_t)960);
	mDurationChoice->Append("Eighth Note (480)", (void*)(intptr_t)480);
	mDurationChoice->Append("Sixteenth Note (240)", (void*)(intptr_t)240);
	mDurationChoice->SetSelection(2);  // Default to quarter note

	controlsSizer->AddSpacer(5);
	controlsSizer->Add(mDurationChoice, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
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
}

void MidiCanvasPanel::Update()
{
	// Auto-scroll during playback/recording to keep playhead visible
	if (mTransport.mState == Transport::State::Playing ||
	    mTransport.mState == Transport::State::Recording)
	{
		wxSize clientSize = GetClientSize();
		int canvasWidth = clientSize.GetWidth();

		uint64_t currentTick = mTransport.GetCurrentTick();
		int playheadX = TickToScreenX(currentTick);

		// If playhead exceeds 80% of screen width, scroll to keep it at 20%
		int scrollThreshold = canvasWidth * 0.8;
		if (playheadX > scrollThreshold)
		{
			int targetPlayheadX = canvasWidth * 0.2;  // Keep playhead at 20% from left
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
	int controlBarHeight = 40;  // Approximate height of control bar

	// Paint background for control area (so it's not black)
	dc.SetBrush(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE)));
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.DrawRectangle(0, 0, clientSize.GetWidth(), controlBarHeight);

	// Only clear the canvas area, not the control area
	dc.SetClippingRegion(0, controlBarHeight, clientSize.GetWidth(), clientSize.GetHeight() - controlBarHeight);
	dc.Clear();

	wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
	if (!gc) return;

	// Draw grid lines first (behind notes)
	DrawGrid(gc);

	// Define colors for each of the 15 tracks (channel 16 reserved for metronome)
	wxColour trackColors[15] = {
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

	// Draw notes from all tracks
	for (int trackIndex = 0; trackIndex < 15; trackIndex++)
	{
		Track& track = mTrackSet.GetTrack(trackIndex);
		if (track.empty()) continue;

		// Set color for this track
		gc->SetBrush(wxBrush(trackColors[trackIndex]));

		size_t end = track.size();
		for (size_t i = 0; i < end; i++)
		{
			const TimedMidiEvent& noteOn = track[i];
			if (noteOn.mm.getEventType() != MidiEvent::NOTE_ON) continue;
			for (size_t j = i + 1; j < end; j++)
			{
				const TimedMidiEvent& noteOff = track[j];
				if (noteOff.mm.getEventType() != MidiEvent::NOTE_OFF ||
					noteOff.mm.mData[1] != noteOn.mm.mData[1]) continue;

				int x = noteOn.tick / mTicksPerPixel + mOriginOffset.x;
				int y = Flip(noteOn.mm.mData[1] * mNoteHeight) + mOriginOffset.y;
				int w = (noteOff.tick - noteOn.tick) / mTicksPerPixel;
				gc->DrawRectangle(x, y, w, mNoteHeight);
				break;
			}
		}
	}

	// Draw recording buffer (semi-transparent red/orange for in-progress recording)
	if (!mRecordingBuffer.empty())
	{
		wxColour recordingColor(255, 100, 50, 180);  // Red-orange, semi-transparent
		gc->SetBrush(wxBrush(recordingColor));

		size_t end = mRecordingBuffer.size();
		for (size_t i = 0; i < end; i++)
		{
			const TimedMidiEvent& noteOn = mRecordingBuffer[i];
			if (noteOn.mm.getEventType() != MidiEvent::NOTE_ON) continue;

			// Find corresponding note off
			for (size_t j = i + 1; j < end; j++)
			{
				const TimedMidiEvent& noteOff = mRecordingBuffer[j];
				if (noteOff.mm.getEventType() != MidiEvent::NOTE_OFF ||
					noteOff.mm.mData[1] != noteOn.mm.mData[1]) continue;

				int x = noteOn.tick / mTicksPerPixel + mOriginOffset.x;
				int y = Flip(noteOn.mm.mData[1] * mNoteHeight) + mOriginOffset.y;
				int w = (noteOff.tick - noteOn.tick) / mTicksPerPixel;
				gc->DrawRectangle(x, y, w, mNoteHeight);
				break;
			}
		}
	}

	// Draw preview note (semi-transparent green for note being added)
	if (mIsPreviewingNote && mMouseMode == MouseMode::Adding)
	{
		wxColour previewColor(100, 255, 100, 180);  // Green, semi-transparent
		gc->SetBrush(wxBrush(previewColor));

		// Calculate preview note position with grid snap applied
		uint64_t snappedTick = ApplyGridSnap(mPreviewStartTick);
		uint64_t duration = GetSelectedDuration();

		int x = snappedTick / mTicksPerPixel + mOriginOffset.x;
		int y = Flip(mPreviewPitch * mNoteHeight) + mOriginOffset.y;
		int w = duration / mTicksPerPixel;

		gc->DrawRectangle(x, y, w, mNoteHeight);
	}

	// Draw white border around hovered note
	if (mHoveredNote.valid)
	{
		gc->SetBrush(*wxTRANSPARENT_BRUSH);
		gc->SetPen(wxPen(*wxWHITE, 2));

		int x = mHoveredNote.startTick / mTicksPerPixel + mOriginOffset.x;
		int y = Flip(mHoveredNote.pitch * mNoteHeight) + mOriginOffset.y;
		int w = (mHoveredNote.endTick - mHoveredNote.startTick) / mTicksPerPixel;

		gc->DrawRectangle(x, y, w, mNoteHeight);
	}

	// Draw playhead (vertical line at current tick)
	uint64_t currentTick = mTransport.GetCurrentTick();
	int playheadX = currentTick / mTicksPerPixel + mOriginOffset.x;
	int canvasHeight = GetSize().GetHeight();

	gc->SetPen(wxPen(*wxRED, 2)); // Red line, 2 pixels wide
	gc->StrokeLine(playheadX, 0, playheadX, canvasHeight);

	delete gc;
}

void MidiCanvasPanel::DrawGrid(wxGraphicsContext* gc)
{
	int canvasWidth = GetSize().GetWidth();
	int canvasHeight = GetSize().GetHeight();

	// Constants for MIDI and timing
	const int TICKS_PER_QUARTER_NOTE = 960;
	const int BEATS_PER_MEASURE = 4;
	const int TICKS_PER_MEASURE = TICKS_PER_QUARTER_NOTE * BEATS_PER_MEASURE;
	const int NOTES_PER_OCTAVE = 12;
	const int MIN_MIDI_NOTE = 0;
	const int MAX_MIDI_NOTE = 127;

	// Grid colors
	wxColour beatLineColor(220, 220, 220);      // Light gray for beats
	wxColour measureLineColor(180, 180, 180);   // Darker gray for measures
	wxColour noteLineColor(240, 240, 240);      // Very light gray for notes
	wxColour octaveLineColor(200, 200, 200);    // Light gray for octaves

	// Draw vertical lines (time grid: beats and measures)
	int startTick = -mOriginOffset.x * mTicksPerPixel;
	int endTick = startTick + (canvasWidth * mTicksPerPixel);

	// Round to nearest beat
	startTick = (startTick / TICKS_PER_QUARTER_NOTE) * TICKS_PER_QUARTER_NOTE;

	for (int tick = startTick; tick <= endTick; tick += TICKS_PER_QUARTER_NOTE)
	{
		if (tick < 0) continue;

		int x = tick / mTicksPerPixel + mOriginOffset.x;
		if (x < 0 || x > canvasWidth) continue;

		bool isMeasure = (tick % TICKS_PER_MEASURE) == 0;
		if (isMeasure)
		{
			gc->SetPen(wxPen(measureLineColor, 2));
		}
		else
		{
			gc->SetPen(wxPen(beatLineColor, 1));
		}
		gc->StrokeLine(x, 0, x, canvasHeight);
	}

	// Draw horizontal lines (pitch grid: notes and octaves)
	for (int midiNote = MIN_MIDI_NOTE; midiNote <= MAX_MIDI_NOTE; midiNote++)
	{
		int y = Flip(midiNote * mNoteHeight) + mOriginOffset.y;
		if (y < 0 || y > canvasHeight) continue;

		bool isOctave = (midiNote % NOTES_PER_OCTAVE) == 0;
		if (isOctave)
		{
			gc->SetPen(wxPen(octaveLineColor, 2));
		}
		else
		{
			gc->SetPen(wxPen(noteLineColor, 1));
		}
		gc->StrokeLine(0, y, canvasWidth, y);
	}
}

int MidiCanvasPanel::Flip(int y)
{
	return GetSize().GetHeight() - y;
}

uint64_t MidiCanvasPanel::ScreenXToTick(int screenX)
{
	int x = screenX - mOriginOffset.x;
	return x * mTicksPerPixel;
}

uint8_t MidiCanvasPanel::ScreenYToPitch(int screenY)
{
	int y = screenY - mOriginOffset.y;
	int flippedY = GetSize().GetHeight() - y;
	int pitch = flippedY / mNoteHeight + 1;
	return std::clamp(pitch, 0, 127);
}

int MidiCanvasPanel::TickToScreenX(uint64_t tick)
{
	return tick / mTicksPerPixel + mOriginOffset.x;
}

int MidiCanvasPanel::PitchToScreenY(uint8_t pitch)
{
	// Center the note on the pitch line (offset by half note height upward)
	return Flip(pitch * mNoteHeight) + mOriginOffset.y - (mNoteHeight / 2);
}

void MidiCanvasPanel::PlayPreviewNote(uint8_t pitch)
{
	const uint8_t PREVIEW_VELOCITY = 100;
	auto& soundBank = mAppModel->GetSoundBank();
	auto midiOut = soundBank.GetMidiOutDevice();
	auto channels = soundBank.GetAllChannels();

	// Clear previous preview channels list
	mPreviewChannels.clear();

	// Send note on to all record-enabled channels
	for (const MidiChannel& channel : channels)
	{
		if (channel.record)
		{
			MidiMessage noteOn = MidiMessage::NoteOn(pitch, PREVIEW_VELOCITY, channel.channelNumber);
			midiOut->sendMessage(noteOn);
			mPreviewChannels.push_back(channel.channelNumber);
		}
	}

	mIsPreviewingNote = true;
	mPreviewPitch = pitch;
}

void MidiCanvasPanel::StopPreviewNote()
{
	if (!mIsPreviewingNote) return;

	auto midiOut = mAppModel->GetSoundBank().GetMidiOutDevice();

	// Send note off to all channels that are playing the preview
	for (uint8_t channelNum : mPreviewChannels)
	{
		MidiMessage noteOff = MidiMessage::NoteOff(mPreviewPitch, channelNum);
		midiOut->sendMessage(noteOff);
	}

	mIsPreviewingNote = false;
	mPreviewChannels.clear();
}

uint64_t MidiCanvasPanel::GetSelectedDuration() const
{
	int selection = mDurationChoice->GetSelection();
	if (selection == wxNOT_FOUND) return 960;  // Default to quarter note

	intptr_t duration = (intptr_t)mDurationChoice->GetClientData(selection);
	return static_cast<uint64_t>(duration);
}

uint64_t MidiCanvasPanel::ApplyGridSnap(uint64_t tick) const
{
	if (!mGridSnapCheckbox->GetValue()) return tick;

	uint64_t duration = GetSelectedDuration();
	return (tick / duration) * duration;  // Round down to nearest multiple
}

MidiCanvasPanel::NoteInfo MidiCanvasPanel::FindNoteAtPosition(int screenX, int screenY)
{
	NoteInfo result;
	result.valid = false;

	uint64_t clickTick = ScreenXToTick(screenX);
	uint8_t clickPitch = ScreenYToPitch(screenY);

	// Search through all tracks
	for (int trackIndex = 0; trackIndex < 15; trackIndex++)
	{
		Track& track = mTrackSet.GetTrack(trackIndex);
		if (track.empty()) continue;

		size_t end = track.size();
		for (size_t i = 0; i < end; i++)
		{
			const TimedMidiEvent& noteOn = track[i];
			if (noteOn.mm.getEventType() != MidiEvent::NOTE_ON) continue;

			// Find corresponding note off
			for (size_t j = i + 1; j < end; j++)
			{
				const TimedMidiEvent& noteOff = track[j];
				if (noteOff.mm.getEventType() != MidiEvent::NOTE_OFF ||
					noteOff.mm.mData[1] != noteOn.mm.mData[1]) continue;

				// Check if click is within this note's bounds
				uint8_t notePitch = noteOn.mm.mData[1];
				uint64_t noteStartTick = noteOn.tick;
				uint64_t noteEndTick = noteOff.tick;

				// Check pitch match (with some tolerance for note height)
				if (clickPitch == notePitch)
				{
					// Check tick range
					if (clickTick >= noteStartTick && clickTick <= noteEndTick)
					{
						result.valid = true;
						result.trackIndex = trackIndex;
						result.noteOnIndex = i;
						result.noteOffIndex = j;
						result.startTick = noteStartTick;
						result.endTick = noteEndTick;
						result.pitch = notePitch;
						return result;  // Return first match
					}
				}
				break;  // Found the note off for this note on
			}
		}
	}

	return result;
}

bool MidiCanvasPanel::IsOnResizeEdge(int screenX, const NoteInfo& note)
{
	if (!note.valid) return false;

	int noteEndX = TickToScreenX(note.endTick);
	// Resize zone: 5 pixels left of edge, 2 pixels right of edge
	return (screenX >= noteEndX - 5 && screenX <= noteEndX + 2);
}

void MidiCanvasPanel::ClampOffset()
{
	int canvasWidth = GetSize().GetWidth();
	int canvasHeight = GetSize().GetHeight();

	// Constants
	const int MAX_MEASURES = 100;  // Allow scrolling to 100 measures
	const int TICKS_PER_MEASURE = 960 * 4;  // 960 ticks/quarter * 4 beats
	const int MAX_TICK = MAX_MEASURES * TICKS_PER_MEASURE;
	const int MIDI_NOTE_COUNT = 128;
	const int MAX_MIDI_NOTE = 127;

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
	int totalMidiHeight = MIDI_NOTE_COUNT * mNoteHeight;

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
		int maxOffsetY = MAX_MIDI_NOTE * mNoteHeight - canvasHeight;

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
	wxPoint pos = event.GetPosition();

	// Check if we clicked on an existing note
	NoteInfo clickedNote = FindNoteAtPosition(pos.x, pos.y);

	if (clickedNote.valid)
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
		// Clicked on empty space - add new note
		uint64_t tick = ScreenXToTick(pos.x);
		uint8_t pitch = ScreenYToPitch(pos.y);

		// Ignore notes above pitch 120 (extreme high range, rarely used)
		if (pitch > 120)
		{
			return;
		}

		// Start preview playback on record-enabled channels
		PlayPreviewNote(pitch);

		// Store the starting tick for note creation
		mPreviewStartTick = tick;
		mMouseMode = MouseMode::Adding;
	}
}

void MidiCanvasPanel::OnLeftUp(wxMouseEvent& event)
{
	if (mMouseMode == MouseMode::Adding && mIsPreviewingNote)
	{
		// Stop the preview playback
		StopPreviewNote();

		// Apply grid snap to starting tick
		uint64_t snappedTick = ApplyGridSnap(mPreviewStartTick);
		uint64_t duration = GetSelectedDuration();
		const uint8_t NOTE_VELOCITY = 100;

		// Add note to all record-enabled channels
		auto& soundBank = mAppModel->GetSoundBank();
		auto channels = soundBank.GetAllChannels();

		for (const MidiChannel& channel : channels)
		{
			if (channel.record)
			{
				// Create note-on and note-off events
				MidiMessage noteOn = MidiMessage::NoteOn(mPreviewPitch, NOTE_VELOCITY, channel.channelNumber);
				MidiMessage noteOff = MidiMessage::NoteOff(mPreviewPitch, channel.channelNumber);

				TimedMidiEvent timedNoteOn{noteOn, snappedTick};
				TimedMidiEvent timedNoteOff{noteOff, snappedTick + duration - 1};  // -1 to prevent overlap with next note

				// Get the track for this channel
				Track& track = mTrackSet.GetTrack(channel.channelNumber);

				// Create and execute command
				auto cmd = std::make_unique<AddNoteCommand>(track, timedNoteOn, timedNoteOff);
				mAppModel->ExecuteCommand(std::move(cmd));
			}
		}
	}
	else if (mMouseMode == MouseMode::MovingNote && mSelectedNote.valid)
	{
		// Finalize move with command (for undo support)
		Track& track = mTrackSet.GetTrack(mSelectedNote.trackIndex);

		// Get current position (after dragging)
		uint64_t currentTick = track[mSelectedNote.noteOnIndex].tick;
		uint8_t currentPitch = track[mSelectedNote.noteOnIndex].mm.mData[1];

		// Restore original position first
		uint64_t duration = mOriginalEndTick - mOriginalStartTick;
		track[mSelectedNote.noteOnIndex].tick = mOriginalStartTick;
		track[mSelectedNote.noteOnIndex].mm.mData[1] = mOriginalPitch;
		track[mSelectedNote.noteOffIndex].tick = mOriginalEndTick;
		track[mSelectedNote.noteOffIndex].mm.mData[1] = mOriginalPitch;

		// Only create command if position actually changed
		if (currentTick != mOriginalStartTick || currentPitch != mOriginalPitch)
		{
			// Execute move command
			auto cmd = std::make_unique<MoveNoteCommand>(track, mSelectedNote.noteOnIndex,
			                                             mSelectedNote.noteOffIndex,
			                                             currentTick, currentPitch);
			mAppModel->ExecuteCommand(std::move(cmd));
		}

		mSelectedNote.valid = false;
	}
	else if (mMouseMode == MouseMode::ResizingNote && mSelectedNote.valid)
	{
		// Finalize resize with command (for undo support)
		Track& track = mTrackSet.GetTrack(mSelectedNote.trackIndex);

		// Get current end tick (after dragging)
		uint64_t currentEndTick = track[mSelectedNote.noteOffIndex].tick;

		// Restore original duration first
		track[mSelectedNote.noteOffIndex].tick = mOriginalEndTick;

		// Only create command if duration actually changed
		uint64_t newDuration = currentEndTick - mOriginalStartTick;
		uint64_t oldDuration = mOriginalEndTick - mOriginalStartTick;

		if (newDuration != oldDuration)
		{
			// Execute resize command
			auto cmd = std::make_unique<ResizeNoteCommand>(track, mSelectedNote.noteOnIndex,
			                                               mSelectedNote.noteOffIndex,
			                                               newDuration);
			mAppModel->ExecuteCommand(std::move(cmd));
		}

		mSelectedNote.valid = false;
	}

	// Reset mode and refresh
	mMouseMode = MouseMode::Idle;
	Refresh();
}

void MidiCanvasPanel::OnMiddleDown(wxMouseEvent& event)
{
	wxPoint pos = event.GetPosition();

	// Check if we clicked on a note
	NoteInfo clickedNote = FindNoteAtPosition(pos.x, pos.y);

	if (clickedNote.valid)
	{
		// Delete the note
		Track& track = mTrackSet.GetTrack(clickedNote.trackIndex);
		auto cmd = std::make_unique<DeleteNoteCommand>(track, clickedNote.noteOnIndex,
		                                               clickedNote.noteOffIndex);
		mAppModel->ExecuteCommand(std::move(cmd));

		// Clear hover state if we deleted the hovered note
		if (mHoveredNote.valid &&
		    mHoveredNote.trackIndex == clickedNote.trackIndex &&
		    mHoveredNote.noteOnIndex == clickedNote.noteOnIndex)
		{
			mHoveredNote.valid = false;
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

	// Handle note preview while adding (left button held)
	if (mMouseMode == MouseMode::Adding && mIsPreviewingNote)
	{
		uint8_t newPitch = ScreenYToPitch(pos.y);
		uint64_t newTick = ScreenXToTick(pos.x);

		bool pitchChanged = (newPitch != mPreviewPitch);
		bool timingChanged = (newTick != mPreviewStartTick);

		// If pitch changed, switch the preview note audio
		if (pitchChanged)
		{
			StopPreviewNote();
			PlayPreviewNote(newPitch);
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
	if (mMouseMode == MouseMode::MovingNote && mSelectedNote.valid)
	{
		// Calculate new position based on mouse delta
		int deltaX = pos.x - mDragStartPos.x;
		int deltaY = pos.y - mDragStartPos.y;

		uint64_t newTick = mOriginalStartTick + (deltaX * mTicksPerPixel);
		int pitchDelta = -deltaY / mNoteHeight;  // Negative because Y is flipped
		int newPitch = std::clamp(static_cast<int>(mOriginalPitch) + pitchDelta, 0, 127);

		// Update the note in the track (temporary, will be finalized on mouse up)
		Track& track = mTrackSet.GetTrack(mSelectedNote.trackIndex);
		uint64_t duration = mOriginalEndTick - mOriginalStartTick;

		track[mSelectedNote.noteOnIndex].tick = newTick;
		track[mSelectedNote.noteOnIndex].mm.mData[1] = newPitch;
		track[mSelectedNote.noteOffIndex].tick = newTick + duration;
		track[mSelectedNote.noteOffIndex].mm.mData[1] = newPitch;

		Refresh();
		return;
	}

	// Handle resizing note
	if (mMouseMode == MouseMode::ResizingNote && mSelectedNote.valid)
	{
		// Calculate new end position based on mouse X
		uint64_t newEndTick = ScreenXToTick(pos.x);
		// Ensure minimum duration
		if (newEndTick <= mOriginalStartTick) newEndTick = mOriginalStartTick + 100;

		// Update note off tick
		Track& track = mTrackSet.GetTrack(mSelectedNote.trackIndex);
		track[mSelectedNote.noteOffIndex].tick = newEndTick;

		Refresh();
		return;
	}

	// Update hover state when idle
	if (mMouseMode == MouseMode::Idle)
	{
		NoteInfo newHover = FindNoteAtPosition(pos.x, pos.y);
		if (newHover.valid != mHoveredNote.valid ||
		    (newHover.valid && (newHover.trackIndex != mHoveredNote.trackIndex ||
		                        newHover.noteOnIndex != mHoveredNote.noteOnIndex)))
		{
			mHoveredNote = newHover;
			Refresh();
		}
	}

	// Update debug message with mouse position
	uint64_t tick = ScreenXToTick(pos.x);
	uint8_t pitch = ScreenYToPitch(pos.y);
	wxString msg = wxString::Format("Mouse: (%d, %d) | Tick: %llu, Pitch: %d",
	                                 pos.x, pos.y, tick, pitch);
	mDebugMessage->SetLabelText(msg);
}

void MidiCanvasPanel::OnSize(wxSizeEvent& event)
{
	const int MIDI_NOTE_COUNT = 128;  // MIDI notes 0-127
	const int MAX_MIDI_NOTE = 127;
	int canvasHeight = GetSize().GetHeight();

	// Calculate minimum note height (fully zoomed out = all notes visible)
	mMinNoteHeight = std::max(1, canvasHeight / MIDI_NOTE_COUNT);

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
	if (mIsPreviewingNote && mMouseMode == MouseMode::Adding)
	{
		StopPreviewNote();
		mMouseMode = MouseMode::Idle;
	}
}
