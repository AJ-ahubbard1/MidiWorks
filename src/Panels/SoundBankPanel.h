#pragma once
#include <wx/wx.h>
#include "AppModel/SoundBank.h"
#include "ChannelControls.h"
#include <array>

class SoundBankPanel : public wxScrolledWindow
{
public: 
	SoundBankPanel(wxWindow* parent, SoundBank& soundBank, wxWindowID id = wxID_ANY)
		: wxScrolledWindow(parent, id), mSoundBank(soundBank)
	{
		SetScrollRate(5, 5);
		CreateControls();
		SetupSizers();
		BindEventHandlers();
	}


private:
	SoundBank& mSoundBank;
	wxStaticText* mMidiOutLabel;
	wxChoice* mMidiOutChoice;
	std::array<ChannelControlsPanel*, 15> mChannelControls;  // 15 channels (channel 16 reserved for metronome)

	void OnMidiOutChoice(wxCommandEvent& event)
	{
		ubyte port = mMidiOutChoice->GetSelection();
		mSoundBank.GetMidiOutDevice()->changePort(port);
		mSoundBank.ApplyChannelSettings();
	}

	void CreateControls()
	{
		auto midiOut = mSoundBank.GetMidiOutDevice();
		auto & portnames = midiOut->getPortNames();

		mMidiOutLabel = new wxStaticText(this, wxID_ANY, "Midi Out Port");
		mMidiOutChoice = new wxChoice(this, wxID_ANY);
		for (auto& port : portnames)
		{
			mMidiOutChoice->Append(port);
		}
		mMidiOutChoice->SetSelection(0);

		// Only create controls for channels 1-15 (channel 16 reserved for metronome)
		for (int i = 0; i < 15; i++)
		{
			mChannelControls[i] = new ChannelControlsPanel(this, mSoundBank.GetChannel(i), midiOut);
		}
	}

	void SetupSizers()
	{
		auto* sizer = new wxBoxSizer(wxVERTICAL);
		sizer->Add(mMidiOutLabel, wxSizerFlags().CenterHorizontal());
		sizer->Add(mMidiOutChoice, wxSizerFlags().CenterHorizontal());
		sizer->AddSpacer(10);
		for (auto& panel : mChannelControls)
		{
			sizer->Add(panel, wxSizerFlags().CenterHorizontal());
		}
		wxGridSizer* outerSizer = new wxGridSizer(1);
		outerSizer->SetSizeHints(this);
		outerSizer->Add(sizer, wxSizerFlags().Border(wxALL, 15).Expand());
		SetSizer(outerSizer);
	}

	void BindEventHandlers()
	{
		mMidiOutChoice->Bind(wxEVT_CHOICE, &SoundBankPanel::OnMidiOutChoice, this);
	}
};
