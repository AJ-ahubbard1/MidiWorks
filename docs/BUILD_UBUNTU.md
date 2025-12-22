# Building MidiWorks on Ubuntu

This guide explains how to build and run MidiWorks on Ubuntu Linux.

## Prerequisites

### 1. Install Build Tools
```bash
sudo apt update
sudo apt install build-essential cmake git pkg-config
```

### 2. Install wxWidgets Development Libraries
```bash
sudo apt install libwxgtk3.2-dev libwxgtk-media3.2-dev

# If wxWidgets 3.2 is not available, try 3.0:
# sudo apt install libwxgtk3.0-gtk3-dev
```

### 3. Install ALSA Development Libraries (for MIDI support)
```bash
sudo apt install libasound2-dev
```

## Build Instructions

### 1. Clone the Repository (if not already done)
```bash
git clone <your-repo-url>
cd MidiWorks
```

### 2. Create Build Directory
```bash
mkdir build
cd build
```

### 3. Configure with CMake
```bash
cmake ..
```

If wxWidgets is not found automatically, you may need to specify the path:
```bash
cmake -DwxWidgets_CONFIG_EXECUTABLE=/usr/bin/wx-config ..
```

### 4. Build the Project
```bash
make -j$(nproc)
```

The `-j$(nproc)` flag uses all available CPU cores for faster compilation.

### 5. Set Up MIDI Audio Output (Required!)

**Important:** MidiWorks requires a MIDI synthesizer to produce audio. Without one, the app will hang when trying to play MIDI notes.

Install and start FluidSynth (software MIDI synthesizer):

```bash
# Install FluidSynth and General MIDI soundfont
sudo apt install fluidsynth fluid-soundfont-gm

# Start FluidSynth (keep this terminal open while using MidiWorks)
fluidsynth -a alsa -m alsa_seq /usr/share/sounds/sf2/FluidR3_GM.sf2
```

**Note:** Keep FluidSynth running in a terminal. You'll hear audio through your speakers when MidiWorks plays notes.

### 6. Run MidiWorks

In a **new terminal**:
```bash
cd ~/Projects/MidiWorks/build/bin
./MidiWorks
```

The app should now open and work correctly!

## Alternative: Build in Release Mode

For optimized release builds:
```bash
mkdir build-release
cd build-release
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
./bin/MidiWorks
```

## Troubleshooting

### wxWidgets Not Found
If CMake cannot find wxWidgets, check which version is installed:
```bash
wx-config --version
dpkg -l | grep libwx
```

### ALSA Not Found
Verify ALSA development libraries are installed:
```bash
dpkg -l | grep libasound2-dev
```

### MIDI Devices Not Detected or App Hangs

**Symptom:** App freezes/becomes unresponsive after startup, or shows "MidiWorks is not responding" dialog.

**Cause:** No MIDI output device available. MidiWorks requires a MIDI synthesizer to function.

**Solution:** Install and run FluidSynth (see Step 5 in Build Instructions above):

```bash
# Install FluidSynth
sudo apt install fluidsynth fluid-soundfont-gm

# Start FluidSynth (keep running while using MidiWorks)
fluidsynth -a alsa -m alsa_seq /usr/share/sounds/sf2/FluidR3_GM.sf2
```

**Alternative:** For testing without audio output, create a virtual MIDI loopback:
```bash
# Install ALSA utilities if needed
sudo apt install alsa-utils

# Load virtual MIDI driver
sudo modprobe snd-virmidi
```

**Verify MIDI devices are available:**
```bash
aplaymidi -l
```

You should see at least one MIDI output port (e.g., "FluidSynth" or "Virtual Raw MIDI").

### GTK Warnings About Widget Sizes

You may see warnings like:
```
Gtk-WARNING: for_size smaller than min-size (18 < 32) while measuring gadget
```

**These are harmless.** They occur because some UI controls (like volume sliders) are sized smaller than GTK's preferred minimum on Linux. The app will still function correctly - you can safely ignore these warnings.

### Permission Issues
If you get MIDI permission errors, add your user to the audio group:
```bash
sudo usermod -a -G audio $USER
# Log out and log back in for changes to take effect
```

## Uninstall

To remove build artifacts:
```bash
cd build
rm -rf *
```

## Platform-Specific Notes

- **Linux** uses ALSA (Advanced Linux Sound Architecture) for MIDI
- RtMidi is automatically configured for ALSA on Linux builds
- The CMakeLists.txt handles platform detection automatically

## Next Steps

Once built, you can:
1. Run the executable from `build/bin/MidiWorks`
2. Install system-wide with `sudo make install` (installs to `/usr/local/bin`)
3. Create a `.desktop` file for application launcher integration

## Creating a Desktop Entry (Optional)

Create `~/.local/share/applications/midiworks.desktop`:
```desktop
[Desktop Entry]
Name=MidiWorks
Comment=Digital Audio Workstation
Exec=/path/to/MidiWorks/build/bin/MidiWorks
Icon=/path/to/icon.png
Terminal=false
Type=Application
Categories=AudioVideo;Audio;Midi;
```
