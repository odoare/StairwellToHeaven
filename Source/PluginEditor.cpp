/*
  ------------------------------------------------------------------------------
    PluginEditor.cpp

    See PluginEditor.h.

    Author: Olivier Doaré, github.com/odoare
    (c) 2026 Olivier Doaré
    SPDX-License-Identifier: AGPL-3.0-or-later OR LicenseRef-FXME-Commercial
  ------------------------------------------------------------------------------
*/

#include "PluginEditor.h"
#include "ParamIDs.h"
#include "BinaryData.h"   // the embedded images and factory presets
#include "Tooltips.h"

using namespace sth;

namespace
{
    const char* const helpBody =
        "Stairwell to Heaven is an algorithmic reverb: early reflections, then a "
        "diffuse tail from a figure-eight tank (Dattorro's), whose decay is set in "
        "seconds and shaped across frequency.\n\n"
        "Floors (the mode, on the left, bottom to top):\n"
        "  Room       a medium room, dense early reflections\n"
        "  Stairwell  hard parallel walls: a fluttering, ringing onset\n"
        "  Plate      bright and dense at once, no early reflections\n"
        "  Hall       slower build-up, long and dark\n"
        "  Heaven     a large hall whose tail climbs in pitch (Shimmer)\n"
        "Changing floors crossfades: the new space takes the sound at once while the "
        "old one's tail fades out over a second.\n\n"
        "The stairs: one step per third of an octave, as high as the tail lasts "
        "there. Damping lowers the steps on the right (above its frequency), Bass "
        "raises or lowers those on the left (under the crossover). The light above "
        "follows the tail as it plays.\n\n"
        "Freeze holds the tail for ever and stops taking sound in: play over it.\n\n"
        "Pre-delay can follow the host tempo (Sync, then a note value).\n\n"
        "Low cut and High cut act on the reverb only; Mix balances it against the "
        "dry signal.";

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
    attachButton (juce::AudioProcessorValueTreeState& apvts, const char* id, juce::Button& b)
    {
        return std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, id, b);
    }
}

StairwellAudioProcessorEditor::StairwellAudioProcessorEditor (StairwellAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p),
      floorIndicator ([this]
                      {
                          FloorIndicator::State st;
                          st.target = juce::roundToInt (processor.getAPVTS().getRawParameterValue (ids::mode)->load());
                          st.sounding = processor.getActiveFloor();
                          st.fade = processor.getModeFade();
                          return st;
                      },
                      juce::StringArray ("1", "2", "3", "4", juce::String (juce::CharPointer_UTF8 ("\xe2\x88\x9e")))),
      floors (*p.getAPVTS().getParameter (ids::mode), choices::modes,
              juce::StringArray ("1", "2", "3", "4", juce::String (juce::CharPointer_UTF8 ("\xe2\x88\x9e")))),
      reflections ([this]
                 {
                     ReflectionsView::Snapshot s;
                     s.params = processor.currentParameters();
                     s.inputPeakDb = processor.takeInputPeakDb();
                     return s;
                 }),
      stairs ([this]
              {
                  DecayStairs::Snapshot s;
                  s.params = processor.currentParameters();
                  s.sampleRate = processor.getSampleRate() > 0.0 ? processor.getSampleRate() : 48000.0;
                  s.tailEnergyDb = processor.getTailEnergyDb();
                  return s;
              }),
      decayKnob     (p.getAPVTS(), ids::decay,     "Decay",      theme::accent),
      sizeKnob      (p.getAPVTS(), ids::size,      "Size",       theme::accent),
      predelayKnob  (p.getAPVTS(), ids::predelay,  "Pre-delay",  theme::accent),
      diffusionKnob (p.getAPVTS(), ids::diffusion, "Diffusion",  theme::accent),
      dampingKnob   (p.getAPVTS(), ids::damping,   "Damping",    theme::accent),
      dampFreqKnob  (p.getAPVTS(), ids::dampFreq,  "Damp freq",  theme::accent),
      bassKnob      (p.getAPVTS(), ids::bass,      "Bass",       theme::accent),
      crossoverKnob (p.getAPVTS(), ids::crossover, "Crossover",  theme::accent),
      lowCutKnob    (p.getAPVTS(), ids::lowCut,    "Low cut",    theme::accent),
      highCutKnob   (p.getAPVTS(), ids::highCut,   "High cut",   theme::accent),
      modRateKnob   (p.getAPVTS(), ids::modRate,   "Mod rate",   theme::accent),
      modDepthKnob  (p.getAPVTS(), ids::modDepth,  "Mod depth",  theme::accent),
      erLateKnob    (p.getAPVTS(), ids::erLate,    "Early/Late", theme::accent),
      widthKnob     (p.getAPVTS(), ids::width,     "Width",      theme::accent),
      shimmerKnob   (p.getAPVTS(), ids::shimmer,   "Shimmer 1",  theme::shimmer),
      shimmer2Knob  (p.getAPVTS(), ids::shimmer2,  "Shimmer 2",  theme::shimmer),
      pitchedInputKnob (p.getAPVTS(), ids::pitchedInput, "Pitched in", theme::shimmer),
      mixFader      (p.getAPVTS(), ids::mix,       "Mix",        theme::accent),
      outputFader   (p.getAPVTS(), ids::output,    "Gain",       theme::accent)
{
    laf.setAccentColour (theme::accent);
    setLookAndFeel (&laf);

    // Top bar.
    topBar.setAccentColour (theme::accent);
    topBar.setBackgroundColour (theme::topBarBg);
    addAndMakeVisible (topBar);
    presetBar.setAccentColour (theme::accent);
    presetBar.setBrowserButtonVisible (true);
    presetBar.setBrowserSize (320, 380);
    tooltipsButton.setButtonText ("?");
    tooltipsButton.setAccent (theme::accent, theme::dimText);
    tooltipsButton.setTooltip (tips::global::tooltips);
    tooltipsButton.setToggleState (getUiTooltips(), juce::dontSendNotification);
    tooltipsButton.onClick = [this] { setUiTooltips (tooltipsButton.getToggleState()); };
    infoButton.setColours ({ theme::accent, theme::text, theme::stone, theme::panelLine });
    infoButton.setInfo (theme::displayName, helpBody);
    topBar.setRightControls ({ { &presetBar, 236 }, { &tooltipsButton, 24 }, { &infoButton, 22 } });
    topBar.setDecoration (assetImage ("icon_png"));
    topBar.setTitleImage (assetImage ("title_png"));
    topBar.onLogoClicked = [this] { splash.show (-1); };

    // Floors and Freeze.
    floorIndicator.setTooltip (tips::global::floorIndicator);
    addAndMakeVisible (floorIndicator);
    floors.setTooltip (tips::global::mode);
    addAndMakeVisible (floors);
    freezeButton.lightColour = theme::frozen;
    freezeButton.setTooltip (tips::global::freeze);
    freezeAtt = attachButton (p.getAPVTS(), ids::freeze, freezeButton);
    addAndMakeVisible (freezeButton);

    reflections.setTooltip (tips::global::reflections);
    addAndMakeVisible (reflections);
    stairs.setTooltip (tips::global::stairs);
    addAndMakeVisible (stairs);

    // Knobs.
    setUpKnob (decayKnob, tips::space::decay);
    setUpKnob (sizeKnob, tips::space::size);
    setUpKnob (predelayKnob, tips::space::predelay);
    setUpKnob (diffusionKnob, tips::space::diffusion);
    setUpKnob (dampingKnob, tips::tone::damping);
    setUpKnob (dampFreqKnob, tips::tone::dampFreq, true);
    setUpKnob (bassKnob, tips::tone::bass);
    setUpKnob (crossoverKnob, tips::tone::crossover, true);
    setUpKnob (lowCutKnob, tips::tone::lowCut, true);
    setUpKnob (highCutKnob, tips::tone::highCut, true);
    setUpKnob (modRateKnob, tips::motion::modRate);
    setUpKnob (modDepthKnob, tips::motion::modDepth);
    setUpKnob (erLateKnob, tips::motion::erLate);
    setUpKnob (widthKnob, tips::motion::width);
    setUpKnob (shimmerKnob, tips::motion::shimmer);
    theme::styleKnob (shimmerKnob, theme::shimmer);
    setUpKnob (shimmer2Knob, tips::motion::shimmer2);
    theme::styleKnob (shimmer2Knob, theme::shimmer);
    setUpKnob (pitchedInputKnob, tips::motion::pitchedInput);
    theme::styleKnob (pitchedInputKnob, theme::shimmer);
    bassKnob.setCentralValue (1.0);
    widthKnob.setCentralValue (100.0);

    syncButton.setButtonText ("Sync");
    syncButton.setAccent (theme::accent, theme::dimText, theme::stoneLight);
    syncButton.setTooltip (tips::space::sync);
    syncAtt = attachButton (p.getAPVTS(), ids::predelaySync, syncButton);
    addAndMakeVisible (syncButton);

    divisionBox.addItemList (choices::divisions, 1);
    theme::styleCombo (divisionBox);
    divisionBox.setTooltip (tips::space::division);
    divisionAtt = std::make_unique<ComboAttachment> (p.getAPVTS(), ids::predelayDiv, divisionBox);
    addAndMakeVisible (divisionBox);

    pitchBox.addItemList (choices::shimmerPitches, 1);
    theme::styleCombo (pitchBox, theme::shimmer);
    pitchBox.setTooltip (tips::motion::pitch);
    pitchAtt = std::make_unique<ComboAttachment> (p.getAPVTS(), ids::shimmerPitch, pitchBox);
    addAndMakeVisible (pitchBox);

    pitch2Box.addItemList (choices::shimmerPitches, 1);
    theme::styleCombo (pitch2Box, theme::shimmer);
    pitch2Box.setTooltip (tips::motion::pitch2);
    pitch2Att = std::make_unique<ComboAttachment> (p.getAPVTS(), ids::shimmerPitch2, pitch2Box);
    addAndMakeVisible (pitch2Box);

    setUpFader (mixFader, tips::global::mix);
    setUpFader (outputFader, tips::global::output);
    outputFader.setCentralValue (0.0);
    setUpMeter (inMeter, tips::global::inputMeter);
    setUpMeter (outMeter, tips::global::outputMeter);

    // CPU: the window light repaints the whole window while the tail plays.
    // Every control and display is cached as an image, redrawn only when it
    // changes itself, so those repaints only draw the backdrop and copy the
    // images over it; the top bar fills its bounds, so nothing is painted
    // under it.
    for (auto* c : getChildren())
        if (c != &tooltipWindow)
            c->setBufferedToImage (true);
    topBar.setOpaque (true);

    setResizable (true, true);
    setResizeLimits (theme::minWidth, theme::minHeight, 1800, 1200);
    setSize (theme::defaultWidth, theme::defaultHeight);

    splash.setImage (assetImage ("splash_png"));
    splash.setLink ("fx-mechanics.com", juce::URL ("https://fx-mechanics.com"), logoImage());
    splash.setLinkColour (theme::accent);
    addChildComponent (splash);
    splash.setBounds (getLocalBounds());
    if (processor.takeFirstSplash())
        splash.show (3000);

    timerCallback();
    startTimerHz (20);
}

StairwellAudioProcessorEditor::~StairwellAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void StairwellAudioProcessorEditor::setUpKnob (fxme::FxmeSlider& knob, const char* tooltip, bool logarithmic)
{
    knob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob.setShowLabel (true);
    knob.setTooltip (tooltip);
    if (logarithmic)
        knob.setLogarithmic();
    theme::styleKnob (knob);
    addAndMakeVisible (knob);
}

void StairwellAudioProcessorEditor::setUpFader (fxme::FxmeSlider& fader, const char* tooltip)
{
    // FxmeLookAndFeel's console fader (as FxmeFX's Controls.h sets it up).
    fader.setSliderStyle (juce::Slider::LinearVertical);
    fader.getProperties().set ("linearStyle", "console");
    fader.setShowLabel (true);
    fader.setTooltip (tooltip);
    fader.setColour (juce::Slider::trackColourId, theme::accent.darker());
    fader.setColour (juce::Slider::thumbColourId, theme::accent);
    fader.setColour (juce::Slider::rotarySliderOutlineColourId, theme::accent.darker (2.0f));
    fader.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xff2b2b2b));
    addAndMakeVisible (fader);
}

void StairwellAudioProcessorEditor::setUpMeter (fxme::StereoLevelMeter& meter, const char* tooltip)
{
    meter.setRange (-60.0f, 6.0f);
    meter.setTooltip (tooltip);
    addAndMakeVisible (meter);
}

void StairwellAudioProcessorEditor::applyAccent (juce::Colour a)
{
    accent = a;
    laf.setAccentColour (a);
    topBar.setAccentColour (a);
    presetBar.setAccentColour (a);
    tooltipsButton.setAccent (a, theme::dimText);
    infoButton.setColours ({ a, theme::text, theme::stone, theme::panelLine });
    syncButton.setAccent (a, theme::dimText, theme::stoneLight);
    splash.setLinkColour (a);

    for (auto* knob : { &decayKnob, &sizeKnob, &predelayKnob, &diffusionKnob, &dampingKnob, &dampFreqKnob,
                        &bassKnob, &crossoverKnob, &lowCutKnob, &highCutKnob, &modRateKnob, &modDepthKnob,
                        &erLateKnob, &widthKnob })
        theme::styleKnob (*knob, a);
    theme::styleCombo (divisionBox, a);
    for (auto* fader : { &mixFader, &outputFader })
    {
        fader->setColour (juce::Slider::trackColourId, a.darker());
        fader->setColour (juce::Slider::thumbColourId, a);
        fader->setColour (juce::Slider::rotarySliderOutlineColourId, a.darker (2.0f));
    }
    for (auto* meter : { &inMeter, &outMeter })
    {
        fxme::StereoLevelMeter::Colours c;
        c.background = theme::stoneDeep;
        c.outline = theme::panelLine;
        c.bar = a.withMultipliedSaturation (0.85f).darker (0.15f);
        c.peak = theme::lightFor (a);
        meter->setColours (c);
    }

    const auto light = theme::lightFor (a);
    reflections.setColours (a, light);
    stairs.setColours (a, light);
    repaint();
}

void StairwellAudioProcessorEditor::updateMeters()
{
    // Each meter's held max is taken (read and reset) here, the one reader.
    auto feed = [] (fxme::StereoLevelMeter& view, int ch, fxme::VuMeter& m)
    {
        view.update (ch, m.getRMS(), m.getMax());
        m.resetMax();
    };
    for (int ch = 0; ch < 2; ++ch)
    {
        feed (inMeter, ch, processor.getInputMeter (ch));
        feed (outMeter, ch, processor.getOutputMeter (ch));
    }
}

void StairwellAudioProcessorEditor::timerCallback()
{
    auto& apvts = processor.getAPVTS();
    const bool synced = apvts.getRawParameterValue (ids::predelaySync)->load() > 0.5f;
    const bool heaven = juce::roundToInt (apvts.getRawParameterValue (ids::mode)->load()) == (int) Mode::heaven;
    auto enable = [] (juce::Component& c, bool on) { if (c.isEnabled() != on) c.setEnabled (on); };
    enable (predelayKnob, ! synced);
    enable (divisionBox, synced);
    enable (shimmerKnob, heaven);
    enable (pitchBox, heaven);
    enable (shimmer2Knob, heaven);
    enable (pitch2Box, heaven);
    enable (pitchedInputKnob, heaven);
    if (heaven != heavenMode)
    {
        heavenMode = heaven;
        repaint (heavenGroup);
    }

    // The floor's colour, gliding to the selected floor (at once on opening).
    const float floor = (float) juce::roundToInt (apvts.getRawParameterValue (ids::mode)->load());
    if (colourFloor < 0.0f || std::abs (floor - colourFloor) > 0.0f)
    {
        const float next = colourFloor < 0.0f || std::abs (floor - colourFloor) < 0.02f
                               ? floor : colourFloor + 0.2f * (floor - colourFloor);
        colourFloor = next;
        applyAccent (theme::accentForFloor (colourFloor));
    }

    updateMeters();

    // The window's light: the tail's energy from -60 dB (dark) to 0 dB, up
    // fast, down slowly; frozen, a steady glow.
    const bool frozen = apvts.getRawParameterValue (ids::freeze)->load() > 0.5f;
    const float energy = juce::jlimit (0.0f, 1.0f, (processor.getTailEnergyDb() + 60.0f) / 60.0f);
    const float target = frozen ? juce::jmax (0.7f, energy) : energy;
    windowLightLevel += (target > windowLightLevel ? 0.4f : 0.06f) * (target - windowLightLevel);
    const auto colour = frozen ? theme::frozen : theme::lightFor (accent);
    // Repainted in steps of 1 % (finer is invisible: the light's alpha is
    // at most 0.28), and only where the light reaches. The level glides on
    // its own, so the small steps of a slow fade add up.
    if (std::abs (windowLightLevel - windowLight) > 0.01f || colour != windowLightColour)
    {
        windowLight = windowLightLevel;
        windowLightColour = colour;
        repaint (getLocalBounds().withHeight (juce::roundToInt ((float) getHeight() * theme::backdropLightReach) + 2));
    }
}

//==============================================================================
juce::Image StairwellAudioProcessorEditor::assetImage (const char* resourceName)
{
    int size = 0;
    if (const char* data = BinaryData::getNamedResource (resourceName, size))
        return juce::ImageCache::getFromMemory (data, size);
    return {};
}

juce::Image StairwellAudioProcessorEditor::logoImage()
{
    return assetImage ("logo686_png");
}

void StairwellAudioProcessorEditor::paint (juce::Graphics& g)
{
    // The backdrop from its two pre-rendered layers (see backdropStone).
    const auto bounds = getLocalBounds();
    if (backdropStone.getBounds() != bounds)
    {
        backdropStone = juce::Image (juce::Image::RGB, juce::jmax (1, bounds.getWidth()), juce::jmax (1, bounds.getHeight()), false);
        juce::Graphics sg (backdropStone);
        theme::paintBackdropStone (sg, bounds.toFloat());
        backdropLight = {};
    }
    g.drawImageAt (backdropStone, 0, 0);
    if (windowLight > 0.001f)
    {
        if (backdropLight.getBounds() != bounds || backdropLightColour != windowLightColour)
        {
            backdropLight = juce::Image (juce::Image::ARGB, bounds.getWidth(), bounds.getHeight(), true);
            juce::Graphics lg (backdropLight);
            theme::paintBackdropLight (lg, bounds.toFloat(), windowLightColour);
            backdropLightColour = windowLightColour;
        }
        g.setOpacity (juce::jmin (1.0f, windowLight));
        g.drawImageAt (backdropLight, 0, 0);
        g.setOpacity (1.0f);
    }

    // Section names, left of each knob row.
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    const std::pair<juce::Rectangle<int>, const char*> rows[] { { spaceRow, "SPACE" }, { toneRow, "TONE" }, { motionRow, "MOTION" } };
    for (const auto& [row, name] : rows)
    {
        if (row.isEmpty())
            continue;
        g.setColour (theme::dimText);
        g.saveState();
        g.addTransform (juce::AffineTransform::rotation (-juce::MathConstants<float>::halfPi,
                                                         (float) row.getX() + 7.0f, (float) row.getCentreY()));
        g.drawText (name, juce::Rectangle<float> ((float) row.getX() + 7.0f - 40.0f, (float) row.getCentreY() - 7.0f, 80.0f, 14.0f),
                    juce::Justification::centred, false);
        g.restoreState();
        g.setColour (theme::panelLine.withAlpha (0.6f));
        g.drawVerticalLine (row.getX() + 15, (float) row.getY() + 8.0f, (float) row.getBottom() - 8.0f);
    }

    // The names above the meters and the faders.
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    const std::pair<juce::Rectangle<int>, const char*> levelNames[] { { inLabel, "In" }, { mixLabel, "Mix" },
                                                                       { gainLabel, "Gain" }, { outLabel, "Out" } };
    for (const auto& [box, name] : levelNames)
    {
        g.setColour (box == mixLabel || box == gainLabel ? theme::text : theme::dimText);
        g.drawText (name, box, juce::Justification::centred, false);
    }

    // The HEAVEN group in Motion: a frame with its title, lit in Heaven's
    // colour on the top floor, dim elsewhere (its controls are disabled).
    if (! heavenGroup.isEmpty())
    {
        const auto frame = heavenGroup.toFloat().reduced (0.5f);
        const auto colour = heavenMode ? theme::shimmer : theme::panelLine;
        g.setColour (colour.withAlpha (heavenMode ? 0.08f : 0.0f));
        g.fillRoundedRectangle (frame, 5.0f);
        g.setColour (colour.withAlpha (heavenMode ? 0.7f : 0.6f));
        g.drawRoundedRectangle (frame, 5.0f, 1.0f);

        g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
        g.setColour (heavenMode ? theme::shimmer : theme::dimText);
        g.drawText ("HEAVEN", juce::Rectangle<float> (frame.getX() + 8.0f, frame.getY() + 2.0f, 60.0f, 11.0f),
                    juce::Justification::centredLeft, false);
    }
}

void StairwellAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    topBar.setBounds (area.removeFromTop (theme::topBarHeight));
    splash.setBounds (getLocalBounds());
    area.reduce (theme::margin, theme::margin);

    // The knob rows are sized first; the displays get the rest.
    const int rowHeight = juce::jlimit (84, 130, (area.getHeight() - 200 - theme::margin) / 3);

    // Top, across the whole width: the early reflections on the left, the
    // decay stairs (the tail) on the right.
    {
        auto display = area.removeFromTop (area.getHeight() - 3 * rowHeight - theme::margin);
        reflections.setBounds (display.removeFromLeft (juce::jmax (220, display.getWidth() / 3)));
        display.removeFromLeft (theme::margin);
        stairs.setBounds (display);
    }
    area.removeFromTop (theme::margin);

    // Under them, left: the floor indicator, the floors, Freeze.
    auto left = area.removeFromLeft (150);
    floorIndicator.setBounds (left.removeFromTop (50).reduced (14, 0));
    left.removeFromTop (6);
    freezeButton.setBounds (left.removeFromBottom (60).reduced (4, 2));
    left.removeFromBottom (6);
    floors.setBounds (left);
    area.removeFromLeft (theme::margin);

    // Right: the input meter, Mix and Gain, the output meter; each named
    // above.
    {
        auto right = area.removeFromRight (170);
        auto names = right.removeFromTop (16);
        auto column = [&] (int w, juce::Rectangle<int>& label)
        {
            label = names.removeFromLeft (w);
            return right.removeFromLeft (w);
        };
        constexpr int meterW = 20, faderW = 62;
        // The meters span the faders' travel, not their value read-outs.
        inMeter.setBounds (column (meterW, inLabel).reduced (3, 0).withTrimmedTop (6).withTrimmedBottom (24));
        mixFader.setBounds (column (faderW, mixLabel));
        outputFader.setBounds (column (faderW, gainLabel));
        names.removeFromLeft (right.getWidth() - meterW);
        right.removeFromLeft (right.getWidth() - meterW);
        outMeter.setBounds (column (meterW, outLabel).reduced (3, 0).withTrimmedTop (6).withTrimmedBottom (24));
    }
    area.removeFromRight (theme::margin);

    // Centre: three rows of knobs.
    spaceRow = area.removeFromTop (rowHeight);
    toneRow = area.removeFromTop (rowHeight);
    motionRow = area;

    auto layoutRow = [] (juce::Rectangle<int> row, std::initializer_list<juce::Component*> items)
    {
        row.removeFromLeft (20);   // the section name
        const int w = row.getWidth() / (int) items.size();
        for (auto* c : items)
            if (c != nullptr)
                c->setBounds (row.removeFromLeft (w).reduced (2));
            else
                row.removeFromLeft (w);
    };

    // Space: the sync switch and the note menu share one column.
    layoutRow (spaceRow, { &decayKnob, &sizeKnob, &predelayKnob, nullptr, &diffusionKnob, &erLateKnob });
    {
        auto r = spaceRow.withTrimmedLeft (20);
        const int w = r.getWidth() / 6;
        auto cell = r.withX (r.getX() + 3 * w).withWidth (w).reduced (6, 0);
        syncButton.setBounds (cell.withSizeKeepingCentre (cell.getWidth(), 24).translated (0, -16));
        divisionBox.setBounds (cell.withSizeKeepingCentre (cell.getWidth(), 24).translated (0, 16));
    }
    layoutRow (toneRow, { &dampingKnob, &dampFreqKnob, &bassKnob, &crossoverKnob, &lowCutKnob, &highCutKnob });
    // Motion: modulation and width, then the HEAVEN group (framed in
    // paint()): each shimmer voice's knob with its interval menu under it,
    // and Pitched in.
    {
        auto r = motionRow.withTrimmedLeft (20);
        const int w = r.getWidth() / 6;
        for (auto* knob : { &modRateKnob, &modDepthKnob, &widthKnob })
            knob->setBounds (r.removeFromLeft (w).reduced (2));
        heavenGroup = r.reduced (2, 1);
        auto group = heavenGroup.reduced (4, 4).withTrimmedTop (8);   // room for the frame's title
        const int gw = group.getWidth() / 3;
        auto voiceCell = [] (juce::Rectangle<int> cell, juce::Component& knob, juce::Component& menu)
        {
            menu.setBounds (cell.removeFromBottom (22).reduced (2, 0));
            knob.setBounds (cell.reduced (2, 0));
        };
        voiceCell (group.removeFromLeft (gw), shimmerKnob, pitchBox);
        voiceCell (group.removeFromLeft (gw), shimmer2Knob, pitch2Box);
        pitchedInputKnob.setBounds (group.reduced (2, 0));
    }
}
