#pragma once
#include <wx/panel.h>
#include "AppModel/AppModel.h"
#include "ChannelControls.h"


class ChannelTrackRow : public wxPanel
{
public:
	ChannelTrackRow(wxWindow* parent, std::shared_ptr<AppModel> appModel, ubyte channel)
		: wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize),
		mAppModel(std::move(appModel)), mChannel(channel)
	{
		CreateControls();
		SetupSizers();
		BindEvents();
	}
private:
	ChannelControlsPanel* mChannelControls;
	std::shared_ptr<AppModel> mAppModel;
	ubyte mChannel;

	void CreateControls()
	{
		auto& soundBank = mAppModel->GetSoundBank();
		mChannelControls = new ChannelControlsPanel(this, soundBank.GetChannel(mChannel), soundBank.GetMidiOutDevice());

	}

	void SetupSizers() { }

	void BindEvents() { }


};
