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
			// Show raw MIDI data for unknown events to help debug
			oss << "Other MIDI Event - Raw: [0x"
				<< std::hex << std::setfill('0') << std::setw(2) << int(msg.mm.mData[0]) << " "
				<< "0x" << std::setw(2) << int(msg.mm.mData[1]) << " "
				<< "0x" << std::setw(2) << int(msg.mm.mData[2])
				<< std::dec << "]";
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

	void LogError(const std::string& message)
	{
		// Format error message with timestamp (cross-platform using wxDateTime)
		wxDateTime now = wxDateTime::Now();
		wxString timestamp = now.Format("%H:%M:%S");
		wxString errorMsg = "[" + timestamp + "] ERROR: " + message + "\n";

		// Set text color to red for errors
		mTextCtrl->SetDefaultStyle(wxTextAttr(*wxRED));
		mTextCtrl->AppendText(errorMsg);

		// Reset color back to black for MIDI events
		mTextCtrl->SetDefaultStyle(wxTextAttr(*wxBLACK));

		mTextCtrl->ShowPosition(mTextCtrl->GetLastPosition()); // scroll to bottom
	}

	void LogWarning(const std::string& message)
	{
		// Format warning message with timestamp (cross-platform using wxDateTime)
		wxDateTime now = wxDateTime::Now();
		wxString timestamp = now.Format("%H:%M:%S");
		wxString warnMsg = "[" + timestamp + "] WARNING: " + message + "\n";

		// Set text color to orange for warnings
		mTextCtrl->SetDefaultStyle(wxTextAttr(wxColour(255, 140, 0)));
		mTextCtrl->AppendText(warnMsg);

		// Reset color back to black
		mTextCtrl->SetDefaultStyle(wxTextAttr(*wxBLACK));

		mTextCtrl->ShowPosition(mTextCtrl->GetLastPosition());
	}

private:
	wxTextCtrl* mTextCtrl;
};
