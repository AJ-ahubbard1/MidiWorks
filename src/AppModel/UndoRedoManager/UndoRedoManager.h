// UndoRedoManager.h
#pragma once
#include <vector>
#include <memory>
#include <functional>
#include "Commands/Command.h"

/// UndoRedoManager handles command execution and undo/redo history.
///
/// Responsibilities:
/// - Execute commands and add to undo stack
/// - Undo/redo operations
/// - Manage undo/redo stack limits
/// - Notify when commands are executed (for dirty state tracking)
///
/// Usage:
///   UndoRedoManager undoManager;
///   undoManager.SetCommandExecutedCallback([&]() {
///       projectManager.MarkDirty();
///   });
///   undoManager.ExecuteCommand(std::make_unique<MyCommand>());
///   undoManager.Undo();
///   undoManager.Redo();
class UndoRedoManager
{
public:
	UndoRedoManager() { }

	/// Execute a command and add it to the undo stack.
	/// Clears redo stack, calls command executed callback, enforces stack size limit.
	void ExecuteCommand(std::unique_ptr<Command> cmd);

	/// Undo the last executed command (moves to redo stack)
	void Undo();

	/// Redo the last undone command (moves back to undo stack)
	void Redo();

	/// Check if undo is available
	bool CanUndo() const { return !mUndoStack.empty(); }

	/// Check if redo is available
	bool CanRedo() const { return !mRedoStack.empty(); }

	/// Get reference to undo stack (for UI display)
	const std::vector<std::unique_ptr<Command>>& GetUndoStack() const { return mUndoStack; }

	/// Get reference to redo stack (for UI display)
	const std::vector<std::unique_ptr<Command>>& GetRedoStack() const { return mRedoStack; }

	/// Clear both undo and redo stacks (called when loading/creating project)
	void ClearHistory();

	// Callbacks

	/// Callback signature for command execution notification
	using CommandExecutedCallback = std::function<void()>;

	/// Set callback to be notified when a command is executed (for dirty state tracking)
	void SetCommandExecutedCallback(CommandExecutedCallback callback) { mCommandExecutedCallback = callback; }

private:
	static const size_t MAX_UNDO_STACK_SIZE = 50;  // Limit to last 50 actions
	std::vector<std::unique_ptr<Command>> mUndoStack;
	std::vector<std::unique_ptr<Command>> mRedoStack;
	CommandExecutedCallback mCommandExecutedCallback;
};
