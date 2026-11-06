# Stairwell to Heaven: done

Newest first. Items that came from `todo.md` keep their number.

## 7 (part). Themed GUI: halo Freeze (2026-11-06)

Proposal F of `architecture.md`: Freeze as a halo (`Components/HaloButton`),
a dim ring off, a glowing breathing ring on; the decay stairs' top edge
glows while frozen, on the same breathing clock. Not built as a plugin yet.

Proposal E (the knob rows as ascending landings on stone slabs, and stone
knob caps with brass rims through a look-and-feel subclass) was built the
same day and reverted at the user's request: the rows are aligned again and
the knobs are FxmeLookAndFeel's.

## 7 (part). Themed GUI: footsteps (2026-11-06)

Revised the same day: the walk now starts on every input onset (the
input's peak, held by the engine until the GUI takes it, 4 dB over its
recent average) rather than on jumps of the tail's energy, which were rare
on continuous material; and the two displays span the whole width, the
floors, knobs and faders under them.

Proposal B of `architecture.md`: `Components/Footsteps`, left of the decay
stairs, draws the early reflections as footprints climbing a flight (time
to the right, size the level, foot by pan, darker when duller, the
pre-delay as the gap before the first step), lit in turn when a sound
starts. The engine gained `earlyReflectionsFor`, `erTimeScaleFor` and
`erLevelFor` (also used by its own processing, so the drawing and the sound
share one computation), and a test that the first reflection drawn is heard
within 0.3 ms of its place (it is within 0.1 ms in every mode). Engine tests
pass; not built as a plugin yet.

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
