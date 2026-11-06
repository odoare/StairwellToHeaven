/*
  ------------------------------------------------------------------------------
    Footsteps.cpp

    See Footsteps.h.

    Author: Olivier Doaré, github.com/odoare
    (c) 2026 Olivier Doaré
    SPDX-License-Identifier: AGPL-3.0-or-later OR LicenseRef-FXME-Commercial
  ------------------------------------------------------------------------------
*/

#include "Footsteps.h"
#include "Theme.h"
#include <cmath>

namespace
{
    /** A round number of milliseconds a little over `ms`. */
    float niceSpan (float ms)
    {
        for (float s : { 20.0f, 30.0f, 50.0f, 75.0f, 100.0f, 150.0f, 200.0f, 300.0f, 500.0f, 750.0f,
                         1000.0f, 1500.0f, 2000.0f, 2500.0f })
            if (ms * 1.1f <= s)
                return s;
        return 3000.0f;
    }

    /** A footprint pointing along +x, centred on the origin, `len` long:
        the sole, the heel, five toes. */
    juce::Path footprint (float len)
    {
        juce::Path p;
        p.addEllipse (-len * 0.10f, -len * 0.21f, len * 0.48f, len * 0.42f);   // ball and sole
        p.addEllipse (-len * 0.50f, -len * 0.15f, len * 0.34f, len * 0.30f);   // heel
        const float toe = len * 0.11f;
        const float toeY[] { -0.20f, -0.10f, 0.0f, 0.09f, 0.17f };
        const float toeX[] { 0.40f, 0.44f, 0.45f, 0.43f, 0.39f };
        for (int i = 0; i < 5; ++i)
        {
            const float s = toe * (i == 0 ? 1.25f : 1.0f - 0.08f * (float) i);
            p.addEllipse (len * toeX[i] - s * 0.5f, len * toeY[i] - s * 0.5f, s, s);
        }
        return p;
    }
}

Footsteps::Footsteps (std::function<Snapshot()> src) : source (std::move (src))
{
    timerCallback();
    startTimerHz (30);
}

Footsteps::~Footsteps()
{
    stopTimer();
}

void Footsteps::timerCallback()
{
    if (source == nullptr)
        return;
    const auto snap = source();

    reflections = sth::StairwellEngine::earlyReflectionsFor (snap.params);
    predelayMs = snap.params.predelaySeconds * 1000.0f;
    level = sth::StairwellEngine::erLevelFor (snap.params);
    mode = snap.params.mode;

    float lastMs = 0.0f;
    for (const auto& r : reflections)
        if (r.gain > 0.0f)
            lastMs = std::max (lastMs, r.ms);
    spanMs = niceSpan (predelayMs + lastMs);

    // An onset: the input's peak 4 dB over its recent average, and audible.
    // It starts the walk, or restarts it once the sweep is mostly done.
    const float peak = std::max (-80.0f, snap.inputPeakDb);
    const bool onset = peak > -50.0f && peak - inputAverageDb > 4.0f;
    inputAverageDb += 0.15f * (peak - inputAverageDb);

    if (onset && (walkClock < 0.0f || walkClock > 0.7f * walkSeconds))
        walkClock = 0.0f;
    else if (walkClock >= 0.0f)
        walkClock = walkClock + 1.0f / 30.0f > walkSeconds + glowSeconds ? -1.0f : walkClock + 1.0f / 30.0f;

    repaint();
}

float Footsteps::climbAt (float ms) const noexcept
{
    const float t = juce::jlimit (0.0f, 0.999f, ms / spanMs);
    return (std::floor (t * (float) numDecorSteps) + 1.0f) / (float) (numDecorSteps + 1);
}

void Footsteps::paint (juce::Graphics& g)
{
    using namespace sth;
    auto bounds = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (theme::stoneDeep);
    g.fillRoundedRectangle (bounds, 5.0f);

    auto plot = bounds.reduced (10.0f).withTrimmedTop (16.0f).withTrimmedBottom (14.0f);
    auto xOf = [&plot, this] (float ms) { return plot.getX() + plot.getWidth() * juce::jlimit (0.0f, 1.0f, ms / spanMs); };
    auto yOf = [&plot] (float climb) { return plot.getBottom() - plot.getHeight() * 0.8f * climb; };

    // The flight they climb: faint stone steps.
    for (int i = 0; i < numDecorSteps; ++i)
    {
        const float x0 = plot.getX() + plot.getWidth() * (float) i / (float) numDecorSteps;
        const float x1 = plot.getX() + plot.getWidth() * (float) (i + 1) / (float) numDecorSteps;
        const float top = yOf ((float) (i + 1) / (float) (numDecorSteps + 1));
        g.setColour (theme::stone.interpolatedWith (theme::stoneLight, (float) i / (float) numDecorSteps));
        g.fillRect (juce::Rectangle<float> (x0, top, x1 - x0, plot.getBottom() - top));
        g.setColour (theme::panelLine.withAlpha (0.8f));
        g.drawHorizontalLine ((int) top, x0, x1);
    }

    // Time axis.
    g.setFont (juce::FontOptions (10.0f));
    g.setColour (theme::dimText);
    for (float frac : { 0.0f, 0.5f, 1.0f })
    {
        const float x = plot.getX() + plot.getWidth() * frac;
        g.drawText (juce::String (juce::roundToInt (spanMs * frac)) + (frac == 1.0f ? " ms" : ""),
                    juce::Rectangle<float> (x - 24.0f, plot.getBottom() + 1.0f, 48.0f, 12.0f),
                    frac == 0.0f ? juce::Justification::centredLeft
                                 : (frac == 1.0f ? juce::Justification::centredRight : juce::Justification::centred), false);
    }

    // The pre-delay: the stretch before the first step.
    if (predelayMs > 0.04f * spanMs)
    {
        const float x1 = xOf (predelayMs);
        const float y = plot.getBottom() - 4.0f;
        g.setColour (theme::accent.withAlpha (0.5f));
        const float dashes[] { 3.0f, 3.0f };
        g.drawDashedLine (juce::Line<float> (plot.getX(), y, x1, y), dashes, 2, 1.2f);
        g.drawVerticalLine ((int) x1, y - 4.0f, y + 2.0f);
        if (x1 - plot.getX() > 50.0f)
            g.drawText ("pre-delay", juce::Rectangle<float> (plot.getX(), y - 14.0f, x1 - plot.getX(), 12.0f),
                        juce::Justification::centred, false);
    }

    // The footprints, in time order (the patterns are sorted).
    const float baseLen = juce::jmin (plot.getHeight() * 0.32f, plot.getWidth() * 0.11f);
    const float litUntil = walkClock < 0.0f ? -1.0f : spanMs * juce::jmin (1.0f, walkClock / walkSeconds);
    const auto light = mode == Mode::heaven ? theme::shimmer : theme::glow;
    for (const auto& r : reflections)
    {
        if (r.gain <= 0.0f)
            continue;
        const float t = predelayMs + r.ms;
        const float x = xOf (t);
        const bool leftFoot = r.pan < 0.0f;
        const float y = yOf (climbAt (t)) - baseLen * (leftFoot ? 0.42f : 0.18f);
        const float len = baseLen * (0.55f + 0.6f * r.gain);

        // Darker for the duller reflections (lower low-pass).
        const float bright = juce::jlimit (0.0f, 1.0f, (r.lowpassHz - 4000.0f) / 12000.0f);
        auto colour = theme::accent.interpolatedWith (theme::dimText, 0.6f * (1.0f - bright));
        float alpha = level * (0.35f + 0.65f * r.gain);

        // The walk: lit as the sweep passes, fading after.
        if (litUntil >= 0.0f && t <= litUntil)
        {
            const float since = walkClock - walkSeconds * t / spanMs;
            const float glow = juce::jlimit (0.0f, 1.0f, 1.0f - since / glowSeconds);
            colour = colour.interpolatedWith (light, glow);
            alpha = juce::jmin (1.0f, alpha + 0.5f * glow * level);
            if (glow > 0.0f)
            {
                g.setColour (light.withAlpha (0.25f * glow * level));
                g.fillEllipse (juce::Rectangle<float> (len * 1.3f, len * 1.0f).withCentre ({ x, y }));
            }
        }

        // Toes to the right, tilted up the climb; a left foot mirrored.
        auto foot = footprint (len);
        auto transform = juce::AffineTransform::scale (1.0f, leftFoot ? -1.0f : 1.0f)
                             .rotated (-0.35f)
                             .translated (x, y);
        g.setColour (colour.withAlpha (juce::jlimit (0.0f, 1.0f, alpha)));
        g.fillPath (foot, transform);
    }

    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.setColour (theme::dimText);
    g.drawText (mode == Mode::stairwell ? "early reflections: flutter" : "early reflections",
                bounds.reduced (10.0f, 4.0f).removeFromTop (14.0f), juce::Justification::centredLeft, false);

    g.setColour (theme::panelLine);
    g.drawRoundedRectangle (bounds, 5.0f, 1.0f);
}
