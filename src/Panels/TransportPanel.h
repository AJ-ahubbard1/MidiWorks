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
		Update();
	}

	void Update()
	{
		unsigned int ticks = mTransport.GetCurrentTick();
		mTickDisplay->SetLabel(wxString::Format("Ticks: %d", ticks));
		mTimeDisplay->SetLabel(mTransport.GetFormattedTime());
	}

	/// <summary>
	/// Updates the tempo control to reflect current Transport tempo.
	/// Called after loading a project to sync UI with loaded tempo value.
	/// </summary>
	void UpdateTempoDisplay()
	{
		auto beats = mTransport.GetBeatSettings();
		mTempoControl->SetValue(beats.tempo);
		UpdateTimeSignatureDisplay(beats);
	}

	void UpdateTimeSignatureDisplay(const Transport::BeatSettings& beats)
	{
		wxString numerator = wxString::Format("%d", beats.timeSignatureNumerator);
		if (!mNumeratorChoice->SetStringSelection(numerator))
		{
			// Fallback if numerator not found
			mNumeratorChoice->SetSelection(3); // "4"
		}
		wxString denominator= wxString::Format("%d", beats.timeSignatureDenominator);
		if (!mDenominatorChoice->SetStringSelection(denominator))
		{
			// Fallback if denominator not found
			mDenominatorChoice->SetSelection(1); // "4"
		}

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
	wxChoice* mNumeratorChoice;
	wxChoice* mDenominatorChoice;
	wxCheckBox* mMetronomeCheckBox;
	wxCheckBox* mLoopCheckBox;
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
		auto beatSettings = mTransport.GetBeatSettings();
		mTempoLabel = new wxStaticText(this, wxID_ANY, "Tempo:");
		mTempoControl = new wxSpinCtrlDouble(this, wxID_ANY, "", wxDefaultPosition, wxSize(80, -1));
		mTempoControl->SetRange(40.0, 300.0);  // 40-300 BPM range
		mTempoControl->SetValue(beatSettings.tempo);
		mTempoControl->SetIncrement(1.0);
		mTempoControl->SetDigits(1);  // Show 1 decimal place

		// Time Signature
		mNumeratorChoice = new wxChoice(this, wxID_ANY);
		mDenominatorChoice = new wxChoice(this, wxID_ANY);
		std::vector<wxString> nums;
		nums.reserve(MidiConstants::NUMERATOR_LIST->size());
		for (auto& numerator : MidiConstants::NUMERATOR_LIST)
		{
			nums.emplace_back(numerator);
		}
		mNumeratorChoice->Append(nums);

		std::vector<wxString> dens;
		dens.reserve(MidiConstants::DENOMINATOR_LIST->size());
		for (auto& denominator: MidiConstants::DENOMINATOR_LIST)
		{
			dens.emplace_back(denominator);
		}
		mDenominatorChoice->Append(dens);
		
		UpdateTimeSignatureDisplay(beatSettings);

		// Metronome checkbox (renamed to "Click")
		mMetronomeCheckBox = new wxCheckBox(this, wxID_ANY, "Click");
		mMetronomeCheckBox->SetValue(mModel->GetMetronomeService().IsEnabled());

		// Loop checkbox
		mLoopCheckBox = new wxCheckBox(this, wxID_ANY, "Loop");
		mLoopCheckBox->SetValue(mTransport.GetLoopSettings().enabled);
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
		sizer->Add(mNumeratorChoice, flags);
		sizer->AddSpacer(5);
		sizer->Add(mDenominatorChoice, flags);
		sizer->AddSpacer(5);
		sizer->Add(mMetronomeCheckBox, flags);
		sizer->AddSpacer(buttonSpace);
		sizer->Add(mLoopCheckBox, flags);
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
		mNumeratorChoice->Bind(wxEVT_CHOICE, &TransportPanel::OnNumeratorChange, this);
		mDenominatorChoice->Bind(wxEVT_CHOICE, &TransportPanel::OnDenominatorChange, this);
		mMetronomeCheckBox->Bind(wxEVT_CHECKBOX, &TransportPanel::OnMetronomeToggle, this);
		mLoopCheckBox->Bind(wxEVT_CHECKBOX, &TransportPanel::OnLoopToggle, this);
	}

	void OnStop(wxCommandEvent&)
	{
		if (mTransport.IsRecording())
		{
			mTransport.SetState(Transport::State::StopRecording);
		}
		else
		{
			mTransport.SetState(Transport::State::StopPlaying);
		}
	}
	void OnPlay(wxCommandEvent&) { mTransport.SetState(Transport::State::ClickedPlay); }
	void OnReset(wxCommandEvent&)
	{
		mTransport.Reset();
		if (mTransport.IsPlaying())
		{
			mTransport.SetState(Transport::State::ClickedPlay);
		}
		else if (mTransport.IsRecording())
		{
			mTransport.SetState(Transport::State::ClickedRecord);
		}
	}
	void OnRecord(wxCommandEvent&) { mTransport.SetState(Transport::State::ClickedRecord); }
	void OnRewindDown(wxMouseEvent& event)
	{
		mPreviousState = mTransport.GetState();
		mTransport.SetState(Transport::State::Rewinding);
		event.Skip();
	}
	void OnFastForwardDown(wxMouseEvent& event)
	{
		mPreviousState = mTransport.GetState();
		mTransport.SetState(Transport::State::FastForwarding);
		event.Skip();
	}
	void StopTransport(wxMouseEvent& event)
	{
		mTransport.ResetShiftRate();
		switch (mPreviousState)
		{
		case Transport::State::Stopped:
			mTransport.SetState(Transport::State::Stopped);
			break;
		case Transport::State::Playing:
			mTransport.SetState(Transport::State::ClickedPlay);
			break;
		case Transport::State::Recording:
			mTransport.SetState(Transport::State::ClickedRecord);
			break;
		}
		event.Skip();
	}

	void OnTempoChange(wxSpinDoubleEvent& event)
	{
		auto settings = mModel->GetTransport().GetBeatSettings();
		settings.tempo = mTempoControl->GetValue();
		mModel->GetTransport().SetBeatSettings(settings);
	}

	void OnNumeratorChange(wxCommandEvent& event)
	{
		auto newNumerator = mNumeratorChoice->GetStringSelection();
		auto beatSettings = mTransport.GetBeatSettings();
		beatSettings.timeSignatureNumerator = wxAtoi(newNumerator);
		mTransport.SetBeatSettings(beatSettings);
	}
	
	void OnDenominatorChange(wxCommandEvent& event)
	{
		auto newDenominator = mDenominatorChoice->GetStringSelection();
		auto beatSettings = mTransport.GetBeatSettings();
		beatSettings.timeSignatureDenominator = wxAtoi(newDenominator);
		mTransport.SetBeatSettings(beatSettings);
	}

	void OnMetronomeToggle(wxCommandEvent& event)
	{
		mModel->GetMetronomeService().SetEnabled(mMetronomeCheckBox->GetValue());
	}

	void OnLoopToggle(wxCommandEvent& event)
	{
		auto settings = mTransport.GetLoopSettings();
		settings.enabled = mLoopCheckBox->GetValue();
		mTransport.SetLoopSettings(settings);
	}
};

