// Log.h
#pragma once
#include <wx/wx.h>
#include "AppModel/AppModel.h"

/// Panel that displays MIDI event log in real-time.
///
/// Responsibilities:
/// - Display incoming and outgoing MIDI events
/// - Show event details (tick, type, pitch, velocity)
/// - Provide scrollable text display
class LogPanel : public wxPanel
{
public:
	LogPanel(wxWindow* parent)
		: wxPanel(parent)
	{
		wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

		mTextCtrl = new wxTextCtrl(this, wxID_ANY, "",
			wxDefaultPosition, wxDefaultSize,
			wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH);

		sizer->Add(mTextCtrl, 1, wxEXPAND | wxALL, 5);
		SetSizer(sizer);
	}

	void LogMidiEvent(const TimedMidiEvent& msg)
	{
		std::ostringstream oss;
		oss << "[" << msg.tick << "] ";

		switch (msg.mm.getEventType()) {
		case MidiEvent::NOTE_ON:
			oss << "Note On - Pitch: " << int(msg.mm.mData[1])
				<< " Velocity: " << int(msg.mm.mData[2]);
			break;
		case MidiEvent::NOTE_OFF:
			oss << "Note Off - Pitch: " << int(msg.mm.mData[1]);
			break;
		default:
			oss << "Other MIDI Event";
			break;
		}

		wxString current = mTextCtrl->GetValue();
		mTextCtrl->SetValue(oss.str() + "\n" + current);
		mTextCtrl->ShowPosition(0); // scroll to top
	}

	void RefreshFromTrack(const std::vector<TimedMidiEvent>& track)
	{
		wxString fullText;
		for (auto it = track.rbegin(); it != track.rend(); ++it) {
			std::ostringstream oss;
			oss << "[" << it->tick << "] ";

			switch (it->mm.getEventType()) {
			case MidiEvent::NOTE_ON:
				oss << "Note On - Pitch: " << int(it->mm.mData[1])
					<< " Velocity: " << int(it->mm.mData[2]);
				break;
			case MidiEvent::NOTE_OFF:
				oss << "Note Off - Pitch: " << int(it->mm.mData[1]);
				break;
			default:
				oss << "Other MIDI Event";
				break;
			}

			fullText += oss.str() + "\n";
		}
		mTextCtrl->SetValue(fullText);
		mTextCtrl->ShowPosition(0);
	}

private:
	wxTextCtrl* mTextCtrl;
};
