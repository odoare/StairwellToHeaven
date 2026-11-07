/*
  ------------------------------------------------------------------------------
    PluginProcessor.cpp

    See PluginProcessor.h.

    Author: Olivier Doaré, github.com/odoare
    (c) 2026 Olivier Doaré
    SPDX-License-Identifier: AGPL-3.0-or-later OR LicenseRef-FXME-Commercial
  ------------------------------------------------------------------------------
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParamIDs.h"
#include "BinaryData.h"   // the embedded images and factory presets
#include <FxmeTools/dsp/DelayTimeResolver.h>

namespace
{
    // State format version, written on the saved XML from day one. Bump it
    // when the state layout changes and migrate in setStateInformation.
    constexpr int kStateVersion = 1;

    juce::String hz (float v, int)
    {
        return v >= 1000.0f ? juce::String (v * 0.001f, 1) + " kHz" : juce::String (juce::roundToInt (v)) + " Hz";
    }
}

StairwellAudioProcessor::StairwellAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameters()),
      presetManager (apvts,
                     fxme::PresetManager::getVendorPresetDirectory ("StairwellToHeaven"),
                     BinaryData::namedResourceList,
                     BinaryData::namedResourceListSize,
                     BinaryData::getNamedResource)
{
    using namespace sth::ids;
    auto p = [this] (const char* id) { return apvts.getRawParameterValue (id); };
    raw = { p (mode), p (decay), p (size), p (predelay), p (predelaySync), p (predelayDiv), p (diffusion),
            p (damping), p (dampFreq), p (bass), p (crossover), p (modRate), p (modDepth), p (erLate), p (width),
            p (lowCut), p (highCut), p (shimmer), p (shimmerPitch), p (shimmer2), p (shimmerPitch2), p (pitchedInput),
            p (freeze), p (mix), p (output) };

    // FxmeNet: everything the editor reads that is not a parameter. On a
    // mirror (the DAW side, which never processes) these are written by the
    // server's telemetry; the editor reads them as usual.
    net.telemetry().addValue ("tail", engine.getTailEnergyDb());   // the stairs' and the window's light
    net.telemetry().addValue ("fade", engine.getModeFade());       // the floor indicator
    net.telemetry().addScalar ("floor", [this] { return (float) engine.getActiveFloor().load (std::memory_order_relaxed); });
    net.telemetry().addPeak ("input", [this] { return engine.takeInputPeak(); });   // the reflections' sweep
}

StairwellAudioProcessor::~StairwellAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout StairwellAudioProcessor::createParameters()
{
    using namespace sth;
    using Range = juce::NormalisableRange<float>;
    using Attr = juce::AudioParameterFloatAttributes;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto addFloat = [&params] (const char* id, const char* name, Range range, float def, Attr attr = {})
    {
        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name, range, def, attr));
    };
    auto addChoice = [&params] (const char* id, const char* name, const juce::StringArray& items, int def)
    {
        params.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, 1 }, name, items, def));
    };
    auto addBool = [&params] (const char* id, const char* name, bool def)
    {
        params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, 1 }, name, def));
    };
    const auto withHz = Attr().withStringFromValueFunction (hz).withLabel ("Hz");

    addChoice (ids::mode, "Mode", choices::modes, (int) Mode::hall);
    addFloat (ids::decay, "Decay", Range (0.1f, 30.0f, 0.01f, 0.35f), 2.5f, Attr().withLabel ("s"));
    addFloat (ids::size, "Size", Range (0.0f, 100.0f, 0.1f), 50.0f, Attr().withLabel ("%"));
    addFloat (ids::predelay, "Pre-delay", Range (0.0f, 500.0f, 0.1f, 0.5f), 20.0f, Attr().withLabel ("ms"));
    addBool (ids::predelaySync, "Pre-delay sync", false);
    addChoice (ids::predelayDiv, "Pre-delay note", choices::divisions, choices::defaultDivision);
    addFloat (ids::diffusion, "Diffusion", Range (0.0f, 100.0f, 0.1f), 80.0f, Attr().withLabel ("%"));
    addFloat (ids::damping, "Damping", Range (0.0f, 100.0f, 0.1f), 40.0f, Attr().withLabel ("%"));
    addFloat (ids::dampFreq, "Damping frequency", Range (1000.0f, 16000.0f, 1.0f, 0.3f), 5000.0f, withHz);
    addFloat (ids::bass, "Bass", Range (0.5f, 2.5f, 0.01f), 1.2f, Attr().withLabel ("x"));
    addFloat (ids::crossover, "Crossover", Range (100.0f, 1000.0f, 1.0f, 0.5f), 300.0f, withHz);
    addFloat (ids::modRate, "Modulation rate", Range (0.05f, 5.0f, 0.01f, 0.4f), 0.7f, Attr().withLabel ("Hz"));
    addFloat (ids::modDepth, "Modulation depth", Range (0.0f, 100.0f, 0.1f), 30.0f, Attr().withLabel ("%"));
    addFloat (ids::erLate, "Early / Late", Range (0.0f, 100.0f, 0.1f), 60.0f, Attr().withLabel ("%"));
    addFloat (ids::width, "Width", Range (0.0f, 150.0f, 0.1f), 100.0f, Attr().withLabel ("%"));
    addFloat (ids::lowCut, "Low cut", Range (20.0f, 1000.0f, 1.0f, 0.3f), 20.0f,
              Attr().withStringFromValueFunction ([] (float v, int n) { return v <= sth::StairwellEngine::lowCutOffHz ? juce::String ("Off") : hz (v, n); }));
    addFloat (ids::highCut, "High cut", Range (1000.0f, 20000.0f, 1.0f, 0.3f), 20000.0f,
              Attr().withStringFromValueFunction ([] (float v, int n) { return v >= sth::StairwellEngine::highCutOffHz ? juce::String ("Off") : hz (v, n); }));
    addFloat (ids::shimmer, "Shimmer", Range (0.0f, 100.0f, 0.1f), 50.0f, Attr().withLabel ("%"));
    addChoice (ids::shimmerPitch, "Shimmer interval", choices::shimmerPitches, choices::defaultShimmerPitch);
    addFloat (ids::shimmer2, "Shimmer 2", Range (0.0f, 100.0f, 0.1f), 0.0f, Attr().withLabel ("%"));
    addChoice (ids::shimmerPitch2, "Shimmer 2 interval", choices::shimmerPitches, choices::defaultShimmerPitch2);
    addFloat (ids::pitchedInput, "Pitched input", Range (0.0f, 100.0f, 0.1f), 0.0f, Attr().withLabel ("%"));
    addBool (ids::freeze, "Freeze", false);
    addFloat (ids::mix, "Mix", Range (0.0f, 100.0f, 0.1f), 30.0f, Attr().withLabel ("%"));
    addFloat (ids::output, "Output", Range (-24.0f, 24.0f, 0.1f), 0.0f, Attr().withLabel ("dB"));

    return { params.begin(), params.end() };
}

sth::Parameters StairwellAudioProcessor::currentParameters() const
{
    sth::Parameters p;
    p.mode = (sth::Mode) juce::jlimit (0, sth::numModes - 1, juce::roundToInt (raw.mode->load()));
    p.decay = raw.decay->load();
    p.size01 = raw.size->load() * 0.01f;

    if (raw.predelaySync->load() > 0.5f)
    {
        fxme::DelayTimeResolver resolver;
        resolver.bpm = hostBpm.load();
        resolver.minSeconds = 0.0f;
        resolver.maxSeconds = sth::StairwellEngine::maxPredelaySeconds;
        const int div = juce::jlimit (0, sth::choices::divisions.size() - 1, juce::roundToInt (raw.predelayDiv->load()));
        p.predelaySeconds = resolver.toSeconds (sth::choices::divisionWholeNotes[div], fxme::DelayTimeMode::tempoSync);
    }
    else
    {
        p.predelaySeconds = raw.predelay->load() * 0.001f;
    }

    p.diffusion01 = raw.diffusion->load() * 0.01f;
    p.damping01 = raw.damping->load() * 0.01f;
    p.dampHz = raw.dampFreq->load();
    p.bass = raw.bass->load();
    p.crossoverHz = raw.crossover->load();
    p.modRateHz = raw.modRate->load();
    p.modDepth01 = raw.modDepth->load() * 0.01f;
    p.erLate01 = raw.erLate->load() * 0.01f;
    p.width01 = raw.width->load() * 0.01f;
    p.lowCutHz = raw.lowCut->load();
    p.highCutHz = raw.highCut->load();
    p.shimmer01 = raw.shimmer->load() * 0.01f;
    const int lastPitch = sth::choices::shimmerPitches.size() - 1;
    p.shimmerSemitones = sth::choices::shimmerSemitonesFor (juce::jlimit (0, lastPitch, juce::roundToInt (raw.shimmerPitch->load())));
    p.shimmer2_01 = raw.shimmer2->load() * 0.01f;
    p.pitchedInput01 = raw.pitchedInput->load() * 0.01f;
    p.shimmerSemitones2 = sth::choices::shimmerSemitonesFor (juce::jlimit (0, lastPitch, juce::roundToInt (raw.shimmerPitch2->load())));
    p.freeze = raw.freeze->load() > 0.5f;
    p.mix01 = raw.mix->load() * 0.01f;
    p.outDb = raw.output->load();
    return p;
}

void StairwellAudioProcessor::prepareToPlay (double sampleRate, int)
{
    engine.setParameters (currentParameters());
    engine.prepare (sampleRate);
}

bool StairwellAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Stereo out; stereo in, or mono in (feeding both sides).
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    const auto in = layouts.getMainInputChannelSet();
    return in == juce::AudioChannelSet::stereo() || in == juce::AudioChannelSet::mono();
}

void StairwellAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    // FxmeNet: a mirror never processes (its audio runs on the server); if a
    // host calls it anyway, leave the engine alone, so the telemetry it
    // shows is not overwritten.
    if (buffer.getNumChannels() < 2 || net.isMirror())
        return;
    if (getTotalNumInputChannels() == 1)
        buffer.copyFrom (1, 0, buffer, 0, 0, numSamples);

    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpm = pos->getBpm())
                hostBpm.store (*bpm);

    engine.setParameters (currentParameters());
    engine.process (buffer.getWritePointer (0), buffer.getWritePointer (1), numSamples);
}

juce::AudioProcessorEditor* StairwellAudioProcessor::createEditor()
{
    return new StairwellAudioProcessorEditor (*this);
}

void StairwellAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
        if (auto xml = state.createXml())
        {
            xml->setAttribute ("stateVersion", kStateVersion);
            copyXmlToBinary (*xml, destData);
        }
}

void StairwellAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            const int stateVersion = xml->getIntAttribute ("stateVersion", 1);
            juce::ignoreUnused (stateVersion);   // nothing to migrate yet
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new StairwellAudioProcessor();
}
