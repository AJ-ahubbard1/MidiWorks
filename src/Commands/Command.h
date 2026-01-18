// Command.h
#pragma once
#include <string>

/// Abstract base class for the Command pattern (undo/redo system).
///
/// Responsibilities:
/// - Encapsulate user actions (add, delete, move notes) as executable objects
/// - Store parameters needed to execute and undo actions
/// - Provide description for UI display
///
/// Each concrete command stores:
/// - References to the data it modifies (e.g., Track)
/// - Parameters needed to execute the action (e.g., note pitch, tick)
/// - State needed to reverse the action (e.g., original position)
///
/// Usage:
///   class MyCommand : public Command {
///       void Execute() override { /* perform action */ }
///       void Undo() override { /* reverse action */ }
///       std::string GetDescription() const override { return "My Action"; }
///   };
class Command
{
public:
	virtual ~Command() = default;

	/// Execute the command (called on action or Redo)
	virtual void Execute() = 0;

	/// Undo the command (called on Ctrl+Z)
	virtual void Undo() = 0;

	/// Get human-readable description for UI display
	virtual std::string GetDescription() const = 0;
};
