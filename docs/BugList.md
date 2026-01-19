# Bug List

Track bugs and issues discovered during testing of MidiWorks.

## Status Legend
- **Open** - Bug identified, not yet fixed
- **In Progress** - Currently being worked on
- **Fixed** - Bug has been resolved
- **Won't Fix** - Intentional behavior or low priority

---

## Bugs

---

### #39 - Loop region editing doesn't respect time signatures with non-4 denominators
**Status:** Open
**Priority:** Medium
**Found:** 2026-01-12

**Description:**
When editing the loop region (dragging loop start/end boundaries), the system doesn't correctly calculate measure boundaries for time signatures with denominators other than 4. This causes loop regions to snap to incorrect positions when the time signature is set to values like 3/8, 6/8, 2/2, or 3/4.

**Problem Examples:**
- **4/4 time** - Works correctly (denominator = 4)
- **3/8 time** - Loop boundaries don't align with actual measure boundaries
- **6/8 time** - Incorrect measure length calculation
- **2/2 time** - Measure duration miscalculated

**Root Cause:**
The measure calculation logic likely assumes a quarter note (denominator = 4) as the beat unit and doesn't account for the actual time signature denominator when calculating ticks per measure.

**Expected Behavior:**
Loop region boundaries should snap to measure lines that respect the current time signature, correctly calculating measure duration based on both the numerator and denominator:
- Measure duration = (numerator / denominator) * 4 * quarter note ticks
- Example for 3/8: (3/8) * 4 * 960 = 1440 ticks per measure
- Example for 6/8: (6/8) * 4 * 960 = 2880 ticks per measure

**Files to Investigate:**
- `src/AppModel/Transport/Transport.h` - `GetTicksPerMeasure()` method
- Loop region drag handlers in MidiCanvas
- Any code that calculates measure boundaries for UI snapping

**Notes:**
This affects the user experience when working with compound meters or time signatures commonly used in classical, jazz, or progressive music. The fix should ensure measure calculations use the time signature denominator in addition to the numerator.

---

---

### #37 - Linux GTK3 widget sizing issues causing missing button/control text
**Status:** Open
**Priority:** Medium
**Found:** 2026-01-10

**Description:**
When running MidiWorks on Linux (Ubuntu/GTK3), certain UI controls have sizing issues that cause text and content to disappear. The application is functionally working, but the visual issues impact usability.

**Affected Controls:**

1. **Buttons - Negative content width**
   - Buttons allocated 20px but need 34px (17px padding on each side + content)
   - Results in -14px for content width, causing button text to disappear
   - Warning: `Gtk-WARNING **: Negative content width -14 (allocation 20, extents 17x17) while allocating gadget (node button, owner GtkButton)`

2. **SpinButtons - Minimum size violation**
   - Spin controls need minimum 32px width but only getting 18px
   - Causes assertion failure: `gtk_box_gadget_distribute: assertion 'size >= 0' failed in GtkSpinButton`
   - Warning: `Gtk-WARNING **: for_size smaller than min-size (18 < 32) while measuring gadget (node entry, owner GtkSpinButton)`

**Root Cause:**
GTK3 has stricter widget sizing constraints than Windows. wxWidgets uses GTK3 as its backend on Linux, and the widget sizing constraints need to be explicitly set to accommodate GTK3's requirements. Controls that auto-size correctly on Windows don't meet GTK3's minimum size requirements.

**Expected Behavior:**
All buttons and spin controls should display their text/content properly on Linux, matching the Windows appearance.

**Proposed Solutions:**

1. **Add explicit minimum sizes to buttons:**
   ```cpp
   button->SetMinSize(wxSize(40, -1));  // Ensure buttons have enough width
   ```

2. **Add explicit minimum sizes to spin controls:**
   ```cpp
   spinCtrl->SetMinSize(wxSize(50, -1));  // GTK3 needs minimum 32px + some margin
   ```

3. **Review parent container sizing:**
   - Check if toolbar or sizer is constraining child widget sizes
   - May need to adjust sizer flags or proportions

**Files to Investigate:**
- Button creation locations across all panels
- SpinButton/SpinCtrl creation (TransportPanel, DrumMachinePanel, etc.)
- Toolbar layout code
- Panel sizer configurations

**Notes:**
- Application is functionally working on Linux - this is purely a visual/UX issue
- Similar issues may exist with other wxWidgets controls
- Need to test fix on both Windows and Linux to ensure cross-platform compatibility
- May want to create a helper function for setting platform-specific minimum sizes

**Related:**
- See `linux-errors.log` for full error output

---

### #31 - Turn off playhead autoscroll when loop is enabled
**Status:** Open
**Priority:** Medium
**Found:** 2026-01-05

**Description:**
During loop playback, the playhead autoscroll causes the view to constantly jump back to the loop start position. This creates a distracting visual experience as the content repeatedly scrolls forward then jumps back.

**Expected Behavior:**
When loop is enabled, the canvas should remain stationary (no autoscroll) and let the playhead move across the visible loop region. This would provide a stable, predictable view during loop playback.

**Notes:**
The current fixed playhead autoscroll (implemented in bug #1) works well for linear playback but becomes problematic during looping. Consider adding a setting or automatically disabling autoscroll when loop mode is active.

---

### #32 - Need faster way to scroll/move loop region
**Status:** Open
**Priority:** Medium
**Found:** 2026-01-05

**Description:**
Currently, adjusting the loop region over long distances requires dragging the start and end boundaries individually, which is tedious for large compositions. Users need a faster workflow to reposition the entire loop region.

**Proposed Solutions:**
1. **Drag entire loop region** - Click and drag the middle of the loop overlay to move both start and end together
2. **Keyboard shortcuts** - Shift+Arrow keys to move loop region by measures
3. **Numeric input** - Dialog to type exact tick/measure values for loop boundaries
4. **Selection to loop** - Right-click on selection → "Set as loop region"

**Notes:**
Option 1 (drag entire region) would be the most intuitive and matches DAW standards. Options could be combined for best workflow.

---

### #33 - Show record-enabled status when channel is minimized
**Status:** Open
**Priority:** Low
**Found:** 2026-01-05

**Description:**
When a channel is minimized in the mixer (using the +/- button from bug #10), there's no visual indication of whether the channel is record-enabled. Users must expand the channel to check the record checkbox state.

**Expected Behavior:**
Minimized channels should show a visual indicator (red dot, icon, background color change, etc.) when record is enabled, allowing users to see recording routing at a glance without expanding all channels.

**Proposed Solutions:**
1. **Red border/outline** around minimized channel when record enabled
2. **Record icon (●)** next to the +/- button when record enabled
3. **Background color tint** (subtle red) for record-enabled minimized channels
4. **Status text** - Show "[R]" or "REC" next to channel name when minimized

**Notes:**
This complements the minimize feature (bug #10) by maintaining full functionality when channels are collapsed. Low priority but improves workflow efficiency.

---

### #25 - DrumMachine plays even when panel is hidden
**Status:** Won't Fix
**Priority:** N/A
**Found:** 2025-12-28
**Resolved:** 2025-12-29

**Description:**
The DrumMachine pattern plays back during loop playback even when the DrumMachine panel is not visible.

**Resolution:**
This is **intentional behavior** that matches professional DAW standards. Instruments and patterns should play regardless of UI panel visibility - this is the expected workflow in all major DAWs (Ableton, FL Studio, Logic, etc.).

**Proper Solution:**
Use the **mute button** (implemented in bug #24 Option 5) to control drum machine playback. This provides:
- Standard DAW workflow (mute/solo controls audio routing, not UI visibility)
- Ability to edit patterns while muted
- Clear separation between UI state and audio routing
- Familiar behavior for users coming from other DAWs

**Why Panel Visibility Should NOT Control Playback:**
- Users may want to hear drums while working in other panels (piano roll, mixer, etc.)
- Closing a panel to silence it is non-standard and confusing
- CPU usage from drum machine playback is negligible
- Mute button provides explicit, discoverable control

**Related:**
- Bug #24 Option 5 - Mute/Solo buttons (provides the proper playback control mechanism)

---


## Template (Copy for new bugs)

```markdown
### #N - [Bug Title]
**Status:** Open
**Priority:** High/Medium/Low
**Found:** YYYY-MM-DD

**Description:**


**Steps to Reproduce:**
1.

**Expected Behavior:**


**Actual Behavior:**


**Notes:**

```
