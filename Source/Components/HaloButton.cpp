/*
  ------------------------------------------------------------------------------
    HaloButton.cpp

    See HaloButton.h.

    Author: Olivier Doaré, github.com/odoare
    (c) 2026 Olivier Doaré
    SPDX-License-Identifier: AGPL-3.0-or-later OR LicenseRef-FXME-Commercial
  ------------------------------------------------------------------------------
*/

#include "HaloButton.h"
#include "Theme.h"
#include <cmath>

HaloButton::HaloButton (const juce::String& text) : juce::Button (text)
{
    setClickingTogglesState (true);
    setMouseClickGrabsKeyboardFocus (false);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

HaloButton::~HaloButton()
{
    stopTimer();
}

void HaloButton::updateTimer()
{
    // The breathing needs a timer only while on (and visible).
    if (getToggleState())
    {
        if (! isTimerRunning())
            startTimerHz (30);
    }
    else
    {
        stopTimer();
    }
}

void HaloButton::buttonStateChanged()
{
    updateTimer();
    repaint();
}

void HaloButton::clicked()
{
    updateTimer();
}

void HaloButton::timerCallback()
{
    if (! getToggleState())
        stopTimer();
    repaint();
}

void HaloButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    using namespace sth;
    const bool on = getToggleState();
    updateTimer();   // a toggle from automation or a preset only repaints
    auto r = getLocalBounds().toFloat().reduced (2.0f);

    // The word under the halo.
    auto label = r.removeFromBottom (juce::jmin (18.0f, r.getHeight() * 0.4f));
    // The halo: a flat ellipse, as a ring seen from slightly below.
    const float w = juce::jmin (r.getWidth() * 0.78f, r.getHeight() * 3.2f);
    const auto ring = juce::Rectangle<float> (w, w * 0.3f).withCentre (r.getCentre());

    if (on)
    {
        // Breathing: 0.85 .. 1 over about four seconds, on the same clock as
        // the decay stairs' frozen edge, so the two breathe together.
        const float breath = 0.85f + 0.15f * (0.5f + 0.5f * std::sin ((float) juce::Time::getMillisecondCounter() * 0.0016f));
        for (int i = 4; i >= 1; --i)
        {
            g.setColour (lightColour.withAlpha (0.10f * breath));
            g.drawEllipse (ring, 2.0f + 3.0f * (float) i);
        }
        g.setColour (lightColour.withAlpha (0.12f * breath));
        g.fillEllipse (ring.reduced (2.0f));
        g.setColour (lightColour.brighter (0.3f).withAlpha (breath));
        g.drawEllipse (ring, 2.4f);
    }
    else
    {
        g.setColour ((highlighted ? theme::panelLine.brighter (0.4f) : theme::panelLine).withAlpha (down ? 1.0f : 0.9f));
        g.drawEllipse (ring, 2.0f);
        if (highlighted)
        {
            g.setColour (lightColour.withAlpha (0.08f));
            g.drawEllipse (ring, 6.0f);
        }
    }

    g.setFont (juce::FontOptions (14.0f, on ? juce::Font::bold : juce::Font::plain));
    g.setColour (on ? lightColour : (highlighted ? theme::text : theme::dimText));
    g.drawText (getButtonText(), label, juce::Justification::centred, false);
}
