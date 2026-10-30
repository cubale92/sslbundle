// 8cee Channel-4K for MPC OS — E/G-series console channel strip (independent port of the desktop DSP, without JUCE).
extern "C" {
#include "engine.h"
}
#include "mpc_console.h"

using namespace mpc4k;

namespace
{
enum P
{
    kDrive, kPhase, kHpf, kLpf, kEqType, kEqIn,
    kHfG, kHfF, kHfBell, kHmfG, kHmfF, kHmfQ, kLmfG, kLmfF, kLmfQ, kLfG, kLfF, kLfBell,
    kCRatio, kCThresh, kCRel, kCFast,
    kGRange, kGThresh, kGRel, kGMode, kGFast,
    kDynIn, kDynPre, kMix, kOutput, kNumParams
};

// Positions (0..100) are log curves: see the *Of() helpers. Defaults match the desktop plug-in.
#include "param_defs.h"

inline float hpfHz (float p)   { return logPos (p, 20.0f, 400.0f); }      // p < 1 = off
inline float lpfHz (float p)   { return logPos (p, 3000.0f, 22000.0f); }  // p > 99 = off
inline float hfHz (float p)    { return logPos (p, 1500.0f, 16000.0f); }
inline float hmfHz (float p)   { return logPos (p, 600.0f, 7000.0f); }
inline float lmfHz (float p)   { return logPos (p, 200.0f, 2500.0f); }
inline float lfHz (float p)    { return logPos (p, 30.0f, 450.0f); }
inline float qOf (float p)     { return logPos (p, 0.4f, 4.0f); }
inline float ratioOf (float p) { return logPos (p, 1.0f, 20.0f); }        // p < 0.5 = 1:1 (off), p > 99.5 = limit
inline float relOf (float p)   { return logPos (p, 0.05f, 4.0f); }        // seconds

const double osRate = 88200.0;

struct Smooth
{
    float v = 0, c = 0;
    void init (float value, float ms, double fs) { v = value; c = ecee::timeCoeff (ms, fs); }
    inline float next (float target) { v = target + c * (v - target); return v; }
};

struct Channel4K
{
    ParamTable<kNumParams> p { kDefs };

    Oversampler2x os[2];
    ecee::Biquad hpf2[2], lpf2[2], hf[2], hfDip[2], hmf[2], lmf[2], lf[2], lfDip[2];
    ecee::OnePole hpf1[2];
    ecee::DCBlocker dc[2];
    bool hpfOn = false, lpfOn = false, hfDipOn = false, lfDipOn = false;

    float compEnv = 0, compGr = 0, gateEnv = 0, gateGain = 1, gateHold = 0;
    bool gateOpen = false;
    Smooth drive, outGain, mix, makeup;

    Channel4K()
    {
        for (auto& d : dc) d.prepare (osRate);
        drive.init (1.0f, 30.0f, osRate);
        outGain.init (1.0f, 30.0f, osRate);
        mix.init (1.0f, 30.0f, osRate);
        makeup.init (1.0f, 80.0f, osRate);
    }

    float& v (int i) { return p.v[i]; }
    bool on (int i) { return p.v[i] > 0.5f; }

    void updateFilters()
    {
        if (! p.dirty) return;
        p.dirty = false;

        const bool black = on (kEqType);
        const float hfG = v (kHfG), hmfG = v (kHmfG), lmfG = v (kLmfG), lfG = v (kLfG);
        const bool hfBell = on (kHfBell), lfBell = on (kLfBell);
        auto qFor = [black] (float q, float g) { return black ? q * (0.55f + 1.25f * std::fabs (g) / 16.0f) : q; };

        hpfOn = v (kHpf) >= 1.0f;
        lpfOn = v (kLpf) <= 99.0f;
        hfDipOn = black && ! hfBell && std::fabs (hfG) > 0.05f;
        lfDipOn = black && ! lfBell && std::fabs (lfG) > 0.05f;

        const double hpfF = hpfHz (v (kHpf)), lpfF = lpfHz (v (kLpf));
        const double hfF = hfHz (v (kHfF)), hmfF = hmfHz (v (kHmfF)), lmfF = lmfHz (v (kLmfF)), lfF = lfHz (v (kLfF));

        for (int c = 0; c < 2; ++c)
        {
            if (hpfOn) { hpf2[c].highPass (hpfF, 0.9, osRate); hpf1[c].setCutoff (hpfF, osRate); }
            if (lpfOn) lpf2[c].lowPass (lpfF, 0.707, osRate);

            if (hfBell) hf[c].peak (hfF, qFor (0.9f, hfG), hfG, osRate);
            else        hf[c].highShelf (hfF, hfG, osRate, black ? 1.0 : 0.6);
            if (hfDipOn) hfDip[c].peak (hfF * 0.42, 1.1, -0.14 * hfG, osRate);

            hmf[c].peak (hmfF, qFor (qOf (v (kHmfQ)), hmfG), hmfG, osRate);
            lmf[c].peak (lmfF, qFor (qOf (v (kLmfQ)), lmfG), lmfG, osRate);

            if (lfBell) lf[c].peak (lfF, qFor (0.9f, lfG), lfG, osRate);
            else        lf[c].lowShelf (lfF, lfG, osRate, black ? 1.0 : 0.6);
            if (lfDipOn) lfDip[c].peak (std::min (lfF * 2.3, 1200.0), 1.1, -0.14 * lfG, osRate);
        }
    }

    inline float eq (int c, float x)
    {
        x = lf[c].process (x);
        if (lfDipOn) x = lfDip[c].process (x);
        x = lmf[c].process (x);
        x = hmf[c].process (x);
        x = hf[c].process (x);
        if (hfDipOn) x = hfDip[c].process (x);
        return x;
    }

    void process (const int16_t* in, int16_t* out, int frames)
    {
        updateFilters();

        const float driveT = ecee::dbToGain (v (kDrive)), outT = ecee::dbToGain (v (kOutput)), mixT = v (kMix) * 0.01f;
        const bool phase = on (kPhase), eqIn = on (kEqIn), dynIn = on (kDynIn), dynPre = on (kDynPre);

        const float ratioPos = v (kCRatio);
        const bool  limit    = ratioPos > 99.5f;
        const float ratio    = limit ? 100.0f : ratioOf (ratioPos);
        const float cThr     = v (kCThresh);
        const bool  compOn   = dynIn && ratioPos >= 0.5f;
        const bool  cFast    = on (kCFast);
        const float cAtk     = ecee::timeCoeff (cFast ? 0.3f : 5.0f, osRate);
        const float cRel     = ecee::timeCoeff (relOf (v (kCRel)) * 1000.0f * 0.4f, osRate);
        const float detC     = ecee::timeCoeff (cFast ? 0.05f : 4.0f, osRate);
        const float knee     = limit ? 1.0f : 5.0f;
        const float makeupT  = compOn ? ecee::dbToGain (0.45f * ecee::computeGR (-10.0f, cThr, ratio, knee)) : 1.0f;

        const float gRange   = v (kGRange);
        const bool  gateOn   = dynIn && gRange > 0.1f;
        const float gThr     = v (kGThresh);
        const bool  hardGate = on (kGMode);
        const float gAtk     = ecee::timeCoeff (on (kGFast) ? 0.1f : 1.5f, osRate);
        const float gRel     = ecee::timeCoeff (relOf (v (kGRel)) * 1000.0f * 0.4f, osRate);
        const float gDet     = ecee::timeCoeff (8.0f, osRate);
        const float holdLen  = (float) (0.02 * osRate);
        const float satA     = 0.24f;

        for (int i = 0; i < frames; ++i)
        {
            float up[2][2];
            for (int c = 0; c < 2; ++c)
                os[c].upsample (fromI16 (in[i * 2 + c]), up[c][0], up[c][1]);

            for (int s = 0; s < 2; ++s)
            {
                const float k = satA * drive.next (driveT);
                float vv[2];
                for (int c = 0; c < 2; ++c)
                {
                    float x = up[c][s];
                    if (phase) x = -x;
                    const float sat = std::tanh (k * x);
                    x = (sat + 0.045f * sat * sat) / k;
                    x = dc[c].process (x);
                    if (hpfOn) { x = hpf2[c].process (x); x = x - hpf1[c].process (x); }
                    if (lpfOn) x = lpf2[c].process (x);
                    if (eqIn && ! dynPre) x = eq (c, x);
                    vv[c] = x;
                }

                float dynGain = 1.0f;
                if (compOn || gateOn)
                {
                    const float key = std::max (std::fabs (vv[0]), std::fabs (vv[1]));
                    if (compOn)
                    {
                        compEnv = detC * compEnv + (1.0f - detC) * key * key;
                        const float lvl = ecee::gainToDb (std::sqrt (compEnv) * (cFast ? 1.0f : 1.4142f));
                        const float t = ecee::computeGR (lvl, cThr, ratio, knee);
                        compGr = t > compGr ? cAtk * compGr + (1.0f - cAtk) * t : cRel * compGr + (1.0f - cRel) * t;
                        dynGain *= ecee::dbToGain (-compGr);
                    }
                    if (gateOn)
                    {
                        gateEnv = std::max (key, gDet * gateEnv);
                        const float lvl = ecee::gainToDb (gateEnv);
                        float targetDb;
                        if (hardGate)
                        {
                            if (lvl > gThr)             { gateOpen = true; gateHold = holdLen; }
                            else if (lvl < gThr - 4.0f) { if (gateHold > 0.0f) gateHold -= 1.0f; else gateOpen = false; }
                            targetDb = gateOpen ? 0.0f : -gRange;
                        }
                        else
                        {
                            targetDb = -std::min (gRange, std::max (0.0f, gThr - lvl));
                        }
                        const float gDb = ecee::gainToDb (gateGain);
                        const float coeff = targetDb > gDb ? gAtk : gRel;
                        gateGain = ecee::dbToGain (coeff * gDb + (1.0f - coeff) * targetDb);
                        dynGain *= gateGain;
                    }
                }
                else
                {
                    compGr = 0.0f; gateGain = 1.0f;
                }

                const float m = mix.next (mixT), mk = makeup.next (makeupT), og = outGain.next (outT);
                for (int c = 0; c < 2; ++c)
                {
                    float x = vv[c];
                    x = x * dynGain * mk * m + x * (1.0f - m);
                    if (eqIn && dynPre) x = eq (c, x);
                    up[c][s] = x * og;
                }
            }

            for (int c = 0; c < 2; ++c)
                out[i * 2 + c] = toI16 (os[c].downsample (up[c][0], up[c][1]));
        }
    }

    int display (const char* key, char* buf, int len)
    {
        const size_t kl = std::strlen (key);
        if (kl < 9 || std::strcmp (key + kl - 8, "_display") != 0) return 0;
        char base[32];
        snprintf (base, sizeof base, "%.*s", (int) (kl - 8), key);
        const int i = p.index (base);
        if (i < 0) return 0;
        const float x = p.v[i];
        switch (i)
        {
            case kHpf:  if (x < 1.0f)  snprintf (buf, len, "OFF"); else fmtHz (buf, len, hpfHz (x)); break;
            case kLpf:  if (x > 99.0f) snprintf (buf, len, "OFF"); else fmtHz (buf, len, lpfHz (x)); break;
            case kHfF:  fmtHz (buf, len, hfHz (x));  break;
            case kHmfF: fmtHz (buf, len, hmfHz (x)); break;
            case kLmfF: fmtHz (buf, len, lmfHz (x)); break;
            case kLfF:  fmtHz (buf, len, lfHz (x));  break;
            case kHmfQ: case kLmfQ: snprintf (buf, len, "Q %.2f", qOf (x)); break;
            case kCRatio:
                if (x < 0.5f) snprintf (buf, len, "OFF");
                else if (x > 99.5f) snprintf (buf, len, "LIMIT");
                else snprintf (buf, len, "%.1f:1", ratioOf (x));
                break;
            case kCRel: case kGRel: fmtSec (buf, len, relOf (x)); break;
            case kDrive: case kHfG: case kHmfG: case kLmfG: case kLfG: case kCThresh: case kGRange: case kGThresh: case kOutput:
                snprintf (buf, len, "%+.1f dB", x); break;
            case kMix: snprintf (buf, len, "%d %%", (int) std::lround (x)); break;
            default: return 0;
        }
        return (int) std::strlen (buf) + 1;
    }
};

void* create (const char*)                  { return new Channel4K(); }
void destroy (void* inst)                   { delete static_cast<Channel4K*> (inst); }
void midi (void*, const uint8_t*, int)      {}
void setParam (void* inst, const char* k, const char* val) { static_cast<Channel4K*> (inst)->p.set (k, val); }
int getParam (void* inst, const char* k, char* buf, int len)
{
    if (! inst || ! k || ! buf || len <= 0) return 0;
    auto* e = static_cast<Channel4K*> (inst);
    const int n = e->p.get (k, buf, len);
    return n > 0 ? n : e->display (k, buf, len);
}
void render (void*, int16_t* out, int frames) { std::memset (out, 0, sizeof (int16_t) * 2 * (size_t) frames); }
void process (void* inst, const int16_t* in, int16_t* out, int frames) { static_cast<Channel4K*> (inst)->process (in, out, frames); }

const mpc_engine_t kEngine = { create, destroy, midi, setParam, getParam, render, process, nullptr };
} // namespace

extern "C" const mpc_engine_t* mpc_engine (void) { return &kEngine; }
