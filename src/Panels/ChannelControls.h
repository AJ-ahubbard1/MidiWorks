#pragma once
#include <wx/panel.h>
#include <wx/statline.h>
#include <wx/colordlg.h>
#include <wx/textdlg.h>
#include <wx/slider.h>
#include "AppModel/AppModel.h"
#include "Commands/ClearTrackCommand.h"

// Individual Control Panel shown for each channel of the SoundBank
class ChannelControlsPanel : public wxPanel
{
public:

	ChannelControlsPanel(wxWindow* parent, std::shared_ptr<AppModel> model, MidiChannel& channel)
		: wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize)
		, mAppModel(model)
		, mChannel(channel)
		, mMidiOut(mAppModel->GetSoundBank().GetMidiOutDevice())
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

		// Update color swatch
		mColorSwatch->SetBackgroundColour(mChannel.customColor);
		mColorSwatch->Refresh();

		// Update label with custom name if set
		wxString displayName = mChannel.customName.empty()
			? wxString::Format("Channel %d", mChannel.channelNumber + 1)
			: wxString(mChannel.customName);
		mLabel->SetLabel(displayName);
	}

private:
	std::shared_ptr<AppModel> mAppModel;
	MidiChannel& mChannel;
	std::shared_ptr<MidiOut> mMidiOut;
	wxStaticLine* mStaticLine;
	wxPanel* mColorSwatch;
	wxStaticText* mLabel;
	wxButton* mMinimizeButton;
	wxChoice* mPatchChoice;
	wxSlider* mVolumeSlider;
	wxCheckBox* mMuteCheck;
	wxCheckBox* mSoloCheck;
	wxCheckBox* mRecordCheck;
	wxButton* mClearButton;
	wxBoxSizer* mMainSizer;


	void CreateControls()
	{
		mStaticLine = new wxStaticLine(this, wxID_ANY);

		// Create color swatch - small colored square
		mColorSwatch = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(15, 15));
		mColorSwatch->SetBackgroundColour(mChannel.customColor);

		// Use custom name if set, otherwise default "Channel N"
		wxString strLabel = mChannel.customName.empty()
			? wxString::Format("Channel %d", mChannel.channelNumber + 1)
			: wxString(mChannel.customName);
		mLabel = new wxStaticText(this, wxID_ANY, strLabel);
		mMinimizeButton = new wxButton(this, wxID_ANY, "-", wxDefaultPosition, wxSize(20, -1));
		mPatchChoice = new wxChoice(this, wxID_ANY);

		// Channel 10 (index 9) is drums - use drum kit names instead of instrument names
		if (mChannel.channelNumber == 9)
		{
			// Populate with drum kit names
			for (int i = 0; i < 128; ++i)
			{
				mPatchChoice->Append(MidiMessage::getDrumKitName(i));
			}
		}
		else
		{
			// Populate with normal instrument names
			for (int i = 0; i < 128; ++i)
			{
				mPatchChoice->Append(MidiMessage::getSoundName(i));
			}
		}
		mPatchChoice->SetSelection(mChannel.programNumber);

		mVolumeSlider = new wxSlider(this, wxID_ANY, mChannel.volume, 0, 127);

		mMuteCheck = new wxCheckBox(this, wxID_ANY, "M");
		mMuteCheck->SetValue(mChannel.mute);

		mSoloCheck = new wxCheckBox(this, wxID_ANY, "S");
		mSoloCheck->SetValue(mChannel.solo);

		mRecordCheck = new wxCheckBox(this, wxID_ANY, "R");
		mRecordCheck->SetValue(mChannel.record);

		mClearButton = new wxButton(this, wxID_ANY, "CLEAR", wxDefaultPosition, wxDefaultSize);
	}
	void BindEvents()
	{
		mColorSwatch->Bind(wxEVT_LEFT_DOWN, &ChannelControlsPanel::OnColorSwatchClicked, this);
		mLabel->Bind(wxEVT_LEFT_DCLICK, &ChannelControlsPanel::OnLabelDoubleClicked, this);
		mMinimizeButton->Bind(wxEVT_BUTTON, &ChannelControlsPanel::OnMinimizeButton, this);
		mPatchChoice->Bind(wxEVT_CHOICE, &ChannelControlsPanel::OnPatchChanged, this);
		mVolumeSlider->Bind(wxEVT_SLIDER, &ChannelControlsPanel::OnVolumeChanged, this);
		mMuteCheck->Bind(wxEVT_CHECKBOX, &ChannelControlsPanel::OnMuteToggled, this);
		mSoloCheck->Bind(wxEVT_CHECKBOX, &ChannelControlsPanel::OnSoloToggled, this);
		mRecordCheck->Bind(wxEVT_CHECKBOX, &ChannelControlsPanel::OnRecordToggled, this);
		mClearButton->Bind(wxEVT_BUTTON, &ChannelControlsPanel::OnClearButton, this);

	}

	void SetupSizers()
	{
		mMainSizer = new wxBoxSizer(wxVERTICAL);
		mMainSizer->Add(mStaticLine, wxSizerFlags().Expand().Border(wxALL, 5));

		auto* topRow = new wxBoxSizer(wxHORIZONTAL);
		topRow->Add(mColorSwatch, wxSizerFlags().CenterVertical().Border(wxRIGHT, 5));
		topRow->Add(mLabel, wxSizerFlags().CenterVertical());
		topRow->AddStretchSpacer();
		topRow->Add(mMinimizeButton, wxSizerFlags().CenterVertical());
		mMainSizer->Add(topRow, wxSizerFlags().Expand());

		mMainSizer->Add(mPatchChoice, wxSizerFlags().Expand().Border(wxALL, 5));

		auto* horizontalSizer = new wxBoxSizer(wxHORIZONTAL);
		horizontalSizer->Add(mClearButton, wxSizerFlags().CenterVertical());
		horizontalSizer->Add(mVolumeSlider, wxSizerFlags().Expand().Border(wxALL, 5));
		horizontalSizer->Add(mMuteCheck, wxSizerFlags().Expand());
		horizontalSizer->Add(mSoloCheck, wxSizerFlags().Expand());
		horizontalSizer->Add(mRecordCheck, wxSizerFlags().Expand());
		mMainSizer->Add(horizontalSizer);
		SetSizer(mMainSizer);
	}

	void OnColorSwatchClicked(wxMouseEvent& event)
	{
		// Open color picker dialog
		wxColourData colorData;
		colorData.SetColour(mChannel.customColor);

		wxColourDialog dialog(this, &colorData);
		if (dialog.ShowModal() == wxID_OK)
		{
			// Update channel color
			mChannel.customColor = dialog.GetColourData().GetColour();

			// Update swatch display
			mColorSwatch->SetBackgroundColour(mChannel.customColor);
			mColorSwatch->Refresh();
		}
	}

	void OnLabelDoubleClicked(wxMouseEvent& event)
	{
		// Get current name or default
		wxString currentName = mChannel.customName.empty()
			? wxString::Format("Channel %d", mChannel.channelNumber + 1)
			: wxString(mChannel.customName);

		// Show rename dialog
		wxTextEntryDialog dialog(this, "Enter channel name:", "Rename Channel", currentName);
		if (dialog.ShowModal() == wxID_OK)
		{
			wxString newName = dialog.GetValue().Trim();

			// Update channel name (empty means use default)
			mChannel.customName = newName.ToStdString();

			// Update label display
			wxString displayName = mChannel.customName.empty()
				? wxString::Format("Channel %d", mChannel.channelNumber + 1)
				: wxString(mChannel.customName);
			mLabel->SetLabel(displayName);
		}
	}

	void OnMinimizeButton(wxCommandEvent& event)
	{
		// Get current width before any changes
		int currentWidth = GetSize().GetWidth();

		if (mChannel.minimized)
		{
			mChannel.minimized = false;
			mMinimizeButton->SetLabel("-");
			mPatchChoice->Show();
			mVolumeSlider->Show();
			mMuteCheck->Show();
			mSoloCheck->Show();
			mRecordCheck->Show();
			mClearButton->Show();
		}
		else
		{
			mChannel.minimized = true;
			mMinimizeButton->SetLabel("+");
			mPatchChoice->Hide();
			mVolumeSlider->Hide();
			mMuteCheck->Hide();
			mSoloCheck->Hide();
			mRecordCheck->Hide();
			mClearButton->Hide();
		}

		// Set minimum width to prevent horizontal shrinking
		SetMinSize(wxSize(currentWidth, -1));

		// Trigger layout recalculation
		GetSizer()->Layout();
		Fit();

		// Update parent layout
		if (GetParent())
		{
			GetParent()->Layout();
			GetParent()->FitInside();
		}
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

	void OnClearButton(wxCommandEvent& event)
	{
		bool trackIsEmpty = mAppModel->GetTrackSet().IsTrackEmpty(mChannel.channelNumber);
		if (trackIsEmpty) return;

		// Warn user that importing will clear the current project
		wxString prompt = wxString::Format(
			"This operation will clear all of the notes on channel %d.\n\nDo you want to continue?",
			mChannel.channelNumber + 1);

		int response = wxMessageBox(prompt, "Warning", wxYES_NO | wxICON_WARNING, this);

		if (response != wxYES)
		{
			return;
		}

		mAppModel->ClearTrack(mChannel.channelNumber);
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
