/*
  ------------------------------------------------------------------------------
    EngineTests.cpp

    sth::StairwellEngine, headless (FxmeCore only):

      1. Mix 0: the output is the input, sample for sample.
      2. Silence in, silence out.
      3. Every mode: an impulse gives a finite tail that decays.
      4. Pre-delay: nothing comes out before it.
      5. Size maps onto each mode's range; Heaven alone has the shimmer.
      6. A mode change mid-tail: finite, the wet fades through (near) zero,
         and no jump bigger than the tail itself had.
      7. Freeze holds the tail.

    Exit code 0 when everything passes.

    Author: Olivier Doaré, github.com/odoare
    (c) 2026 Olivier Doaré
    SPDX-License-Identifier: AGPL-3.0-or-later OR LicenseRef-FXME-Commercial
  ------------------------------------------------------------------------------
*/

#include "StairwellEngine.h"

#include <cmath>
#include <cstdio>
#include <vector>

static int failures = 0;
static void check (bool ok, const char* what)
{
    std::printf ("  [%s] %s\n", ok ? "PASS" : "FAIL", what);
    if (! ok)
        ++failures;
}

namespace
{
    constexpr double fs = 48000.0;

    struct Run
    {
        std::vector<float> l, r;
    };

    Run render (sth::StairwellEngine& e, int n, bool impulse, int block = 256)
    {
        Run out { std::vector<float> ((size_t) n, 0.0f), std::vector<float> ((size_t) n, 0.0f) };
        if (impulse)
            out.l[0] = out.r[0] = 1.0f;
        for (int start = 0; start < n; start += block)
            e.process (out.l.data() + start, out.r.data() + start, std::min (block, n - start));
        return out;
    }

    double energy (const std::vector<float>& x, size_t from, size_t to)
    {
        double s = 0.0;
        for (size_t i = from; i < to && i < x.size(); ++i)
            s += (double) x[i] * x[i];
        return s;
    }
}

int main()
{
    using sth::Mode;

    std::printf ("1. Mix 0\n");
    {
        sth::Parameters p;
        p.mix01 = 0.0f;
        sth::StairwellEngine e;
        e.setParameters (p);
        e.prepare (fs);
        std::vector<float> l (4800), r (4800);
        for (size_t i = 0; i < l.size(); ++i)
        {
            l[i] = std::sin (0.01f * (float) i);
            r[i] = std::cos (0.013f * (float) i);
        }
        auto l0 = l, r0 = r;
        e.process (l.data(), r.data(), (int) l.size());
        check (l == l0 && r == r0, "dry passes untouched");
    }

    std::printf ("2. Silence\n");
    {
        sth::StairwellEngine e;
        e.prepare (fs);
        const auto out = render (e, 48000, false);
        check (energy (out.l, 0, out.l.size()) == 0.0 && energy (out.r, 0, out.r.size()) == 0.0, "silence out");
    }

    std::printf ("3. Modes\n");
    {
        const char* names[] { "room", "stairwell", "plate", "hall", "heaven" };
        for (int m = 0; m < sth::numModes; ++m)
        {
            sth::Parameters p;
            p.mode = (Mode) m;
            p.mix01 = 1.0f;
            p.decay = 1.5f;
            sth::StairwellEngine e;
            e.setParameters (p);
            e.prepare (fs);
            const auto out = render (e, 4 * 48000, true);
            bool finite = true;
            for (float v : out.l) finite = finite && std::isfinite (v);
            const double early = energy (out.l, 0, 48000), late = energy (out.l, 3 * 48000, 4 * 48000);
            std::printf ("     %-9s first second %.3g, fourth %.3g\n", names[m], early, late);
            check (finite && early > 1.0e-3 && late < early * 1.0e-3, "finite, audible, decays over 30 dB");
        }
    }

    std::printf ("4. Pre-delay\n");
    {
        sth::Parameters p;
        p.mix01 = 1.0f;
        p.predelaySeconds = 0.1f;
        sth::StairwellEngine e;
        e.setParameters (p);
        e.prepare (fs);
        const auto out = render (e, 24000, true);
        size_t first = 0;
        while (first < out.l.size() && std::abs (out.l[first]) + std::abs (out.r[first]) < 1.0e-6f)
            ++first;
        std::printf ("     first output at %.1f ms\n", 1000.0 * (double) first / fs);
        check ((double) first / fs >= 0.1, "nothing before the pre-delay");
    }

    std::printf ("5. Voicings\n");
    {
        bool inRange = true, shimmerOnlyInHeaven = true;
        for (int m = 0; m < sth::numModes; ++m)
        {
            const auto v = sth::StairwellEngine::modeVoicing ((Mode) m);
            for (float s : { 0.0f, 0.5f, 1.0f })
            {
                sth::Parameters p;
                p.mode = (Mode) m;
                p.size01 = s;
                const auto t = sth::StairwellEngine::tankSettingsFor (p);
                inRange = inRange && t.size >= v.sizeLow - 1.0e-5f && t.size <= v.sizeHigh + 1.0e-5f;
                shimmerOnlyInHeaven = shimmerOnlyInHeaven && ((t.shimmer > 0.0f) == ((Mode) m == Mode::heaven));
            }
        }
        check (inRange, "size within each mode's range");
        check (shimmerOnlyInHeaven, "shimmer in Heaven only");
    }

    std::printf ("6. Mode change\n");
    {
        sth::Parameters p;
        p.mode = Mode::hall;
        p.mix01 = 1.0f;
        p.decay = 4.0f;
        sth::StairwellEngine e;
        e.setParameters (p);
        e.prepare (fs);
        render (e, 24000, true);   // a tail going

        // Steady input during the switch, to see the wet fade through zero.
        std::vector<float> l (9600), r (9600);
        float maxStepBefore = 0.0f, maxStepDuring = 0.0f, minLevel = 1.0e9f;
        auto feed = [&] (int count, float& maxStep, bool trackMin)
        {
            for (int start = 0; start < count; start += 256)
            {
                const int n = std::min (256, count - start);
                for (int i = 0; i < n; ++i) { l[(size_t) i] = 0.0f; r[(size_t) i] = 0.0f; }
                static float last = 0.0f;
                e.process (l.data(), r.data(), n);
                for (int i = 0; i < n; ++i)
                {
                    maxStep = std::max (maxStep, std::abs (l[(size_t) i] - last));
                    last = l[(size_t) i];
                    if (trackMin) minLevel = std::min (minLevel, std::abs (l[(size_t) i]) + std::abs (r[(size_t) i]));
                }
            }
        };
        feed (4800, maxStepBefore, false);
        p.mode = Mode::room;
        e.setParameters (p);
        feed (2880, maxStepDuring, true);   // 60 ms: through the fade
        std::printf ("     largest step before %.4f, during %.4f, lowest level %.2g\n", maxStepBefore, maxStepDuring, minLevel);
        check (std::isfinite (maxStepDuring) && maxStepDuring <= maxStepBefore * 1.5f + 1.0e-6f, "no click");
        check (minLevel < 1.0e-3f, "the wet goes through silence");
    }

    std::printf ("7. Freeze\n");
    {
        sth::Parameters p;
        p.mix01 = 1.0f;
        p.decay = 1.0f;
        sth::StairwellEngine e;
        e.setParameters (p);
        e.prepare (fs);
        render (e, 4800, true);
        p.freeze = true;
        e.setParameters (p);
        const auto out = render (e, 5 * 48000, false);
        const double first = energy (out.l, 24000, 48000), last = energy (out.l, 4 * 48000 + 24000, 5 * 48000);
        std::printf ("     energy change over 4 s frozen: %+.2f dB\n", 10.0 * std::log10 (last / first));
        check (std::abs (10.0 * std::log10 (last / first)) < 3.0, "holds within 3 dB (a 1 s decay unfrozen would lose 240 dB)");
    }

    std::printf (failures == 0 ? "All engine tests passed\n" : "%d engine test(s) FAILED\n", failures);
    return failures == 0 ? 0 : 1;
}
