/*
  ------------------------------------------------------------------------------
    PluginProcessor.h

    StairwellToHeaven's processor: the parameters (APVTS), the global presets
    (fxme::PresetManager, FX-Mechanics/StairwellToHeaven/Presets plus the
    factory ones embedded from Source/Assets), and the glue to the JUCE-free
    engine (Source/Dsp/StairwellEngine): once per block it reads the
    parameters, resolves a synced pre-delay against the host tempo, and hands
    the engine one sth::Parameters.

    Author: Olivier Doaré, github.com/odoare
    (c) 2026 Olivier Doaré
    SPDX-License-Identifier: AGPL-3.0-or-later OR LicenseRef-FXME-Commercial
  ------------------------------------------------------------------------------
*/

#pragma once

#include <JuceHeader.h>
#include "Dsp/StairwellEngine.h"
#include <atomic>
#include <utility>

class StairwellAudioProcessor : public juce::AudioProcessor
{
public:
    StairwellAudioProcessor();
    ~StairwellAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                     { return true; }

    const juce::String getName() const override         { return JucePlugin_Name; }
    bool acceptsMidi() const override                   { return false; }
    bool producesMidi() const override                  { return false; }
    bool isMidiEffect() const override                  { return false; }
    double getTailLengthSeconds() const override        { return tailSeconds.load(); }

    int getNumPrograms() override                       { return 1; }
    int getCurrentProgram() override                    { return 0; }
    void setCurrentProgram (int) override               {}
    const juce::String getProgramName (int) override    { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept  { return apvts; }
    fxme::PresetManager& getPresetManager() noexcept         { return presetManager; }

    /** The parameters as the engine sees them now (message thread too: the
        decay display draws from them). */
    sth::Parameters currentParameters() const;

    /** The tail's energy in dB, for the display's glow. */
    float getTailEnergyDb() noexcept     { return engine.getTailEnergyDb().load (std::memory_order_relaxed); }

    /** The input's peak in dB since the last call (which resets it), for
        the reflections view's onsets. Message thread, one caller. */
    float takeInputPeakDb() noexcept
    {
        const float p = engine.takeInputPeak();
        return p > 1.0e-5f ? 20.0f * std::log10 (p) : -100.0f;
    }

    /** The floor sounding now (0 Room .. 4 Heaven) and the mode-change
        fade (1 settled), for the floor indicator. */
    int getActiveFloor() noexcept        { return engine.getActiveFloor().load (std::memory_order_relaxed); }
    float getModeFade() noexcept         { return engine.getModeFade().load (std::memory_order_relaxed); }

    /** True the first time it is asked in this plugin instance: the splash is
        shown once, not each time the window opens. */
    bool takeFirstSplash() noexcept      { return ! std::exchange (splashShown, true); }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

    juce::AudioProcessorValueTreeState apvts;
    fxme::PresetManager presetManager;
    sth::StairwellEngine engine;

    struct Raw
    {
        std::atomic<float>* mode, * decay, * size, * predelay, * predelaySync, * predelayDiv, * diffusion,
                          * damping, * dampFreq, * bass, * crossover, * modRate, * modDepth, * erLate, * width,
                          * lowCut, * highCut, * shimmer, * shimmerPitch, * shimmer2, * shimmerPitch2, * pitchedInput,
                          * freeze, * mix, * output;
    } raw {};

    std::atomic<double> hostBpm { 120.0 };
    std::atomic<double> tailSeconds { 3.0 };
    bool splashShown = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StairwellAudioProcessor)
};
