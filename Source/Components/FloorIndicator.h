/*
  ------------------------------------------------------------------------------
    FloorIndicator.h

    The lift's floor display, above the floor selector: the floor sounding
    now ("1" to "4", an infinity sign for Heaven) in glowing amber over the
    dim ghost of the display's segments, and an up or down arrow lit while a
    mode change travels (the parameter has moved, the engine is fading out,
    switching and fading back in). On arrival the display flashes once.

    It shows the engine's floor, not the parameter's: during the fade the old
    floor is still the one sounding, which is the point of showing it (a mode
    change clears the tail).

    Author: Olivier Doaré, github.com/odoare
    (c) 2026 Olivier Doaré
    SPDX-License-Identifier: AGPL-3.0-or-later OR LicenseRef-FXME-Commercial
  ------------------------------------------------------------------------------
*/

#pragma once

#include <JuceHeader.h>
#include <functional>

class FloorIndicator : public juce::Component,
                       public juce::SettableTooltipClient,
                       private juce::Timer
{
public:
    struct State
    {
        int target = 0;      // the mode parameter
        int sounding = 0;    // the engine's floor
        float fade = 1.0f;   // the engine's mode fade, 1 settled
    };

    FloorIndicator (std::function<State()> source, juce::StringArray glyphs);
    ~FloorIndicator() override;

    void paint (juce::Graphics& g) override;

private:
    void timerCallback() override;

    std::function<State()> source;
    juce::StringArray glyphs;
    State state;
    int direction = 0;          // +1 up, -1 down, 0 still
    int travelTicks = 0;        // keeps the arrow lit long enough to be seen
    float arrival = 0.0f;       // the flash on arrival, 1 .. 0
    int lastSounding = -1;
};
