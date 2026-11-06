/*
  ------------------------------------------------------------------------------
    HaloButton.h

    Freeze, as a halo: a ring of light floating over the word. Off, a dim
    stone ring; on, it glows in the frozen colour (soft layers of light
    around the ring), breathing slowly while the tail is held. A latching
    juce::Button, so a ButtonAttachment binds it to the parameter.

    Author: Olivier Doaré, github.com/odoare
    (c) 2026 Olivier Doaré
    SPDX-License-Identifier: AGPL-3.0-or-later OR LicenseRef-FXME-Commercial
  ------------------------------------------------------------------------------
*/

#pragma once

#include <JuceHeader.h>

class HaloButton : public juce::Button,
                   private juce::Timer
{
public:
    explicit HaloButton (const juce::String& text);
    ~HaloButton() override;

    /** The halo's colour when on. */
    juce::Colour lightColour { juce::Colours::lightblue };

    void paintButton (juce::Graphics& g, bool highlighted, bool down) override;
    void buttonStateChanged() override;
    void clicked() override;

private:
    void timerCallback() override;
    void updateTimer();
};
