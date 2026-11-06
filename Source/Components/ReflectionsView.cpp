/*
  ------------------------------------------------------------------------------
    ReflectionsView.cpp

    See ReflectionsView.h.

    Author: Olivier Doaré, github.com/odoare
    (c) 2026 Olivier Doaré
    SPDX-License-Identifier: AGPL-3.0-or-later OR LicenseRef-FXME-Commercial
  ------------------------------------------------------------------------------
*/

#include "ReflectionsView.h"
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
}

ReflectionsView::ReflectionsView (std::function<Snapshot()> src) : source (std::move (src))
{
    timerCallback();
    startTimerHz (30);
}

ReflectionsView::~ReflectionsView()
{
    stopTimer();
}

void ReflectionsView::timerCallback()
{
    if (source == nullptr)
        return;
    const auto snap = source();

    reflections = sth::StairwellEngine::earlyReflectionsFor (snap.params);
    predelayMs = snap.params.predelaySeconds * 1000.0f;
    level = sth::StairwellEngine::erLevelFor (snap.params);
    width = snap.params.width01;
    mode = snap.params.mode;

    float lastMs = 0.0f;
    for (const auto& r : reflections)
        if (r.gain > 0.0f)
            lastMs = std::max (lastMs, r.ms);
    spanMs = niceSpan (predelayMs + lastMs);

    // An onset: the input's peak 4 dB over its recent average, and audible.
    // It starts the sweep, or restarts it once the sweep is mostly done.
    const float peak = std::max (-80.0f, snap.inputPeakDb);
    const bool onset = peak > -50.0f && peak - inputAverageDb > 4.0f;
    inputAverageDb += 0.15f * (peak - inputAverageDb);

    if (onset && (walkClock < 0.0f || walkClock > 0.7f * walkSeconds))
        walkClock = 0.0f;
    else if (walkClock >= 0.0f)
        walkClock = walkClock + 1.0f / 30.0f > walkSeconds + glowSeconds ? -1.0f : walkClock + 1.0f / 30.0f;

    repaint();
}

void ReflectionsView::paint (juce::Graphics& g)
{
    using namespace sth;
    auto bounds = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (theme::stoneDeep);
    g.fillRoundedRectangle (bounds, 5.0f);

    // Time up the left (labels), pan across, L and R under.
    auto plot = bounds.reduced (10.0f).withTrimmedTop (16.0f).withTrimmedBottom (14.0f).withTrimmedLeft (34.0f);
    const float maxRadius = juce::jmin (plot.getWidth() * 0.08f, plot.getHeight() * 0.07f, 14.0f);
    const auto inner = plot.reduced (maxRadius + 2.0f, 0.0f);
    auto xOf = [&inner] (float pan) { return inner.getCentreX() + 0.5f * inner.getWidth() * juce::jlimit (-1.0f, 1.0f, pan); };
    auto yOf = [&plot, this] (float ms) { return plot.getBottom() - plot.getHeight() * juce::jlimit (0.0f, 1.0f, ms / spanMs); };

    // Grid: the centre (pan 0), quarters of the time span.
    g.setColour (theme::panelLine.withAlpha (0.6f));
    g.drawVerticalLine ((int) inner.getCentreX(), plot.getY(), plot.getBottom());
    g.setFont (juce::FontOptions (10.0f));
    for (float frac : { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f })
    {
        const float y = yOf (spanMs * frac);
        g.setColour (theme::panelLine.withAlpha (frac == 0.0f ? 0.8f : 0.35f));
        g.drawHorizontalLine ((int) y, plot.getX(), plot.getRight());
        if (frac == 0.0f || frac == 0.5f || frac == 1.0f)
        {
            g.setColour (theme::dimText);
            g.drawText (juce::String (juce::roundToInt (spanMs * frac)) + " ms",
                        juce::Rectangle<float> (bounds.getX() + 4.0f, y - 6.0f, 40.0f, 12.0f),
                        juce::Justification::centredLeft, false);
        }
    }
    g.setColour (theme::dimText);
    g.drawText ("L", juce::Rectangle<float> (inner.getX() - 6.0f, plot.getBottom() + 1.0f, 12.0f, 12.0f), juce::Justification::centred, false);
    g.drawText ("R", juce::Rectangle<float> (inner.getRight() - 6.0f, plot.getBottom() + 1.0f, 12.0f, 12.0f), juce::Justification::centred, false);

    // The dry sound at the bottom, centred; the pre-delay up the centre.
    g.setColour (theme::text.withAlpha (0.7f));
    g.fillEllipse (juce::Rectangle<float> (5.0f, 5.0f).withCentre ({ inner.getCentreX(), plot.getBottom() }));
    if (predelayMs > 0.02f * spanMs)
    {
        const float y1 = yOf (predelayMs);
        g.setColour (theme::accent.withAlpha (0.5f));
        const float dashes[] { 3.0f, 3.0f };
        g.drawDashedLine (juce::Line<float> (inner.getCentreX(), plot.getBottom(), inner.getCentreX(), y1), dashes, 2, 1.4f);
        g.drawHorizontalLine ((int) y1, inner.getCentreX() - 4.0f, inner.getCentreX() + 4.0f);
        if (plot.getBottom() - y1 > 40.0f)
        {
            g.saveState();
            g.addTransform (juce::AffineTransform::rotation (-juce::MathConstants<float>::halfPi,
                                                             inner.getCentreX() - 8.0f, 0.5f * (plot.getBottom() + y1)));
            g.drawText ("pre-delay", juce::Rectangle<float> (inner.getCentreX() - 8.0f - 40.0f,
                                                             0.5f * (plot.getBottom() + y1) - 6.0f, 80.0f, 12.0f),
                        juce::Justification::centred, false);
            g.restoreState();
        }
    }

    // The reflections, bottom to top.
    const float litUntil = walkClock < 0.0f ? -1.0f : spanMs * juce::jmin (1.0f, walkClock / walkSeconds);
    const auto light = mode == Mode::heaven ? theme::shimmer : theme::glow;
    for (const auto& r : reflections)
    {
        if (r.gain <= 0.0f)
            continue;
        const float t = predelayMs + r.ms;
        // Where it is heard: Width scales the wet's side signal.
        const auto centre = juce::Point<float> (xOf (sth::StairwellEngine::panAfterWidth (r.pan, width)), yOf (t));
        const float radius = maxRadius * (0.35f + 0.65f * r.gain);

        // Darker for the duller reflections (lower low-pass).
        const float bright = juce::jlimit (0.0f, 1.0f, (r.lowpassHz - 4000.0f) / 12000.0f);
        auto colour = theme::accent.interpolatedWith (theme::dimText, 0.6f * (1.0f - bright));
        float alpha = level * (0.35f + 0.65f * r.gain);

        // The sweep: lit as it passes, fading after.
        if (litUntil >= 0.0f && t <= litUntil)
        {
            const float since = walkClock - walkSeconds * t / spanMs;
            const float glow = juce::jlimit (0.0f, 1.0f, 1.0f - since / glowSeconds);
            colour = colour.interpolatedWith (light, glow);
            alpha = juce::jmin (1.0f, alpha + 0.5f * glow * level);
            if (glow > 0.0f)
            {
                g.setColour (light.withAlpha (0.25f * glow * level));
                g.fillEllipse (juce::Rectangle<float> (radius * 3.2f, radius * 3.2f).withCentre (centre));
            }
        }

        const auto circle = juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre);
        g.setColour (colour.withAlpha (juce::jlimit (0.0f, 1.0f, alpha)));
        g.fillEllipse (circle);
        g.setColour (colour.brighter (0.4f).withAlpha (juce::jlimit (0.0f, 1.0f, alpha)));
        g.drawEllipse (circle, 1.0f);
    }

    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.setColour (theme::dimText);
    g.drawText (mode == Mode::stairwell ? "early reflections: flutter" : "early reflections",
                bounds.reduced (10.0f, 4.0f).removeFromTop (14.0f), juce::Justification::centredLeft, false);

    g.setColour (theme::panelLine);
    g.drawRoundedRectangle (bounds, 5.0f, 1.0f);
}
