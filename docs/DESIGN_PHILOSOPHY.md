# MidiWorks Design Philosophy

*Written December 17, 2025*

## Why MidiWorks Exists

> "Creating music on my own software is an amazing feeling."

MidiWorks is not trying to be "Yet Another DAW." It's a specialized composition tool built around a clear philosophy: **constraints breed creativity, and flow state beats feature count.**

## The Core Philosophy

### MIDI Works, Because MIDI Works

MidiWorks is intentionally a **pure MIDI sequencer**. No VST hosting, no audio engine, no thousand-preset sound libraries. Just MIDI.

This is not a limitation - it's the entire point.

**The Problem:** Modern DAWs offer unlimited choice. Browse through 10,000 Kontakt presets while trying to capture a musical idea. Spend hours tweaking synth parameters instead of composing. The burden of too many choices kills flow state.

**The Solution:** Separate the creative phase from the production phase.

- **Creative Phase (MidiWorks):** Fast MIDI composition with minimal decisions. 128 General MIDI sounds. No browsing, no tweaking, just composing.
- **Production Phase (External):** Route MIDI out to Kontakt/VST hosts. Match instrumentation. Polish sounds.

The idea comes first. Sound design comes later.

## Architectural Decisions

### Why Pure MIDI?

By staying pure MIDI and routing to external sound sources, MidiWorks gains:

1. **Simplicity** - No VST hosting complexity (plugin formats, audio threading, GUI embedding, licensing)
2. **Stability** - Plugin crashes don't take down the sequencer
3. **Performance** - No CPU spikes from heavy VSTs affecting transport timing
4. **Flexibility** - Can drive any sound source:
   - Microsoft GS Wavetable (built-in, instant nostalgia)
   - Kontakt/VST hosts (professional sounds)
   - Hardware synths (via MIDI interface)
   - Other DAWs (via virtual MIDI)
5. **Focus** - The app stays lean and does one thing well

Some "full" DAWs can't even route to external sources easily. MidiWorks is a dedicated MIDI brain that can drive anything.

### The Unix Philosophy

> "Do one thing and do it well."

MidiWorks is a MIDI sequencer. Not an audio editor, not a VST host, not a mixing console. A MIDI sequencer.

This focus allows depth instead of breadth. Every feature serves the core workflow: fast, intuitive composition.

## The Personal Connection

### Nostalgia and Origins

The love of music comes from 90s video games. SNES soundtracks, PS1 MIDI, Windows 98 MIDI files. That slightly thin, reverb-heavy GS Wavetable sound is pure nostalgia.

Nobuo Uematsu, Koji Kondo, Yasunori Mitsuda - all working within MIDI constraints and making timeless music. The limitations didn't hinder them. They enabled creativity.

MidiWorks honors that tradition. The GS Wavetable isn't just "adequate for testing" - it's the sound of home.

## Design Principles

### 1. Flow State Over Feature Count

**Goal:** Keep the user in flow state during composition.

**How:**
- Minimize decisions during creative process
- Fast, intuitive piano roll editing
- Keyboard shortcuts for everything
- No interruptions, no browsing, no waiting

**Anti-pattern:** Adding features because "other DAWs have it"

### 2. K.I.S.S. (Keep It Simple, Stupid)

**Example:** MIDI latency compensation
- ❌ Complex: Automatic calibration with loopback testing and ASIO integration
- ✅ Simple: Manual offset slider (~1 hour to implement, solves 80% of problem)

Ship simple solutions that work. Add complexity only when proven necessary.

### 3. Constraints Breed Creativity

**Intentional Limitations:**
- 128 General MIDI sounds (not thousands of presets)
- Pure MIDI (not audio + MIDI hybrid)
- 15 channels (standard MIDI limitation)

**Why:** Decision fatigue is real. Research shows that arbitrary constraints force creative decisions instead of technical ones. Some of the best composers deliberately limit themselves (only piano sounds, only 4 tracks, etc.).

MidiWorks enforces this constraint by design.

### 4. Separation of Concerns

**Model-View Architecture:**
- AppModel contains all application data and business logic
- Panels contain UI layout and event bindings
- Event handlers call AppModel functions, not manipulate data directly

**Composition-Production Separation:**
- Compose with simple MIDI sounds first
- Route to professional sounds later
- Don't let sound design interrupt the creative flow

### 5. Workflow First, Polish Later

**Feature Priority:**
- **High:** Multi-note operations, loop recording reliability, recording latency - core workflow
- **Medium:** Virtual piano, cross-channel copy, measure navigation - composition efficiency
- **Low:** Custom colors, collapsible channels - nice-to-have polish

Workflow efficiency beats visual polish. Always.

## What MidiWorks Is Not

- **Not a VST host** - Use external VST players (Kontakt, etc.)
- **Not an audio editor** - Pure MIDI sequencing only
- **Not trying to replace Ableton/FL Studio** - Different tool for different workflow
- **Not feature-complete on day one** - Focused MVP, iterate based on use

## What Success Looks Like

**Success is:**
- Capturing musical ideas quickly without friction
- Staying in flow state during composition
- Having fun making music
- The app getting out of the way

**Success is NOT:**
- Feature parity with commercial DAWs
- Maximum user count
- Complexity for complexity's sake

## Remember This When...

### ...Feature creep tempts you:
Ask: "Does this help the core workflow of fast MIDI composition?" If not, it's scope creep.

### ...You compare to other DAWs:
Remember: You're not building a DAW. You're building a specialized composition tool.

### ...The bug list feels overwhelming:
Focus on workflow efficiency (bugs #5, #8, #9). Polish can wait.

### ...You forget why you're building this:
> "Creating music on my own software is an amazing feeling."

That's why. Everything else is details.

## The Vision

MidiWorks is a love letter to 90s video game music, to MIDI simplicity, to the joy of constraints, and to the flow state of pure composition.

It's a tool built by a composer, for composers, who want to capture ideas fast and worry about production later.

It's not for everyone. That's okay. It's for you.

---

*"MidiWorks, because MIDI works."*
