// UndoRedoManager.cpp
#include "UndoRedoManager.h"

void UndoRedoManager::ExecuteCommand(std::unique_ptr<Command> cmd)
{
	cmd->Execute();
	mUndoStack.push_back(std::move(cmd));

	// Clear redo stack - can't redo after new action
	mRedoStack.clear();

	// Notify that a command was executed (for dirty state tracking)
	if (mCommandExecutedCallback)
	{
		mCommandExecutedCallback();
	}

	// Limit stack size to prevent unbounded memory growth
	if (mUndoStack.size() > MAX_UNDO_STACK_SIZE)
	{
		mUndoStack.erase(mUndoStack.begin());
	}
}

void UndoRedoManager::Undo()
{
	if (mUndoStack.empty()) return;

	// Get command from undo stack
	auto cmd = std::move(mUndoStack.back());
	mUndoStack.pop_back();

	// Undo the command
	cmd->Undo();

	// Move to redo stack
	mRedoStack.push_back(std::move(cmd));
}

void UndoRedoManager::Redo()
{
	if (mRedoStack.empty()) return;

	// Get command from redo stack
	auto cmd = std::move(mRedoStack.back());
	mRedoStack.pop_back();

	// Re-execute the command
	cmd->Execute();

	// Move back to undo stack
	mUndoStack.push_back(std::move(cmd));
}

void UndoRedoManager::ClearHistory()
{
	mUndoStack.clear();
	mRedoStack.clear();
}
