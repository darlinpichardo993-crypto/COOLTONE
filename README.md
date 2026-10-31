# COOLTONE — BY ORYMY
YMY RECORDS | Vocal tuning VST3 | Windows x64

## Build 0.2.0
This repository contains a C++17 DSP core and a JUCE VST3/Standalone target.

Implemented in this foundation:
- Offline YIN pitch analysis, note segmentation, 14 scale definitions, key estimation and WAV read/write in the Core.
- Real-time monophonic pitch tracking and scale/key snapping in the plugin.
- Pitch strength and retune controls.
- Magic Vibrato with amount, rate and depth controls, enabled after a stable sustained pitch estimate.
- Original starting presets: ORYMY Signature, Romantic Bachata, R&B Expressive, Natural Vocal and Hard Tune.
- Dark resizable interface with live pitch trace, parameter state persistence, mono/stereo support.
- Windows build workflow, core tests, VST3/Standalone package and Setup.exe.

## Limitations (not falsely claimed complete)
This is a first plugin foundation, not yet a Melodyne/NewTone replacement. The real-time pitch shifter is a dual-head modulated-delay prototype and may introduce artifacts. The display is a live pitch monitor, not an editable note/blob piano roll. Full offline note editing, formant preservation, melisma generation, breath/air separation, full expression modeling, custom curves, A/B history and Drag to Playlist remain future work. The style presets are original broad parameter starting points, not artist voice clones.

## Local build
Requires CMake 3.22+, Git, Visual Studio 2022 C++ workload and internet access for JUCE FetchContent.
cmake -S . -B build -A x64 -DCOOLTONE_BUILD_PLUGIN=ON -DCOOLTONE_BUILD_TESTS=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure

JUCE 8.0.8 is fetched by CMake. After a successful GitHub Actions run, download the COOLTONE-Windows-x64 artifact from Actions.
