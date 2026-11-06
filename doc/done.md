# Stairwell to Heaven: done

Newest first. Items that came from `todo.md` keep their number.

## First implementation (2026-11-06)

The advanced reverb planned in FxmeFX's `todo.md` (section 4), as a new
project on the FX-Mechanics template:

- FxmeTools core: `fxme::TankReverb` (Dattorro tank, calibrated and
  frequency-dependent decay, modulation, freeze, shimmer, safety) and
  `fxme::EarlyReflections` (four patterns, one of them the stairwell
  flutter), with `CoreReverbTests` (21 checks pass).
- The engine (`Source/Dsp/StairwellEngine`, JUCE-free): five floors (Room,
  Stairwell, Plate, Hall, Heaven), synced pre-delay, Early/Late, Width, wet
  low and high cuts, Mix, Output, click-free mode change; `EngineTests` (11
  checks pass).
- The plugin: APVTS (22 parameters, version 1), global presets with nine
  factory ones, the house shell (top bar, preset bar and browser, splash,
  tooltips with the "?" switch, info), the lift-panel mode selector, the
  decay stairs display, CLAP, release workflow.
- Checked: the core and engine tests (compiled with g++ against the core
  headers), a CMake configure with the sibling clones. Not built as a plugin
  yet (todo 2).
