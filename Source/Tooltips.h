/*
  ------------------------------------------------------------------------------
    Tooltips.h

    Every hover-help string, in one place (no tooltip literal anywhere else).
    Shown unless the "?" switch is off (AppSettings.h).

    Author: Olivier Doaré, github.com/odoare
    (c) 2026 Olivier Doaré
    SPDX-License-Identifier: AGPL-3.0-or-later OR LicenseRef-FXME-Commercial
  ------------------------------------------------------------------------------
*/

#pragma once

namespace sth::tips::space
{
    inline constexpr auto decay     = "Decay. \n How long the tail takes to fall by 60 dB at mid frequencies (RT60), in seconds.";
    inline constexpr auto size      = "Size. \n How big the space is, within the mode's range: longer delays, a slower build-up, later reflections.";
    inline constexpr auto predelay  = "Pre-delay. \n Time before the reverb starts, so it stands apart from the dry attack.";
    inline constexpr auto sync      = "Sync. \n Pre-delay as a note value at the host's tempo instead of milliseconds.";
    inline constexpr auto division  = "Pre-delay note value, when synced (T: triplet, dot: dotted).";
    inline constexpr auto diffusion = "Diffusion. \n How quickly the echoes blur into a smooth tail. Low values keep a grainy, echoey onset.";
}

namespace sth::tips::tone
{
    inline constexpr auto damping   = "Damping. \n How much faster the highs die than the mids, as air absorbs them. 0: they last as long.";
    inline constexpr auto dampFreq  = "Damping frequency. \n Above it, the highs decay faster (by the Damping amount).";
    inline constexpr auto bass      = "Bass. \n Decay of the lows against the mids: above 1 the lows ring longer (a big stone hall), below 1 they die sooner.";
    inline constexpr auto crossover = "Crossover. \n Under it, Bass sets the decay.";
    inline constexpr auto lowCut    = "Low cut. \n High-pass on the reverb only, to keep the tail out of the bass. Off at the bottom.";
    inline constexpr auto highCut   = "High cut. \n Low-pass on the reverb only. Off at the top.";
}

namespace sth::tips::motion
{
    inline constexpr auto modRate   = "Modulation rate. \n How fast the tail's delays drift, which keeps it from ringing metallic.";
    inline constexpr auto modDepth  = "Modulation depth. \n How far they drift: a little is lush, a lot is a chorus.";
    inline constexpr auto erLate    = "Early / Late. \n Balance between the first reflections (the walls) and the diffuse tail.";
    inline constexpr auto width     = "Width. \n Stereo spread of the reverb: 0 is mono, 150 % wider than the source.";
    inline constexpr auto shimmer   = "Shimmer (Heaven mode). \n How much of the tail is pitched up and fed back, so it climbs as it rings.";
    inline constexpr auto pitch     = "Shimmer interval (Heaven mode).";
}

namespace sth::tips::global
{
    inline constexpr auto mode      = "Mode. \n Each floor is a kind of space: Room, Stairwell (hard parallel walls, a fluttering onset), "
                                      "Plate, Hall, and Heaven (a large hall whose tail climbs in pitch).";
    inline constexpr auto freeze    = "Freeze. \n Holds the tail for ever and stops taking new sound in. Play over it.";
    inline constexpr auto mix       = "Mix. \n Balance between the dry signal and the reverb.";
    inline constexpr auto output    = "Output level.";
    inline constexpr auto stairs    = "The decay stairs: one step per third of an octave, its height the time the tail lasts there. "
                                      "Damping lowers the steps on the right, Bass raises or lowers them on the left. "
                                      "The light at the top is the tail itself.";
    inline constexpr auto footsteps = "The early reflections as footsteps up the first flight: time to the right, one footprint "
                                      "per reflection (its size the level, left or right foot by its side, darker when duller). "
                                      "The gap before the first is the pre-delay. They light up in turn when a sound starts.";
    inline constexpr auto floorIndicator = "The floor sounding now. An arrow lights while the reverb travels to another "
                                           "floor (it fades out, switches and fades back in: the tail is cleared).";
    inline constexpr auto tooltips  = "Hover help on the controls: on or off.";
}
