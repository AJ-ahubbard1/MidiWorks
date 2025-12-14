#pragma once
#include <wx/panel.h>
#include <wx/statline.h>
#include "AppModel/SoundBank/SoundBank.h"

class ChannelControlsPanel : public wxPanel
{
public:

	ChannelControlsPanel(wxWindow* parent, MidiChannel& channel, std::shared_ptr<MidiOut> midiOut)
		: wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize), mChannel(channel), mMidiOut(std::move(midiOut))
	{
		CreateControls();
		SetupSizers();
		BindEvents();
	}

	/// <summary>
	/// Updates all UI controls to reflect current channel data.
	/// Called after loading a project to sync UI with loaded values.
	/// </summary>
	void UpdateFromModel()
	{
		mPatchChoice->SetSelection(mChannel.programNumber);
		mVolumeSlider->SetValue(mChannel.volume);
		mMuteCheck->SetValue(mChannel.mute);
		mSoloCheck->SetValue(mChannel.solo);
		mRecordCheck->SetValue(mChannel.record);
	}

private:
	MidiChannel& mChannel;
	std::shared_ptr<MidiOut> mMidiOut;
	wxStaticLine* mStaticLine;
	wxStaticText* mLabel;
	wxChoice* mPatchChoice;
	wxSlider* mVolumeSlider;
	wxCheckBox* mMuteCheck;
	wxCheckBox* mSoloCheck;
	wxCheckBox* mRecordCheck;


	void CreateControls()
	{
		mStaticLine = new wxStaticLine(this, wxID_ANY);
		wxString strLabel = wxString::Format("Channel %d", mChannel.channelNumber + 1);
		mLabel = new wxStaticText(this, wxID_ANY, strLabel);

		mPatchChoice = new wxChoice(this, wxID_ANY);
		for (int i = 0; i < 128; ++i)
		{
			mPatchChoice->Append(MidiMessage::getSoundName(i));
			//mPatchChoice->Append(wxString::Format("Patch %d", i));
		}
		mPatchChoice->SetSelection(mChannel.programNumber);

		mVolumeSlider = new wxSlider(this, wxID_ANY, mChannel.volume, 0, 127);

		mMuteCheck = new wxCheckBox(this, wxID_ANY, "M");
		mMuteCheck->SetValue(mChannel.mute);

		mSoloCheck = new wxCheckBox(this, wxID_ANY, "S");
		mSoloCheck->SetValue(mChannel.solo);
		
		mRecordCheck = new wxCheckBox(this, wxID_ANY, "R");
		mRecordCheck->SetValue(mChannel.record);

	}
	void BindEvents()
	{
		mPatchChoice->Bind(wxEVT_CHOICE, &ChannelControlsPanel::OnPatchChanged, this);
		mVolumeSlider->Bind(wxEVT_SLIDER, &ChannelControlsPanel::OnVolumeChanged, this);
		mMuteCheck->Bind(wxEVT_CHECKBOX, &ChannelControlsPanel::OnMuteToggled, this);
		mSoloCheck->Bind(wxEVT_CHECKBOX, &ChannelControlsPanel::OnSoloToggled, this);
		mRecordCheck->Bind(wxEVT_CHECKBOX, &ChannelControlsPanel::OnRecordToggled, this);

	}

	void SetupSizers()
	{
		auto* sizer = new wxBoxSizer(wxVERTICAL);
		sizer->Add(mStaticLine, wxSizerFlags().Expand().Border(wxALL, 5));

		sizer->Add(mLabel, wxSizerFlags().CenterHorizontal());

		sizer->Add(mPatchChoice, wxSizerFlags().Expand().Border(wxALL, 5));

		auto* horizontalSizer = new wxBoxSizer(wxHORIZONTAL);
		horizontalSizer->Add(mVolumeSlider, wxSizerFlags().Expand().Border(wxALL, 5));
		horizontalSizer->Add(mMuteCheck, wxSizerFlags().Expand());
		horizontalSizer->Add(mSoloCheck, wxSizerFlags().Expand());
		horizontalSizer->Add(mRecordCheck, wxSizerFlags().Expand());
		sizer->Add(horizontalSizer);
		SetSizer(sizer);
	}

	void OnPatchChanged(wxCommandEvent& event)
	{
		mChannel.programNumber = mPatchChoice->GetSelection();
		SendPatch();
	}

	void OnVolumeChanged(wxCommandEvent& event)
	{
		mChannel.volume = static_cast<ubyte>(mVolumeSlider->GetValue());
		SendVolume();
	}

	// @TODO: currently Mute|Solo|Record checks don't have an affect yet on playback
	void OnMuteToggled(wxCommandEvent& event)
	{
		mChannel.mute = mMuteCheck->GetValue();
	}

	void OnSoloToggled(wxCommandEvent& event)
	{
		mChannel.solo = mSoloCheck->GetValue();
	}
	
	void OnRecordToggled(wxCommandEvent& event)
	{
		mChannel.record = mRecordCheck->GetValue();
	}

	void SendPatch()
	{
		if (mMidiOut)
		{
			auto pc = MidiMessage::ProgramChange(mChannel.programNumber, mChannel.channelNumber);
			mMidiOut->sendMessage(pc);
		}
	}

	void SendVolume()
	{
		if (mMidiOut)
		{
			auto vol = MidiMessage::ControlChange(VOLUME, mChannel.volume, mChannel.channelNumber);
			mMidiOut->sendMessage(vol);
		}
	}
};
