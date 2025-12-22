// UndoRedoManager.h
#pragma once
#include <vector>
#include <memory>
#include <functional>
#include "Commands/Command.h"

/**
 * UndoRedoManager handles command execution and undo/redo history.
 *
 * Responsibilities:
 * - Execute commands and add to undo stack
 * - Undo/redo operations
 * - Manage undo/redo stack limits
 * - Notify when commands are executed (for dirty state tracking)
 *
 * Usage:
 *   UndoRedoManager undoManager;
 *   undoManager.SetCommandExecutedCallback([&]() {
 *       projectManager.MarkDirty();
 *   });
 *
 *   undoManager.ExecuteCommand(std::make_unique<MyCommand>());
 *   undoManager.Undo();
 *   undoManager.Redo();
 */
class UndoRedoManager
{
public:
	UndoRedoManager();

	/**
	 * Execute a command and add it to the undo stack
	 * - Executes the command immediately
	 * - Adds to undo stack
	 * - Clears redo stack (can't redo after new action)
	 * - Calls command executed callback
	 * - Enforces MAX_UNDO_STACK_SIZE limit
	 * @param cmd Command to execute
	 */
	void ExecuteCommand(std::unique_ptr<Command> cmd);

	/**
	 * Undo the last executed command
	 * Moves command from undo stack to redo stack
	 */
	void Undo();

	/**
	 * Redo the last undone command
	 * Moves command from redo stack to undo stack
	 */
	void Redo();

	/**
	 * Check if undo is available
	 * @return true if undo stack is not empty
	 */
	bool CanUndo() const;

	/**
	 * Check if redo is available
	 * @return true if redo stack is not empty
	 */
	bool CanRedo() const;

	/**
	 * Get reference to undo stack (for UI display)
	 * @return const reference to undo stack
	 */
	const std::vector<std::unique_ptr<Command>>& GetUndoStack() const;

	/**
	 * Get reference to redo stack (for UI display)
	 * @return const reference to redo stack
	 */
	const std::vector<std::unique_ptr<Command>>& GetRedoStack() const;

	/**
	 * Clear both undo and redo stacks
	 * Called when loading a project or creating a new one
	 */
	void ClearHistory();

	/**
	 * Callback signature for command execution notification
	 */
	using CommandExecutedCallback = std::function<void()>;

	/**
	 * Set callback to be notified when a command is executed
	 * Used to mark project as dirty when edits occur
	 * @param callback Function to call after command execution
	 */
	void SetCommandExecutedCallback(CommandExecutedCallback callback);

private:
	static const size_t MAX_UNDO_STACK_SIZE = 50;  // Limit to last 50 actions
	std::vector<std::unique_ptr<Command>> mUndoStack;
	std::vector<std::unique_ptr<Command>> mRedoStack;
	CommandExecutedCallback mCommandExecutedCallback;
};
