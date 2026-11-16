# Stairwell to Heaven: architecture

An algorithmic stereo reverb by FX-Mechanics: early reflections, then a
diffuse tail from Dattorro's figure-eight tank, with a calibrated decay
shaped across frequency, five voicings ("floors"), freeze, shimmer and a
tempo-synced pre-delay.

The display name is "Stairwell to Heaven"; the product, the bundle, the
preset folder and the settings file are "StairwellToHeaven".

## Status (2026-11-06)

- Written, not yet compiled as a plugin. The DSP is: the FxmeTools core
  classes and the engine are compiled and tested headless (all checks pass,
  see Verification). The project configures (CMake, siblings).
- Waiting on FxmeTools: `dsp/TankReverb.h` and `dsp/EarlyReflections.h` are
  in the local FxmeTools clone, not pushed. The pin in
  `cmake/Dependencies.cmake` still names the commit before them, so a
  pinned build (`-DFXME_USE_SIBLINGS=OFF`, CI) fails until FxmeTools is
  pushed and the pin bumped (todo 1).
- Not yet: plugin build, host test, CPU measurement, screenshot, real
  artwork (the icon and splash are placeholders drawn by a script).

## Plan

The plan comes from FxmeFX's `todo.md` (section 4, "Advanced algorithmic
reverb"). Its phases, and where each stands:

| Phase | Content | Status |
|---|---|---|
| 0 | Name, repository, topology, internal rate | Done: StairwellToHeaven; Dattorro tank (FDN left for later, todo 4); host rate, lengths scaled from 29761 Hz |
| 1 | The late tank in FxmeTools core | Done: `fxme::TankReverb`, tested |
| 2 | Calibrated decay: RT60, high damping, bass multiplier | Done in the tank; verified by T30 on impulse responses and `rt60At()`. A per-octave measurement report is todo 3 |
| 3 | Early reflections | Done: `fxme::EarlyReflections`, four patterns, ER into the tank per mode, Early/Late balance. Patterns from `RoomAcoustics` (image source) are todo 6 |
| 4 | Modes | Done: Room, Stairwell, Plate, Hall, Heaven (shimmer). A mode change crossfades two spaces: the old tail fades out over 1 s while the new mode takes the input |
| 5 | Freeze | Done, in the tank (ramped, safety limiter) |
| 6 | Pre-delay sync | Done (`fxme::DelayTimeResolver`, 15 note values) |
| 7 | GUI, presets, docs | GUI and presets done (to be seen built); docs here. CPU, release: todo 2 and 9 |

## Repository and build

```
StairwellToHeaven/
  CMakeLists.txt            the plugin (AU, VST3, Standalone, CLAP), the engine test
  cmake/FxmeDeps.cmake      sibling-or-pin resolution (verbatim copy from FxmeFX)
  cmake/Dependencies.cmake  pins: JUCE 9.0.4, FxmeTools, clap-juce-extensions
  Source/
    PluginProcessor.*       APVTS, PresetManager, glue to the engine
    PluginEditor.*          the window (shell + controls)
    ParamIDs.h              parameter IDs and choice lists
    Theme.h                 colours, sizes, control styling
    Tooltips.h              every hover-help string
    AppSettings.h           machine-wide settings (the "?" switch)
    Dsp/StairwellEngine.*   the signal path, JUCE-free (FxmeCore only)
    Components/DecayStairs.*    the decay display
    Components/FloorSelector.*  the mode selector (lift panel)
    Components/FloorIndicator.* the lift's floor display above it
    Components/ReflectionsView.* the early reflections (time up, pan across)
    Components/HaloButton.*     Freeze, as a halo
    Assets/                 logo, icon, splash, factory presets (*.xml)
  Tests/EngineTests.cpp     headless engine tests (FxmeCore only)
  tools/make_presets.py     writes the factory presets
  doc/                      this file, todo.md, done.md
  .github/workflows/release.yml
```

Dependencies are sibling clones (`../JUCE`, `../FxmeTools`,
`../clap-juce-extensions`) when present, else the pinned commits; release
builds use `-DFXME_USE_SIBLINGS=OFF`. Build (one target, `-j2`):

```
cmake -S . -B build -DSTH_BUILD_TESTS=ON
cmake --build build --target StairwellToHeaven_VST3 -j2
cmake --build build --target StairwellEngineTests -j2 && ctest --test-dir build
```

Plugin type: audio effect (`IS_SYNTH FALSE`, no MIDI, `kAudioUnitType_Effect`,
VST3 `Fx Reverb`), code `STWH`, CLAP ID `com.fx-mechanics.StairwellToHeaven`
(permanent once released), CLAP features `audio-effect reverb stereo`.
Buses: stereo out; stereo in, or mono in feeding both sides.

## Where the code lives

| Piece | Where | Why |
|---|---|---|
| `fxme::TankReverb` | FxmeTools `core/FxmeTools/dsp/TankReverb.h` | generic, JUCE-free; a tape delay's reverb or another plugin can reuse it |
| `fxme::EarlyReflections` | FxmeTools `core/FxmeTools/dsp/EarlyReflections.h` | same |
| their tests | FxmeTools `core/tests/CoreReverbTests.cpp` | |
| `sth::StairwellEngine` (modes, pre-delay, mix) | `Source/Dsp/` | specific to this plugin, but JUCE-free so it is tested headless |
| processor, editor, components | `Source/` | the JUCE shell |

Also from FxmeTools: `ModDelayLine` (every line of the tank, the ER and the
pre-delay), `SplicePitchShifter` (shimmer), `TptSvf` (the wet low and high
cuts), `DelayTimeResolver` (synced pre-delay), `Sanitise`, and on the JUCE
side `TopBar`, `PresetBarComponent`, `PresetManager`, `SplashOverlay`,
`InfoButton`, `AccentToggle`, `FxmeSlider`, `FxmeLookAndFeel`,
`TextEntryFocusFixer`.

## Signal flow

```
in L,R ─┬───────────────────────────────────────────────────────── dry ──┐
        └─ pre-delay (0..500 ms or a note value, glides)                  │
             ├─ early reflections (8 taps, pattern per mode) ── ER ──┐    │
             │        └── x erIntoTank (per mode) ──┐                 │    │
             └────────────────────────────────────── + ─ tank ─ late ─┤    │
                                                                      │    │
        wet = ER x cos(b) + late x sin(b)  (b: Early/Late)            │    │
            -> Width (M/S) -> Low cut, High cut (12 dB/oct)           │    │
out = (dry + Mix (wet - dry)) x Gain  ◄───────────────────────────────┴────┘

After a mode change the old space (its own tank and ER, no more input) is
added into ER and late, times its tail fade, until the fade ends.
```

### The tank (`fxme::TankReverb`)

Dattorro's structure ("Effect Design, Part 1", JAES 1997), lengths at 29761
Hz scaled to the running rate and by Size:

- input: (L+R)/2, one-pole band-limit (per mode), four allpasses (142, 107,
  379, 277; coefficients 0.75, 0.75, 0.625, 0.625 times Diffusion);
- two branches, each fed the diffused input plus the other's output:
  modulated allpass (672 / 908, coefficient -decayDiffusion1), delay
  (4453 / 4217), absorption, allpass (1800 / 2656, decayDiffusion2), delay
  (3720 / 3163);
- output: Dattorro's seven signed taps per side, times 0.6.

**Calibrated decay.** Each branch applies, once, the gain a recirculation
over its total length L needs to fall 60 dB in RT60: g = 10^(-3 L / (RT60
fs)). Two first-order shelves around it make the loss depend on frequency:
a low shelf at the crossover with ratio gLow/g (gLow for RT60 x Bass) and a
high shelf at the damping frequency with ratio gHigh/g (gHigh for RT60 x
hfRatio, hfRatio = 1 - 0.9 Damping). Each shelf's magnitude stays between 1
and its ratio, so the branch never exceeds its largest target gain and the
loop is stable for every setting. `rt60At (settings, hz, fs)` computes the
resulting decay at any frequency from the same formulas: the display draws
exactly what the tank does.

**Modulation.** The two modulated allpasses swing by up to 24 samples (at
29761 Hz, so the same time at any rate) x Depth: a sine in quadrature
between the branches plus a slow seeded random drift. Every line is a
`ModDelayLine` (Catmull-Rom), so Size also glides (0.2 s) instead of
clicking.

**Freeze.** Every loss ramps to unity over 50 ms (decay gain and both
shelves), the input to zero, the shimmer to zero: the allpasses are
lossless, so the tail holds (measured: -0.3 dB over 4 s). The early
reflections keep running from the input, so playing over a frozen tail
still sounds in the space. Safety net: a soft limit on each branch's output
(identity to 1, then 1 + tanh (|x| - 1)).

**Shimmer** (Heaven). The tank's output, high-passed at 400 Hz, through
`SplicePitchShifter` (+12 by default, any interval from -24 to +24
semitones), fed back to the tank's input. A second voice (its own shifter
and interval, off by default) climbs alongside, so the tail can rise in a
chord; the two voices share the loop's headroom (their gains sum to at
most one full voice's). Its gain is Shimmer x 0.9 x (1 - loop gain): a long decay
leaves less headroom, so the pitched copies climb without running away
(measured: bounded and decaying at full amount on a 20 s decay, with one
voice and with two).

**Pitched input** (Heaven, in the engine, not the tank). The shimmer lives
in the tank's loop, so the tank's output always holds the reverb at the
original pitch, the climbing copies on top, and nothing after the tank can
take that pitch out again. Pitched input crossfades what enters the early
reflections and the tank between the pre-delayed signal and a pitched copy:
two mono `SplicePitchShifter`s at the voices' intervals, weighted by the
voices' amounts (voice 1 alone when both are at 0). At 100 % the original
pitch is gone from the wet (measured: a 440 Hz sine comes out 68 dB under
its octave), and with Mix at 100 % only the shimmer is heard. The shifters
add about 24 ms to that path only, as extra pre-delay.

**Safety.** Inputs clamped (`sanitiseSample`), a branch output that goes
bad anyway (NaN, beyond +-8) clears the tank.

### Modes (floors)

A mode is a voicing (`StairwellEngine::modeVoicing`), not another engine.
Size (0..100 %) maps onto the mode's range of tank scale.

| Floor | Input diffusion | Decay diffusion | Band-limit | Size range | ER pattern (time range) | ER into tank | Mod | Shimmer |
|---|---|---|---|---|---|---|---|---|
| 1 Room | 0.75 / 0.625 | 0.60 / 0.45 | 9 kHz | 0.30 .. 0.80 | room (0.6 .. 1.4) | 0.35 | x0.5 | |
| 2 Stairwell | 0.70 / 0.55 | 0.50 / 0.40 | 12 kHz | 0.35 .. 0.90 | stairwell flutter (0.7 .. 1.5) | 0.50 | x0.4 | |
| 3 Plate | 0.75 / 0.625 | 0.70 / 0.50 | 14 kHz | 0.50 .. 1.30 | plate, almost none | 0 | x0.8 | |
| 4 Hall | 0.60 / 0.50 | 0.70 / 0.50 | 8 kHz | 0.90 .. 2.20 | hall (0.8 .. 2.0) | 0.20 | x1 | |
| 5 Heaven | 0.60 / 0.50 | 0.70 / 0.50 | 9 kHz | 1.20 .. 2.50 | hall (1.0 .. 2.2) | 0.15 | x1 | yes |

The Stairwell floor is the plugin's namesake: two hard parallel walls give
a regular flutter (a reflection every 8.5 ms at Size 1, alternating sides,
barely darkened), and less decay diffusion keeps the onset ringing.

Changing floors crossfades two spaces (a tank and its ER each,
`StairwellEngine::Space`). The new mode starts in the idle space, cleared
(its lengths jump), and takes the input from then on; the old space stops
taking input and its tail rings on, fading out over 1 s (squared, so it
ends smoothly), or sooner once its loop is under -100 dB over a block. The
tail is kept and nothing clicks; the tank's CPU doubles during the fade
only. A further change while a tail is still fading speeds that fade up
(30 ms) and waits for it, so a tail is never cut. `EngineTests` 6 checks
both (no step bigger than the tail's own, the tail kept within 6 dB, gone
after 1.6 s; a second change 10 ms later).

### Early reflections (`fxme::EarlyReflections`)

Eight taps (time, gain, equal-power pan, a one-pole low-pass darker for
later taps) from one line fed (L+R)/2. Patterns: room (4 to 39 ms,
irregular), hall (10 to 79 ms, darker), plate (two faint taps), stairwell
(regular flutter). Times scale with Size within the mode's range.

## Parameters

IDs are permanent (sessions and presets store them). Version 1 for all.

| ID | Name | Range | Default | Notes |
|---|---|---|---|---|
| `mode` | Mode | Room, Stairwell, Plate, Hall, Heaven | Hall | the floor |
| `decay` | Decay | 0.1 .. 30 s (skewed) | 2.5 s | mid RT60 |
| `size` | Size | 0 .. 100 % | 50 % | within the mode's range |
| `predelay` | Pre-delay | 0 .. 500 ms | 20 ms | when not synced |
| `predelaySync` | Pre-delay sync | off / on | off | |
| `predelayDiv` | Pre-delay note | 1/64 .. 1/2 (T, dotted) | 1/16 | when synced; capped at 2 s |
| `diffusion` | Diffusion | 0 .. 100 % | 80 % | input diffusers |
| `damping` | Damping | 0 .. 100 % | 40 % | hfRatio = 1 - 0.9 x |
| `dampFreq` | Damping frequency | 1 .. 16 kHz (log knob) | 5 kHz | |
| `bass` | Bass | 0.5 .. 2.5 x | 1.2 x | low RT60 / mid RT60 |
| `crossover` | Crossover | 100 .. 1000 Hz (log knob) | 300 Hz | |
| `modRate` | Modulation rate | 0.05 .. 5 Hz | 0.7 Hz | |
| `modDepth` | Modulation depth | 0 .. 100 % | 30 % | scaled per mode |
| `erLate` | Early / Late | 0 .. 100 % | 60 % | equal power |
| `width` | Width | 0 .. 150 % | 100 % | M/S on the wet |
| `lowCut` | Low cut | 20 .. 1000 Hz | 20 (off) | wet only |
| `highCut` | High cut | 1 .. 20 kHz | 20 kHz (off) | wet only |
| `shimmer` | Shimmer 1 | 0 .. 100 % | 50 % | Heaven only |
| `shimmerPitch` | Shimmer interval | -24 .. +24 semitones (index = semitones + 24) | +12 | Heaven only |
| `shimmer2` | Shimmer 2 | 0 .. 100 % | 0 % (off) | Heaven only, the second voice |
| `shimmerPitch2` | Shimmer 2 interval | -24 .. +24 semitones | +7 | Heaven only |
| `pitchedInput` | Pitched input | 0 .. 100 % | 0 % | Heaven only: the reverb's input shifted by the voices' intervals |
| `freeze` | Freeze | off / on | off | |
| `mix` | Mix | 0 .. 100 % | 30 % | |
| `output` | Output | -24 .. 24 dB | 0 dB | |

## GUI

### As built

- **The shell** (house standard): `fxme::TopBar` with the title (the
  lettered `title.png`, `TopBar::setTitleImage`), version and
  the FX-Mechanics link, the preset bar with its browser ("..."), the "?"
  (hover help on or off, machine-wide in `AppSettings.h`) and the info
  button (help text); the icon in the bar and the logo open the splash; the
  splash shows for 3 s the first time an instance opens its window.
  Tooltips all in `Tooltips.h`. One `TextEntryFocusFixer`. Look-and-feel set
  on the editor (combos inherit it).
- **The floor's colour**: the accent follows the floor
  (`theme::accentForFloor`): pale gold in the Room, toward salmon up to the
  Hall, blue in Heaven. On a floor change the editor glides it over about a
  quarter of a second and restyles everything with it (`applyAccent`: the
  look-and-feel, top bar, preset bar, knobs, faders, meters, the two
  displays, the window's light). The HEAVEN group keeps Heaven's pale blue.
- **The floors** (left, `FloorSelector`): the mode as a lift's button panel,
  Room on the ground floor up to Heaven at the top (each button lit in its
  floor's colour). One click, one undoable gesture; follows automation.
- **The floor indicator** (above the floors, `FloorIndicator`): the lift's
  display, the floor sounding now ("1" to "4", an infinity sign for Heaven)
  in amber over the dim ghost of its segments. An up or down arrow blinks
  while a mode change travels (at least 0.6 s, and as long as the old
  tail fades), and the display flashes once on arrival. It shows the
  engine's floor (`StairwellEngine::getActiveFloor`, with the fade level),
  not the parameter's, so it tells the truth during the switch.
- **Freeze** under the floors, as a halo (`HaloButton`): a ring of light
  over the word, a dim stone ring when off; on, it glows pale ice blue in
  soft layers and breathes slowly (about 4 s), on the same clock as the
  decay stairs' frozen edge, so the two breathe together. A latching
  `juce::Button` bound by a ButtonAttachment; its breathing timer runs only
  while on.
- **The layout**: the two displays (early reflections, then the decay stairs) across
  the whole width under the top bar; under them, the floors on the left, the
  knob rows in the centre, the faders on the right.
- **The early reflections** (top, left of the stairs, `ReflectionsView`):
  time runs up, from the dry sound at the bottom (a small dot, centred) to a
  round number of milliseconds past the last reflection (labelled on the
  left). Each reflection is a circle placed across by its pan (L to R, as it
  is heard): its size the level, darker the more it is low-passed; the
  whole picture fades as Early/Late moves toward the tail. The pre-delay is
  a dashed stretch up the centre before the first one. On every onset of the
  input (its peak 4 dB over its own recent average, and above -50 dB), the
  circles light up in turn from the bottom over half a second (slowed down:
  the real ones take tens of milliseconds); a new onset restarts the sweep
  once it is mostly done. The engine holds the input's peak until the GUI
  takes it (`takeInputPeak`), so no hit between two timer ticks is missed.
  The Stairwell's flutter reads as an even ladder swinging left and right,
  the Room's as short and irregular, the Hall's as tall and sparse, the
  Plate's as two faint circles near the bottom. The taps come from
  `StairwellEngine::earlyReflectionsFor`, the ones the engine plays; an
  engine test checks that the first one drawn is heard within 0.3 ms of
  where it is drawn.
- **The decay stairs** (top centre, `DecayStairs`): RT60 against frequency as
  a staircase, one step per third of an octave, stone at the bottom turning
  gold as it rises; each tread lit. Damping lowers the right of the flight,
  Bass the left, Decay the whole. Above it, the light: a glow that follows
  the tail's energy as it plays (blue-white in Heaven), a steady pale halo
  when frozen, when every step runs off the top and the flight's top edge
  glows, breathing with the Freeze halo. A read-out gives the RT60 at
  1 kHz ("Frozen" with an infinity sign when frozen).
- **Three rows of knobs**, six columns each, named on their left: Space
  (Decay, Size, Pre-delay, Sync with its note under it, Diffusion,
  Early/Late), Tone (Damping, Damp freq, Bass, Crossover, Low cut, High
  cut), Motion (Mod rate, Mod depth, Width, then the framed HEAVEN group
  over three columns: Shimmer 1 and Shimmer 2, each with its interval menu
  under the knob, and Pitched in; the frame is lit in Heaven's colour on the
  top floor, dim elsewhere). Disabled when they do nothing
  (the pre-delay knob while synced, the note while not, the shimmer outside
  Heaven).
- **Levels** on the right: the input meter, Mix and Gain (the Output
  parameter) as console faders, the output meter, each named above.
  The meters are `fxme::StereoLevelMeter`s (RMS bar, peak held a second)
  fed from the engine's `fxme::VuMeter`s (300 ms window); the editor takes
  each held max (read and reset) on its timer.
- **The backdrop**: stone at the bottom of the window, a warm light from the
  top whose strength follows the tail's energy (-60 dB dark to 0 dB full,
  rising fast, falling slowly): the window brightens as the reverb blooms and
  dims as it dies. Frozen it stays lit (pale blue), in Heaven it turns pale
  blue-white. The light stops above the knob rows and is held at 28 %
  opacity, so the controls stay readable. Driven by the editor's 20 Hz timer,
  repainting only when the level moves.

### Proposals around the name (not built; to choose)

Ideas that keep the pun on the plugin's name to the name itself: nothing
borrowed from the famous song or its band (no imagery, lettering, lyrics or
symbols), which would be a copyright and trademark risk as well as a cliché.

- **A. The floor indicator.** (Built, see above.) A small segment display above the lift panel
  showing the floor ("1" to "4", an infinity sign for Heaven), with an up or
  down arrow lit while a mode change fades. Cheap, and it makes the mode
  change (which clears the tail) visible.
- **B. Footsteps on the stairs.** (Built, then replaced by the early reflections view above: circles, time up, pan across.) The early reflections drawn as marks on the
  first steps (time to the right, level as size): the Stairwell floor's
  flutter reads as regular footsteps, the Hall's as sparse ones. Needs the
  ER pattern and time scale exposed to the display (the core already has
  `EarlyReflections::getTaps`).
- **C. The window lights up.** (Built, see above.) The backdrop's warm light follows the tail's
  energy (already metered): the whole plugin brightens as the reverb blooms
  and dims as it dies, frozen it stays lit. Subtle, but the strongest
  "heaven" cue for little code; needs care to stay readable.
- **D. Rising sparkles in Heaven.** Small points of light drifting up the
  stairs, their number following the Shimmer amount and their pitch steps
  following the interval (an octave climbs twice as fast as a fifth).
- **E. Ascending layout.** (Tried both ways on 2026-11-06, landings and stone and brass knobs, and reverted.) The three knob rows offset to the right as they
  go down (each row a landing), or the knob caps drawn as stone with brass
  rims. A bigger change; worth a mock-up before code.
- **F. A halo Freeze.** (Built.) The Freeze button as a ring of light rather than a
  rectangle, with the decay stairs' top edge glowing while frozen.
- **G. Real artwork** for the splash and the icon (todo 8): the
  placeholders are a gold staircase climbing into light, drawn by a script.

A, B, C and F were chosen and built (2026-11-06), B reworked into the early reflections view; E was tried and reverted. Left: D.

## Presets

Global presets with `fxme::PresetManager` in
`FX-Mechanics/StairwellToHeaven/Presets`; factory presets embedded from
`Source/Assets/*.xml` (complete APVTS states, written by
`tools/make_presets.py`): Init, Small Room, Concrete Stairwell, Vocal Plate,
Big Hall, Stone Cathedral, Heavens Door, Fifth Heaven, Choir Loft (an
octave and a fifth), Undertow (an octave and a fourth down), Only Heaven
(Pitched input at 100 %, Mix at 100 %: the shimmer alone), Synced
Slapback Room. No module presets (the plugin has no embeddable part).

## FxmeNet

The plugin is FxmeNet-aware (`FxmeTools/doc/fxmenet-howto.md`): its DSP can
run on an FxmeNet server while its editor runs in the DAW (a mirror that
never processes). The processor holds a `fxme::net::NetBridge` (declared
last, returned by `getVST3ClientExtensions()`) and registers what the
editor reads that is not a parameter:

| Name | Kind | What | Read by |
|---|---|---|---|
| `tail` | value (`atomic<float>`) | the tank's energy, dB | the decay stairs' light, the window's light |
| `fade` | value (`atomic<float>`) | the mode-change crossfade (1 settled) | the floor indicator |
| `inL`, `inR`, `outL`, `outR` | meter (`fxme::VuMeter`) | input and output levels | the level meters (written into the same meters on a mirror) |
| `floor` | scalar | the floor sounding | the floor indicator (`getActiveFloor` answers from telemetry on a mirror) |
| `input` | peak (read and reset) | the input's peak since last asked | the reflections view's sweep (`takeInputPeakDb` takes it from telemetry on a mirror) |

Everything else the editor shows comes from the parameters (synced by
FxmeNet), including the decay stairs' curve and the reflections' places.
On a mirror `processBlock` returns at once (so a stray call cannot
overwrite the telemetry), and `getTailLengthSeconds` computes from the
parameters rather than from the audio thread. There is no non-parameter
state, so `notifyStateChanged()` is never needed. The names are stable:
renaming one breaks that channel between versions. Without FxmeNet nothing
changes. Check: FxmeNet's `FxmeNetLoopback path/to/StairwellToHeaven.vst3`
(source and mirror side by side; the mirror's floor, light and sweep must
follow the source's).

## Threading and state

- Audio thread: `processBlock` reads the parameters (raw atomics) into one
  `sth::Parameters`, resolves a synced pre-delay against the host tempo,
  and runs the engine. No allocation, no locks.
- Message thread: the editor's components poll on timers (the reflections
  view takes the input's held peak, an atomic exchange); the decay stairs
  call `currentParameters()` (atomics) and `TankReverb::rt60At()`, and read
  the engine's tail energy (an atomic); the floor indicator reads the
  engine's active floor and mode fade (atomics); the editor reads the tail
  energy for the window's light.
- State: the APVTS tree as XML with a `stateVersion` attribute (1).
- Latency: none. Tail length reported: pre-delay + Decay x max (1, Bass),
  x1.5 in Heaven.

## Verification

- FxmeTools `CoreReverbTests` (21 checks): finite and decaying impulse
  response; T30 within 1 to 12 % of the RT60 setting (1, 2.5, 6 s, sizes 1
  and 2); `rt60At` mid / high / low against RT60, x hfRatio, x Bass; bad
  input; freeze holds (-0.3 dB over 4 s) and releases; shimmer bounded at
  full amount on 20 s; bit-exact reset; ER timing and the stairwell's
  regularity.
- `Tests/EngineTests.cpp` (12 checks; the last: the reflections drawn are the
  ones heard): Mix 0 is the dry signal exactly;
  silence; every mode finite, audible, decaying; nothing before the
  pre-delay; Size within each mode's range; shimmer only in Heaven; a mode
  change mid-tail without a click (largest step smaller than the tail's
  own) and through silence; freeze holds through the engine (-0.9 dB over
  4 s).
- Cost: the tank runs at about 5 % of a core at 48 kHz (all core tests
  together render about 70 s of audio in 3.5 s).
- Not yet: plugin build, host test, CPU of the whole plugin at 48 and 96 kHz.
