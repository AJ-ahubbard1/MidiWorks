#pragma once
#include <wx/wx.h>
#include "AppModel/AppModel.h"

class TransportPanel : public wxPanel
{
public:
	TransportPanel(wxWindow* parent, Transport& transport, const wxColour& bgColor, const wxString& label)
		: wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, label),
		mTransport(transport)
	{
		SetBackgroundColour(bgColor);
		CreateControls();
		SetupSizers();
		BindEventHandlers();
		UpdateDisplay();
	}

	void UpdateDisplay()
	{
		unsigned int ticks = mTransport.GetCurrentTick();
		mTickDisplay->SetLabel(wxString::Format("Ticks: %d", ticks));
		mTimeDisplay->SetLabel(mTransport.GetFormattedTime());
	}

private:
	Transport& mTransport;
	wxStaticText* mTickDisplay;
	wxStaticText* mTimeDisplay;
	wxButton* mResetButton;
	wxButton* mRewindButton;
	wxButton* mStopButton;
	wxButton* mPlayButton;
	wxButton* mRecordButton;
	wxButton* mFastForwardButton;
	Transport::State mPreviousState;

	void CreateControls()
	{
		mTickDisplay = new wxStaticText(this, wxID_ANY, "");
		mTimeDisplay = new wxStaticText(this, wxID_ANY, "");
		mResetButton = new wxButton(this, wxID_ANY, "|<");
		mRewindButton = new wxButton(this, wxID_ANY, "<<");
		mStopButton = new wxButton(this, wxID_ANY, "[ ]");
		mPlayButton = new wxButton(this, wxID_ANY, ">");
		mRecordButton = new wxButton(this, wxID_ANY, "Rec");
		mFastForwardButton = new wxButton(this, wxID_ANY, ">>");
	}

	void SetupSizers()
	{
		auto* sizer = new wxBoxSizer(wxHORIZONTAL);
		auto& flags = wxSizerFlags().CenterVertical();
		int buttonSpace = 10;
		int outerBorder = 100;
		sizer->AddSpacer(outerBorder);
		sizer->Add(mTickDisplay, flags);
		sizer->AddStretchSpacer();
		sizer->Add(mTimeDisplay, flags);
		sizer->AddStretchSpacer();
		sizer->Add(mResetButton, flags);
		sizer->AddSpacer(buttonSpace);
		sizer->Add(mRewindButton, flags);
		sizer->AddSpacer(buttonSpace);
		sizer->Add(mStopButton, flags);
		sizer->AddSpacer(buttonSpace);
		sizer->Add(mPlayButton, flags);
		sizer->AddSpacer(buttonSpace);
		sizer->Add(mRecordButton, flags);
		sizer->AddSpacer(buttonSpace);
		sizer->Add(mFastForwardButton, flags);
		sizer->AddStretchSpacer();
		SetSizer(sizer);
	}

	void BindEventHandlers()
	{
		mStopButton->Bind(wxEVT_BUTTON, &TransportPanel::OnStop, this);
		mPlayButton->Bind(wxEVT_BUTTON, &TransportPanel::OnPlay, this);
		mRewindButton->Bind(wxEVT_LEFT_DOWN, &TransportPanel::OnRewindDown, this);
		mRewindButton->Bind(wxEVT_LEFT_UP, &TransportPanel::StopTransport, this);
		mFastForwardButton->Bind(wxEVT_LEFT_DOWN, &TransportPanel::OnFastForwardDown, this);
		mFastForwardButton->Bind(wxEVT_LEFT_UP, &TransportPanel::StopTransport, this);
		mResetButton->Bind(wxEVT_BUTTON, &TransportPanel::OnReset, this);
		mRecordButton->Bind(wxEVT_BUTTON, &TransportPanel::OnRecord, this);
	}

	void OnStop(wxCommandEvent&)
	{
		if (mTransport.mState == Transport::State::Recording)
		{
			mTransport.mState = Transport::State::StopRecording;
		}
		else
		{
			mTransport.mState = Transport::State::StopPlaying;
		}
	}
	void OnPlay(wxCommandEvent&) { mTransport.mState = Transport::State::ClickedPlay; }
	void OnReset(wxCommandEvent&) 
	{ 
		mTransport.Reset(); 
		if (mTransport.mState == Transport::State::Playing)
		{
			mTransport.mState = Transport::State::ClickedPlay;
		} 
		else if (mTransport.mState == Transport::State::Recording)
		{
			mTransport.mState = Transport::State::ClickedRecord;
		}
	}
	void OnRecord(wxCommandEvent&) { mTransport.mState = Transport::State::ClickedRecord; }

	void OnRewindDown(wxMouseEvent& event)
	{
		mPreviousState = mTransport.mState;
		mTransport.mState = Transport::State::Rewinding;
		event.Skip();
	}

	void OnFastForwardDown(wxMouseEvent& event)
	{
		mPreviousState = mTransport.mState;
		mTransport.mState = Transport::State::FastForwarding;
		event.Skip();
	}

	void StopTransport(wxMouseEvent& event)
	{
		switch (mPreviousState)
		{
		case Transport::State::Stopped:
			mTransport.mState = Transport::State::Stopped;
			break;
		case Transport::State::Playing:
			mTransport.mState = Transport::State::ClickedPlay;
			break;
		case Transport::State::Recording:
			mTransport.mState = Transport::State::ClickedRecord;
			break;
		}
		event.Skip();
	}
};

