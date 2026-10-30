// 8cee Bus-4K for MPC OS — centre-section bus compressor with console summing drive (independent port of the desktop DSP).
extern "C" {
#include "engine.h"
}
#include "mpc_console.h"

using namespace mpc4k;

namespace
{
enum P { kThresh, kRatio, kAttack, kRelease, kMakeup, kHpf, kMix, kDrive, kCompIn, kNumParams };

// ratio: 0=2 1=4 2=10 | attack: 0=.1 1=.3 2=1 3=3 4=10 5=30 ms | release: 0=.1 1=.3 2=.6 3=1.2 s 4=AUTO | schpf: 0=off 1=60 2=90 3=150 4=250 Hz
#include "param_defs.h"

const double osRate = 88200.0;

struct Smooth
{
    float v = 0, c = 0;
    void init (float value, float ms, double fs) { v = value; c = ecee::timeCoeff (ms, fs); }
    inline float next (float target) { v = target + c * (v - target); return v; }
};

inline int idx (float v, int maxIdx) { const int i = (int) std::lround (v); return i < 0 ? 0 : (i > maxIdx ? maxIdx : i); }

struct Bus4K
{
    ParamTable<kNumParams> p { kDefs };
    Oversampler2x os[2];
    ecee::Biquad scHpf[2];
    ecee::OnePole sumTilt[2];
    ecee::DCBlocker dc[2];
    float env2 = 0, gr = 0, grAvg = 0;
    int lastHpf = -1;
    Smooth makeup, mix, thresh, drive;

    Bus4K()
    {
        for (auto& d : dc) d.prepare (osRate);
        for (auto& f : sumTilt) f.setCutoff (9000.0, osRate);
        makeup.init (1.0f, 30.0f, osRate);
        mix.init (1.0f, 30.0f, osRate);
        thresh.init (p.v[kThresh], 50.0f, osRate);
        drive.init (p.v[kDrive] * 0.01f, 50.0f, osRate);
    }

    void process (const int16_t* in, int16_t* out, int frames)
    {
        const int hpfIdx = idx (p.v[kHpf], 4);
        if (hpfIdx != lastHpf)
        {
            lastHpf = hpfIdx;
            const double freqs[] = { 0.0, 60.0, 90.0, 150.0, 250.0 };
            for (auto& f : scHpf) { if (hpfIdx == 0) f.setIdentity(); else f.highPass (freqs[hpfIdx], 0.707, osRate); }
        }

        const float ratios[]   = { 2.0f, 4.0f, 10.0f };
        const float attacks[]  = { 0.1f, 0.3f, 1.0f, 3.0f, 10.0f, 30.0f };
        const float releases[] = { 100.0f, 300.0f, 600.0f, 1200.0f, -1.0f };

        const bool  compIn  = p.v[kCompIn] > 0.5f;
        const float ratio   = ratios[idx (p.v[kRatio], 2)];
        const float atkMs   = attacks[idx (p.v[kAttack], 5)];
        const float relMs   = releases[idx (p.v[kRelease], 4)];
        const bool  autoRel = relMs < 0.0f;
        const float knee    = 4.0f;
        const float atkC    = ecee::timeCoeff (atkMs, osRate);
        const float relC    = ecee::timeCoeff (autoRel ? 600.0f : relMs, osRate);
        const float relFast = ecee::timeCoeff (60.0f, osRate);
        const float relSlow = ecee::timeCoeff (1000.0f, osRate);
        const float rmsC    = ecee::timeCoeff (2.0f, osRate);
        const float avgC    = ecee::timeCoeff (1500.0f, osRate);

        const float mkT = ecee::dbToGain (p.v[kMakeup]), mixT = p.v[kMix] * 0.01f, thrT = p.v[kThresh], drvT = p.v[kDrive] * 0.01f;

        for (int i = 0; i < frames; ++i)
        {
            float up[2][2];
            for (int c = 0; c < 2; ++c)
                os[c].upsample (fromI16 (in[i * 2 + c]), up[c][0], up[c][1]);

            for (int s = 0; s < 2; ++s)
            {
                const float thr = thresh.next (thrT), mk = makeup.next (mkT), m = mix.next (mixT), d = drive.next (drvT);

                if (compIn)
                {
                    float sc = 0.0f;
                    for (int c = 0; c < 2; ++c) sc = std::max (sc, std::fabs (scHpf[c].process (up[c][s])));
                    env2 = rmsC * env2 + (1.0f - rmsC) * sc * sc;
                    const float t = ecee::computeGR (ecee::gainToDb (std::sqrt (env2) * 1.4142f), thr, ratio, knee);
                    if (t > gr) gr = atkC * gr + (1.0f - atkC) * t;
                    else
                    {
                        float rc = relC;
                        if (autoRel)
                        {
                            const float sustained = ecee::clamp01 (grAvg / std::max (gr, 0.05f));
                            rc = relFast + (relSlow - relFast) * sustained;
                        }
                        gr = rc * gr + (1.0f - rc) * t;
                    }
                    grAvg = avgC * grAvg + (1.0f - avgC) * gr;
                }
                else gr = 0.0f;

                const float g = compIn ? ecee::dbToGain (-gr) * mk : 1.0f;
                const float k = 0.15f + 1.6f * d * d;
                const float x0 = up[0][s], x1 = up[1][s];
                for (int c = 0; c < 2; ++c)
                {
                    const float dry = c == 0 ? x0 : x1, other = c == 0 ? x1 : x0;
                    float y = dry * g + 0.006f * d * other * g;
                    const float sat = std::tanh (k * y);
                    y = (sat + 0.06f * d * sat * sat) / k;
                    y = dc[c].process (y);
                    const float lp = sumTilt[c].process (y);
                    y += (lp - y) * 0.25f * d;
                    up[c][s] = compIn ? y * m + dry * (1.0f - m) : y;
                }
            }

            for (int c = 0; c < 2; ++c)
                out[i * 2 + c] = toI16 (os[c].downsample (up[c][0], up[c][1]));
        }
    }

    int display (const char* key, char* buf, int len)
    {
        if (std::strcmp (key, "threshold_display") == 0) { snprintf (buf, len, "%+.1f dB", p.v[kThresh]); }
        else if (std::strcmp (key, "makeup_display") == 0) { snprintf (buf, len, "+%.1f dB", p.v[kMakeup]); }
        else if (std::strcmp (key, "mix_display") == 0)    { snprintf (buf, len, "%d %%", (int) std::lround (p.v[kMix])); }
        else if (std::strcmp (key, "drive_display") == 0)  { snprintf (buf, len, "%d %%", (int) std::lround (p.v[kDrive])); }
        else return 0;
        return (int) std::strlen (buf) + 1;
    }
};

void* create (const char*)                  { return new Bus4K(); }
void destroy (void* inst)                   { delete static_cast<Bus4K*> (inst); }
void midi (void*, const uint8_t*, int)      {}
void setParam (void* inst, const char* k, const char* val) { static_cast<Bus4K*> (inst)->p.set (k, val); }
int getParam (void* inst, const char* k, char* buf, int len)
{
    if (! inst || ! k || ! buf || len <= 0) return 0;
    auto* e = static_cast<Bus4K*> (inst);
    const int n = e->p.get (k, buf, len);
    return n > 0 ? n : e->display (k, buf, len);
}
void render (void*, int16_t* out, int frames) { std::memset (out, 0, sizeof (int16_t) * 2 * (size_t) frames); }
void process (void* inst, const int16_t* in, int16_t* out, int frames) { static_cast<Bus4K*> (inst)->process (in, out, frames); }

const mpc_engine_t kEngine = { create, destroy, midi, setParam, getParam, render, process, nullptr };
} // namespace

extern "C" const mpc_engine_t* mpc_engine (void) { return &kEngine; }
