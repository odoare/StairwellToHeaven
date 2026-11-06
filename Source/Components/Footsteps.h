/*
  ------------------------------------------------------------------------------
    Footsteps.h

    The early reflections as footsteps climbing the first flight, left of the
    decay stairs: time runs to the right (from the dry sound to the last
    reflection), the path climbs as it goes. Each reflection is a footprint:
    its size is its level, left or right foot by its pan, darker the more it
    is low-passed; the whole walk fades as Early/Late moves toward the tail.
    The pre-delay is the stretch before the first step.

    So the floors read at a glance: the Stairwell's flutter is an even,
    regular stride; the Room's, short and irregular; the Hall's, long and
    sparse; the Plate's, two faint steps.

    On every onset of the input (a note, a hit: its peak 4 dB over its own
    recent average), the footprints light up one after another, in order, at
    a pace slowed down to be seen (the real ones take a few tens of
    milliseconds). A new onset restarts the walk once the sweep is mostly
    done.

    The reflections come from StairwellEngine::earlyReflectionsFor, the very
    taps the engine plays (Tests/EngineTests.cpp checks they agree).

    Author: Olivier Doaré, github.com/odoare
    (c) 2026 Olivier Doaré
    SPDX-License-Identifier: AGPL-3.0-or-later OR LicenseRef-FXME-Commercial
  ------------------------------------------------------------------------------
*/

#pragma once

#include <JuceHeader.h>
#include "Dsp/StairwellEngine.h"
#include <functional>

class Footsteps : public juce::Component,
                  public juce::SettableTooltipClient,
                  private juce::Timer
{
public:
    struct Snapshot
    {
        sth::Parameters params;
        float inputPeakDb = -100.0f;    // the input's peak since the last snapshot
    };

    explicit Footsteps (std::function<Snapshot()> source);
    ~Footsteps() override;

    void paint (juce::Graphics& g) override;

private:
    void timerCallback() override;

    /** The climb's height (0 at the bottom, 1 at the top) at time `ms`: the
        tread of the decorative staircase there. */
    float climbAt (float ms) const noexcept;

    static constexpr int numDecorSteps = 7;
    static constexpr float walkSeconds = 0.5f;    // the lit sweep, over the whole walk
    static constexpr float glowSeconds = 0.6f;    // how long a lit footprint stays bright

    std::function<Snapshot()> source;
    sth::StairwellEngine::Reflections reflections {};
    float predelayMs = 0.0f, spanMs = 100.0f, level = 1.0f;
    sth::Mode mode = sth::Mode::hall;

    float inputAverageDb = -80.0f;    // a slow follower of the input's peaks
    float walkClock = -1.0f;    // seconds since the last onset, -1: none
};
