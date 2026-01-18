# Guide: Creating Callback Functions in MidiWorks

## When to Use Callbacks

Use callbacks when:
- A component needs to **notify** another component about events 
 	e.g., "project became dirty", "command executed"
- You want to **avoid tight coupling** - the component shouldn't know who needs the notification
- The component is **reusable** and different users might need different behavior

**Examples from MidiWorks:**
- `ProjectManager` notifies when project becomes dirty → UI updates title bar
- `UndoRedoManager` notifies when command executes → ProjectManager marks dirty
- `Transport` notifies when loop changes → UI updates drum machine grid

## Implementation Pattern

### 1. In the Header (.h)

```cpp
class MyComponent
{
public:
    // 1. Define callback signature using std::function
    using MyEventCallback = std::function<void(int someData)>;

    // 2. Provide setter for registering the callback
    void SetMyEventCallback(MyEventCallback callback);

    // Other methods...
    void DoSomething();

private:
    // 3. Store the callback as a member variable
    MyEventCallback mMyEventCallback;
};
```

**Real example from UndoRedoManager.h:86-95:**
```cpp
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
```

### 2. In the Implementation (.cpp)

```cpp
// Setter implementation - just store the callback
void MyComponent::SetMyEventCallback(MyEventCallback callback)
{
    mMyEventCallback = callback;
}

// Call the callback when event occurs
void MyComponent::DoSomething()
{
    // ... do work ...

    // 4. Check if callback is set before calling (optional but safe)
    if (mMyEventCallback)
    {
        mMyEventCallback(42);  // Pass data to callback
    }
}
```

**Real example from UndoRedoManager.cpp:85-88, 16-20:**
```cpp
void UndoRedoManager::SetCommandExecutedCallback(CommandExecutedCallback callback)
{
    mCommandExecutedCallback = callback;
}

void UndoRedoManager::ExecuteCommand(std::unique_ptr<Command> cmd)
{
    cmd->Execute();
    mUndoStack.push_back(std::move(cmd));

    // Notify that a command was executed (for dirty state tracking)
    if (mCommandExecutedCallback)
    {
        mCommandExecutedCallback();
    }

    // ... rest of method ...
}
```

### 3. Registering the Callback

**In AppModel (coordinating components):**
```cpp
// AppModel.cpp:24-27
mUndoRedoManager.SetCommandExecutedCallback([this]() {
    mProjectManager.MarkDirty();
});
```

**In UI code (MainFrame):**
```cpp
// MainFrame.cpp:79-82
mAppModel->GetProjectManager().SetDirtyStateCallback([this](bool isDirty)
{
    UpdateTitle();
});
```

## Common Callback Signatures

```cpp
// No parameters - just notification
using EventCallback = std::function<void()>;

// Single parameter - pass data
using DataCallback = std::function<void(bool isDirty)>;

// Multiple parameters
using LogCallback = std::function<void(const TimedMidiEvent& event)>;

// Return value (less common for callbacks)
using ValidationCallback = std::function<bool(int value)>;
```

## Best Practices

1. **Always check if callback is set** before calling (prevents crashes):
   ```cpp
   if (mMyCallback) {
       mMyCallback();
   }
   ```

2. **Use clear, descriptive names**:
   - ✅ `SetDirtyStateCallback`
   - ✅ `SetCommandExecutedCallback`
   - ❌ `SetCallback` (too vague)

3. **Document the callback purpose** in header comments

4. **Put callbacks in the right place**:
   - Component → UI: Register in MainFrame
   - Component → Component: Wire in AppModel (the coordinator)

5. **Don't create pass-through callbacks** (like the one we removed from AppModel's DirtyStateCallback) - if the callback just forwards to another callback with no coordination logic, it's probably unnecessary

## Complete Example

```cpp
// MyComponent.h
class MyComponent
{
public:
    using ValueChangedCallback = std::function<void(int newValue)>;
    void SetValueChangedCallback(ValueChangedCallback callback);
    void SetValue(int value);

private:
    int mValue = 0;
    ValueChangedCallback mValueChangedCallback;
};

// MyComponent.cpp
void MyComponent::SetValueChangedCallback(ValueChangedCallback callback)
{
    mValueChangedCallback = callback;
}

void MyComponent::SetValue(int value)
{
    if (mValue != value)
    {
        mValue = value;

        // Notify listeners of change
        if (mValueChangedCallback)
        {
            mValueChangedCallback(mValue);
        }
    }
}

// Usage in AppModel or MainFrame
myComponent.SetValueChangedCallback([this](int newValue) {
    std::cout << "Value changed to: " << newValue << std::endl;
    UpdateUI();
});
```

## Architecture Decision: Where Callbacks Live

### ✅ Good - Callbacks in AppModel (Component Coordination)

When callbacks wire together multiple AppModel components:

```cpp
// AppModel.cpp - Coordinating ProjectManager and UndoRedoManager
mProjectManager.SetClearUndoHistoryCallback([this]() {
    mUndoRedoManager.ClearHistory();
});

mUndoRedoManager.SetCommandExecutedCallback([this]() {
    mProjectManager.MarkDirty();
});
```

**Why this is good:**
- AppModel owns both components
- Creates bidirectional communication between components
- Contains coordination logic, not just forwarding

### ✅ Good - Callbacks in UI (MainFrame)

When callbacks notify the UI about model changes:

```cpp
// MainFrame.cpp - UI responding to model events
mAppModel->GetProjectManager().SetDirtyStateCallback([this](bool isDirty) {
    UpdateTitle();
});
```

**Why this is good:**
- Direct connection from model to UI
- No unnecessary intermediate layers
- UI code stays in UI layer

### ❌ Bad - Pass-Through Callbacks

When a callback just forwards with no coordination:

```cpp
// BAD EXAMPLE (removed from MidiWorks)
// AppModel forwarding ProjectManager's callback to UI
mProjectManager.SetDirtyStateCallback([this](bool dirty) {
    if (mDirtyStateCallback) {
        mDirtyStateCallback(dirty);  // Just forwarding, no coordination!
    }
});
```

**Why this is bad:**
- Extra layer with no added value
- Violates YAGNI (You Ain't Gonna Need It)
- Makes the call chain harder to follow
- More code to maintain

**Better approach:** Let UI register directly with the component that generates the event.
