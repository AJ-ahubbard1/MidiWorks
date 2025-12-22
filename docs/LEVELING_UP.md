# Leveling Up: From Mid to Senior Engineer

*Honest Technical Assessment - December 17, 2025*

**Current Rating: 6.5/10 - Solid mid-level engineer**

---

## What You're Good At

### ✅ Architecture
- Clean Model-View separation (AppModel vs Panels)
- Proper command pattern implementation for undo/redo
- Good separation of concerns
- Reasonable abstraction layers (RtMidiWrapper)

### ✅ Shipping
- Built a working DAW from scratch
- Shipped functional features (piano roll, loop recording, save/load, etc.)
- Using your own software (dogfooding)
- Most developers don't get this far

### ✅ Product Sense
- Clear vision of what you're building and why
- Prioritize features based on workflow, not ego
- Understand tradeoffs (simplicity vs features)
- Think like a product person, not just an engineer

### ✅ Pragmatism
- K.I.S.S. approach (manual offset vs complex ASIO)
- Avoided VST hosting complexity trap
- Don't over-engineer solutions
- Make smart scope decisions

---

## What Needs Work

### ❌ Complex State Management

**Problem:** Loop recording logic (AppModel.cpp:120-154) is complex and error-prone.
- Multiple interacting pieces: `mActiveNotes`, `mRecordingBuffer`, `mRecordingBufferIterator`
- Auto-closing notes at loop boundaries has edge cases
- You identified it yourself as bug #8

**Why It Matters:** Complex state = bugs. You built working code but didn't think through all edge cases upfront.

**How to Fix:**
1. **Draw state machines before coding** - Visualize all possible states and transitions
2. **Simplify state** - Can you reduce the number of moving parts?
3. **Make state transitions explicit** - Don't let state change implicitly
4. **Write invariants** - What must ALWAYS be true? Assert it.

**Action Item:** Refactor loop recording with a clear state machine diagram. Document all edge cases.

---

### ❌ Defensive Programming

**Problem:** Not enough bounds checking, assertions, or validation. Code assumes happy paths.

**Examples:**
```cpp
// AppModel.cpp:176-179 - What if mRecordingBufferIterator is out of bounds?
while (mRecordingBufferIterator != -1 &&
       mRecordingBufferIterator < mRecordingBuffer.size() &&
       mRecordingBuffer[mRecordingBufferIterator].tick <= currentTick)
{
    messages.push_back(mRecordingBuffer[mRecordingBufferIterator].mm);
    mRecordingBufferIterator++;
}
```

**Missing:**
- Assertion that `mRecordingBufferIterator` is valid before access
- Bounds check before array access
- Logging when unexpected conditions occur

**How to Fix:**
1. **Add assertions liberally** - Make assumptions explicit
   ```cpp
   assert(mRecordingBufferIterator >= -1 && mRecordingBufferIterator < mRecordingBuffer.size());
   ```
2. **Validate inputs** - Check parameters at function entry
3. **Check return values** - Don't assume success
4. **Handle edge cases** - Empty arrays, zero values, null pointers

**Action Item:** Go through AppModel.cpp and add assertions for all state assumptions. Add bounds checks before array access.

#### 📚 Defensive Programming Deep Dive: Where to Learn

**Books (Start Here)**

1. **"Code Complete" by Steve McConnell - Chapter 8: Defensive Programming**
   - THE canonical resource (~50 pages)
   - Covers assertions, error handling, input validation, fail-fast
   - **START HERE. Read Chapter 8 first, then the rest later.**

2. **"The Pragmatic Programmer" by Hunt & Thomas**
   - "Design by Contract" section (assertions, preconditions, postconditions)
   - "Dead Programs Tell No Lies" (fail-fast principle)
   - "Assertive Programming" chapter

3. **"Effective C++" by Scott Meyers - Items 2-4, 13, 54**
   - Resource management (RAII)
   - Const correctness
   - Avoiding undefined behavior

**Online Resources**

- **NASA's "Power of 10" Rules for Safety-Critical Code**
  - https://en.wikipedia.org/wiki/The_Power_of_10:_Rules_for_Developing_Safety-Critical_Code
  - 10 simple rules used for Mars Rover code
  - Extreme but illustrative of defensive mindset
  - Key rules: no dynamic memory, bounded loops, check return values, assert liberally

- **Microsoft's Secure Coding Guidelines**
  - https://learn.microsoft.com/en-us/cpp/code-quality/
  - Input validation, buffer overruns, integer overflow

- **Google C++ Style Guide - Assertions Section**
  - https://google.github.io/styleguide/cppguide.html
  - When to use assertions vs exceptions vs return codes

**Practical Exercises for MidiWorks**

**Week 1: Add Assertions**
Go through `AppModel.cpp` and add assertions for every assumption:

```cpp
// Before
void AppModel::MoveNote(const NoteLocation& note, uint64_t newStartTick, ubyte newPitch)
{
    Track& track = mTrackSet.GetTrack(note.trackIndex);
    // ...
}

// After (defensive)
void AppModel::MoveNote(const NoteLocation& note, uint64_t newStartTick, ubyte newPitch)
{
    assert(note.found && "Cannot move note that wasn't found");
    assert(note.trackIndex < MidiConstants::CHANNEL_COUNT && "Invalid track index");
    assert(newPitch < 128 && "MIDI pitch must be 0-127");
    assert(newStartTick < UINT64_MAX - 1000 && "Tick overflow risk");

    Track& track = mTrackSet.GetTrack(note.trackIndex);
    assert(!track.empty() && "Track should not be empty if note found");
    // ...
}
```

**Exercise:** Add 50+ assertions across AppModel.cpp. Every function with parameters should validate them.

**Week 2: Bounds Checking**
Find every array/vector access and add bounds checking:

```cpp
// Before
mRecordingBuffer[mRecordingBufferIterator].tick

// After (defensive)
if (mRecordingBufferIterator >= 0 &&
    mRecordingBufferIterator < mRecordingBuffer.size())
{
    mRecordingBuffer[mRecordingBufferIterator].tick
}
else
{
    // Log error, this should never happen
    assert(false && "Recording buffer iterator out of bounds");
}
```

**Exercise:** Find 10+ places with array access. Add bounds checks.

**Week 3: Input Validation**
Add validation to all public API functions:

```cpp
bool AppModel::LoadProject(const std::string& filepath)
{
    // Defensive: validate input
    if (filepath.empty())
    {
        LogError("LoadProject: empty filepath");
        return false;
    }

    if (!std::filesystem::exists(filepath))
    {
        LogError("LoadProject: file does not exist: " + filepath);
        return false;
    }

    // ... rest of function
}
```

**Exercise:** Add input validation to SaveProject, LoadProject, MoveNote, ResizeNote, etc.

**Week 4: Error Handling Audit**
Go through every `catch` block and return value:

```cpp
// Before
try {
    json project = json::parse(file);
}
catch (const std::exception& e) {
    return false;  // Silent failure
}

// After (defensive)
try {
    json project = json::parse(file);
}
catch (json::parse_error& e) {
    LogError("Failed to parse project file: " + filepath);
    LogError("Parse error: " + std::string(e.what()));
    ShowErrorDialog("Invalid project file format");
    return false;
}
catch (std::exception& e) {
    LogError("Unexpected error loading project: " + std::string(e.what()));
    ShowErrorDialog("Failed to load project: " + std::string(e.what()));
    return false;
}
```

**Exercise:** Improve error handling in LoadProject, SaveProject, and MIDI device initialization.

**Key Defensive Programming Principles**

1. **Fail Fast** - Don't limp along with bad state. Assert/crash in debug builds when assumptions violated.
2. **Assert Liberally** - Every function should assert its preconditions, postconditions, and invariants.
3. **Validate Inputs** - Never trust external input (files, network, user input). Check ranges, nulls, empty strings.
4. **Check Return Values** - Don't ignore function return values. Check for errors explicitly.
5. **Use RAII** - Automatic cleanup (no manual delete/free). Files close automatically, locks release automatically.
6. **Const Correctness** - Mark everything const that can be. Prevents accidental modification.
7. **Avoid Undefined Behavior** - No null pointer dereference, array out of bounds, integer overflow, or uninitialized variables.

**Tools to Help**

**Static Analysis:**
- **Clang-Tidy** - Catches bugs at compile time
- **Cppcheck** - Static analysis tool
- **Visual Studio Code Analysis** - Built into VS

**Runtime Checking:**
- **Address Sanitizer (ASAN)** - Catches memory errors
- **Undefined Behavior Sanitizer (UBSAN)** - Catches UB

```bash
# Compile with sanitizers (GCC/Clang)
g++ -fsanitize=address,undefined -g -O1 *.cpp
```

**Recommended Learning Path**

- **Week 1-2:** Read "Code Complete" Chapter 8, add assertions to AppModel.cpp
- **Week 3-4:** Read "Pragmatic Programmer" chapters on assertions, add bounds checking
- **Month 2:** Read "Effective C++", audit error handling in MidiWorks
- **Month 3+:** Read NASA Power of 10 rules, make defensive programming a habit

**The Mindset Shift**

Defensive programming is about shifting from:
- "It works" → "It can't break"
- "Happy path" → "What can go wrong?"
- "Ship it" → "Ship it safely"

**YOUR HOMEWORK: Read "Code Complete" Chapter 8 this week. Then add 50 assertions to AppModel.cpp.**

---

### ❌ No Testing

**Problem:** No unit tests or integration tests visible in codebase.

**Why It Matters:**
- Timing-critical MIDI code has edge cases you won't catch manually
- Loop recording is perfect for automated testing
- Tests force you to think through edge cases
- Tests prevent regressions when refactoring

**How to Fix:**
1. **Start with unit tests** - Test individual functions in isolation
   ```cpp
   TEST(TransportTest, LoopBoundaryCalculation) {
       Transport t;
       t.mLoopStartTick = 0;
       t.mLoopEndTick = 3840;
       // Test loop-back logic
   }
   ```
2. **Add integration tests** - Test full workflows (record → loop → stop)
3. **Test edge cases** - Empty tracks, zero duration, boundary conditions
4. **Use a testing framework** - Google Test, Catch2, etc.

**Action Item:** Add Google Test or Catch2. Write tests for Transport and loop recording logic. Aim for 50% coverage of critical paths.

---

### ❌ Error Handling

**Problem:** Not enough error handling or user feedback.

**Example:**
```cpp
bool AppModel::LoadProject(const std::string& filepath)
{
    try {
        // ... parsing ...
    }
    catch (const std::exception& e) {
        return false;  // User has no idea what failed
    }
}
```

**Missing:**
- What failed? (File not found? Corrupted JSON? Wrong version?)
- User feedback (error dialog, log message)
- Actionable error messages ("Cannot open file X" vs "Failed to load")
- Logging for debugging

**How to Fix:**
1. **Log errors** - Use a logging library (spdlog, etc.)
2. **Show user feedback** - Error dialogs with specific messages
3. **Make errors actionable** - Tell user what to do
4. **Don't swallow exceptions silently** - At minimum, log them

**Action Item:** Add logging framework. Update LoadProject/SaveProject to show specific error messages. Add error dialog to MainFrame.

---

### ❌ Threading Understanding

**Problem:** Asked about adding threads for MIDI In/Out/Rendering, but didn't fully understand when threading helps vs hurts.

**Misconceptions:**
- MIDI In already threaded (RtMidi internal)
- MIDI Out is too fast to need threading
- wxWidgets UI isn't thread-safe (can't render on separate thread)

**Why It Matters:** Threading is complex. Adding it unnecessarily creates bugs (race conditions, deadlocks). Not adding it when needed creates performance issues.

**How to Fix:**
1. **Study concurrent programming** - Books: "C++ Concurrency in Action"
2. **Learn when to thread** - I/O-bound vs CPU-bound vs real-time
3. **Understand synchronization** - Mutexes, atomics, lock-free queues
4. **Profile first** - Don't add threads without measuring bottlenecks

**Action Item:** Read "C++ Concurrency in Action" (first 5 chapters). Profile MidiWorks to find actual bottlenecks before adding threads.

---

### ❌ Sparse Documentation

**Problem:** CLAUDE.md is great, but inline code comments are sparse.

**Example:** Loop recording logic (AppModel.cpp:120-154) needs a paragraph explaining the algorithm, not just "// Auto-close notes at loop end."

**Why It Matters:**
- Future you won't remember the reasoning
- Contributors can't understand complex logic
- Code review is harder without comments
- Maintenance takes longer

**How to Fix:**
1. **Comment complex algorithms** - Explain the "why," not the "what"
2. **Document invariants** - What must always be true?
3. **Add pre/post conditions** - What must be true before/after function?
4. **Use Doxygen-style comments** - `/** @brief ... */`

**Action Item:** Add block comments to loop recording logic explaining the algorithm, edge cases, and invariants.

---

## The Gap: Mid-Level vs Senior

### Mid-Level Engineer
- "It works on my machine with normal usage"
- Builds features that work
- Fixes bugs when they appear
- Thinks about happy paths

### Senior Engineer
- "It handles corrupted input, thread races, OOM conditions, and logs actionable errors"
- Builds features that handle edge cases
- Prevents bugs before they appear
- Thinks about failure modes

**You're in the first camp. Getting to the second requires discipline, not just skill.**

---

## Actionable Roadmap to Senior

### Phase 1: Defensive Programming (1-2 months)
- [ ] Add assertions throughout AppModel.cpp
- [ ] Add bounds checking before all array access
- [ ] Add input validation to all public functions
- [ ] Add logging framework (spdlog)
- [ ] Update error handling in LoadProject/SaveProject

### Phase 2: Testing Culture (2-3 months)
- [ ] Set up Google Test or Catch2
- [ ] Write unit tests for Transport class
- [ ] Write tests for loop recording logic
- [ ] Write integration tests for record → loop → stop workflow
- [ ] Aim for 50% coverage of critical code paths
- [ ] Run tests in CI (GitHub Actions)

### Phase 3: State Management (1-2 months)
- [ ] Draw state machine diagram for loop recording
- [ ] Document all edge cases and invariants
- [ ] Refactor loop recording to simplify state
- [ ] Add state validation checks
- [ ] Test all edge cases

### Phase 4: Concurrency (ongoing)
- [ ] Read "C++ Concurrency in Action" (chapters 1-5)
- [ ] Profile MidiWorks to find bottlenecks
- [ ] Only add threading where proven necessary
- [ ] Use lock-free queues for MIDI callback
- [ ] Add thread sanitizer to test builds

### Phase 5: Documentation (ongoing)
- [ ] Add block comments to complex algorithms
- [ ] Document invariants and assumptions
- [ ] Add Doxygen comments to public APIs
- [ ] Update CLAUDE.md as architecture evolves
- [ ] Write ARCHITECTURE.md for contributors

---

## Metrics for Success

**You'll know you're senior-level when:**
1. You think about failure modes BEFORE writing code
2. You write tests WHILE developing features, not after
3. You can explain complex code to junior engineers clearly
4. Your code handles edge cases without needing bug fixes later
5. Other engineers trust your code reviews
6. You prevent problems instead of just fixing them

---

## Resources

### Books
- **"C++ Concurrency in Action"** by Anthony Williams (threading)
- **"Effective C++"** by Scott Meyers (best practices)
- **"Code Complete"** by Steve McConnell (software construction)
- **"The Pragmatic Programmer"** by Hunt & Thomas (general wisdom)

### Testing
- **Google Test** - https://github.com/google/googletest
- **Catch2** - https://github.com/catchorg/Catch2

### Logging
- **spdlog** - https://github.com/gabime/spdlog (fast C++ logging)

### Concurrency
- **C++ Reference** - https://en.cppreference.com/w/cpp/thread
- **Lock-Free Programming** - Research ring buffers for MIDI callback

---

## Final Thoughts

**You're self-aware** (you identified bug #8), **pragmatic** (K.I.S.S.), and **shipping**. Those traits beat perfect code.

Senior engineers who can't ship are useless. You're shipping. That's valuable.

The gap is **discipline and depth**, not intelligence. Add tests. Think through edge cases. Assert your assumptions. Handle errors gracefully.

You'll get there.

**Current: 6.5/10**
**Target: 8.5/10 (solid senior)**
**Timeline: 6-12 months of focused effort**

Keep building. Keep shipping. Keep learning.

---

*"The only way to get better is to write more code, make more mistakes, and learn from them."*
