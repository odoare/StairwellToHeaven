/*
  ------------------------------------------------------------------------------
    FloorIndicator.cpp

    See FloorIndicator.h.

    Author: Olivier Doaré, github.com/odoare
    (c) 2026 Olivier Doaré
    SPDX-License-Identifier: AGPL-3.0-or-later OR LicenseRef-FXME-Commercial
  ------------------------------------------------------------------------------
*/

#include "FloorIndicator.h"
#include "Theme.h"

namespace
{
    const juce::Colour segmentOn  { 0xffffb347 };   // lift-display amber
    const juce::Colour segmentOff { 0xff2e2418 };

    juce::Path arrow (juce::Rectangle<float> r, bool up)
    {
        juce::Path p;
        if (up) p.addTriangle (r.getCentreX(), r.getY(), r.getRight(), r.getBottom(), r.getX(), r.getBottom());
        else    p.addTriangle (r.getX(), r.getY(), r.getRight(), r.getY(), r.getCentreX(), r.getBottom());
        return p;
    }
}

FloorIndicator::FloorIndicator (std::function<State()> src, juce::StringArray floorGlyphs)
    : source (std::move (src)), glyphs (std::move (floorGlyphs))
{
    timerCallback();
    startTimerHz (30);
}

FloorIndicator::~FloorIndicator()
{
    stopTimer();
}

void FloorIndicator::timerCallback()
{
    if (source == nullptr)
        return;
    const auto next = source();

    // Travelling while the parameter is ahead of the engine and until the
    // fade back in is over, the direction kept for that last part. The
    // engine's change takes 60 ms: the arrow stays lit at least 0.6 s, or no
    // one would see it.
    if (next.target != next.sounding)
    {
        direction = next.target > next.sounding ? 1 : -1;
        travelTicks = 18;
    }
    else if (travelTicks > 0)
        --travelTicks;
    else if (next.fade >= 0.999f)
        direction = 0;

    if (lastSounding >= 0 && next.sounding != lastSounding)
        arrival = 1.0f;   // ding
    lastSounding = next.sounding;
    arrival = std::max (0.0f, arrival - 0.06f);

    if (next.target != state.target || next.sounding != state.sounding || next.fade != state.fade
        || direction != 0 || arrival > 0.0f)
        repaint();
    state = next;
}

void FloorIndicator::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (juce::Colour (0xff0a0806));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (sth::theme::panelLine);
    g.drawRoundedRectangle (r, 5.0f, 1.0f);

    auto inner = r.reduced (8.0f, 5.0f);

    // Arrows, on the left: lit in the direction of travel, blinking gently.
    auto arrows = inner.removeFromLeft (inner.getHeight() * 0.45f);
    const float blink = 0.6f + 0.4f * std::sin ((float) juce::Time::getMillisecondCounter() * 0.012f);
    const float a = arrows.getWidth() * 0.8f;
    auto upBox = juce::Rectangle<float> (a, a * 0.75f).withCentre ({ arrows.getCentreX(), arrows.getCentreY() - a * 0.55f });
    auto downBox = juce::Rectangle<float> (a, a * 0.75f).withCentre ({ arrows.getCentreX(), arrows.getCentreY() + a * 0.55f });
    g.setColour (direction > 0 ? segmentOn.withAlpha (blink) : segmentOff);
    g.fillPath (arrow (upBox, true));
    g.setColour (direction < 0 ? segmentOn.withAlpha (blink) : segmentOff);
    g.fillPath (arrow (downBox, false));

    // The floor: the display's dim ghost, then the lit figure with a glow.
    const auto glyph = glyphs[juce::jlimit (0, glyphs.size() - 1, state.sounding)];
    auto digits = inner.withTrimmedLeft (6.0f);
    const auto font = juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), digits.getHeight() * 0.95f, juce::Font::bold);
    g.setFont (font);
    g.setColour (segmentOff);
    g.drawText ("8", digits, juce::Justification::centred, false);

    const float level = 0.75f + 0.25f * state.fade + 0.3f * arrival;
    for (float spread : { 3.0f, 1.5f })
    {
        g.setColour (segmentOn.withAlpha (0.12f * level));
        g.drawText (glyph, digits.translated (spread, 0.0f), juce::Justification::centred, false);
        g.drawText (glyph, digits.translated (-spread, 0.0f), juce::Justification::centred, false);
    }
    g.setColour (segmentOn.withMultipliedBrightness (juce::jmin (1.2f, level)));
    g.drawText (glyph, digits, juce::Justification::centred, false);

    if (arrival > 0.0f)
    {
        g.setColour (segmentOn.withAlpha (0.15f * arrival));
        g.fillRoundedRectangle (r, 5.0f);
    }
}
