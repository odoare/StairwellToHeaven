/*
  ------------------------------------------------------------------------------
    ReflectionsView.h

    The early reflections, left of the decay stairs: time runs from the
    bottom (the dry sound) to the top, and each reflection is a circle placed
    left or right by its pan as it is heard, Width included (it scales the
    wet's side signal, so 0 % stacks them in the centre). Its size is its level, it is
    darker the more it is low-passed, and the whole picture fades as
    Early/Late moves toward the tail. The pre-delay is the dashed stretch up
    the centre before the first one.

    So the floors read at a glance: the Stairwell's flutter is an even ladder
    swinging left and right; the Room's, short and irregular; the Hall's, tall
    and sparse; the Plate's, two faint circles near the bottom.

    On every onset of the input (a note, a hit: its peak 4 dB over its own
    recent average), the circles light up one after another from the bottom,
    at a pace slowed down to be seen (the real ones take a few tens of
    milliseconds). A new onset restarts the sweep once it is mostly done.

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

class ReflectionsView : public juce::Component,
                        public juce::SettableTooltipClient,
                        private juce::Timer
{
public:
    struct Snapshot
    {
        sth::Parameters params;
        float inputPeakDb = -100.0f;    // the input's peak since the last snapshot
    };

    explicit ReflectionsView (std::function<Snapshot()> source);
    ~ReflectionsView() override;

    void paint (juce::Graphics& g) override;

private:
    void timerCallback() override;

    static constexpr float walkSeconds = 0.5f;    // the lit sweep, bottom to top
    static constexpr float glowSeconds = 0.6f;    // how long a lit circle stays bright

    std::function<Snapshot()> source;
    sth::StairwellEngine::Reflections reflections {};
    float predelayMs = 0.0f, spanMs = 100.0f, level = 1.0f, width = 1.0f;
    sth::Mode mode = sth::Mode::hall;

    float inputAverageDb = -80.0f;    // a slow follower of the input's peaks
    float walkClock = -1.0f;          // seconds since the last onset, -1: none
};
