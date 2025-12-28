// DrumMachinePanel.cpp
#include "DrumMachinePanel.h"

DrumMachinePanel::DrumMachinePanel(wxWindow* parent, std::shared_ptr<AppModel> appModel)
	: wxPanel(parent, wxID_ANY)
	, mAppModel(appModel)
	, mDrumMachine(appModel->GetDrumMachine())
	, mTransport(appModel->GetTransport())
{
	CreateControls();
	SetupSizers();
	BindEventHandlers();
}

void DrumMachinePanel::CreateControls()
{
	// Column count spinner
	mColumnCountSpinner = new wxSpinCtrl(this, wxID_ANY, "16",
		wxDefaultPosition, wxSize(60, -1));
	mColumnCountSpinner->SetRange(4, 32);
	mColumnCountSpinner->SetValue(mDrumMachine.GetColumnCount());

	// Channel selector (1-16, but show 1-based indexing)
	mChannelChoice = new wxChoice(this, wxID_ANY);
	for (int i = 1; i <= 16; ++i)
	{
		mChannelChoice->Append(wxString::Format("Channel %d", i));
	}
	mChannelChoice->SetSelection(mDrumMachine.GetChannel());

	// Row management buttons
	mAddRowButton = new wxButton(this, wxID_ANY, "Add Row");
	mRemoveRowButton = new wxButton(this, wxID_ANY, "Remove Row");

	// Record button
	mRecordButton = new wxButton(this, wxID_ANY,
		wxString::Format("Record to Channel %d", mDrumMachine.GetChannel() + 1));

	// Build initial grid
	mDrumGrid = nullptr;
	RebuildGrid();
}

void DrumMachinePanel::SetupSizers()
{
	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

	// Top controls row
	wxBoxSizer* topSizer = new wxBoxSizer(wxHORIZONTAL);
	topSizer->Add(new wxStaticText(this, wxID_ANY, "Columns:"), wxSizerFlags().CenterVertical().Border(wxRIGHT, 5));
	topSizer->Add(mColumnCountSpinner, wxSizerFlags().Border(wxRIGHT, 15));
	topSizer->Add(new wxStaticText(this, wxID_ANY, "Channel:"), wxSizerFlags().CenterVertical().Border(wxRIGHT, 5));
	topSizer->Add(mChannelChoice, wxSizerFlags().Border(wxRIGHT, 15));
	mainSizer->Add(topSizer, wxSizerFlags().Expand().Border(wxALL, 10));

	// Drum grid (created in RebuildGrid)
	if (mDrumGrid)
	{
		mainSizer->Add(mDrumGrid, wxSizerFlags(1).Expand().Border(wxLEFT | wxRIGHT | wxBOTTOM, 10));
	}

	// Row management buttons
	wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
	buttonSizer->Add(mAddRowButton, wxSizerFlags().Border(wxRIGHT, 5));
	buttonSizer->Add(mRemoveRowButton, wxSizerFlags().Border(wxRIGHT, 15));
	buttonSizer->Add(mRecordButton, wxSizerFlags());
	mainSizer->Add(buttonSizer, wxSizerFlags().Border(wxALL, 10));

	SetSizer(mainSizer);
	Layout();
}

void DrumMachinePanel::BindEventHandlers()
{
	mColumnCountSpinner->Bind(wxEVT_SPINCTRL, &DrumMachinePanel::OnColumnCountChanged, this);
	mChannelChoice->Bind(wxEVT_CHOICE, &DrumMachinePanel::OnChannelChanged, this);
	mAddRowButton->Bind(wxEVT_BUTTON, &DrumMachinePanel::OnAddRow, this);
	mRemoveRowButton->Bind(wxEVT_BUTTON, &DrumMachinePanel::OnRemoveRow, this);
	mRecordButton->Bind(wxEVT_BUTTON, &DrumMachinePanel::OnRecordToTrack, this);
}

void DrumMachinePanel::RebuildGrid()
{
	// Clear existing grid
	if (mDrumGrid)
	{
		mDrumGrid->Clear(true);
		GetSizer()->Detach(mDrumGrid);
		delete mDrumGrid;
	}
	mPadButtons.clear();

	int columns = mDrumMachine.GetColumnCount();
	int rows = static_cast<int>(mDrumMachine.GetRowCount());

	// Create grid: columns+1 (for row labels), rows+1 (for column numbers)
	mDrumGrid = new wxFlexGridSizer(columns + 1, wxSize(2, 2));

	// Top-left corner empty cell
	mDrumGrid->Add(new wxStaticText(this, wxID_ANY, ""), wxSizerFlags());

	// Column number headers (1, 2, 3, ...)
	for (int col = 0; col < columns; ++col)
	{
		wxStaticText* header = new wxStaticText(this, wxID_ANY,
			wxString::Format("%d", col + 1));
		mDrumGrid->Add(header, wxSizerFlags().Center());
	}

	// For each drum row
	for (size_t row = 0; row < rows; ++row)
	{
		const DrumRow& drumRow = mDrumMachine.GetRow(row);

		// Row label
		wxStaticText* label = new wxStaticText(this, wxID_ANY, drumRow.name);
		mDrumGrid->Add(label, wxSizerFlags().CenterVertical().Border(wxRIGHT, 5));

		// Create pad buttons for each column
		std::vector<wxButton*> rowButtons;
		for (int col = 0; col < columns; ++col)
		{
			wxButton* padButton = new wxButton(this, wxID_ANY, "",
				wxDefaultPosition, wxSize(30, 30));

			// Set color based on enabled state
			if (mDrumMachine.IsPadEnabled(row, col))
			{
				padButton->SetBackgroundColour(wxColour(100, 200, 100));  // Green when enabled
			}
			else
			{
				padButton->SetBackgroundColour(wxColour(80, 80, 80));     // Dark gray when disabled
			}

			// Store row/column indices as client data for event handling
			padButton->SetClientData(reinterpret_cast<void*>((row << 16) | col));
			padButton->Bind(wxEVT_BUTTON, &DrumMachinePanel::OnPadToggle, this);

			rowButtons.push_back(padButton);
			mDrumGrid->Add(padButton, wxSizerFlags().Center());
		}
		mPadButtons.push_back(rowButtons);
	}

	// Re-add to sizer if it exists
	if (GetSizer())
	{
		wxSizer* mainSizer = GetSizer();
		// Insert grid before the button row (which is the last item)
		mainSizer->Insert(1, mDrumGrid, wxSizerFlags(1).Expand().Border(wxLEFT | wxRIGHT | wxBOTTOM, 10));
	}

	Layout();
}

void DrumMachinePanel::OnPadToggle(wxCommandEvent& event)
{
	wxButton* button = static_cast<wxButton*>(event.GetEventObject());

	// Extract row and column from client data
	uintptr_t data = reinterpret_cast<uintptr_t>(button->GetClientData());
	size_t row = (data >> 16) & 0xFFFF;
	size_t col = data & 0xFFFF;

	// Update model
	mDrumMachine.TogglePad(row, col);

	// Update button color based on new state
	bool enabled = mDrumMachine.IsPadEnabled(row, col);
	if (enabled)
	{
		button->SetBackgroundColour(wxColour(100, 200, 100));  // Green when enabled
	}
	else
	{
		button->SetBackgroundColour(wxColour(80, 80, 80));     // Dark gray when disabled
	}
	button->Refresh();  // Force button to redraw with new color
}

void DrumMachinePanel::OnColumnCountChanged(wxSpinEvent& event)
{
	int newCount = mColumnCountSpinner->GetValue();
	mDrumMachine.SetColumnCount(newCount);
	RebuildGrid();
}

void DrumMachinePanel::OnChannelChanged(wxCommandEvent& event)
{
	ubyte channel = static_cast<ubyte>(mChannelChoice->GetSelection());
	mDrumMachine.SetChannel(channel);

	// Update record button label to show new channel
	mRecordButton->SetLabel(wxString::Format("Record to Channel %d", channel + 1));
}

void DrumMachinePanel::OnAddRow(wxCommandEvent& event)
{
	// TODO: Implement add row dialog
	// For now, add a default row
	mDrumMachine.AddRow("New", 60);  // Middle C
	RebuildGrid();
}

void DrumMachinePanel::OnRemoveRow(wxCommandEvent& event)
{
	size_t rowCount = mDrumMachine.GetRowCount();
	if (rowCount > 0)
	{
		mDrumMachine.RemoveRow(rowCount - 1);
		RebuildGrid();
	}
}

void DrumMachinePanel::OnRecordToTrack(wxCommandEvent& event)
{
	mAppModel->RecordDrumPatternToTrack();
}

void DrumMachinePanel::UpdateFromModel()
{
	// Sync UI with loaded pattern
	mColumnCountSpinner->SetValue(mDrumMachine.GetColumnCount());
	mChannelChoice->SetSelection(mDrumMachine.GetChannel());
	mRecordButton->SetLabel(wxString::Format("Record to Channel %d", mDrumMachine.GetChannel() + 1));
	RebuildGrid();
}
