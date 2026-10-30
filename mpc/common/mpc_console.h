// 8cee Console 4K — shared pieces for the MPC OS (VST2, armhf/aarch64) builds.
// Plain C++11, no JUCE: MPC OS loads these through the engine interface of mpc-vst-plugins (wrapper/engine.h).
#pragma once
#include "EceeDSP.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>

namespace mpc4k
{
// --- parameter curves -------------------------------------------------------------------------------------------
// MPC maps a VST2 parameter linearly onto its min..max, so frequency/time/Q controls are a 0..100 "position" that
// the engine turns into real units on a log curve. tools/make_presets.py uses the same formulas.
inline float logPos (float pos, float lo, float hi) { return lo * std::pow (hi / lo, ecee::clamp01 (pos * 0.01f)); }

inline void fmtHz (char* buf, int len, float v)
{
    if (v >= 10000.0f)     snprintf (buf, len, "%.1f kHz", v / 1000.0f);
    else if (v >= 1000.0f) snprintf (buf, len, "%.2f kHz", v / 1000.0f);
    else                   snprintf (buf, len, "%d Hz", (int) std::lround (v));
}
inline void fmtSec (char* buf, int len, float s)
{
    if (s < 1.0f) snprintf (buf, len, "%d ms", (int) std::lround (s * 1000.0f));
    else          snprintf (buf, len, "%.2f s", s);
}

// --- 2x oversampler: 31-tap windowed-sinc half-band FIR, per channel --------------------------------------------
class Oversampler2x
{
public:
    static const int kTaps = 31, kHalf = 15;

    Oversampler2x()
    {
        for (int n = 0; n < kTaps; ++n)
        {
            const double m = n - kHalf;
            const double sinc = m == 0 ? 0.5 : std::sin (M_PI * m * 0.5) / (M_PI * m);
            const double w = 0.42 - 0.5 * std::cos (2.0 * M_PI * n / (kTaps - 1)) + 0.08 * std::cos (4.0 * M_PI * n / (kTaps - 1));
            h[n] = (float) (sinc * w);
        }
        double sum = 0;
        for (int n = 0; n < kTaps; ++n) sum += h[n];
        for (int n = 0; n < kTaps; ++n) h[n] = (float) (h[n] / sum);
        reset();
    }

    void reset() { std::memset (up, 0, sizeof up); std::memset (dn, 0, sizeof dn); upPos = dnPos = 0; }

    /** One input sample -> two samples at 2x. */
    inline void upsample (float x, float& y0, float& y1)
    {
        // zero-stuffed input: x, 0. Gain 2 to keep the level.
        push (up, upPos, 2.0f * x); y0 = fir (up, upPos);
        push (up, upPos, 0.0f);     y1 = fir (up, upPos);
    }

    /** Two samples at 2x -> one output sample. */
    inline float downsample (float a, float b)
    {
        push (dn, dnPos, a);
        push (dn, dnPos, b);
        return fir (dn, dnPos);
    }

private:
    static inline void push (float* buf, int& pos, float v) { pos = (pos + 1) & (kLen - 1); buf[pos] = v; buf[pos + kLen] = v; }
    inline float fir (const float* buf, int pos) const
    {
        // buf holds each sample twice (pos and pos + kLen), so a window ending at pos is contiguous
        const float* x = buf + pos + kLen - (kTaps - 1);
        float s = 0.0f;
        for (int n = 0; n < kTaps; ++n) s += h[n] * x[n];
        return s;
    }

    static const int kLen = 32;   // power of two >= kTaps
    float h[kTaps];
    float up[kLen * 2], dn[kLen * 2];
    int upPos, dnPos;
};

// --- int16 <-> float ---------------------------------------------------------------------------------------------
inline float fromI16 (int16_t v) { return (float) v * (1.0f / 32768.0f); }
inline int16_t toI16 (float v)
{
    if (! (v == v)) v = 0.0f;   // NaN guard: a crash or a NaN takes MPC down, never pass one on
    v = std::fmax (-1.0f, std::fmin (1.0f, v));
    return (int16_t) std::lrint (v * 32767.0f);
}

// --- a tiny parameter table: key -> float, with state save/restore ----------------------------------------------
struct ParamDef { const char* key; float def; };

template <int N>
struct ParamTable
{
    const ParamDef* defs;
    float v[N];
    bool dirty = true;

    explicit ParamTable (const ParamDef* d) : defs (d) { for (int i = 0; i < N; ++i) v[i] = d[i].def; }

    int index (const char* key) const
    {
        for (int i = 0; i < N; ++i) if (std::strcmp (defs[i].key, key) == 0) return i;
        return -1;
    }

    bool set (const char* key, const char* val)
    {
        if (std::strcmp (key, "state") == 0) { restore (val); return true; }
        const int i = index (key);
        if (i < 0) return false;
        v[i] = (float) std::atof (val);
        dirty = true;
        return true;
    }

    /** Value (as the wrapper expects: the number in parameter units) or the "state" chunk. > 0 when answered. */
    int get (const char* key, char* buf, int len) const
    {
        if (std::strcmp (key, "state") == 0)
        {
            std::string s;
            char item[64];
            for (int i = 0; i < N; ++i) { snprintf (item, sizeof item, "%s=%g;", defs[i].key, v[i]); s += item; }
            snprintf (buf, len, "%s", s.c_str());
            return (int) std::strlen (buf) + 1;
        }
        const int i = index (key);
        if (i < 0) return 0;
        snprintf (buf, len, "%g", v[i]);
        return (int) std::strlen (buf) + 1;
    }

    void restore (const char* s)
    {
        char key[48], val[32];
        while (s && *s)
        {
            const char* eq = std::strchr (s, '=');
            const char* end = std::strchr (s, ';');
            if (! eq || ! end || eq > end) break;
            const size_t kl = std::min ((size_t) (eq - s), sizeof key - 1), vl = std::min ((size_t) (end - eq - 1), sizeof val - 1);
            std::memcpy (key, s, kl); key[kl] = 0;
            std::memcpy (val, eq + 1, vl); val[vl] = 0;
            const int i = index (key);
            if (i >= 0) v[i] = (float) std::atof (val);
            s = end + 1;
        }
        dirty = true;
    }
};
} // namespace mpc4k
