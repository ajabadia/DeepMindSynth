# DeepMindSynth

An advanced Virtual Analog Synthesizer emulation inspired by the DeepMind 12/6 architecture, built with JUCE.

## Features

### 1. Dual-DCO Architecture
- **DCO 1**: Sawtooth/Pulse with PWM.
- **DCO 2**: Pulse with Tone/Pitch modifications.
- **Unison**: Up to 12 voices, configurable Detune and Spread.
- **Drift**: Analog drift emulation for pitch and filter instability.

### 2. Filter & Modulation
- **VCF**: 12/24dB Low Pass Filter (IR3109 emulation with self-oscillation).
- **VCF 2**: 2-Pole State Variable Filter (MS-20 style).
- **Envelopes**: 3 x Analog-modeled ADSRs (VCA, VCF, Mod) with Curve control (Log/Lin/Exp).
- **LFOs**: 2 x LFO (Sine, Tri, Sqr, Ramp, S&H, S&G) with Slew and Delay keysync.
- **Mod Matrix**: 8-slot Modulation Matrix bridging sources to targets.
- **Control Sequencer**: 32-step modulation source freely assignable in Matrix.

### 3. Extended Effects Engine
A fully modular, insertable effects chain:
- **Distortion**: Tube, Hard Clip, BitCrush.
- **Phaser**: Multi-stage analog phaser simulation (DeepPhase).
- **Chorus**: Stereo chorus based on BBD emulation.
- **Delay**: Stereo delay with feedback filtering.
- **Reverb**: Algorithmic reverb.
- **EQ**: 4-Band Parametric EQ (Low/Hi Shelf + 2 Mids).
- **Routing**: Switchable **Series** or **Parallel** signal path.

### 4. Performance Features
- **Arpeggiator**: Multiple modes, Octave range, Gate, and User Patterns (32-step).
- **Chord Memory**: Capture chords and play them with single keys.
- **CPU Meter**: Real-time DSP load monitoring.

### 5. Connectivity & Audio Input
- **WiFi / OSC Control**:
    - **Port 8000 (RX)**: Control parameters via OSC (Address: `/deepmind/{param_id}`).
    - **Port 9000 (TX)**: Receive parameter feedback.
- **Standalone Multi-FX**:
    - Process external audio (Guitars, Vocals) through the synth's FX engine using the **Ext Input Gain** parameter.

## Build Instructions
1.  Ensure CMake 3.22+ and a C++17 compiler (MSVC 2022 recommended on Windows).
2.  Clone repository.
3.  Configure: `cmake -B build`
4.  Build: `cmake --build build --config Release`

## Credits
Built by ABDMind.
