/*
  ------------------------------------------------------------------------------
    FloorSelector.h

    The mode, as the button panel of a lift: one lit button per floor,
    Room at the bottom and Heaven at the top (each mode a floor of the
    climb). Bound to a choice parameter whose items are listed bottom to top;
    a click selects that floor as one undoable gesture, automation and preset
    loads light the right button.

    Author: Olivier Doaré, github.com/odoare
    (c) 2026 Olivier Doaré
    SPDX-License-Identifier: AGPL-3.0-or-later OR LicenseRef-FXME-Commercial
  ------------------------------------------------------------------------------
*/

#pragma once

#include <JuceHeader.h>

class FloorSelector : public juce::Component,
                      public juce::SettableTooltipClient
{
public:
    FloorSelector (juce::RangedAudioParameter& choiceParameter, juce::StringArray floorNames,
                   juce::StringArray floorGlyphs);
    ~FloorSelector() override;

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent& e) override;

    /** The top floor is drawn in this colour when lit (Heaven's sparkle). */
    juce::Colour topFloorColour { juce::Colours::lightblue };

private:
    int floorAt (juce::Point<float> p) const;
    juce::Rectangle<float> floorBounds (int index) const;

    juce::StringArray names, glyphs;
    int selected = 0, hovered = -1;
    juce::ParameterAttachment attachment;
};
