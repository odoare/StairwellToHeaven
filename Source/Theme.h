/*
  ------------------------------------------------------------------------------
    Theme.h

    StairwellToHeaven's look, in one place. The identity is a climb: dark
    stone at the bottom of the window, warm light at the top. The panels are
    the stone; the accent is the light (a pale gold), brightest where the
    tail goes (the top of the decay stairs, Heaven, Freeze's halo).

    Author: Olivier Doaré, github.com/odoare
    (c) 2026 Olivier Doaré
    SPDX-License-Identifier: AGPL-3.0-or-later OR LicenseRef-FXME-Commercial
  ------------------------------------------------------------------------------
*/

#pragma once

#include <JuceHeader.h>

namespace sth::theme
{
    /** The name shown to the user (the product and folders say
        "StairwellToHeaven"). */
    inline const juce::String displayName { "Stairwell to Heaven" };

    // Stone, from the deepest to the lightest.
    inline const juce::Colour stoneDeep  { 0xff0f0e13 };
    inline const juce::Colour stone      { 0xff1a1920 };
    inline const juce::Colour stoneLight { 0xff2a2832 };
    inline const juce::Colour panelLine  { 0xff3c3a46 };

    // Light.
    inline const juce::Colour accent     { 0xffe8c46a };   // pale gold
    inline const juce::Colour glow       { 0xfffff1cf };   // the light at the top
    inline const juce::Colour shimmer    { 0xffb9d8ff };   // Heaven's pitched sparkle
    inline const juce::Colour frozen     { 0xffaee6ff };   // Freeze

    inline const juce::Colour text       { 0xffdedbe6 };
    inline const juce::Colour dimText    { 0xff8e8a9c };
    inline const juce::Colour topBarBg   { 0xff121117 };

    inline constexpr int topBarHeight  = 44;
    inline constexpr int defaultWidth  = 900;
    inline constexpr int defaultHeight = 600;
    inline constexpr int minWidth      = 760;
    inline constexpr int minHeight     = 520;
    inline constexpr int margin        = 10;

    inline void styleKnob (juce::Slider& s, juce::Colour a = accent)
    {
        s.setColour (juce::Slider::rotarySliderFillColourId,    juce::Colour (0xff24222c));
        s.setColour (juce::Slider::rotarySliderOutlineColourId, a.darker (1.6f));
        s.setColour (juce::Slider::trackColourId,               a);
        s.setColour (juce::Slider::thumbColourId,               a.brighter (0.4f));
        s.setColour (juce::Slider::textBoxTextColourId,         text);
    }

    inline void styleCombo (juce::ComboBox& c, juce::Colour a = accent)
    {
        c.setColour (juce::ComboBox::backgroundColourId, stoneLight);
        c.setColour (juce::ComboBox::outlineColourId,    a.darker (1.2f));
        c.setColour (juce::ComboBox::arrowColourId,      a);
        c.setColour (juce::ComboBox::textColourId,       text);
    }

    /** The window's backdrop: stone at the bottom, a faint warm light at the
        top. */
    inline void paintBackdrop (juce::Graphics& g, juce::Rectangle<float> r)
    {
        juce::ColourGradient grad (glow.withAlpha (1.0f).interpolatedWith (stone, 0.88f), r.getX(), r.getY(),
                                   stoneDeep, r.getX(), r.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRect (r);
    }
}
