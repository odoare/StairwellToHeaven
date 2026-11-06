/*
  ------------------------------------------------------------------------------
    PluginEditor.h

    StairwellToHeaven's window:

      top bar     name, version, the preset bar with its browser, the "?"
                  (hover help on/off) and the info button; the icon opens
                  the splash.
      left        the floor indicator (the floor sounding, an arrow while a
                  mode change travels), the floor selector (the mode, as a
                  lift panel: Room at the bottom, Heaven at the top) and
                  Freeze under it.
      centre      the decay stairs (RT60 against frequency, with the tail's
                  light above), then three rows of knobs: Space (decay, size,
                  pre-delay and its sync, diffusion), Tone (damping, its
                  frequency, bass, crossover, low and high cut), Motion
                  (modulation, early/late, width, shimmer and its interval).
      right       Mix and Output faders.

    The backdrop's light follows the reverb's tail: the window brightens as
    it blooms and dims as it dies (steady while frozen).

    Controls that do nothing in the current state are disabled (the
    pre-delay knob while synced and the note menu while not, the shimmer
    outside Heaven).

    Author: Olivier Doaré, github.com/odoare
    (c) 2026 Olivier Doaré
    SPDX-License-Identifier: AGPL-3.0-or-later OR LicenseRef-FXME-Commercial
  ------------------------------------------------------------------------------
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "AppSettings.h"
#include "Theme.h"
#include "Components/DecayStairs.h"
#include "Components/FloorSelector.h"
#include "Components/FloorIndicator.h"

class StairwellAudioProcessorEditor : public juce::AudioProcessorEditor,
                                      private juce::Timer
{
public:
    explicit StairwellAudioProcessorEditor (StairwellAudioProcessor&);
    ~StairwellAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setUpKnob (fxme::FxmeSlider& knob, const char* tooltip, bool logarithmic = false);
    void setUpFader (fxme::FxmeSlider& fader, const char* tooltip);

    static juce::Image logoImage();
    static juce::Image assetImage (const char* resourceName);

    StairwellAudioProcessor& processor;
    fxme::FxmeLookAndFeel laf;

    fxme::TopBar topBar { sth::theme::displayName, "Algorithmic reverb", JucePlugin_VersionString, logoImage() };
    fxme::PresetBarComponent presetBar { processor.getPresetManager() };
    fxme::AccentToggle tooltipsButton;
    fxme::InfoButton infoButton;

    FloorIndicator floorIndicator;
    FloorSelector floors;
    fxme::AccentToggle freezeButton;
    DecayStairs stairs;

    // Space
    fxme::FxmeSlider decayKnob, sizeKnob, predelayKnob, diffusionKnob;
    fxme::AccentToggle syncButton;
    juce::ComboBox divisionBox;
    // Tone
    fxme::FxmeSlider dampingKnob, dampFreqKnob, bassKnob, crossoverKnob, lowCutKnob, highCutKnob;
    // Motion
    fxme::FxmeSlider modRateKnob, modDepthKnob, erLateKnob, widthKnob, shimmerKnob;
    juce::ComboBox pitchBox;
    // Levels
    fxme::FxmeSlider mixFader, outputFader;

    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<ButtonAttachment> freezeAtt, syncAtt;
    std::unique_ptr<ComboAttachment> divisionAtt, pitchAtt;

    juce::Rectangle<int> spaceRow, toneRow, motionRow;

    /** The window's light (0 .. 1), following the tail: rises fast, falls
        slowly, held while frozen; and its colour (pale blue in Heaven). */
    float windowLight = 0.0f;
    juce::Colour windowLightColour = sth::theme::glow;

    /** The splash: over everything, once per instance for 3 s, and until
        clicked from the top bar's icon or logo. */
    fxme::SplashOverlay splash;

    /** Hover help, asking the "?" switch before each tip. */
    struct ToggleableTooltipWindow : public juce::TooltipWindow
    {
        using juce::TooltipWindow::TooltipWindow;
        juce::String getTipFor (juce::Component& c) override
        {
            return sth::getUiTooltips() ? juce::TooltipWindow::getTipFor (c) : juce::String();
        }
    };
    ToggleableTooltipWindow tooltipWindow { this, 700 };

    // Exactly one per editor, after the child components.
    fxme::TextEntryFocusFixer textEntryFixer { *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StairwellAudioProcessorEditor)
};
