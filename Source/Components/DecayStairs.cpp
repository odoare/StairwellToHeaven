/*
  ------------------------------------------------------------------------------
    DecayStairs.cpp

    See DecayStairs.h.

    Author: Olivier Doaré, github.com/odoare
    (c) 2026 Olivier Doaré
    SPDX-License-Identifier: AGPL-3.0-or-later OR LicenseRef-FXME-Commercial
  ------------------------------------------------------------------------------
*/

#include "DecayStairs.h"
#include "Theme.h"
#include <cmath>
#include <limits>

namespace
{
    constexpr double lowHz = 20.0, highHz = 22000.0;

    double stepCentreHz (int i) { return 25.0 * std::pow (2.0, (double) i / 3.0); }

    float freqToX (double hz, juce::Rectangle<float> r)
    {
        return r.getX() + r.getWidth() * (float) (std::log (hz / lowHz) / std::log (highHz / lowHz));
    }

    juce::String seconds (float s)
    {
        return s < 10.0f ? juce::String (s, 2) + " s" : juce::String (s, 1) + " s";
    }
}

DecayStairs::DecayStairs (std::function<Snapshot()> src) : source (std::move (src))
{
    timerCallback();
    startTimerHz (30);
}

DecayStairs::~DecayStairs()
{
    stopTimer();
}

void DecayStairs::timerCallback()
{
    if (source == nullptr)
        return;
    const auto snap = source();
    const auto settings = sth::StairwellEngine::tankSettingsFor (snap.params);

    frozen = snap.params.freeze;
    heaven = snap.params.mode == sth::Mode::heaven;
    float highest = 0.0f;
    for (int i = 0; i < numSteps; ++i)
    {
        const double t = fxme::TankReverb::rt60At (settings, stepCentreHz (i), snap.sampleRate);
        rt60[(size_t) i] = std::isfinite (t) ? (float) t : std::numeric_limits<float>::infinity();
        if (std::isfinite (t))
            highest = std::max (highest, (float) t);
    }
    const double mid = fxme::TankReverb::rt60At (settings, 1000.0, snap.sampleRate);
    midRt60 = std::isfinite (mid) ? (float) mid : std::numeric_limits<float>::infinity();

    // The scale: a round number of seconds over the highest step, eased so
    // it does not jump with every knob move.
    const float wanted = std::max (1.0f, std::ceil (highest * 1.15f));
    yMax = frozen ? yMax : (wanted > yMax ? wanted : yMax + 0.1f * (wanted - yMax));

    // The light follows the tail: -60 dB dark, 0 dB full.
    const float target = juce::jlimit (0.0f, 1.0f, (snap.tailEnergyDb + 60.0f) / 60.0f);
    glow += (target > glow ? 0.5f : 0.08f) * (target - glow);
    repaint();
}

void DecayStairs::paint (juce::Graphics& g)
{
    using namespace sth;
    auto bounds = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (theme::stoneDeep);
    g.fillRoundedRectangle (bounds, 5.0f);

    auto plot = bounds.reduced (10.0f).withTrimmedLeft (26.0f).withTrimmedBottom (14.0f);

    // The light above the stairs.
    {
        const auto lightColour = frozen ? theme::frozen : (heaven ? theme::shimmer : theme::glow);
        const float a = frozen ? 0.55f : 0.12f + 0.45f * glow;
        juce::ColourGradient light (lightColour.withAlpha (a), plot.getCentreX(), plot.getY() - plot.getHeight() * 0.2f,
                                    lightColour.withAlpha (0.0f), plot.getCentreX(), plot.getY() + plot.getHeight() * 0.75f,
                                    true);
        g.setGradientFill (light);
        g.fillRect (plot);
    }

    // Grid: seconds, and 100 Hz / 1 kHz / 10 kHz.
    g.setFont (juce::FontOptions (10.0f));
    const float stepSeconds = yMax <= 4.0f ? 1.0f : (yMax <= 10.0f ? 2.0f : (yMax <= 25.0f ? 5.0f : 10.0f));
    for (float s = stepSeconds; s < yMax; s += stepSeconds)
    {
        const float y = plot.getBottom() - plot.getHeight() * s / yMax;
        g.setColour (theme::panelLine.withAlpha (0.5f));
        g.drawHorizontalLine ((int) y, plot.getX(), plot.getRight());
        g.setColour (theme::dimText);
        g.drawText (juce::String ((int) s) + " s", juce::Rectangle<float> (bounds.getX() + 2.0f, y - 6.0f, 32.0f, 12.0f),
                    juce::Justification::centredLeft, false);
    }
    for (double f : { 100.0, 1000.0, 10000.0 })
    {
        const float x = freqToX (f, plot);
        g.setColour (theme::panelLine.withAlpha (0.5f));
        g.drawVerticalLine ((int) x, plot.getY(), plot.getBottom());
        g.setColour (theme::dimText);
        g.drawText (f >= 1000.0 ? juce::String ((int) (f / 1000.0)) + "k" : juce::String ((int) f),
                    juce::Rectangle<float> (x - 15.0f, plot.getBottom() + 1.0f, 30.0f, 12.0f), juce::Justification::centred, false);
    }

    // The steps: stone at the bottom, light toward the top.
    for (int i = 0; i < numSteps; ++i)
    {
        const double edgeLow = stepCentreHz (i) / std::pow (2.0, 1.0 / 6.0);
        const double edgeHigh = stepCentreHz (i) * std::pow (2.0, 1.0 / 6.0);
        const float x0 = freqToX (std::max (edgeLow, lowHz), plot);
        const float x1 = freqToX (std::min (edgeHigh, highHz), plot);
        const float t = rt60[(size_t) i];
        const float frac = std::isfinite (t) ? juce::jlimit (0.0f, 1.0f, t / yMax) : 1.0f;
        const float top = plot.getBottom() - plot.getHeight() * frac;

        auto step = juce::Rectangle<float> (x0, top, std::max (1.0f, x1 - x0), plot.getBottom() - top);
        const auto topColour = frozen ? theme::frozen : theme::stoneLight.interpolatedWith (theme::accent, 0.35f + 0.65f * frac);
        juce::ColourGradient body (topColour.withAlpha (0.9f), 0.0f, top, theme::stone, 0.0f, plot.getBottom(), false);
        g.setGradientFill (body);
        g.fillRect (step);

        // The tread: a bright edge on top of each step.
        g.setColour ((frozen ? theme::frozen : theme::glow).withAlpha (0.55f + 0.45f * frac));
        g.fillRect (step.withHeight (1.5f));
        g.setColour (theme::stoneDeep.withAlpha (0.6f));
        g.drawVerticalLine ((int) x1, top, plot.getBottom());
    }

    // Frozen: the flight's top edge glows, where every step now runs out,
    // breathing with the Freeze halo.
    if (frozen)
    {
        const float breath = 0.85f + 0.15f * (0.5f + 0.5f * std::sin ((float) juce::Time::getMillisecondCounter() * 0.0016f));
        for (int i = 5; i >= 1; --i)
        {
            g.setColour (theme::frozen.withAlpha (0.09f * breath));
            g.fillRect (plot.withHeight (2.0f * (float) i + 1.0f));
        }
        g.setColour (theme::frozen.brighter (0.3f).withAlpha (breath));
        g.fillRect (plot.withHeight (2.0f));
    }

    // The read-out.
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.setColour (frozen ? theme::frozen : theme::text);
    const auto readout = frozen ? juce::String (juce::CharPointer_UTF8 ("Frozen \xe2\x88\x9e"))
                                : "RT60 " + seconds (midRt60) + " at 1 kHz";
    g.drawText (readout, plot.reduced (6.0f, 4.0f).removeFromTop (16.0f), juce::Justification::topRight, false);

    g.setColour (theme::panelLine);
    g.drawRoundedRectangle (bounds, 5.0f, 1.0f);
}
