# COOLTONE — BY ORYMY
YMY RECORDS | Vocal tuning VST3 / Standalone | Windows x64

## Current development build

COOLTONE is being developed as a vocal tuning and note-editing tool, not as a one-knob auto-tune effect.

### Implemented in the current source
- VST3 and Standalone JUCE application.
- Import a vocal WAV/AIFF/FLAC by dropping it on the editor or using OPEN WAV.
- Offline YIN pitch analysis and note segmentation.
- Piano-roll-style note display; drag detected note blobs vertically to change their target pitch.
- Automatic scale/key correction on analysis, with key, scale and correction-strength controls.
- Export the tuned audio to a WAV file in the user's Documents folder.
- DRAG TO FL action to send the rendered WAV file to a DAW that accepts external file drops.
- Real-time monophonic input pitch tracking, fractional-lag refinement, scale snapping, retune speed, output level and bypass.
- Separate manual Vibrato switch and Magic Vibrato mode, with amount, rate and depth controls.
- Dark, resizable UI, parameter-state persistence and Windows x64 CI packaging.

### Important engineering status
This is an actively developed early build, not yet a Melodyne/NewTone-class product. The current pitch-shift engine is a dual-head delay/granular prototype; it can create audible transposition and vibrato but may produce artifacts, transient smearing and formant changes. It is not yet a production-quality formant-preserving PSOLA/phase-vocoder engine.

Not yet implemented: formant-preserving correction, detailed per-note pitch curves and drift/vibrato sub-editing, breath/air separation, melisma generation, undo/redo history, A/B comparison and a full artist-preset library. Preset names are starting points, not artist voice clones.

## Local build
Requires CMake 3.22+, Git, Visual Studio 2022 C++ workload and internet access for JUCE FetchContent.

```powershell
cmake -S . -B build -A x64 -DCOOLTONE_BUILD_PLUGIN=ON -DCOOLTONE_BUILD_TESTS=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

JUCE 8.0.8 is fetched by CMake. Windows builds are packaged by GitHub Actions as VST3, Standalone and Setup.exe.
