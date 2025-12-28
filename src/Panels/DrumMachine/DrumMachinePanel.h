// DrumMachinePanel.h
#pragma once
#include <wx/wx.h>
#include <wx/spinctrl.h>
#include <vector>
#include <memory>
#include "AppModel/AppModel.h"

class DrumMachinePanel : public wxPanel
{
public:
	DrumMachinePanel(wxWindow* parent, std::shared_ptr<AppModel> appModel);

	void UpdateFromModel();  // Sync UI with loaded pattern

private:
	// Model references
	std::shared_ptr<AppModel> mAppModel;
	DrumMachine& mDrumMachine;
	Transport& mTransport;

	// UI Controls
	wxSpinCtrl* mColumnCountSpinner;
	wxChoice* mChannelChoice;
	wxFlexGridSizer* mDrumGrid;
	wxButton* mAddRowButton;
	wxButton* mRemoveRowButton;
	wxButton* mRecordButton;

	// Grid of buttons [rowIndex][columnIndex]
	std::vector<std::vector<wxButton*>> mPadButtons;

	// UI Construction
	void CreateControls();
	void SetupSizers();
	void BindEventHandlers();
	void RebuildGrid();

	// Event Handlers
	void OnColumnCountChanged(wxSpinEvent& event);
	void OnChannelChanged(wxCommandEvent& event);
	void OnPadToggle(wxCommandEvent& event);
	void OnAddRow(wxCommandEvent& event);
	void OnRemoveRow(wxCommandEvent& event);
	void OnRecordToTrack(wxCommandEvent& event);
};
