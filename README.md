# Stairwell to Heaven

An algorithmic stereo reverb by [FX-Mechanics](https://fx-mechanics.com):
early reflections, then a diffuse tail from a figure-eight tank (after Jon
Dattorro's), whose decay is set in seconds and shaped across frequency.

- Five floors (modes): **Room**, **Stairwell** (hard parallel walls: a
  fluttering, ringing onset), **Plate**, **Hall**, and **Heaven**, a large
  hall whose tail climbs in pitch (shimmer: any interval from two octaves
  down to two octaves up, a second voice to climb in a chord, and Pitched
  input to take the original pitch out of the reverb, down to hearing the
  shimmer alone).
- A calibrated decay: Decay is the real RT60 in seconds; Damping makes the
  highs die faster above its frequency, Bass makes the lows ring longer or
  shorter under its crossover. The decay stairs show the result, one step
  per third of an octave.
- Size, Diffusion, modulation (rate, depth), Early/Late balance, Width, low
  and high cut on the reverb only, Mix and Gain, input and output meters.
- Pre-delay in milliseconds or as a note value at the host tempo.
- Freeze: holds the tail for ever, play over it.
- Twelve factory presets, your own in the preset bar.
- VST3, AU (macOS), CLAP and a standalone application; Linux, Windows,
  macOS (universal).

Status: in development (0.1.0, not released yet).

## Installing

Download the zip for your platform from the
[releases page](https://github.com/odoare/StairwellToHeaven/releases) and copy
the plugin into the usual folder:

| | VST3 | AU | CLAP |
|---|---|---|---|
| Linux | `~/.vst3/` | | `~/.clap/` |
| Windows | `C:\Program Files\Common Files\VST3\` | | `C:\Program Files\Common Files\CLAP\` |
| macOS | `/Library/Audio/Plug-Ins/VST3/` | `/Library/Audio/Plug-Ins/Components/` | `/Library/Audio/Plug-Ins/CLAP/` |

Then rescan in your DAW (it caches the plugin list).

### macOS: one extra step

Stairwell to Heaven is free software and is **not signed with an Apple
Developer ID** (a paid subscription). macOS tags anything downloaded through
a browser as untrusted and refuses to load it: depending on the macOS version
and the host, the plugin does not appear in the scan at all, or macOS says it
"could not verify" it is free of malware. That is the standard warning for any
unsigned download, not a detection.

Clear the tag after copying the files, in Terminal:

```sh
xattr -dr com.apple.quarantine /Library/Audio/Plug-Ins/VST3/StairwellToHeaven.vst3
xattr -dr com.apple.quarantine /Library/Audio/Plug-Ins/Components/StairwellToHeaven.component
xattr -dr com.apple.quarantine /Library/Audio/Plug-Ins/CLAP/StairwellToHeaven.clap
xattr -dr com.apple.quarantine /Applications/StairwellToHeaven.app
```

(Run only the lines for the parts you installed.) Then rescan.

If macOS refused to open the `.pkg` installer: try to open it once, then go
to **System Settings → Privacy & Security** and click **Open Anyway** next to
the message about it.

The macOS build is a universal binary (Apple Silicon and Intel), from macOS
10.13 up.

## Building

Needs CMake 3.22+ and a C++17 compiler (and on Linux the usual JUCE
dependencies: ALSA, X11, freetype, fontconfig, GL, libxi, libegl).

JUCE, [FxmeTools](https://github.com/odoare/FxmeTools) and
clap-juce-extensions are taken from sibling clones (`../JUCE`,
`../FxmeTools`, `../clap-juce-extensions`) when present, else downloaded at
the commits pinned in `cmake/Dependencies.cmake`. For a release build, use
the pins:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DFXME_USE_SIBLINGS=OFF
cmake --build build --target StairwellToHeaven_VST3 -j2
```

Targets: `StairwellToHeaven_VST3`, `_AU`, `_CLAP`, `_Standalone`, `_All`.
Engine tests (headless): configure with `-DSTH_BUILD_TESTS=ON`, build
`StairwellEngineTests`, run `ctest --test-dir build`.

The version is the latest `vX.Y.Z` git tag's, everywhere (the window, the
hosts, the macOS installer): `0.1.0` built on the tag `v0.1.0`,
`0.1.0-3-gabc1234` three commits after it, `0.0.0-dev` before any tag.
`-DSTH_VERSION=X.Y.Z` overrides it. To release, tag and push the tag:

```sh
git tag v0.1.0 && git push origin v0.1.0
```

How it works, the parameters and the plan: `doc/architecture.md`.

## Licence

The plugin is free software under the GNU AGPL version 3 or later, or under
commercial terms available from the author (see `LICENSE.md`). The DSP it is
built on (FxmeTools' core: the tank, the early reflections) is LGPL.

The tank follows Jon Dattorro, "Effect Design, Part 1: Reverberator and
Other Filters", J. Audio Eng. Soc. 45 (9), 1997.
