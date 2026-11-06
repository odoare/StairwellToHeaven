/*
  ------------------------------------------------------------------------------
    FloorSelector.cpp

    See FloorSelector.h.

    Author: Olivier Doaré, github.com/odoare
    (c) 2026 Olivier Doaré
    SPDX-License-Identifier: AGPL-3.0-or-later OR LicenseRef-FXME-Commercial
  ------------------------------------------------------------------------------
*/

#include "FloorSelector.h"
#include "Theme.h"

FloorSelector::FloorSelector (juce::RangedAudioParameter& param, juce::StringArray floorNames,
                              juce::StringArray floorGlyphs)
    : names (std::move (floorNames)), glyphs (std::move (floorGlyphs)),
      attachment (param, [this] (float value)
                  {
                      selected = juce::jlimit (0, names.size() - 1, juce::roundToInt (value));
                      repaint();
                  })
{
    attachment.sendInitialUpdate();
}

FloorSelector::~FloorSelector() = default;

juce::Rectangle<float> FloorSelector::floorBounds (int index) const
{
    // Floor 0 at the bottom.
    auto r = getLocalBounds().toFloat().reduced (6.0f, 8.0f);
    const float h = r.getHeight() / (float) names.size();
    return { r.getX(), r.getBottom() - h * (float) (index + 1), r.getWidth(), h };
}

int FloorSelector::floorAt (juce::Point<float> p) const
{
    for (int i = 0; i < names.size(); ++i)
        if (floorBounds (i).contains (p))
            return i;
    return -1;
}

void FloorSelector::paint (juce::Graphics& g)
{
    using namespace sth;
    auto panel = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (theme::stoneDeep);
    g.fillRoundedRectangle (panel, 6.0f);
    g.setColour (theme::panelLine);
    g.drawRoundedRectangle (panel, 6.0f, 1.0f);

    for (int i = 0; i < names.size(); ++i)
    {
        auto row = floorBounds (i);
        const bool lit = i == selected, hover = i == hovered;
        const auto light = i == names.size() - 1 ? topFloorColour : theme::accent;

        const float d = juce::jmin (row.getHeight() - 10.0f, 30.0f);
        auto button = juce::Rectangle<float> (d, d).withCentre ({ row.getX() + 8.0f + d * 0.5f, row.getCentreY() });

        if (lit)
        {
            g.setColour (light.withAlpha (0.25f));
            g.fillEllipse (button.expanded (5.0f));
        }
        g.setColour (lit ? light : (hover ? theme::stoneLight.brighter (0.2f) : theme::stoneLight));
        g.fillEllipse (button);
        g.setColour (lit ? light.brighter (0.4f) : theme::panelLine);
        g.drawEllipse (button, 1.2f);

        g.setColour (lit ? theme::stoneDeep : theme::dimText);
        g.setFont (juce::FontOptions (d * 0.5f, juce::Font::bold));
        g.drawText (glyphs[i], button, juce::Justification::centred, false);

        g.setColour (lit ? theme::text : theme::dimText);
        g.setFont (juce::FontOptions (14.0f, lit ? juce::Font::bold : juce::Font::plain));
        g.drawText (names[i], row.withTrimmedLeft (d + 16.0f), juce::Justification::centredLeft, true);
    }
}

void FloorSelector::mouseDown (const juce::MouseEvent& e)
{
    const int f = floorAt (e.position);
    if (f >= 0 && f != selected)
        attachment.setValueAsCompleteGesture ((float) f);
}

void FloorSelector::mouseMove (const juce::MouseEvent& e)
{
    const int f = floorAt (e.position);
    if (f != hovered)
    {
        hovered = f;
        setMouseCursor (f >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void FloorSelector::mouseExit (const juce::MouseEvent&)
{
    hovered = -1;
    repaint();
}
