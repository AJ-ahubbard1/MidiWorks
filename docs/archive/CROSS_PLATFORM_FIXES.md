# Making MidiWorks Cross-Platform

This guide provides step-by-step instructions to fix Linux/Ubuntu build errors while **maintaining full Windows compatibility**. All changes are additive and improve code standards without breaking existing Windows builds.

## Important Note

**These changes will NOT break your Windows build.** They add missing standard library includes that should have been there all along. MSVC (Windows compiler) is lenient and includes headers implicitly, while GCC (Linux compiler) is strict and requires explicit includes. Adding these includes makes the code more correct and portable.

---

## Issue Summary

Based on the Ubuntu build log, there are 4 main categories of errors:

1. **Missing `<cstdint>` includes** - For `uint64_t`, `uint8_t` types
2. **Missing `<memory>` includes** - For `std::shared_ptr`, `std::make_shared`
3. **TimedMidiEvent struct member access** - Accessing `.tick` member that doesn't exist
4. **wxWidgets API differences** - wxAuiPaneInfoArray iteration syntax

---

## Fix 1: Add Missing `<cstdint>` Header

### Files to Fix:
- `src/MidiConstants.h`
- `src/AppModel/TrackSet/TrackSet.h`
- `src/Commands/NoteEditCommands.h`

### Why This Fix is Safe:
The `<cstdint>` header is part of the C++11 standard and works on all platforms. MSVC includes it implicitly, but GCC requires it explicitly. Adding this include does not break Windows builds.

---

### Fix 1a: `src/MidiConstants.h`

**Location:** Line 1 (after `#pragma once`)

**Add this include:**
```cpp
#pragma once
#include <cstdint>  // ADD THIS LINE

namespace MidiInterface
{
    // ... rest of file
```

**Why:** This file uses `uint64_t` at line 25 but doesn't include the header that defines it.

---

### Fix 1b: `src/AppModel/TrackSet/TrackSet.h`

**Location:** After existing includes, before line 7

**Current code (around line 1-7):**
```cpp
#pragma once

#include "RtMidiWrapper/RtMidiWrapper.h"
#include "MidiConstants.h"
#include <vector>
#include <algorithm>
using namespace MidiInterface;
```

**Change to:**
```cpp
#pragma once

#include "RtMidiWrapper/RtMidiWrapper.h"
#include "MidiConstants.h"
#include <vector>
#include <algorithm>
#include <cstdint>  // ADD THIS LINE
using namespace MidiInterface;
```

**Why:** This file defines structs using `uint64_t` but doesn't include the header.

---

### Fix 1c: `src/Commands/NoteEditCommands.h`

**Location:** After existing includes, before line 4

**Current code (around line 1-5):**
```cpp
#pragma once
#include "Command.h"
#include "AppModel/TrackSet/TrackSet.h"
#include <algorithm>
```

**Change to:**
```cpp
#pragma once
#include "Command.h"
#include "AppModel/TrackSet/TrackSet.h"
#include <algorithm>
#include <cstdint>  // ADD THIS LINE
```

**Why:** This file uses `uint64_t` and `uint8_t` in class member variables and function parameters.

---

## Fix 2: Add Missing `<memory>` Header

### Files to Fix:
- `src/AppModel/SoundBank/SoundBank.h`

### Why This Fix is Safe:
The `<memory>` header is part of the C++11 standard and works on all platforms. Adding this include does not break Windows builds.

---

### Fix 2a: `src/AppModel/SoundBank/SoundBank.h`

**Location:** After existing includes, before line 5

**Current code (around line 1-6):**
```cpp
#pragma once

#include "RtMidiWrapper/RtMidiWrapper.h"
#include "MidiConstants.h"
using namespace MidiInterface;
```

**Change to:**
```cpp
#pragma once

#include "RtMidiWrapper/RtMidiWrapper.h"
#include "MidiConstants.h"
#include <memory>  // ADD THIS LINE
using namespace MidiInterface;
```

**Why:** This file uses `std::shared_ptr` and the implementation file uses `std::make_shared`, but the header isn't included.

---

## Fix 3: TimedMidiEvent Member Access Issue

### Problem:
The build log shows errors like:
```
error: 'const struct TimedMidiEvent' has no member named 'tick'
```

### Root Cause:
**This is NOT a separate issue!** The `TimedMidiEvent` struct (defined in `src/AppModel/TrackSet/TrackSet.h` at line 9) DOES have a `tick` member at line 12:

```cpp
struct TimedMidiEvent
{
    MidiMessage mm;
    uint64_t    tick = 0;  // Line 12
};
```

However, because `uint64_t` is not defined (missing `<cstdint>` include), the compiler can't parse line 12, so it effectively skips the `tick` member definition. This causes all the `.tick` access errors throughout the codebase.

### Solution:
**Fix #1b will solve this automatically!** When you add `#include <cstdint>` to `TrackSet.h`, the compiler will be able to parse the `uint64_t tick` member, and all the `.tick` access errors will disappear.

**No additional code changes needed** - this is a cascading error from the missing include.

---

## Fix 4: wxAuiPaneInfoArray Iteration

### File to Fix:
- `src/MainFrame/MainFrame.cpp`

### Why This Fix is Needed:
On Linux, `wxAuiPaneInfoArray` cannot be used with range-based for loops the same way as on Windows. The array returns pointers, not references.

---

### Fix 4a: `src/MainFrame/MainFrame.cpp`

**Location:** Line 231 in `OnAuiRender()` function

**Current code:**
```cpp
void MainFrame::OnAuiRender(wxAuiManagerEvent& evt)
{
    wxString msg = "Pane Sizes: ";
    for (const auto& pane : mAuiManager.GetAllPanes())
    {
        wxSize sz = pane.window->GetSize();
        msg += wxString::Format("|%s: %d x %d", pane.name, sz.GetWidth(), sz.GetHeight());
    }
    // ...
}
```

**Change to:**
```cpp
void MainFrame::OnAuiRender(wxAuiManagerEvent& evt)
{
    wxString msg = "Pane Sizes: ";
    wxAuiPaneInfoArray& panes = mAuiManager.GetAllPanes();
    for (size_t i = 0; i < panes.GetCount(); i++)
    {
        wxAuiPaneInfo& pane = panes.Item(i);
        wxSize sz = pane.window->GetSize();
        msg += wxString::Format("|%s: %d x %d", pane.name, sz.GetWidth(), sz.GetHeight());
    }
    // ...
}
```

**Why:** On Linux wxWidgets 3.2, `wxAuiPaneInfoArray` uses the older `Item()` accessor pattern instead of being directly iterable. This syntax works on both Windows and Linux.

**Windows Compatibility:** This change works perfectly on Windows too - it's the traditional wxWidgets array access pattern.

---

## Testing Strategy

### After Each Fix:
1. **Test on Windows first** - Make sure Visual Studio still builds cleanly
2. Commit the change with a descriptive message
3. Test on Ubuntu

### Recommended Order:
1. Fix all missing includes first (Fixes 1 & 2)
   - These are safe, standard, and guaranteed not to break anything
2. Test Windows build
3. Fix wxAui iteration (Fix 4)
   - Also safe and cross-platform compatible
4. Test Windows build again
5. Investigate and fix TimedMidiEvent issue (Fix 3)
   - This might be a real bug that needs careful testing

---

## Verification Checklist

After all fixes:

### Windows (Visual Studio):
- [ ] Debug build compiles without errors
- [ ] Release build compiles without errors
- [ ] Application runs and loads correctly
- [ ] All features work (piano roll, playback, recording, etc.)

### Ubuntu (CMake):
- [ ] `cmake ..` configures successfully
- [ ] `make` compiles without errors
- [ ] Application launches (if GUI/MIDI available)
- [ ] Basic functionality works

---

## Additional Notes

### Why MSVC Hides These Issues:
- **Implicit includes**: MSVC includes many standard headers transitively
- **Lenient parsing**: MSVC is more forgiving of certain syntax
- **Precompiled headers**: May pull in headers automatically

### Why GCC Exposes These Issues:
- **Strict compliance**: Requires explicit includes per C++ standard
- **Minimal includes**: Only includes what you explicitly request
- **Better standards compliance**: Follows C++ specification more closely

**The GCC behavior is actually more correct** - code should explicitly include what it uses. These fixes make the code more portable and standards-compliant.

---

## Summary

All fixes are **additive** and **safe** for Windows:
- ✅ Adding `#include <cstdint>` to 3 files - Standard C++11, works everywhere
- ✅ Adding `#include <memory>` to 1 file - Standard C++11, works everywhere
- ✅ Fixing wxAui iteration in 1 file - Uses traditional API, works on all platforms
- ✅ TimedMidiEvent errors - Automatically fixed by the `<cstdint>` include (not a separate issue)

**You can make these changes with confidence** - your Windows build will continue to work perfectly.

---

## Quick Fix Checklist

If you want to tackle this systematically, here's the minimal set of changes:

### Step 1: Add `#include <cstdint>` to 3 files
1. `src/MidiConstants.h` - Add after line 1 (`#pragma once`)
2. `src/AppModel/TrackSet/TrackSet.h` - Add after line 6 (after existing includes)
3. `src/Commands/NoteEditCommands.h` - Add after line 4 (after existing includes)

### Step 2: Add `#include <memory>` to 1 file
1. `src/AppModel/SoundBank/SoundBank.h` - Add after line 4 (after existing includes)

### Step 3: Fix wxAui iteration in 1 file
1. `src/MainFrame/MainFrame.cpp` - Replace range-based for loop at line 231 with traditional iteration (see Fix 4a above)

**That's it!** Just 5 one-line additions and 1 loop change. The build log makes it look like hundreds of errors, but they're all cascading from these few missing includes.
