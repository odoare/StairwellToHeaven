/*
  ------------------------------------------------------------------------------
    Theme.h

    StairwellToHeaven's look, in one place. The identity is a climb: dark
    stone at the bottom of the window, warm light at the top. The panels are
    the stone; the accent is the light, brightest where the tail goes (the
    top of the decay stairs, Heaven, Freeze's halo). The light changes with
    the floor (accentForFloor): pale gold in the Room, salmon in the Hall,
    blue in Heaven; the editor glides it when the floor changes.

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
    inline const juce::Colour accent     { 0xffe8c46a };   // pale gold: the Room's, and the default
    inline const juce::Colour hallAccent   { 0xffef9479 };   // salmon
    inline const juce::Colour heavenAccent { 0xff9fc6ff };   // blue
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

    /** The accent on floor `floor` (0 Room .. 4 Heaven, fractional while
        gliding): gold to salmon up to the Hall, then salmon to blue. */
    inline juce::Colour accentForFloor (float floor)
    {
        const float f = juce::jlimit (0.0f, 4.0f, floor);
        return f <= 3.0f ? accent.interpolatedWith (hallAccent, f / 3.0f)
                         : hallAccent.interpolatedWith (heavenAccent, f - 3.0f);
    }

    /** The light an accent gives (the window's glow, the stairs' light). */
    inline juce::Colour lightFor (juce::Colour a)
    {
        return a.interpolatedWith (juce::Colours::white, 0.72f);
    }

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

    /** The window's backdrop: stone, with a warm light from the top whose
        strength (0 .. 1) follows the reverb's tail (the editor passes it), so
        the window brightens as the reverb blooms and dims as it dies. The
        light stays in the upper part, where it reads as a glow from above,
        and is held to a level that keeps the controls readable.

        In two layers, so the editor can render each once into an image and
        blend the light at the tail's level (a gradient computed pixel by
        pixel at every frame is what costs): the stone, then the light at
        full strength, drawn with opacity light01. */
    inline void paintBackdropStone (juce::Graphics& g, juce::Rectangle<float> r)
    {
        juce::ColourGradient base (glow.interpolatedWith (stone, 0.88f), r.getX(), r.getY(),
                                   stoneDeep, r.getX(), r.getBottom(), false);
        g.setGradientFill (base);
        g.fillRect (r);
    }

    /** The backdrop's light at full strength; it reaches down to
        backdropLightReach of the height. */
    inline constexpr float backdropLightReach = 0.55f;

    inline void paintBackdropLight (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour lightColour)
    {
        juce::ColourGradient light (lightColour.withAlpha (0.28f), r.getCentreX(), r.getY() - r.getHeight() * 0.15f,
                                    lightColour.withAlpha (0.0f), r.getCentreX(), r.getY() + r.getHeight() * backdropLightReach,
                                    true);
        g.setGradientFill (light);
        g.fillRect (r);
    }
}
