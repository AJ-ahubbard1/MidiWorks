#pragma once
#include <string>

/// <summary>
/// Abstract base class for the Command pattern.
///
/// Commands encapsulate user actions (like adding, deleting, moving notes) as objects
/// that can be executed and undone. This enables the undo/redo functionality.
///
/// Each concrete command stores:
/// - References to the data it modifies (e.g., Track)
/// - Parameters needed to execute the action (e.g., note pitch, tick)
/// - State needed to reverse the action (e.g., original position)
/// </summary>
class Command
{
public:
	virtual ~Command() = default;

	/// <summary>
	/// Execute the command - perform the action (add note, delete note, etc.)
	/// Called when the user performs an action or presses Redo (Ctrl+Y)
	/// </summary>
	virtual void Execute() = 0;

	/// <summary>
	/// Undo the command - reverse the action
	/// Called when the user presses Undo (Ctrl+Z)
	/// </summary>
	virtual void Undo() = 0;

	/// <summary>
	/// Get a human-readable description of this command
	/// Used for displaying command history in the UI
	/// </summary>
	virtual std::string GetDescription() const = 0;
};
