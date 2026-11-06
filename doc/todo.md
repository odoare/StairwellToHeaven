# Stairwell to Heaven: todo

Numbered; an item keeps its number when it moves to `done.md`. Priority:
high (blocks a release), medium, low.

1. **Bump the FxmeTools pin** (high). `cmake/Dependencies.cmake` pins the
   FxmeTools commit before `dsp/TankReverb.h` and `dsp/EarlyReflections.h`.
   Once FxmeTools is pushed, pin that commit; CI and pinned builds fail
   until then.
2. **Build, host test, CPU** (high). Build the VST3 and the CLAP, load them
   in Reaper, check every floor, Freeze, Sync, presets, the splash and the
   "?" switch. Measure the CPU of the whole plugin at 48 and 96 kHz.
3. **Per-octave RT60 report** (medium). A harness in `doc/` (as FxmeFX's
   `doc/Tape/tape_measure.cpp`, on the core headers) measuring the decay per
   octave band against `rt60At()` for a few Damping and Bass settings, with
   its results committed. FxmeTools.
4. **An FDN tank** (low, open question). An 8-line FDN (Householder) behind
   the same interface, for rooms and a calibrated RT60 at any size, chosen
   per mode. FxmeTools core.
5. **A true crossfade on mode change** (low). Two tanks, the old one fading
   out while the new one fades in, instead of clearing the tail. Twice the
   tank's CPU during the fade.
6. **ER from `RoomAcoustics`** (low). Early reflection patterns from the
   image-source model (FxmeTools `acoustics/RoomAcoustics.h`) instead of
   fixed tables, scaled by Size.
7. **Themed GUI** (medium). A (floor indicator), C (the window lights up)
   and F (halo Freeze) are built, B became the early reflections view
   (circles, time up, pan across); E (ascending landings, stone and brass
   knobs) was tried and reverted. Left: D (sparkles in Heaven);
   see `architecture.md`, GUI, "Proposals around the name".
8. **Artwork** (medium). A real splash and icon (the current ones are
   placeholders drawn by a script), then a screenshot for the README.
9. **First release** (medium). After 1, 2 and 8: README checked, tag
   `v0.1.0`, `release.yml` on the three OSes.
10. **FxmeNet** (low). A `fxme::net::NetBridge` like FxmeFX's processors
    (telemetry: the tail energy), if the plugin is to be remote-controlled.
11. **Stereo into the tank** (low). The tank takes (L+R)/2, as Dattorro's;
    feeding L and R into the two branches would keep more of the source's
    image in the tail. Changes the sound: a voicing option, not a default.
