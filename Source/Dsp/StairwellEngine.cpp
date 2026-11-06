/*
  ------------------------------------------------------------------------------
    StairwellEngine.cpp

    See StairwellEngine.h.

    Author: Olivier Doaré, github.com/odoare
    (c) 2026 Olivier Doaré
    SPDX-License-Identifier: AGPL-3.0-or-later OR LicenseRef-FXME-Commercial
  ------------------------------------------------------------------------------
*/

#include "StairwellEngine.h"
#include <FxmeTools/dsp/Sanitise.h>
#include <algorithm>
#include <cmath>

namespace sth
{

namespace
{
    constexpr double kSmoothingSeconds = 0.05;
    constexpr double kModeFadeSeconds  = 0.03;
    constexpr double kButterworthK     = 1.4142135623730951;
    constexpr float  kHalfPi           = 1.5707963f;

    float dbToGain (float db) { return std::pow (10.0f, db / 20.0f); }
}

ModeVoicing StairwellEngine::modeVoicing (Mode m) noexcept
{
    using P = fxme::EarlyReflections::Pattern;
    switch (m)
    {
        case Mode::room:
            return { { 0.75f, 0.625f, 0.60f, 0.45f,  9000.0f }, 0.30f, 0.80f, P::room,      0.6f, 1.4f, 0.35f, 0.5f, false };
        case Mode::stairwell:
            return { { 0.70f, 0.55f,  0.50f, 0.40f, 12000.0f }, 0.35f, 0.90f, P::stairwell, 0.7f, 1.5f, 0.50f, 0.4f, false };
        case Mode::plate:
            return { { 0.75f, 0.625f, 0.70f, 0.50f, 14000.0f }, 0.50f, 1.30f, P::plate,     1.0f, 1.0f, 0.00f, 0.8f, false };
        case Mode::hall:
            return { { 0.60f, 0.50f,  0.70f, 0.50f,  8000.0f }, 0.90f, 2.20f, P::hall,      0.8f, 2.0f, 0.20f, 1.0f, false };
        case Mode::heaven:
            return { { 0.60f, 0.50f,  0.70f, 0.50f,  9000.0f }, 1.20f, 2.50f, P::hall,      1.0f, 2.2f, 0.15f, 1.0f, true };
    }
    return modeVoicing (Mode::hall);
}

fxme::TankReverb::Settings StairwellEngine::tankSettingsFor (const Parameters& p) noexcept
{
    const auto v = modeVoicing (p.mode);
    fxme::TankReverb::Settings s;
    s.rt60 = std::clamp (p.decay, 0.1f, 60.0f);
    s.size = v.sizeLow + (v.sizeHigh - v.sizeLow) * std::clamp (p.size01, 0.0f, 1.0f);
    s.diffusion = std::clamp (p.diffusion01, 0.0f, 1.0f);
    s.dampHz = p.dampHz;
    s.hfRatio = 1.0f - 0.9f * std::clamp (p.damping01, 0.0f, 1.0f);
    s.bassMultiplier = p.bass;
    s.crossoverHz = p.crossoverHz;
    s.modRateHz = p.modRateHz;
    s.modDepth = std::clamp (p.modDepth01, 0.0f, 1.0f) * v.modScale;
    s.freeze = p.freeze;
    s.shimmer = v.shimmer ? std::clamp (p.shimmer01, 0.0f, 1.0f) : 0.0f;
    s.shimmerSemitones = p.shimmerSemitones;
    s.shimmer2 = v.shimmer ? std::clamp (p.shimmer2_01, 0.0f, 1.0f) : 0.0f;
    s.shimmerSemitones2 = p.shimmerSemitones2;
    s.voicing = v.tank;
    return s;
}

float StairwellEngine::erTimeScaleFor (Mode m, float size01) noexcept
{
    const auto v = modeVoicing (m);
    return v.erTimeLow + (v.erTimeHigh - v.erTimeLow) * std::clamp (size01, 0.0f, 1.0f);
}

float StairwellEngine::panAfterWidth (float pan, float width01) noexcept
{
    // The engine's ER: equal-power gains, angle (pan + 1) pi / 4.
    const float angle = (std::clamp (pan, -1.0f, 1.0f) + 1.0f) * 0.25f * 3.14159265f;
    const float l = std::cos (angle), r = std::sin (angle);
    // Its Width step: M = (L + R) / 2, S = (L - R) / 2 x width.
    const float m = 0.5f * (l + r), sd = 0.5f * (l - r) * std::clamp (width01, 0.0f, 1.5f);
    const float l2 = m + sd, r2 = m - sd;
    if (l2 <= 0.0f) return 1.0f;    // out of phase on the left: hard right
    if (r2 <= 0.0f) return -1.0f;
    return std::atan2 (r2, l2) / (0.25f * 3.14159265f) - 1.0f;
}

float StairwellEngine::erLevelFor (const Parameters& p) noexcept
{
    return std::cos (std::clamp (p.erLate01, 0.0f, 1.0f) * kHalfPi);
}

StairwellEngine::Reflections StairwellEngine::earlyReflectionsFor (const Parameters& p) noexcept
{
    const auto taps = fxme::EarlyReflections::getTaps (modeVoicing (p.mode).erPattern);
    const float scale = std::clamp (erTimeScaleFor (p.mode, p.size01), 0.25f, fxme::EarlyReflections::maxTimeScale);
    Reflections r {};
    for (size_t i = 0; i < r.size(); ++i)
        r[i] = { taps[i].ms * scale, taps[i].gain, taps[i].pan, taps[i].lowpassHz };
    return r;
}

double StairwellEngine::tailSeconds (const Parameters& p) noexcept
{
    return (double) p.predelaySeconds + 0.1 + (double) p.decay * std::max (1.0, (double) p.bass)
           * (modeVoicing (p.mode).shimmer ? 1.5 : 1.0);
}

void StairwellEngine::prepare (double sampleRate)
{
    fs = sampleRate > 0.0 ? sampleRate : 48000.0;
    tank.prepare (fs);
    er.prepare (fs);
    for (auto& line : predelay)
        line.prepare (fs, maxPredelaySeconds + 0.01f);
    inputShifter1.prepare (fs, 1, 40.0f);
    inputShifter2.prepare (fs, 1, 40.0f);

    const float coef = 1.0f - (float) std::exp (-1.0 / (kSmoothingSeconds * fs));
    for (auto* s : { &predelaySamples, &mix, &outGain, &erGain, &lateGain, &width, &pitchedMix })
        s->coef = coef;
    modeFadeStep = (float) (1.0 / (kModeFadeSeconds * fs));

    applyMode (params.mode);
    setParameters (params);
    predelaySamples.snap (predelaySamples.target);
    mix.snap (mix.target);
    pitchedMix.snap (pitchedMix.target);
    outGain.snap (outGain.target);
    erGain.snap (erGain.target);
    lateGain.snap (lateGain.target);
    width.snap (width.target);
    reset();
}

void StairwellEngine::reset()
{
    tank.reset();
    er.reset();
    for (auto& line : predelay) line.reset();
    inputShifter1.reset();
    inputShifter2.reset();
    for (auto& f : lowCut)  f.reset();
    for (auto& f : highCut) f.reset();
    modeFade = 1.0f;
    switchingMode = false;
}

void StairwellEngine::applyMode (Mode m) noexcept
{
    activeMode = m;
    activeFloor.store ((int) m, std::memory_order_relaxed);
    const auto v = modeVoicing (m);
    er.setPattern (v.erPattern);
    erIntoTank = v.erIntoTank;
}

void StairwellEngine::setParameters (const Parameters& p) noexcept
{
    params = p;
    if (p.mode != activeMode)
        switchingMode = true;   // process() fades out, then applies it

    // The tank keeps the active mode's voicing until the switch.
    auto forTank = p;
    forTank.mode = activeMode;
    tank.setSettings (tankSettingsFor (forTank));

    er.setTimeScale (erTimeScaleFor (activeMode, p.size01));

    predelaySamples.target = (float) (std::clamp (p.predelaySeconds, 0.0f, maxPredelaySeconds) * fs);
    mix.target = std::clamp (p.mix01, 0.0f, 1.0f);

    // Pitched input: Heaven only; the two voices' intervals, weighted by
    // their amounts (voice 1 alone when both are at 0).
    const bool heavenNow = modeVoicing (activeMode).shimmer;
    pitchedMix.target = heavenNow ? std::clamp (p.pitchedInput01, 0.0f, 1.0f) : 0.0f;
    inputShifter1.setPitchSemitones (std::clamp (p.shimmerSemitones, -24.0f, 24.0f));
    inputShifter2.setPitchSemitones (std::clamp (p.shimmerSemitones2, -24.0f, 24.0f));
    const float a1 = std::clamp (p.shimmer01, 0.0f, 1.0f), a2 = std::clamp (p.shimmer2_01, 0.0f, 1.0f);
    inputWeight1 = a1 + a2 > 0.0f ? a1 / (a1 + a2) : 1.0f;
    inputWeight2 = a1 + a2 > 0.0f ? a2 / (a1 + a2) : 0.0f;
    outGain.target = dbToGain (p.outDb);
    const float b = std::clamp (p.erLate01, 0.0f, 1.0f);
    erGain.target = erLevelFor (p);
    lateGain.target = std::sin (b * kHalfPi);
    width.target = std::clamp (p.width01, 0.0f, 1.5f);

    lowCutOn = p.lowCutHz > lowCutOffHz;
    highCutOn = p.highCutHz < highCutOffHz;
    for (auto& f : lowCut)  f.setup (fxme::TptSvf::gFor (p.lowCutHz, fs), kButterworthK);
    for (auto& f : highCut) f.setup (fxme::TptSvf::gFor (p.highCutHz, fs), kButterworthK);
}

void StairwellEngine::process (float* left, float* right, int numSamples) noexcept
{
    float energy = 0.0f, inputPeak = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        // Mode change: fade the wet out, switch, fade back in.
        if (switchingMode)
        {
            modeFade -= modeFadeStep;
            if (modeFade <= 0.0f)
            {
                modeFade = 0.0f;
                tank.reset();
                er.reset();
                applyMode (params.mode);
                setParameters (params);
                switchingMode = false;
            }
        }
        else if (modeFade < 1.0f)
        {
            modeFade = std::min (1.0f, modeFade + modeFadeStep);
        }

        const float dryL = fxme::sanitiseSample (left[i], fxme::kMainPathLimit);
        const float dryR = fxme::sanitiseSample (right[i], fxme::kMainPathLimit);
        inputPeak = std::max (inputPeak, std::max (std::abs (dryL), std::abs (dryR)));

        // Pre-delay (glides when changed).
        const float d = predelaySamples.next();
        predelay[0].write (dryL);
        predelay[1].write (dryR);
        float pl = d < 2.0f ? dryL : predelay[0].read (d);
        float pr = d < 2.0f ? dryR : predelay[1].read (d);

        // Pitched input: the shifters run while it is up (or fading), and
        // start clean when it comes back.
        const float pm = pitchedMix.next();
        if (pm > 1.0e-4f)
        {
            if (! pitchedInputActive)
            {
                inputShifter1.reset();
                inputShifter2.reset();
                pitchedInputActive = true;
            }
            const float mono = 0.5f * (pl + pr);
            // Both run, so a voice brought in later starts with live history.
            const float pitched = inputWeight1 * inputShifter1.processSample (mono)
                                + inputWeight2 * inputShifter2.processSample (mono);
            pl += pm * (pitched - pl);
            pr += pm * (pitched - pr);
        }
        else
        {
            pitchedInputActive = false;
        }

        float el, eR, tl, tr;
        er.process (pl, pr, el, eR);
        const float erMono = 0.5f * (el + eR);
        tank.process (pl + erIntoTank * erMono, pr + erIntoTank * erMono, tl, tr);
        energy = std::max (energy, tank.getEnergy());

        const float eg = erGain.next(), lg = lateGain.next();
        float wl = eg * el + lg * tl;
        float wr = eg * eR + lg * tr;

        const float w = width.next();
        const float m = 0.5f * (wl + wr), s = 0.5f * (wl - wr) * w;
        wl = m + s;
        wr = m - s;

        if (lowCutOn)  { wl = lowCut[0].highPass (wl);  wr = lowCut[1].highPass (wr); }
        if (highCutOn) { wl = highCut[0].lowPass (wl);  wr = highCut[1].lowPass (wr); }

        wl *= modeFade;
        wr *= modeFade;

        const float mx = mix.next(), g = outGain.next();
        left[i]  = (dryL + mx * (wl - dryL)) * g;
        right[i] = (dryR + mx * (wr - dryR)) * g;
    }

    tailEnergyDb.store (energy > 1.0e-10f ? 10.0f * std::log10 (energy) : -100.0f, std::memory_order_relaxed);
    for (float held = inputPeakHold.load (std::memory_order_relaxed);
         inputPeak > held && ! inputPeakHold.compare_exchange_weak (held, inputPeak, std::memory_order_relaxed);) {}
    activeFloor.store ((int) activeMode, std::memory_order_relaxed);
    modeFadeLevel.store (switchingMode ? modeFade : std::min (1.0f, modeFade), std::memory_order_relaxed);
}

} // namespace sth
