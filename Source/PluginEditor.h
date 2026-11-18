/*
  ------------------------------------------------------------------------------
    PluginEditor.h

    StairwellToHeaven's window:

      top bar     the title (artwork), version, the preset bar with its
                  browser, the "?" (hover help on/off) and the info button;
                  the icon opens the splash.
      top         across the whole width: the early reflections (time up,
                  one circle per reflection placed by its pan)
                  and the decay stairs (RT60 against frequency, with the
                  tail's light above).
      under them  left, the floor indicator (the floor sounding, an arrow
                  while a mode change travels), the floor selector (the mode,
                  as a lift panel: Room at the bottom, Heaven at the top) and
                  Freeze (a halo); centre, three rows of six columns: Space
                  (decay, size, pre-delay and its sync, diffusion,
                  early/late), Tone (damping, its frequency, bass,
                  crossover, low and high cut), Motion (modulation rate and
                  depth, width, then the framed HEAVEN group: the two
                  shimmer voices, each with its interval menu under it, and
                  Pitched input);
                  right, the input meter, the Mix and Gain faders (named
                  above them) and the output meter.

    The backdrop's light follows the reverb's tail: the window brightens as
    it blooms and dims as it dies (steady while frozen).

    The whole window takes the floor's colour (theme::accentForFloor): pale
    gold in the Room, salmon in the Hall, blue in Heaven, gliding from one
    to the next when the floor changes (applyAccent()).

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
#include "StairwellVersion.h"   // generated: STH_VERSION_DISPLAY
#include "Theme.h"
#include "Components/DecayStairs.h"
#include "Components/FloorSelector.h"
#include "Components/FloorIndicator.h"
#include "Components/ReflectionsView.h"
#include "Components/HaloButton.h"

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
    void setUpMeter (fxme::StereoLevelMeter& meter, const char* tooltip);
    void applyAccent (juce::Colour accent);
    void updateMeters();

    static juce::Image logoImage();
    static juce::Image assetImage (const char* resourceName);

    StairwellAudioProcessor& processor;
    fxme::FxmeLookAndFeel laf;

    // The version from the git tag (cmake/Version.cmake): "0.1.0", or
    // "0.1.0-3-gabc1234" for a build after it.
    fxme::TopBar topBar { sth::theme::displayName, "Algorithmic reverb", STH_VERSION_DISPLAY, logoImage() };
    fxme::PresetBarComponent presetBar { processor.getPresetManager() };
    fxme::AccentToggle tooltipsButton;
    fxme::InfoButton infoButton;

    FloorIndicator floorIndicator;
    FloorSelector floors;
    HaloButton freezeButton { "Freeze" };
    ReflectionsView reflections;
    DecayStairs stairs;

    // Space
    fxme::FxmeSlider decayKnob, sizeKnob, predelayKnob, diffusionKnob;
    fxme::AccentToggle syncButton;
    juce::ComboBox divisionBox;
    // Tone
    fxme::FxmeSlider dampingKnob, dampFreqKnob, bassKnob, crossoverKnob, lowCutKnob, highCutKnob;
    // Motion
    fxme::FxmeSlider modRateKnob, modDepthKnob, erLateKnob, widthKnob, shimmerKnob, shimmer2Knob, pitchedInputKnob;
    juce::ComboBox pitchBox, pitch2Box;   // the two shimmer voices' intervals, stacked in one column
    // Levels
    fxme::FxmeSlider mixFader, outputFader;
    fxme::StereoLevelMeter inMeter, outMeter;
    juce::Rectangle<int> inLabel, mixLabel, gainLabel, outLabel;   // the names above them

    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<ButtonAttachment> freezeAtt, syncAtt;
    std::unique_ptr<ComboAttachment> divisionAtt, pitchAtt, pitch2Att;

    juce::Rectangle<int> spaceRow, toneRow, motionRow;
    juce::Rectangle<int> heavenGroup;   // the shimmer controls' frame in Motion
    bool heavenMode = false;            // drawn lit on the top floor

    /** The window's light (0 .. 1), following the tail: rises fast, falls
        slowly, held while frozen; and its colour (pale blue in Heaven). */
    float windowLight = 0.0f;        // as drawn
    float windowLightLevel = 0.0f;   // following the tail
    juce::Colour windowLightColour = sth::theme::glow;

    /** The backdrop, rendered once (stone; the light at full strength, in
        its colour): paint() copies the stone and blends the light at
        windowLight. Rebuilt on resize, and the light on a colour change. */
    juce::Image backdropStone, backdropLight;
    juce::Colour backdropLightColour;

    /** The floor whose colour the window wears (fractional while gliding to
        the selected one) and that colour. */
    float colourFloor = -1.0f;
    juce::Colour accent = sth::theme::accent;

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
