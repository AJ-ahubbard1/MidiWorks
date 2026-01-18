// DrumMachinePanel.h
#pragma once
#include <wx/wx.h>
#include <wx/spinctrl.h>
#include <vector>
#include <memory>
#include "AppModel/AppModel.h"

/// Panel for drum machine sequencer interface.
///
/// Responsibilities:
/// - Display drum pad grid for pattern programming
/// - Allow user to toggle pads on/off for each column/pitch
/// - Support variable column count and rows
/// - Enable recording patterns to MIDI tracks
/// - Sync UI with loaded patterns
class DrumMachinePanel : public wxPanel
{
public:
	DrumMachinePanel(wxWindow* parent, std::shared_ptr<AppModel> appModel);

	/// Rebuild drum grid (called when column count or rows change)
	void RebuildGrid();

	/// Sync UI with loaded DrumMachine pattern
	void UpdateFromModel();

	/// Update single pad button color (efficient)
	void RefreshPadButtonColor(size_t rowIndex, size_t columnIndex);

	/// Refresh all pad colors (efficient for loop changes)
	void RefreshAllPadButtonColors();

	/// Update ticks-per-column display
	void UpdateTicksPerColumnDisplay();

private:
	// Model references
	std::shared_ptr<AppModel> mAppModel;
	DrumMachine& mDrumMachine;
	Transport& mTransport;

	// UI Controls
	wxCheckBox* mMuteCheckBox;
	wxSpinCtrl* mColumnCountSpinner;
	wxStaticText* mTicksPerColumnLabel;
	wxChoice* mChannelChoice;
	wxFlexGridSizer* mDrumGrid;
	wxButton* mAddRowButton;
	wxButton* mRemoveRowButton;
	wxButton* mRecordButton;
	wxButton* mClearButton;

	// Grid of buttons [rowIndex][columnIndex]
	std::vector<std::vector<wxButton*>> mPadButtons;
	std::vector<wxSpinCtrl*> mPitchSpinners;  // One pitch spinner per row

	// UI Construction
	void CreateControls();
	void SetupSizers();
	void BindEventHandlers();

	// Event Handlers
	void OnMuteToggle(wxCommandEvent& event);
	void OnColumnCountChanged(wxSpinEvent& event);
	void OnChannelChanged(wxCommandEvent& event);
	void OnPadToggle(wxCommandEvent& event);
	void OnPitchChanged(wxSpinEvent& event);
	void OnAddRow(wxCommandEvent& event);
	void OnRemoveRow(wxCommandEvent& event);
	void OnRecordToTrack(wxCommandEvent& event);
	void OnClear(wxCommandEvent& event);
};
