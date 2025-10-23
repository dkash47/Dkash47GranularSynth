# Dkash47GranularSynth

A JUCE-based granular synthesizer. Goal: constant granular sound upon MIDI note-on, without LFO-style amplitude or pitch modulation. Grains are continuously generated while a key is held; no LFO.

## Requirements
- Windows with Visual Studio 2022
- CMake (in `D:\CMake`)
- JUCE (in `D:\JUCE`)
- Git

## Build (Standalone App)
```powershell
# From project root
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DJUCE_DIR="D:/JUCE"
cmake --build build --config Release
```
The executable will be under `build/Release`.

## Structure
- `src/` core code
- `Assets/` audio samples

## Audio behavior
- On MIDI note-on: start a voice that spawns grains from the loaded sample with fixed density and random micro-variations (no periodic LFO).
- On note-off: let active grains finish; voice stops.

## Configure JUCE path
If JUCE is not at `D:/JUCE`, set `-DJUCE_DIR` accordingly.

# Dkash47 Granular Synthesizer

A JUCE-based VST3 granular synthesizer inspired by modern granular UIs like Quanta.

## Prerequisites
- Visual Studio 2022 (Desktop development with C++)
- CMake 3.22+
- JUCE (cloned or extracted locally). Set `JUCE_DIR` to the JUCE folder that contains `CMakeLists.txt`.

## Build (Windows, VS 2022)
```pwsh
# From the project root
mkdir build
cmake -S . -B build -G "Visual Studio 17 2022" -DJUCE_DIR="D:/JUCE"
cmake --build build --config Release
```
The built VST3 will be copied to the default JUCE VST3 location for your system (you can adjust in CMake if desired).

## Next steps
- Implement granular engine features (grain position, length, density, pitch, randomization, envelopes).
- Design the UI: waveform display, grain markers, filter section, modulation matrix.