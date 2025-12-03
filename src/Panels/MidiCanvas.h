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

	MidiCanvasPanel(wxWindow* parent, Transport& transport, Track& track, const wxString& label)
		: wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, label),
		mTransport(transport), mTrack(track)
	{
		SetBackgroundStyle(wxBG_STYLE_PAINT);
		mDebugMessage = new wxStaticText(this, wxID_ANY, "");
		Bind(wxEVT_PAINT, &MidiCanvasPanel::Draw, this);
		Bind(wxEVT_MOUSEWHEEL, &MidiCanvasPanel::OnMouseWheel, this);
		Bind(wxEVT_RIGHT_DOWN, &MidiCanvasPanel::OnRightDown, this);
		Bind(wxEVT_RIGHT_UP, &MidiCanvasPanel::OnRightUp, this);
		Bind(wxEVT_MOTION, &MidiCanvasPanel::OnMouseMove, this);

	}

	void Update()
	{
		Refresh();
	}

private:
	Transport& mTransport;
	Track& mTrack;
	wxStaticText* mDebugMessage;
	int mNoteHeight = 5;
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

		gc->SetBrush(*wxBLUE_BRUSH);
		size_t end = mTrack.size();
		for (size_t i = 0; i < end; i++)
		{
			const TimedMidiEvent& noteOn = mTrack[i];
			if (noteOn.mm.getEventType() != MidiEvent::NOTE_ON) continue;
			for (size_t j = i + 1; j < end; j++)
			{
				const TimedMidiEvent& noteOff = mTrack[j];
				if (noteOff.mm.getEventType() != MidiEvent::NOTE_OFF || 
					noteOff.mm.mData[1] != noteOn.mm.mData[1]) continue;

				int x = noteOn.tick / mTicksPerPixel + mOriginOffset.x;
				int y = Flip(noteOn.mm.mData[1] * mNoteHeight) + mOriginOffset.y;
				int w = (noteOff.tick - noteOn.tick) / mTicksPerPixel;
				gc->DrawRectangle(x, y, w, mNoteHeight);
				break;
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

	int Flip(int y) { return GetSize().GetHeight() - y; }

	void OnMouseWheel(wxMouseEvent& event)
	{
		int rotation = event.GetWheelRotation();     // positive = scroll up, negative = scroll down
		int delta = event.GetWheelDelta();           // usually 120
		int lines = rotation / delta;

		if (lines > 0)
			mTicksPerPixel = std::max(1, mTicksPerPixel - lines); // zoom in
		else if (lines < 0)
			mTicksPerPixel += -lines;                             // zoom out

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
		mLastMouse = pos;
		Refresh();
	}
};