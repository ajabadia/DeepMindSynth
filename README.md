# DeepMindSynth

An open-source VST3/Standalone software synthesizer modeled after the **Behringer DeepMind 12**, built with the **JUCE Framework**.

## Features

- **Architecture:** 12-Voice Polyphony.
- **Oscillators:** 2 DCOs per voice (Square/Saw + Pulse) with PWM and Sync.
- **Filters:**
  - 4-Pole Low Pass Filter (24dB/oct).
  - High Pass Filter with Boost.
  - Models: Jupiter-8, MS-20, TB-303 emulation.
- **Modulation:**
  - 8-Slot Modulation Matrix.
  - 3 Envelopes (VCA, VCF, Mod).
  - 2 LFOs with Rate/Delay and S&H.
- **Effects:** Built-in Chorus, Delay, and Reverb.
- **Arpeggiator:** Basic Up/Down/Up-Down modes with Rate and Gate control.
- **SysEx:** Partial support for loading DeepMind 12 SysEx patches.
- **Presets:** XML-based preset management system.

## Build Instructions (Windows)

**Prerequisites:**
- Visual Studio 2022 (C++ Desktop Development workload)
- CMake
- Git

**Steps:**
1. Clone the repository.
2. Open the folder in Visual Studio or use command line:
   ```bash
   cmake -B out/build
   cmake --build out/build --config Release
   ```

## License

MIT (or your preferred license)
