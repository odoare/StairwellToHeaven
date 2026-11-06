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
    inline constexpr auto shimmer2      = "shimmer2";
    inline constexpr auto shimmerPitch2 = "shimmerPitch2";
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

    /** The shimmer voices' intervals: every semitone from -24 to +24, the
        musical ones named ("+7 fifth", "-12 octave down"). The index is the
        interval plus 24. */
    inline constexpr int minShimmerSemitones = -24, maxShimmerSemitones = 24;

    inline juce::String shimmerPitchName (int semitones)
    {
        static const char* const names[] { "unison", "minor 2nd", "major 2nd", "minor 3rd", "major 3rd", "fourth",
                                           "tritone", "fifth", "minor 6th", "major 6th", "minor 7th", "major 7th" };
        const int a = std::abs (semitones), octaves = a / 12, step = a % 12;
        juce::String name;
        if (step == 0)
            name = octaves == 0 ? "unison" : (octaves == 1 ? "octave" : "two octaves");
        else
            name = (octaves > 0 ? "oct. + " : "") + juce::String (names[step]);
        const auto sign = semitones > 0 ? "+" : (semitones < 0 ? "-" : "");
        return juce::String (sign) + juce::String (a) + " " + name + (semitones < 0 ? " down" : "");
    }

    inline const juce::StringArray shimmerPitches = []
    {
        juce::StringArray list;
        for (int st = minShimmerSemitones; st <= maxShimmerSemitones; ++st)
            list.add (shimmerPitchName (st));
        return list;
    }();

    inline float shimmerSemitonesFor (int index) { return (float) (minShimmerSemitones + index); }
    inline constexpr int pitchIndexFor (int semitones) { return semitones - minShimmerSemitones; }
    inline constexpr int defaultShimmerPitch  = pitchIndexFor (12);   // an octave up
    inline constexpr int defaultShimmerPitch2 = pitchIndexFor (7);    // a fifth up
}
