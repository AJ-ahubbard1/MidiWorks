#pragma once
#include <wx/wx.h>
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include "AppModel/Transport.h"
#include "AppModel/TrackSet.h"

using namespace MidiInterface;

class MidiCanvasPanel : public wxPanel
{
public:

	MidiCanvasPanel(wxWindow* parent, Transport& transport, TrackSet& trackSet, Track& recordingBuffer, const wxString& label)
		: wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, label),
		mTransport(transport), mTrackSet(trackSet), mRecordingBuffer(recordingBuffer)
	{
		SetBackgroundStyle(wxBG_STYLE_PAINT);
		mDebugMessage = new wxStaticText(this, wxID_ANY, "");
		Bind(wxEVT_PAINT, &MidiCanvasPanel::Draw, this);
		Bind(wxEVT_MOUSEWHEEL, &MidiCanvasPanel::OnMouseWheel, this);
		Bind(wxEVT_RIGHT_DOWN, &MidiCanvasPanel::OnRightDown, this);
		Bind(wxEVT_RIGHT_UP, &MidiCanvasPanel::OnRightUp, this);
		Bind(wxEVT_MOTION, &MidiCanvasPanel::OnMouseMove, this);
		Bind(wxEVT_SIZE, &MidiCanvasPanel::OnSize, this);
	}

	void Update()
	{
		Refresh();
	}

private:
	Transport& mTransport;
	TrackSet& mTrackSet;
	Track& mRecordingBuffer;
	wxStaticText* mDebugMessage;
	int mNoteHeight = 5;  // Current note height in pixels (pixels per MIDI note)
	int mMinNoteHeight = 1;  // Minimum zoom: canvasHeight / 128 (all notes visible)
	int mTicksPerPixel = 30;
	bool mIsDragging = false;
	wxPoint mLastMouse;
	wxPoint mOriginOffset;

	void Draw(wxPaintEvent&)
	{
		wxAutoBufferedPaintDC dc(this);
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

		// Draw playhead (vertical line at current tick)
		uint64_t currentTick = mTransport.GetCurrentTick();
		int playheadX = currentTick / mTicksPerPixel + mOriginOffset.x;
		int canvasHeight = GetSize().GetHeight();

		gc->SetPen(wxPen(*wxRED, 2)); // Red line, 2 pixels wide
		gc->StrokeLine(playheadX, 0, playheadX, canvasHeight);

		delete gc;
	}

	void DrawGrid(wxGraphicsContext* gc)
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

	int Flip(int y) { return GetSize().GetHeight() - y; }

	void ClampOffset()
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
			// Top boundary: note 127 should be visible at top (y = 0)
			// 0 = canvasHeight - (127 * noteHeight) + offset.y
			// offset.y = 127 * noteHeight - canvasHeight
			int minOffsetY = MAX_MIDI_NOTE * mNoteHeight - canvasHeight;

			// Bottom boundary: note 0 should stay at bottom
			int maxOffsetY = 0;

			// Clamp vertical offset
			mOriginOffset.y = std::max(minOffsetY, std::min(mOriginOffset.y, maxOffsetY));
		}
	}

	void OnMouseWheel(wxMouseEvent& event)
	{
		int rotation = event.GetWheelRotation();     // positive = scroll up, negative = scroll down
		int delta = event.GetWheelDelta();           // usually 120
		int lines = rotation / delta;

		if (lines > 0)
			mTicksPerPixel = std::max(1, mTicksPerPixel - lines); // zoom in
		else if (lines < 0)
			mTicksPerPixel += -lines;                             // zoom out

		ClampOffset(); // Apply boundaries after zoom

		wxString msg = wxString::Format("Ticks Per Pixel: %d", mTicksPerPixel);
		mDebugMessage->SetLabelText(msg);
		Refresh(); // trigger redraw

	}
	void OnRightDown(wxMouseEvent& event) 
	{ 
		mIsDragging = true;
		mLastMouse = event.GetPosition();
	}
	void OnRightUp(wxMouseEvent& event) 
	{ 
		mIsDragging = false;
	}
	void OnMouseMove(wxMouseEvent& event)
	{
		if (!mIsDragging) return;

		wxPoint pos = event.GetPosition();
		wxPoint delta = pos - mLastMouse;
		mOriginOffset += delta;
		ClampOffset(); // Apply boundaries after panning
		mLastMouse = pos;
		Refresh();
	}

	void OnSize(wxSizeEvent& event)
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
};