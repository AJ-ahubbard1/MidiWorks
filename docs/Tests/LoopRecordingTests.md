# Loop Recording Tests

## Overview

Comprehensive testing plan for loop recording fixes implemented in bug #8 and bug #23.

**Fixes Covered:**
1. Notes held across loop boundaries create proper pairs (bug #23)
2. Mid-loop stop properly closes active notes (bug #8 issue #3)
3. No orphaned Note On/Off events
4. Buffer iterator remains safe during playback (bug #8 issue #2)

---

## Test Environment Setup

### Build Configuration
- **Platform:** Windows x64
- **Configuration:** Debug (for better error reporting)
- **Build Tool:** MSBuild via Visual Studio 2022

### Required Setup
1. MIDI keyboard connected and configured as input device
2. MIDI output device selected (can be virtual or physical)
3. Loop mode enabled
4. Default loop settings: start=0, end=3840 (one measure in 4/4 time)

---

## Test Cases

### Test 1: Notes Held Across Loop Boundary (Primary Bug #23 Fix)
**Purpose:** Verify WrapActiveNotesAtLoop creates proper note pairs

**Steps:**
1. Enable loop recording (loop start=0, end=3840)
2. Start recording
3. Press C4 at tick ~100 and HOLD through loop boundary
4. Release C4 at tick ~200 (in second loop iteration)
5. Stop recording
6. Enable "Show MIDI Events" debug mode
7. Inspect recorded events in recording buffer

**Expected Results:**
- ✓ Iteration 1: Note On (C4) at ~tick 100 → Note Off at ~tick 3830
- ✓ Iteration 2: Note On (C4) at tick 0 → Note Off at ~tick 200
- ✓ All events show correct type ("Note On" or "Note Off", not "Other")
- ✓ Velocity preserved from original Note On
- ✓ No orphaned events

**Before Fix:**
- ✗ Orphaned Note Off at tick 200 (no matching Note On)

**Status:** ⬜ Not Tested

---

### Test 2: Multiple Notes Held Across Boundary
**Purpose:** Verify simultaneous held notes all wrap correctly

**Steps:**
1. Enable loop recording (loop start=0, end=3840)
2. Start recording
3. Press and HOLD C4, E4, G4 (C major chord) at tick ~100
4. Keep holding through loop boundary
5. Release all notes at tick ~300 (in second iteration)
6. Stop recording
7. Inspect all three note tracks

**Expected Results:**
- ✓ Each note has proper pairs in both iterations
- ✓ All three notes close at ~tick 3830 (iteration 1)
- ✓ All three notes reopen at tick 0 (iteration 2)
- ✓ All three notes close at ~tick 300 (iteration 2)
- ✓ No orphaned events for any note

**Status:** ⬜ Not Tested

---

### Test 3: Mid-Loop Stop with Active Notes (Bug #8 Issue #3 Fix)
**Purpose:** Verify CloseAllActiveNotes is called when stopping mid-loop

**Steps:**
1. Enable loop recording
2. Start recording
3. Press and HOLD C4 at tick ~500
4. STOP recording while still holding C4 (before loop wraps)
5. Release C4 after recording stopped
6. Inspect recording buffer

**Expected Results:**
- ✓ Note On (C4) at ~tick 500
- ✓ Note Off (C4) at current tick when stop button pressed
- ✓ No orphaned Note On
- ✓ Recording buffer is valid

**Before Fix:**
- ✗ Note On exists without matching Note Off
- ✗ Orphaned Note On causes playback issues

**Status:** ⬜ Not Tested

---

### Test 4: Quick Press-Release Within Loop
**Purpose:** Verify normal recording behavior unchanged

**Steps:**
1. Enable loop recording
2. Start recording
3. Quick tap C4 (press and release within same frame, ~10ms)
4. Wait for loop to wrap
5. Quick tap E4 in second iteration
6. Stop recording
7. Inspect buffer

**Expected Results:**
- ✓ Proper note pairs for both notes
- ✓ No duplicate events at loop boundary
- ✓ Loop playback sounds correct

**Status:** ⬜ Not Tested

---

### Test 5: Overlapping Notes Separation
**Purpose:** Verify SeparateOverlappingNotes runs before WrapActiveNotesAtLoop

**Steps:**
1. Enable loop recording
2. Start recording
3. Press C4 at tick ~100, hold through loop
4. During second iteration, press C4 again at tick ~500 (while first C4 still held from wrap)
5. Release first C4 at tick ~600
6. Release second C4 at tick ~800
7. Stop recording
8. Inspect buffer for proper separation

**Expected Results:**
- ✓ First note: tick 0 → tick ~600 (10 ticks gap before next note)
- ✓ Second note: tick ~500 → tick ~800
- ✓ NOTE_SEPARATION_TICKS (10 ticks) gap maintained
- ✓ No merged or overlapping notes

**Status:** ⬜ Not Tested

---

### Test 6: Buffer Iterator Safety (Bug #8 Issue #2 Verification)
**Purpose:** Verify no crashes or out-of-bounds access during playback

**Steps:**
1. Enable loop recording
2. Record several notes in first iteration
3. On second iteration, record MORE notes while previous notes play back
4. Continue for 5+ loop iterations, constantly adding notes
5. Monitor for crashes or audio glitches
6. Stop recording

**Expected Results:**
- ✓ No crashes
- ✓ No out-of-bounds errors
- ✓ All notes play back correctly
- ✓ Smooth playback throughout

**Verification Points:**
- GetLoopPlaybackMessages bounds checking (RecordingSession.cpp:130-141)
- Iterator reset after buffer modifications (AppModel.cpp:363)
- Safe execution order (playback before recording in Update cycle)

**Status:** ⬜ Not Tested

---

### Test 7: Extended Loop Recording Session
**Purpose:** Stress test for memory leaks and stability

**Steps:**
1. Enable loop recording
2. Start recording
3. Play random notes continuously for 50+ loop iterations
4. Mix held notes, quick taps, chords, releases
5. Monitor memory usage
6. Stop recording
7. Playback recorded material

**Expected Results:**
- ✓ No memory leaks
- ✓ No performance degradation over time
- ✓ All notes play back correctly
- ✓ Clean stop with no stuck notes

**Status:** ⬜ Not Tested

---

## Verification Checklist

After completing all tests, verify:

- [ ] Build succeeds without errors or warnings
- [ ] All 7 test cases pass
- [ ] "Show MIDI Events" tool correctly identifies all Note On/Off types
- [ ] No orphaned MIDI events in any scenario
- [ ] Loop playback sounds musically correct
- [ ] No stuck notes after stopping
- [ ] No crashes or memory issues during extended sessions
- [ ] RecordingSession::HasActiveNotes() correctly tracks state
- [ ] WrapActiveNotesAtLoop preserves velocity
- [ ] CloseAllActiveNotes works correctly on stop
- [ ] Buffer iterator remains within bounds

---

## Test Execution Steps

### 1. Build Application
```bash
msbuild MidiWorks.sln /p:Configuration=Debug /p:Platform=x64 /t:Build
```

### 2. Launch Application
```bash
./x64/Debug/MidiWorks.exe
```

### 3. Configure MIDI
- Open MIDI Settings panel
- Select MIDI input device
- Select MIDI output device (or use Microsoft GS Wavetable Synth)

### 4. Enable Debug Mode
- Check "Show MIDI Events" in MidiCanvas panel
- This displays Note On/Off events with timestamps for verification

### 5. Run Each Test Case
- Follow test steps precisely
- Document actual results
- Compare against expected results
- Take screenshots if needed for documentation

### 6. Document Results
- Mark each test as PASS/FAIL
- Note any unexpected behavior
- Record any edge cases discovered
- Update BugList.md if new issues found

---

## Success Criteria

All tests must pass with these outcomes:
- ✓ Zero orphaned MIDI events
- ✓ All note pairs properly matched
- ✓ Velocity preserved across loop boundaries
- ✓ Clean stop behavior
- ✓ Stable performance during extended recording
- ✓ Correct "Show MIDI Events" display

---

## Files to Monitor During Testing

- `src/AppModel/AppModel.cpp` - HandleStopRecording (lines 255-277), loop wrap logic (lines 336-365)
- `src/AppModel/RecordingSession/RecordingSession.cpp` - WrapActiveNotesAtLoop (lines 69-85), CloseAllActiveNotes (lines 87-97)
- `src/Panels/MidiCanvas/MidiCanvas.cpp` - Event type detection (lines 580-594)

---

## Rollback Plan

If critical issues are discovered during testing:
1. Document the failure scenario precisely
2. Add new test case to capture the bug
3. Create bug entry in BugList.md
4. Investigate root cause before proceeding
5. Reopen bug #8 in BugList.md if necessary

---

## Post-Testing Actions

After successful test completion:
1. Update test statuses in this document
2. Document any edge cases discovered
3. Consider adding automated unit tests for critical scenarios
4. Update CLAUDE.md with any new architectural insights

---

## Test Results

### Test Run: [Date]

| Test # | Name | Status | Notes |
|--------|------|--------|-------|
| 1 | Notes Held Across Loop Boundary | ⬜ | |
| 2 | Multiple Notes Held Across Boundary | ⬜ | |
| 3 | Mid-Loop Stop with Active Notes | ⬜ | |
| 4 | Quick Press-Release Within Loop | ⬜ | |
| 5 | Overlapping Notes Separation | ⬜ | |
| 6 | Buffer Iterator Safety | ⬜ | |
| 7 | Extended Loop Recording Session | ⬜ | |

**Overall Result:** ⬜ Not Tested

**Tester:**
**Date:**
**Build Version:**
**Additional Notes:**
