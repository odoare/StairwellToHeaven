/*
  ------------------------------------------------------------------------------
    DecayStairs.h

    The decay against frequency, drawn as a staircase: one step per third of
    an octave (25 Hz to 20 kHz), each as high as the tail lasts there (RT60,
    from fxme::TankReverb::rt60At on the very settings the engine uses). So
    Damping lowers the steps on the right, Bass raises or lowers them on the
    left, Decay lifts the whole flight. Frozen, every step runs off the top
    into the light, and the top edge glows (breathing with the Freeze halo).

    Above the stairs, the light: a glow whose brightness follows the tail's
    energy as it plays (the engine's meter), a pale blue halo while frozen.

    It pulls what it draws on a timer through `source`, so it holds no
    reference to the processor's internals.

    Author: Olivier Doaré, github.com/odoare
    (c) 2026 Olivier Doaré
    SPDX-License-Identifier: AGPL-3.0-or-later OR LicenseRef-FXME-Commercial
  ------------------------------------------------------------------------------
*/

#pragma once

#include <JuceHeader.h>
#include "Dsp/StairwellEngine.h"
#include "Theme.h"
#include <array>
#include <functional>

class DecayStairs : public juce::Component,
                    public juce::SettableTooltipClient,
                    private juce::Timer
{
public:
    struct Snapshot
    {
        sth::Parameters params;
        double sampleRate = 48000.0;
        float tailEnergyDb = -100.0f;
    };

    explicit DecayStairs (std::function<Snapshot()> source);
    ~DecayStairs() override;

    void paint (juce::Graphics& g) override;

    /** The floor's accent and light (the editor sets them as the floor changes). */
    void setColours (juce::Colour accentColour, juce::Colour lightColour)
    {
        accent = accentColour;
        light = lightColour;
        layerDirty = true;
        repaint();
    }

private:
    void timerCallback() override;

    /** The steps, the grid, the read-out and the frame: what changes with
        the parameters, not with the tail (painted into `layer`). */
    void paintLayer (juce::Graphics& g, juce::Rectangle<float> bounds, juce::Rectangle<float> plot);

    /** The plot (the stairs and their light), inside the frame and labels. */
    juce::Rectangle<float> plotArea() const
    {
        return getLocalBounds().toFloat().reduced (1.0f).reduced (10.0f).withTrimmedLeft (26.0f).withTrimmedBottom (14.0f);
    }

    static constexpr int numSteps = 30;   // thirds of an octave, 25 Hz .. 20 kHz

    std::function<Snapshot()> source;
    std::array<float, numSteps> rt60 {};
    float midRt60 = 0.0f, yMax = 4.0f, glow = 0.0f, glowDrawn = 0.0f;
    bool frozen = false;
    juce::Colour accent = sth::theme::accent, light = sth::theme::glow;

    // CPU: the light follows the tail at 30 Hz. Its gradient is rendered
    // once at full strength and blended at the tail's level; the rest is
    // rendered once into `layer` (at the screen's pixel scale) and redrawn
    // only when the parameters, the colours or the size change.
    juce::Image beam, layer;
    juce::Colour beamColour;
    float layerScale = 0.0f;
    bool layerDirty = true;
};
