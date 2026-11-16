/*
  ------------------------------------------------------------------------------
    StairwellEngine.h

    StairwellToHeaven's signal path, JUCE-free (FxmeCore only), so it is
    tested headless (Tests/EngineTests.cpp) and the processor is thin glue.

      input -> pre-delay -> early reflections (fxme::EarlyReflections)
                         -> late tank (fxme::TankReverb), fed by the
                            pre-delayed input plus part of the ER (per mode)
      wet = ER and tank balanced (ER/Late, equal power) -> Width (M/S)
            -> Low cut, High cut (12 dB/oct, on the wet only)
      out = dry and wet mixed (Mix), then Output.

    A mode is a voicing, not another engine (see modeVoicing()): the tank's
    diffusion coefficients and input band-limit, the range Size maps onto,
    the ER pattern and its time range, how much ER feeds the tank, the
    modulation depth's scale, and the shimmer (Heaven only).

      Room       a medium room: short tank, dense ER, a little ER into the
                 tank.
      Stairwell  hard parallel walls: the stairwell flutter ER, less decay
                 diffusion (a ringing, metallic onset), brighter.
      Plate      high diffusion, fast build-up, bright, no ER to speak of.
      Hall       lower input diffusion, long delays, darker, sparse late ER.
      Heaven     the Hall, larger, with the shimmer: an octave (or any
                 interval from -24 to +24 semitones) climbing in the tail,
                 and an optional second voice at its own interval, so the
                 tail climbs in a chord. Pitched input shifts what enters
                 the reverb too (see below), up to removing the original
                 pitch from the wet altogether.

    Pitched input (Heaven). The shimmer lives in the tank's loop, so the
    tank's output always holds the reverb at the original pitch, the climbing
    copies on top. Pitched input crossfades what enters the early
    reflections and the tank between the pre-delayed signal and a
    pitch-shifted copy of it: the two voices' intervals, each through its own
    shifter, weighted by the voices' amounts (voice 1 alone when both are at
    0). At 100 % no original pitch reaches the wet: the tail starts at the
    interval and climbs from there. The shifters delay that path by about
    24 ms (half their 40 ms window), which only adds to the pre-delay.

    Changing the mode crossfades two spaces (a tank and its early
    reflections each). The new mode starts in the idle space, cleared, and
    takes the input from then on; the old space stops taking input and its
    tail rings on, fading out over a second (sooner once it has died away).
    No click, and the tail is kept: it costs twice the tank's CPU during the
    fade only. A further change while a tail is still fading speeds that
    fade up (30 ms) and waits for it, so no tail is ever cut. Size within a
    mode glides instead.

    Safety: the input is sanitised; the tank clears itself if its loop goes
    bad; and a wet that goes bad anyway (NaN, or past +60 dBFS) is muted and
    the whole reverb cleared at the end of the block (reset()), so no bad
    state can stay in a filter or a line.

    Threading: prepare() allocates; setParameters() and process() are
    realtime safe (setParameters() once per block).

    Author: Olivier Doaré, github.com/odoare
    (c) 2026 Olivier Doaré
    SPDX-License-Identifier: AGPL-3.0-or-later OR LicenseRef-FXME-Commercial
  ------------------------------------------------------------------------------
*/

#pragma once

#include <FxmeTools/dsp/EarlyReflections.h>
#include <FxmeTools/dsp/ModDelayLine.h>
#include <FxmeTools/dsp/TankReverb.h>
#include <FxmeTools/dsp/SplicePitchShifter.h>
#include <FxmeTools/dsp/TptSvf.h>
#include <FxmeTools/dsp/VuMeter.h>
#include <array>
#include <atomic>

namespace sth
{

enum class Mode { room = 0, stairwell, plate, hall, heaven };
inline constexpr int numModes = 5;

struct Parameters
{
    Mode  mode = Mode::hall;
    float decay = 2.5f;             // RT60, seconds
    float size01 = 0.5f;
    float predelaySeconds = 0.02f;
    float diffusion01 = 0.8f;
    float damping01 = 0.4f;         // 0: highs last as long as the mids, 1: a tenth
    float dampHz = 5000.0f;
    float bass = 1.2f;              // low RT60 / mid RT60
    float crossoverHz = 300.0f;
    float modRateHz = 0.7f;
    float modDepth01 = 0.3f;
    float erLate01 = 0.6f;          // 0: early reflections only, 1: tail only
    float width01 = 1.0f;           // 0 .. 1.5
    float lowCutHz = 20.0f;
    float highCutHz = 20000.0f;
    float shimmer01 = 0.5f;         // Heaven only
    float shimmerSemitones = 12.0f;
    float shimmer2_01 = 0.0f;       // the second voice, Heaven only (0: off)
    float shimmerSemitones2 = 7.0f;
    float pitchedInput01 = 0.0f;    // Heaven only: how much of the reverb's input is pitched
    bool  freeze = false;
    float mix01 = 0.3f;
    float outDb = 0.0f;
};

struct ModeVoicing
{
    fxme::TankReverb::Voicing tank;
    float sizeLow, sizeHigh;
    fxme::EarlyReflections::Pattern erPattern;
    float erTimeLow, erTimeHigh;
    float erIntoTank;
    float modScale;
    bool  shimmer;
};

class StairwellEngine
{
public:
    static constexpr float maxPredelaySeconds = 2.0f;
    static constexpr float lowCutOffHz = 20.5f, highCutOffHz = 19999.0f;

    static ModeVoicing modeVoicing (Mode m) noexcept;

    /** The tank settings these parameters give (for the engine, and for a
        GUI drawing the decay against frequency with TankReverb::rt60At). */
    static fxme::TankReverb::Settings tankSettingsFor (const Parameters& p) noexcept;

    /** One early reflection as it sounds with these parameters: its time
        (ms after the pre-delay, the mode's pattern scaled by Size), its
        level, pan (-1 .. 1) and low-pass. For a GUI drawing them. */
    struct Reflection { float ms, gain, pan, lowpassHz; };
    using Reflections = std::array<Reflection, fxme::EarlyReflections::numTaps>;
    static Reflections earlyReflectionsFor (const Parameters& p) noexcept;

    /** Where a reflection panned `pan` (-1 .. 1, equal power) is heard once
        Width (0 .. 1.5) has scaled the wet's side signal: the pan whose
        equal-power gains match the left and right levels after the M/S
        step. Width 1 leaves it, 0 centres it; past 1 it spreads, up to the
        edge (where one side would go out of phase, it stays at the edge). */
    static float panAfterWidth (float pan, float width01) noexcept;

    /** The time scale Size gives the mode's reflection pattern. */
    static float erTimeScaleFor (Mode m, float size01) noexcept;

    /** The early reflections' share of the wet (Early/Late, equal power). */
    static float erLevelFor (const Parameters& p) noexcept;

    /** How long the tail can last, for the host. */
    static double tailSeconds (const Parameters& p) noexcept;

    void prepare (double sampleRate);
    void reset();
    void setParameters (const Parameters& p) noexcept;
    void process (float* left, float* right, int numSamples) noexcept;

    /** The tank's energy in dB (about -100 .. 0), as of the last block: for
        the GUI's tail glow and FxmeNet. */
    std::atomic<float>& getTailEnergyDb() noexcept { return tailEnergyDb; }

    /** The input and output meters (left, right), for the GUI and FxmeNet. */
    fxme::VuMeter& getInputMeter (int ch) noexcept   { return inMeters[(size_t) (ch & 1)]; }
    fxme::VuMeter& getOutputMeter (int ch) noexcept  { return outMeters[(size_t) (ch & 1)]; }

    /** The input's peak (linear) since the last call, which resets it: the
        GUI polls it on a timer slower than the blocks, so the peak is held
        until taken and no hit is missed. The reflections view sweeps on its onsets. */
    float takeInputPeak() noexcept { return inputPeakHold.exchange (0.0f, std::memory_order_relaxed); }

    /** The floor actually sounding (it lags the parameter while a change
        waits for the previous tail to fade), and how far the crossfade is
        (0 at the switch, 1 once the old tail is gone): for the GUI's floor
        indicator. */
    std::atomic<int>& getActiveFloor() noexcept    { return activeFloor; }
    std::atomic<float>& getModeFade() noexcept     { return modeFadeLevel; }

private:
    /** A tank and its early reflections, voiced for one mode. Two of them:
        the one sounding, and the one whose tail fades after a mode change. */
    struct Space
    {
        fxme::TankReverb tank;
        fxme::EarlyReflections er;
        Mode mode = Mode::hall;
        float erIntoTank = 0.0f;
    };

    void voice (Space& s, Mode m) noexcept;
    void updateSpace (Space& s) noexcept;
    void startSwitch() noexcept;

    struct Smoothed
    {
        float current = 0.0f, target = 0.0f, coef = 0.001f;
        void snap (float v) noexcept { current = target = v; }
        float next() noexcept { current += coef * (target - current); return current; }
    };

    double fs = 48000.0;
    Parameters params;

    std::array<Space, 2> spaces;
    int live = 0;                       // spaces[live] sounds and takes the input
    Space& liveSpace() noexcept         { return spaces[(size_t) live]; }
    Space& fadingSpace() noexcept       { return spaces[(size_t) (1 - live)]; }

    std::array<fxme::ModDelayLine, 2> predelay;
    fxme::SplicePitchShifter inputShifter1, inputShifter2;   // Pitched input, mono
    float inputWeight1 = 1.0f, inputWeight2 = 0.0f;
    bool  pitchedInputActive = false;
    std::array<fxme::TptSvf, 2> lowCut, highCut;
    bool lowCutOn = false, highCutOn = false;
    std::array<fxme::VuMeter, 2> inMeters, outMeters;

    Smoothed predelaySamples, mix, outGain, erGain, lateGain, width, pitchedMix;

    // Mode change: the old space's tail fades (1 .. 0) while the new one plays.
    bool  tailFading = false;
    float tailLevel = 0.0f, tailStep = 0.001f;

    std::atomic<float> tailEnergyDb { -100.0f };
    std::atomic<float> inputPeakHold { 0.0f };
    std::atomic<int>   activeFloor { (int) Mode::hall };
    std::atomic<float> modeFadeLevel { 1.0f };
};

} // namespace sth
