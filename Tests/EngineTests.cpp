/*
  ------------------------------------------------------------------------------
    EngineTests.cpp

    sth::StairwellEngine, headless (FxmeCore only):

      1. Mix 0: the output is the input, sample for sample.
      2. Silence in, silence out.
      3. Every mode: an impulse gives a finite tail that decays.
      4. Pre-delay: nothing comes out before it.
      5. Size maps onto each mode's range; Heaven alone has the shimmer.
      6. A mode change mid-tail: no jump bigger than the tail itself had,
         the tail kept (crossfaded, not cleared) and gone after the fade;
         a second change straight after waits for it, also without a click.
      7. Freeze holds the tail.
      8. The reflections a GUI draws (earlyReflectionsFor) are the ones that
         sound: with the tail off, the first one arrives at the pre-delay
         plus its time, in every mode with reflections.
      9. panAfterWidth: Width 1 keeps the pan, 0 centres it, 1.5 widens it,
         never past the edge; and it matches the levels the engine outputs.
     10. Pitched input (Heaven, +12, the shimmer loop off): at 100 % the wet
         holds no original pitch (a 440 Hz sine comes out at 880 Hz, the
         440 Hz at least 30 dB under); at 0 % the original pitch dominates.

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

        // Silence in from here: what comes out is the tail.
        float last = 0.0f;
        auto feed = [&] (int count, float& maxStep, double& sumSquares)
        {
            std::vector<float> l (256), r (256);
            for (int start = 0; start < count; start += 256)
            {
                const int n = std::min (256, count - start);
                std::fill (l.begin(), l.end(), 0.0f);
                std::fill (r.begin(), r.end(), 0.0f);
                e.process (l.data(), r.data(), n);
                for (int i = 0; i < n; ++i)
                {
                    maxStep = std::max (maxStep, std::abs (l[(size_t) i] - last));
                    last = l[(size_t) i];
                    sumSquares += (double) l[(size_t) i] * l[(size_t) i];
                }
            }
        };
        float stepBefore = 0.0f, stepDuring = 0.0f, stepAfter = 0.0f;
        double before = 0.0, during = 0.0, after = 0.0;
        feed (2400, stepBefore, before);       // 50 ms before the change
        p.mode = Mode::room;
        e.setParameters (p);
        feed (2400, stepDuring, during);       // 50 ms after it
        feed (72000, stepAfter, after);        // the next 1.5 s
        const double keptDb = 10.0 * std::log10 (during / before);
        std::printf ("     largest step before %.4f, during %.4f; tail 50 ms after vs before %+.2f dB\n",
                     stepBefore, stepDuring, keptDb);
        check (std::isfinite (stepDuring) && stepDuring <= stepBefore * 1.5f + 1.0e-6f, "no click");
        check (keptDb > -6.0, "the tail is kept through the change");
        check (e.getActiveFloor().load() == (int) Mode::room && e.getModeFade().load() > 0.999f,
               "the new floor sounds, the crossfade over");
        float tailPeak = 0.0f;
        {
            std::vector<float> l (4800, 0.0f), r (4800, 0.0f);
            e.process (l.data(), r.data(), 4800);
            for (float v : l) tailPeak = std::max (tailPeak, std::abs (v));
        }
        std::printf ("     1.6 s after the change: peak %.2g\n", tailPeak);
        check (tailPeak < 1.0e-3f, "the old tail has faded out");

        // Two changes in a row: the second waits for the first's tail (sped
        // up), without a click, and ends on the last floor.
        render (e, 24000, true);
        last = 0.0f;
        float stepQuick = 0.0f;
        double ignored = 0.0;
        float stepRef = 0.0f;
        feed (2400, stepRef, ignored);
        p.mode = Mode::plate;
        e.setParameters (p);
        feed (512, stepQuick, ignored);
        p.mode = Mode::heaven;
        e.setParameters (p);
        feed (9600, stepQuick, ignored);
        std::printf ("     two changes: largest step %.4f (before %.4f), floor %d\n",
                     stepQuick, stepRef, e.getActiveFloor().load());
        check (std::isfinite (stepQuick) && stepQuick <= stepRef * 1.5f + 1.0e-6f, "no click on a quick second change");
        check (e.getActiveFloor().load() == (int) Mode::heaven, "the last change wins");
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

    std::printf ("8. Reflections drawn = reflections heard\n");
    {
        bool ok = true;
        for (Mode m : { Mode::room, Mode::stairwell, Mode::hall, Mode::heaven })
        {
            sth::Parameters p;
            p.mode = m;
            p.size01 = 0.7f;
            p.predelaySeconds = 0.015f;
            p.erLate01 = 0.0f;   // reflections only
            p.mix01 = 1.0f;
            sth::StairwellEngine e;
            e.setParameters (p);
            e.prepare (fs);
            const auto out = render (e, 24000, true);
            size_t first = 0;
            while (first < out.l.size() && std::abs (out.l[first]) + std::abs (out.r[first]) < 1.0e-4f)
                ++first;
            const double heard = 1000.0 * (double) first / fs;
            const double drawn = 15.0 + sth::StairwellEngine::earlyReflectionsFor (p)[0].ms;
            std::printf ("     mode %d: drawn %.2f ms, heard %.2f ms\n", (int) m, drawn, heard);
            ok = ok && std::abs (heard - drawn) < 0.3;
        }
        check (ok, "within 0.3 ms");
    }

    std::printf ("9. Pan after Width\n");
    {
        using E = sth::StairwellEngine;
        bool ok = std::abs (E::panAfterWidth (0.6f, 1.0f) - 0.6f) < 1.0e-4f
               && std::abs (E::panAfterWidth (-0.7f, 0.0f)) < 1.0e-4f
               && E::panAfterWidth (0.5f, 1.5f) > 0.5f
               && E::panAfterWidth (0.5f, 1.5f) <= 1.0f
               && std::abs (E::panAfterWidth (-0.3f, 0.5f) + E::panAfterWidth (0.3f, 0.5f)) < 1.0e-4f;
        check (ok, "identity at 1, centre at 0, wider at 1.5, symmetric");

        // Against the engine: Room's first reflection alone (Width 0.5),
        // its left and right levels give back the predicted pan.
        sth::Parameters p;
        p.mode = Mode::room;
        p.erLate01 = 0.0f;
        p.mix01 = 1.0f;
        p.width01 = 0.5f;
        p.predelaySeconds = 0.0f;
        sth::StairwellEngine e;
        e.setParameters (p);
        e.prepare (fs);
        const auto out = render (e, 2400, true);
        const auto first = E::earlyReflectionsFor (p)[0];
        const size_t at = (size_t) std::lround (first.ms * 0.001 * fs);
        float l = 0.0f, r = 0.0f;
        for (size_t i = at - 3; i <= at + 3; ++i) { l = std::max (l, std::abs (out.l[i])); r = std::max (r, std::abs (out.r[i])); }
        const float heard = std::atan2 (r, l) / (0.25f * 3.14159265f) - 1.0f;
        const float predicted = E::panAfterWidth (first.pan, 0.5f);
        std::printf ("     first reflection: pan %.2f, predicted at width 0.5 %.3f, heard %.3f\n", first.pan, predicted, heard);
        check (std::abs (heard - predicted) < 0.05f, "the engine agrees");
    }

    std::printf ("10. Pitched input\n");
    {
        auto partials = [] (float pitched, double& at440, double& at880)
        {
            sth::Parameters p;
            p.mode = Mode::heaven;
            p.shimmer01 = 0.0f;   // no climbing: only the input shift
            p.shimmer2_01 = 0.0f;
            p.shimmerSemitones = 12.0f;
            p.pitchedInput01 = pitched;
            p.mix01 = 1.0f;
            p.decay = 1.5f;
            sth::StairwellEngine e;
            e.setParameters (p);
            e.prepare (fs);
            const int n = 96000;
            std::vector<float> l ((size_t) n), r ((size_t) n);
            for (int i = 0; i < n; ++i)
                l[(size_t) i] = r[(size_t) i] = 0.3f * (float) std::sin (2.0 * 3.141592653589793 * 440.0 * i / fs);
            for (int start = 0; start < n; start += 256)
                e.process (l.data() + start, r.data() + start, std::min (256, n - start));
            auto amp = [&l] (double hz)
            {
                double re = 0.0, im = 0.0;
                for (int i = 48000; i < 96000; ++i)
                {
                    re += l[(size_t) i] * std::cos (2.0 * 3.141592653589793 * hz * i / fs);
                    im -= l[(size_t) i] * std::sin (2.0 * 3.141592653589793 * hz * i / fs);
                }
                return 2.0 * std::sqrt (re * re + im * im) / 48000.0;
            };
            at440 = amp (440.0);
            at880 = amp (880.0);
        };
        double a440, a880, b440, b880;
        partials (1.0f, a440, a880);
        partials (0.0f, b440, b880);
        std::printf ("     100 %%: 440 Hz %.1f dB under 880 Hz; 0 %%: 440 Hz %.1f dB over 880 Hz\n",
                     20.0 * std::log10 (a880 / a440), 20.0 * std::log10 (b440 / b880));
        check (a440 < a880 * 0.0316, "100 %: the original pitch at least 30 dB under the interval");
        check (b440 > b880 * 10.0, "0 %: the original pitch dominates");
    }

    std::printf (failures == 0 ? "All engine tests passed\n" : "%d engine test(s) FAILED\n", failures);
    return failures == 0 ? 0 : 1;
}
