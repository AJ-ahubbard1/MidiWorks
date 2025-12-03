#pragma once
#include <wx/wx.h>
#include <wx/spinctrl.h>
#include "AppModel/AppModel.h"

class TransportPanel : public wxPanel
{
public:
	TransportPanel(wxWindow* parent, std::shared_ptr<AppModel> model, const wxColour& bgColor, const wxString& label)
		: wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, label),
		mModel(model),
		mTransport(model->GetTransport())
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
	std::shared_ptr<AppModel> mModel;
	Transport& mTransport;
	wxStaticText* mTickDisplay;
	wxStaticText* mTimeDisplay;
	wxButton* mResetButton;
	wxButton* mRewindButton;
	wxButton* mStopButton;
	wxButton* mPlayButton;
	wxButton* mRecordButton;
	wxButton* mFastForwardButton;
	wxStaticText* mTempoLabel;
	wxSpinCtrlDouble* mTempoControl;
	wxCheckBox* mMetronomeCheckBox;
	Transport::State mPreviousState;

	void CreateControls()
	{
		mTickDisplay = new wxStaticText(this, wxID_ANY, "");
		mTimeDisplay = new wxStaticText(this, wxID_ANY, "");
		mResetButton = new wxButton(this, wxID_ANY, "|<");
		mRewindButton = new wxButton(this, wxID_ANY, "<<");
		mStopButton = new wxButton(this, wxID_ANY, "STOP");
		mPlayButton = new wxButton(this, wxID_ANY, "PLAY");
		mRecordButton = new wxButton(this, wxID_ANY, "REC");
		mFastForwardButton = new wxButton(this, wxID_ANY, ">>");

		// Tempo control
		mTempoLabel = new wxStaticText(this, wxID_ANY, "Tempo:");
		mTempoControl = new wxSpinCtrlDouble(this, wxID_ANY, "", wxDefaultPosition, wxSize(80, -1));
		mTempoControl->SetRange(40.0, 300.0);  // 40-300 BPM range
		mTempoControl->SetValue(mTransport.mTempo);
		mTempoControl->SetIncrement(1.0);
		mTempoControl->SetDigits(1);  // Show 1 decimal place

		// Metronome checkbox (renamed to "Click")
		mMetronomeCheckBox = new wxCheckBox(this, wxID_ANY, "Click");
		mMetronomeCheckBox->SetValue(mModel->mMetronomeEnabled);
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
		sizer->Add(mTempoLabel, flags);
		sizer->AddSpacer(5);
		sizer->Add(mTempoControl, flags);
		sizer->AddSpacer(buttonSpace);
		sizer->Add(mMetronomeCheckBox, flags);
		sizer->AddSpacer(buttonSpace);
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
		mTempoControl->Bind(wxEVT_SPINCTRLDOUBLE, &TransportPanel::OnTempoChange, this);
		mMetronomeCheckBox->Bind(wxEVT_CHECKBOX, &TransportPanel::OnMetronomeToggle, this);
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

	void OnTempoChange(wxSpinDoubleEvent& event)
	{
		mModel->GetTransport().mTempo = mTempoControl->GetValue();
	}

	void OnMetronomeToggle(wxCommandEvent& event)
	{
		mModel->mMetronomeEnabled = mMetronomeCheckBox->GetValue();
	}
};

