/*
  ------------------------------------------------------------------------------
    ParamIDs.h

    Every parameter: its ID (permanent once released: sessions and presets
    store it), and the choice lists. The layout itself is in
    PluginProcessor.cpp (createParameters).

    Author: Olivier Doaré, github.com/odoare
    (c) 2026 Olivier Doaré
    SPDX-License-Identifier: AGPL-3.0-or-later OR LicenseRef-FXME-Commercial
  ------------------------------------------------------------------------------
*/

#pragma once

#include <JuceHeader.h>

namespace sth::ids
{
    inline constexpr auto mode          = "mode";
    inline constexpr auto decay         = "decay";
    inline constexpr auto size          = "size";
    inline constexpr auto predelay      = "predelay";
    inline constexpr auto predelaySync  = "predelaySync";
    inline constexpr auto predelayDiv   = "predelayDiv";
    inline constexpr auto diffusion     = "diffusion";
    inline constexpr auto damping       = "damping";
    inline constexpr auto dampFreq      = "dampFreq";
    inline constexpr auto bass          = "bass";
    inline constexpr auto crossover     = "crossover";
    inline constexpr auto modRate       = "modRate";
    inline constexpr auto modDepth      = "modDepth";
    inline constexpr auto erLate        = "erLate";
    inline constexpr auto width         = "width";
    inline constexpr auto lowCut        = "lowCut";
    inline constexpr auto highCut       = "highCut";
    inline constexpr auto shimmer       = "shimmer";
    inline constexpr auto shimmerPitch  = "shimmerPitch";
    inline constexpr auto freeze        = "freeze";
    inline constexpr auto mix           = "mix";
    inline constexpr auto output        = "output";
}

namespace sth::choices
{
    /** In sth::Mode order. The order only ever extends at the tail. */
    inline const juce::StringArray modes { "Room", "Stairwell", "Plate", "Hall", "Heaven" };

    /** Pre-delay divisions when synced, and their length as a fraction of a
        whole note. */
    inline const juce::StringArray divisions { "1/64", "1/32T", "1/32", "1/16T", "1/32.", "1/16", "1/8T",
                                               "1/16.", "1/8", "1/4T", "1/8.", "1/4", "1/2T", "1/4.", "1/2" };
    inline constexpr float divisionWholeNotes[] { 1.0f / 64, 1.0f / 48, 1.0f / 32, 1.0f / 24, 3.0f / 64, 1.0f / 16,
                                                  1.0f / 12, 3.0f / 32, 1.0f / 8, 1.0f / 6, 3.0f / 16, 1.0f / 4,
                                                  1.0f / 3, 3.0f / 8, 1.0f / 2 };
    inline constexpr int defaultDivision = 5;   // 1/16

    /** The shimmer's interval, and its semitones. */
    inline const juce::StringArray shimmerPitches { "+12 octave", "+7 fifth", "+19 oct. + fifth", "+24 two oct.", "-12 octave down" };
    inline constexpr float shimmerSemitones[] { 12.0f, 7.0f, 19.0f, 24.0f, -12.0f };
}
