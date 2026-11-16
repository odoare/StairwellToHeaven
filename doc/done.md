# Stairwell to Heaven: done

Newest first. Items that came from `todo.md` keep their number.

## Fix: output bursts and a reverb stuck silent (2026-11-16)

Seen in Reaper: a very loud burst (Reaper muted the track), then nothing
from the plugin, meters included, until the instance was reloaded. Two
bugs, both in FxmeTools (not pushed yet: todo 1), found by fuzzing the
engine (random parameters, modes and Freeze, 44.1/48/96 kHz):

- `TankReverb`, Freeze glide: the mid gain and the shelves' ratios blended
  to 1 separately, so their product (the loop gain) passed 1 halfway; with
  a short Decay and Bass up the low shelf's ratio is in the thousands
  (+127 dB in the test). The guard then cleared the tank, and a frozen empty
  tank stays silent. Now each band's gain (mid, low, high) blends to 1 on
  its own, the low shelf is computed without the ratio, and the guard also
  checks the absorption's output (before the lines the taps read fill up).
  `CoreReverbTests` 9.
- `ModDelayLine::read`: a position a hair behind the write pointer rounded
  to one past the buffer's end (a read out of bounds, of whatever lies
  there: up to 1e23, or a NaN that then stayed in a filter). Every line was
  exposed (pre-delay, ER, tank). `CoreReverbTests` 10.

The engine also mutes a wet that goes bad anyway (NaN, past +60 dBFS) and
clears itself at the end of the block. After the fixes the fuzz's worst
output is 2.9 (input up to 1), never non-finite.

## 5, 13, 14, 15, 16. Crossfade, meters, fader names, title, floor colours (2026-11-16)

- 5. A true crossfade on mode change: two spaces (tank and ER each); the
  new mode starts clean in the idle one and takes the input, the old one's
  tail fades out over 1 s (or sooner once it is under -100 dB). A change
  while a tail still fades hurries it (30 ms) and waits. `EngineTests` 6
  rewritten: no click, the tail kept (-1.8 dB 50 ms after the switch), gone
  after 1.6 s, a quick second change without a click.
- 13. Input and output meters (`fxme::StereoLevelMeter`, from four
  `fxme::VuMeter`s in the engine), registered with FxmeNet as `inL`, `inR`,
  `outL`, `outR`.
- 14. "In", "Mix", "Gain" and "Out" above the meters and faders; the Output
  fader is named Gain on screen (the parameter keeps its name and ID).
- 15. The title is `Source/Assets/title.png`, through a new
  `fxme::TopBar::setTitleImage` (FxmeTools, not pushed yet: see todo 1).
- 16. The accent follows the floor: pale gold (Room) to salmon (Hall) to
  blue (Heaven), gliding on a change; every control, both displays, the
  meters and the window's light take it.
- Built (VST3, CLAP, Standalone, Linux) and checked in the standalone on the
  three colours; not yet in a host.

## 8. Artwork (2026-11-16)

The splash and the icon are the real ones (by the author), replacing the
placeholders drawn by a script.

## 10. FxmeNet (2026-11-07)

The processor holds a `fxme::net::NetBridge` and registers the editor's
non-parameter reads: `tail` and `fade` (the engine's atomics), `floor` (a
scalar; `getActiveFloor` answers from telemetry on a mirror) and `input` (a
peak; `takeInputPeakDb` takes it from telemetry on a mirror). On a mirror
`processBlock` returns at once; `getTailLengthSeconds` now computes from the
parameters (right on a mirror, which never processes). The editor is
unchanged. Not built yet; to check with FxmeNet's `FxmeNetLoopback`.

## Knob rows rebalanced, a HEAVEN group (2026-11-06)

Motion had seven knobs and a column of two menus. Now every row has six
columns: Early/Late moved into Space's empty cell; Motion keeps Mod rate,
Mod depth and Width, then a framed HEAVEN group (Shimmer 1 and 2, each with
its interval menu under it, and Pitched in), lit on the top floor and dim
elsewhere. Not built as a plugin yet.

## Pitched input: the shimmer alone (2026-11-06)

Heaven gained Pitched input (0 to 100 %): the reverb's input (early
reflections and tank) crossfaded with a pitched copy, two input
`SplicePitchShifter`s at the two voices' intervals weighted by their
amounts. At 100 % the original pitch leaves the wet entirely; with Mix at
100 % only the shimmer is heard. `EngineTests` 10: a 440 Hz sine at +12
comes out with 440 Hz 68 dB under 880 Hz at 100 %, and 66 dB over it at 0 %
(the plain path untouched). A new preset, Only Heaven. A seventh knob in
Motion. Not built as a plugin yet.

## Shimmer: every interval, a second voice; reflections follow Width (2026-11-06)

- The shimmer interval is any semitone from -24 to +24 (49 choices, the
  musical ones named), instead of five.
- A second shimmer voice (Shimmer 2, its own interval, off by default):
  `fxme::TankReverb` gained `shimmer2` / `shimmerSemitones2` (a second
  `SplicePitchShifter`); the two voices share the loop's headroom, and
  `CoreReverbTests` checks both at full stay bounded and decay. In the GUI:
  a second knob, the two interval menus stacked in Motion's last column.
- Presets regenerated (interval indices changed; nothing released, so no
  migration), two new ones using both voices: Choir Loft, Undertow.
- The early reflections view places each circle where it is heard after
  Width (`StairwellEngine::panAfterWidth`, checked against the engine's
  output in `EngineTests` 9).
- Not built as a plugin yet. MIDI-driven intervals: todo 12.

## 7 (part). Early reflections view replaces the footsteps (2026-11-06)

The footsteps did not convince: `Components/Footsteps` became
`Components/ReflectionsView`. Time now runs from the bottom (the dry sound)
to the top, each reflection a circle placed left or right by its pan, sized
by its level, darker when duller; the pre-delay a dashed stretch up the
centre. The sweep on input onsets is kept as it was (now bottom to top).
Not built as a plugin yet.

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
