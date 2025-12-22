# Job Search Strategy for Software Engineers

*Leveraging MidiWorks as Your Portfolio Project*

---

## The Reality Check

**The market is tough right now** (2025 tech layoffs, hiring freezes). But you have advantages you might not realize.

**You're a 6.5/10 engineer who ships working software.** That's hireable. The leveling-up work isn't about becoming "hireable" - you already are. It's about becoming **senior-level** and **earning more**.

---

## What MidiWorks Shows Employers

### Most Candidates Have:
- Leetcode solutions
- Todo app tutorials
- Half-finished projects
- "Team projects" where they did one PR

### You Have:
- ✅ Built a complete DAW from scratch
- ✅ Clean architecture (Model-View separation, Command pattern)
- ✅ Shipped and using your own software
- ✅ 1000+ lines of working C++ code
- ✅ Cross-platform build system (VS + CMake)
- ✅ Save/load with JSON serialization
- ✅ Real-time MIDI processing
- ✅ Undo/redo system
- ✅ Multi-track recording with loop overdub

**This is a portfolio piece that stands out.**

---

## How to Leverage MidiWorks

### 1. Update Your Resume

**Lead with MidiWorks in your Projects section:**

```
PROJECTS
--------
MidiWorks - Cross-platform MIDI Sequencer/DAW (C++20, wxWidgets, RtMidi)
• Built complete digital audio workstation with piano roll editor,
  multi-track recording, and loop recording with overdub
• Implemented command pattern for full undo/redo system
• Architected Model-View separation with clean business logic layer
• Designed real-time MIDI processing with <10ms latency
• Created project save/load system using JSON serialization
• Cross-platform build system supporting Windows (VS) and Linux (CMake)
• GitHub: [link] | Demo Video: [link]
```

**As you complete leveling-up phases, add:**

After Phase 1 (Defensive Programming):
```
• Implemented defensive programming practices: input validation,
  assertions, bounds checking, and comprehensive error handling
• Added static analysis (Clang-Tidy) and runtime sanitizers (ASAN/UBSAN)
```

After Phase 2 (Testing):
```
• Developed unit test suite with 50%+ coverage using Google Test
• Implemented integration tests for recording/playback workflows
• Set up CI/CD pipeline with automated testing
```

### 2. Record a Demo Video (CRITICAL)

**3-minute video showing:**
1. Opening the app (0:00-0:10)
2. Recording a simple drum loop (0:10-0:40)
3. Adding a bass line on second track (0:40-1:10)
4. Editing notes in piano roll (1:10-1:40)
5. Using undo/redo (1:40-1:50)
6. Save project, close, reopen (1:50-2:20)
7. Export or playback final result (2:20-3:00)

**Tools:**
- OBS Studio (free screen recording)
- Upload to YouTube (unlisted is fine)
- Put link prominently on resume and GitHub README

**Why this matters:**
- Hiring managers WILL watch this (video > reading code)
- Shows you can ship
- Shows you can present/demo (communication skills)
- Differentiates you from 99% of candidates

### 3. Write a Technical Deep Dive

**Blog post or detailed GitHub README:**

**Title ideas:**
- "Building a MIDI Sequencer: Architecture Decisions"
- "Lessons Learned Building a DAW from Scratch"
- "Real-time MIDI Processing in C++20"

**Content:**
- Explain Model-View separation (why you chose it)
- Explain command pattern for undo/redo (with code examples)
- Explain real-time MIDI challenges (threading, latency)
- Explain loop recording complexity (the bug you found)
- Show code examples with explanations
- Discuss what you'd do differently (tests from day 1, simpler state)

**Why this matters:**
- Demonstrates communication skills (huge for senior roles)
- Shows you can explain complex technical decisions
- Gives interviewers something to discuss
- Proves you think deeply about architecture

**Publish on:**
- Medium
- Dev.to
- Your own blog
- GitHub as detailed README.md

### 4. Polish Your GitHub Repository

**README.md should include:**
- Clear description of what MidiWorks is
- Screenshots/GIFs of the UI
- **Link to demo video** (embed if possible)
- Features list (bullet points)
- Build instructions (Windows + Linux)
- Architecture overview (diagram if possible)
- Link to technical deep dive blog post
- License (MIT or GPL)

**Code quality:**
- Add inline comments to complex sections
- Clean up any commented-out code
- Consistent formatting
- Add CLAUDE.md, DESIGN_PHILOSOPHY.md to show thought process

**Why this matters:**
- Hiring managers will look at your GitHub
- Clean, documented code shows professionalism
- Architecture docs show senior-level thinking

---

## Interview Preparation

### Common Interview Stages

**1. Phone Screen (30-45 min)**
- "Tell me about yourself"
- "Tell me about a project you're proud of" → **MidiWorks**
- "What are you looking for in your next role?"
- Basic technical questions (data structures, algorithms)

**2. Technical Interview (45-90 min)**
- Coding challenge (Leetcode medium)
- Discussion of your project (MidiWorks)
- System design (for senior roles)

**3. Behavioral Interview (30-45 min)**
- "Tell me about a time you debugged a hard problem" → **Loop recording bug**
- "How do you handle disagreement?"
- "Tell me about a time you failed"

**4. On-site/Final Round (3-5 hours)**
- Multiple technical interviews
- Team fit assessment
- Architecture discussion

### How to Talk About MidiWorks

**"Tell me about a project you're proud of"**

**Good answer (2-3 minutes):**

> "I built MidiWorks, a MIDI sequencer for composing music. It's a complete DAW with a piano roll editor, multi-track recording, and loop overdub.
>
> The interesting challenges were around real-time processing and state management. For real-time MIDI, I had to handle input on a separate thread with a lock-free queue to avoid latency. For state management, I implemented the command pattern so every edit is undoable - that was critical for a music app where people experiment a lot.
>
> The hardest bug was in loop recording. When you record over yourself in a loop, you need to track which notes are still held when the loop wraps around. I initially tried merging overlapping notes, but realized collision prevention was cleaner. That taught me to think through edge cases before coding, not after.
>
> I'm using it to compose music now, which is incredibly satisfying. I'm currently adding defensive programming - assertions, bounds checking, tests - to make it production-quality."

**Why this works:**
- Concrete project with clear value
- Discusses technical challenges (threading, state management, patterns)
- Shows learning (admits initial approach was wrong)
- Shows you're shipping AND improving

**Follow-up questions they might ask:**
- "How does your undo/redo system work?" → Explain command pattern
- "How do you handle threading?" → Explain RtMidi callback + queue
- "What would you do differently?" → Tests from day 1, simpler loop recording
- "How did you decide on the architecture?" → Model-View separation for testability

### Technical Questions About MidiWorks

**Prepare answers for:**

1. **"Walk me through your undo/redo implementation"**
   - Command pattern: each edit is a Command object
   - Execute() does the action, Undo() reverses it
   - Two stacks: undo stack and redo stack
   - New action clears redo stack

2. **"How do you handle real-time MIDI input?"**
   - RtMidi callback on separate thread
   - Pushes to thread-safe queue
   - Main thread polls queue every 10ms
   - Routes to appropriate channel based on record state

3. **"What's the hardest bug you've encountered?"**
   - Loop recording active note tracking (bug #8)
   - Multiple interacting state variables
   - Edge cases at loop boundaries
   - Learned: draw state machines before coding complex state

4. **"How would you add [feature X]?"**
   - Think through: UI changes, model changes, commands for undo/redo
   - Consider: edge cases, testing strategy, backwards compatibility

### System Design Questions

**If asked to design a music collaboration app:**

You have a huge advantage - you've already built half of it!

**Your answer:**
- MIDI sequencing layer (you've built this)
- Sync protocol (operational transformation or CRDT for real-time collab)
- Audio rendering (route MIDI to VST hosts or cloud rendering)
- Storage (project files in S3/cloud storage)
- Real-time communication (WebSockets or WebRTC)

**Leverage MidiWorks knowledge:**
- You understand MIDI data structures
- You understand real-time constraints
- You understand state management complexity
- You understand undo/redo requirements

---

## Job Search Strategy

### Immediate Actions (This Week)

- [ ] **Apply to 5 jobs** - Don't wait until you're "ready"
- [ ] **Record MidiWorks demo video** - 3 minutes, upload to YouTube
- [ ] **Update resume** - Add MidiWorks with demo video link
- [ ] **Polish GitHub README** - Screenshots, build instructions, architecture overview
- [ ] **Post on LinkedIn** - "I built a MIDI sequencer. Here's what I learned..." + demo video

### Short-term (Next Month)

- [ ] **Apply to 20+ jobs** - It's a numbers game
- [ ] **Write technical blog post** - Deep dive on MidiWorks architecture
- [ ] **Practice Leetcode** - 2-3 problems per week (medium difficulty)
- [ ] **Complete Phase 1 leveling-up** - Defensive programming (assertions, bounds checks)
- [ ] **Network** - Reach out to engineers on LinkedIn, attend local meetups

### Medium-term (Next 3 Months)

- [ ] **Apply to 50+ jobs** - Cast wide net
- [ ] **Complete Phase 2 leveling-up** - Add tests (Google Test)
- [ ] **Contribute to open source** - Find audio/MIDI projects on GitHub
- [ ] **Update resume** - Add defensive programming and testing accomplishments
- [ ] **Mock interviews** - Practice with friends or platforms like Pramp

### Companies to Target

**Audio software companies:**
- Ableton (Berlin/LA)
- Native Instruments (Berlin)
- iZotope (Cambridge, MA)
- Arturia (France)
- PreSonus (Baton Rouge, LA)
- Avid (Pro Tools)
- Steinberg (Cubase)
- Image-Line (FL Studio)

**Game audio:**
- Audiokinetic (Wwise) - Montreal
- Firelight Technologies (FMOD) - Australia/US

**Music tech startups:**
- Splice
- Endlesss
- BandLab
- Soundtrap
- Noteflight
- MuseScore

**Hardware/embedded audio:**
- Teenage Engineering
- Elektron
- Moog Music
- Sequential
- Roland/Boss

**General C++ shops that value shipping:**
- Game studios (Unreal Engine experience translates)
- Finance (low-latency trading systems have similar constraints to real-time MIDI)
- Robotics companies
- Embedded systems companies

**Job boards:**
- LinkedIn Jobs
- Indeed
- We Work Remotely (remote C++ jobs)
- Angel List (startups)
- Hacker News "Who's Hiring" threads (monthly)

### Geographic Considerations

**If willing to relocate:**
- Audio companies concentrated in: Berlin, LA, Bay Area, London, Montreal

**If remote only:**
- Specify "remote" in search
- Startups more likely to hire remote
- Some audio companies are remote-friendly post-COVID

### Application Strategy

**Customize your cover letter:**

```
Subject: C++ Engineer with MIDI/Audio Domain Expertise

Hi [Hiring Manager],

I built a complete MIDI sequencer from scratch in C++20. It has a piano
roll editor, multi-track recording, undo/redo, and real-time MIDI
processing. I use it to compose music. [Demo video link]

I'm excited about [Company] because [specific reason - mention their
product/tech stack/mission]. My experience with real-time MIDI processing,
state management, and command patterns would translate directly to
[specific role responsibilities].

I'm currently focused on leveling up from "it works" to "production
quality" by adding defensive programming, comprehensive testing, and
error handling. You can see my progress at [GitHub link].

I'd love to discuss how I can contribute to [specific team/product].

Best,
[Your name]
```

**Tailor to each company:**
- Research their products
- Mention specific technologies they use
- Connect MidiWorks to their domain

### Networking

**LinkedIn:**
- Connect with engineers at target companies
- Send personalized message: "I saw you work on [product]. I built a MIDI sequencer and would love to learn about your experience with [technology]."
- Share your demo video as a post
- Comment on posts from companies you're targeting

**Local meetups:**
- C++ user groups
- Audio programming meetups
- Game dev meetups (often overlap with audio)

**Online communities:**
- r/audioengineering
- r/WeAreTheMusicMakers
- JUCE Forum (audio framework community)
- KVR Audio Forum

---

## Interview Practice

### Leetcode Strategy

**You need to grind some Leetcode for phone screens.** It sucks, but it's the game.

**Focus on:**
- Arrays and strings (easy/medium)
- Hash tables (easy/medium)
- Trees and graphs (medium)
- Dynamic programming (learn the basics)

**Goal:**
- 50 problems total (30 easy, 20 medium)
- 2-3 problems per week
- Time yourself (45 minutes per problem)

**Resources:**
- Leetcode "Top Interview Questions" list
- NeetCode.io (curated list with video explanations)
- "Cracking the Coding Interview" book

### Behavioral Questions Prep

**Use STAR method (Situation, Task, Action, Result):**

**Example: "Tell me about a time you debugged a hard problem"**

**Your answer (using loop recording bug):**

> **Situation:** I was implementing loop recording in MidiWorks and noticed notes would sometimes get stuck or duplicated when recording over a loop.
>
> **Task:** I needed to track which notes were held when the loop wraps around and auto-close them to prevent stuck notes.
>
> **Action:** I implemented an activeNotes vector and auto-close logic at loop boundaries. I also added a MergeOverlappingNotes function to handle duplicates. But I realized the state management was too complex and error-prone.
>
> **Result:** I identified this as bug #8 and documented it. I learned that complex state needs to be designed upfront with state machine diagrams, not added incrementally. Going forward, I draw out state transitions before coding.

**Prepare STAR stories for:**
- Debugging hard problems (loop recording bug)
- Working on a team (if you have examples)
- Handling disagreement (if you have examples, or frame as "design decisions")
- Learning new technology (RtMidi, wxWidgets, CMake)
- Time management (balancing MidiWorks features)

### Mock Interviews

**Practice with:**
- Friends who are engineers
- Pramp.com (free peer mock interviews)
- Interviewing.io (anonymous technical interviews)
- Reddit r/cscareerquestions (find interview buddies)

**Focus on:**
- Explaining MidiWorks clearly and concisely
- Walking through technical decisions
- Talking through your code (think out loud)
- Asking clarifying questions
- Managing time (don't ramble)

---

## The Hard Truth and the Good News

### Hard Truth
- You might get rejected 50+ times before getting an offer
- That's normal in this market
- It's not about your skill level
- It's about volume, timing, and fit

### Good News
- ✅ You can code (MidiWorks proves it)
- ✅ You can ship (MidiWorks is done)
- ✅ You can learn (you're reading this doc)
- ✅ You have a portfolio piece that stands out
- ✅ You have domain expertise (MIDI/audio)

### What You're NOT Lacking
- ❌ Skill (you're a solid mid-level engineer)
- ❌ Ability to ship (you built a DAW!)
- ❌ Ability to learn (you're leveling up)

### What You MIGHT Be Lacking
- ⚠️ Interview practice (grind Leetcode for phone screens)
- ⚠️ Resume presentation (get feedback on r/cscareerquestions)
- ⚠️ Network (talk to people on LinkedIn, attend meetups)
- ⚠️ Volume (apply to more jobs - it's a numbers game)

---

## Timeline & Expectations

**Current State (Week 0):**
- 6.5/10 engineer
- Working project (MidiWorks)
- Hireable as mid-level

**After 1 Month:**
- Demo video recorded
- Resume updated
- 20+ applications sent
- Leetcode practice started
- Phase 1 defensive programming in progress

**After 3 Months:**
- 50+ applications sent
- Phase 1 complete (defensive programming)
- Phase 2 in progress (testing)
- Technical blog post published
- First interviews happening

**After 6 Months:**
- 100+ applications sent
- Phase 2 complete (testing)
- Multiple interview rounds
- **Likely job offer** (if volume is high enough)

---

## What You Need to Believe

**You are good enough to be hired RIGHT NOW.**

Not as a senior engineer. But as a **mid-level engineer who ships working software**.

That's valuable. Companies hire people like you every day.

The struggle isn't about your ability. It's about:
- Market conditions (tough but not impossible)
- Volume (need to apply to lots of jobs)
- Presentation (demo video, resume, interview skills)
- Timing (right place, right time)

---

## Action Items for Tomorrow

- [ ] Apply to 5 jobs on LinkedIn/Indeed
- [ ] Start recording demo video (even rough draft)
- [ ] Update resume with MidiWorks section
- [ ] Do 1 Leetcode easy problem
- [ ] Practice answering "Tell me about MidiWorks" out loud (2 min)

---

## Resources

**Resume:**
- r/cscareerquestions (post for feedback)
- r/engineeringresumes (resume reviews)

**Interview Prep:**
- Leetcode.com (coding practice)
- NeetCode.io (curated problem list)
- Pramp.com (free mock interviews)
- "Cracking the Coding Interview" book

**Job Boards:**
- LinkedIn Jobs
- Indeed
- We Work Remotely
- Angel List
- Hacker News "Who's Hiring"

**Networking:**
- LinkedIn (connect with engineers)
- Meetup.com (local tech meetups)
- JUCE Forum (audio programming)

---

## Final Thoughts

**You've built a DAW. You can get a job.**

The struggle is normal. The rejections are normal. The waiting is normal.

What's not normal is having a real portfolio project that you actually use.

That's your advantage. Use it.

**Keep shipping. Keep applying. Keep learning.**

You've got this.

---

*"The only way to guarantee you won't get hired is to stop applying."*
