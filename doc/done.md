# Stairwell to Heaven: done

Newest first. Items that came from `todo.md` keep their number.

## 7 (part). Themed GUI: floor indicator, a window lit by the tail (2026-11-06)

Proposals A and C of `architecture.md`:

- A: `Components/FloorIndicator`, the lift's floor display above the floor
  selector: the floor sounding now, an arrow while a mode change travels
  (held 0.6 s so it is seen), a flash on arrival. The engine now exposes the
  floor it plays and its mode fade (`getActiveFloor`, `getModeFade`).
- C: the backdrop's light follows the tail's energy (`theme::paintBackdrop`
  takes a level and a colour; the editor's timer drives it): brighter as the
  reverb blooms, dimmer as it dies, steady while frozen, pale blue in Heaven.
- Checked: the engine tests still pass. Not built as a plugin yet.

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
