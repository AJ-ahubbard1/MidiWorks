#pragma once
#include <wx/wx.h>
#include <memory>
#include "AppModel/AppModel.h"

/// <summary>
/// Panel that displays the undo and redo command stacks.
/// Shows command descriptions in two lists for visual feedback of history.
/// </summary>
class UndoHistoryPanel : public wxPanel
{
public:
	UndoHistoryPanel(wxWindow* parent, std::shared_ptr<AppModel> appModel)
		: wxPanel(parent, wxID_ANY), mAppModel(appModel)
	{
		// Create main vertical sizer
		wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

		// Undo Stack Section
		wxStaticText* undoLabel = new wxStaticText(this, wxID_ANY, "Undo Stack (Ctrl+Z)");
		mainSizer->Add(undoLabel, 0, wxALL | wxEXPAND, 5);

		mUndoList = new wxListBox(this, wxID_ANY);
		mainSizer->Add(mUndoList, 1, wxALL | wxEXPAND, 5);

		// Redo Stack Section
		wxStaticText* redoLabel = new wxStaticText(this, wxID_ANY, "Redo Stack (Ctrl+Y)");
		mainSizer->Add(redoLabel, 0, wxALL | wxEXPAND, 5);

		mRedoList = new wxListBox(this, wxID_ANY);
		mainSizer->Add(mRedoList, 1, wxALL | wxEXPAND, 5);

		SetSizer(mainSizer);
		UpdateDisplay();
	}

	/// <summary>
	/// Update the display to show current command stacks.
	/// Call this after any command execution, undo, or redo.
	/// </summary>
	void UpdateDisplay()
	{
		// Clear both lists
		mUndoList->Clear();
		mRedoList->Clear();

		// Display undo stack (most recent at bottom)
		const auto& undoStack = mAppModel->GetUndoStack();
		if (undoStack.empty())
		{
			mUndoList->Append("(Empty - No actions to undo)");
		}
		else
		{
			for (const auto& cmd : undoStack)
			{
				mUndoList->Append(cmd->GetDescription());
			}
		}

		// Display redo stack (most recent at bottom)
		const auto& redoStack = mAppModel->GetRedoStack();
		if (redoStack.empty())
		{
			mRedoList->Append("(Empty - No actions to redo)");
		}
		else
		{
			for (const auto& cmd : redoStack)
			{
				mRedoList->Append(cmd->GetDescription());
			}
		}
	}

private:
	std::shared_ptr<AppModel> mAppModel;
	wxListBox* mUndoList;
	wxListBox* mRedoList;
};
