// 8cee Listen-4K for MPC OS — the console's talkback (listen mic) compressor (independent port of the desktop DSP).
extern "C" {
#include "engine.h"
}
#include "mpc_console.h"

using namespace mpc4k;

namespace
{
enum P { kInput, kRelease, kTone, kCrunch, kMix, kOutput, kNumParams };

// release is a 0..100 position on a log curve, 30..1200 ms | tone: 0 full, 1 talkback, 2 dark
#include "param_defs.h"

inline float releaseMs (float pos) { return logPos (pos, 30.0f, 1200.0f); }

const double osRate = 88200.0;

struct Smooth
{
    float v = 0, c = 0;
    void init (float value, float ms, double fs) { v = value; c = ecee::timeCoeff (ms, fs); }
    inline float next (float target) { v = target + c * (v - target); return v; }
};

struct Listen4K
{
    ParamTable<kNumParams> p { kDefs };
    Oversampler2x os[2];
    ecee::Biquad hp[2], bump[2], lp[2];
    ecee::DCBlocker dc[2];
    float env = 0, gr = 0;
    int lastTone = -1;
    Smooth inGain, outGain, mix, crunch;

    Listen4K()
    {
        for (auto& d : dc) d.prepare (osRate);
        inGain.init (ecee::dbToGain (p.v[kInput]), 30.0f, osRate);
        outGain.init (1.0f, 30.0f, osRate);
        mix.init (1.0f, 30.0f, osRate);
        crunch.init (p.v[kCrunch] * 0.01f, 30.0f, osRate);
    }

    void updateTone (int tone)
    {
        if (tone == lastTone) return;
        lastTone = tone;
        for (int c = 0; c < 2; ++c)
        {
            switch (tone)
            {
                case 1:  hp[c].highPass (140.0, 0.8, osRate); bump[c].peak (2400.0, 0.9, 6.0, osRate); lp[c].lowPass (7500.0, 0.8, osRate); break;
                case 2:  hp[c].highPass (45.0, 0.7, osRate);  bump[c].peak (250.0, 0.8, 3.0, osRate);  lp[c].lowPass (3800.0, 0.7, osRate); break;
                default: hp[c].highPass (25.0, 0.7, osRate);  bump[c].setIdentity();                   lp[c].lowPass (18000.0, 0.7, osRate); break;
            }
        }
    }

    void process (const int16_t* in, int16_t* out, int frames)
    {
        int tone = (int) std::lround (p.v[kTone]);
        updateTone (tone < 0 ? 0 : (tone > 2 ? 2 : tone));

        const float thrDb = -24.0f, ratio = 30.0f, knee = 3.0f;
        const float makeupG = ecee::dbToGain (12.0f);
        const float atkC = ecee::timeCoeff (0.15f, osRate);
        const float relC = ecee::timeCoeff (releaseMs (p.v[kRelease]) * 0.5f, osRate);
        const float detC = ecee::timeCoeff (1.0f, osRate);
        const float inT = ecee::dbToGain (p.v[kInput]), outT = ecee::dbToGain (p.v[kOutput]);
        const float mixT = p.v[kMix] * 0.01f, crT = p.v[kCrunch] * 0.01f;

        for (int i = 0; i < frames; ++i)
        {
            float up[2][2];
            for (int c = 0; c < 2; ++c)
                os[c].upsample (fromI16 (in[i * 2 + c]), up[c][0], up[c][1]);

            for (int s = 0; s < 2; ++s)
            {
                const float ig = inGain.next (inT), m = mix.next (mixT), cr = crunch.next (crT), og = outGain.next (outT);
                float v[2], key = 0.0f;
                for (int c = 0; c < 2; ++c)
                {
                    float x = up[c][s] * ig;
                    x = lp[c].process (bump[c].process (hp[c].process (x)));
                    v[c] = x;
                    key = std::max (key, std::fabs (x));
                }

                env = std::max (key, detC * env);
                const float t = ecee::computeGR (ecee::gainToDb (env), thrDb, ratio, knee);
                gr = t > gr ? atkC * gr + (1.0f - atkC) * t : relC * gr + (1.0f - relC) * t;
                const float g = ecee::dbToGain (-gr) * makeupG;

                const float k = 0.6f + 5.0f * cr * cr;
                for (int c = 0; c < 2; ++c)
                {
                    float y = v[c] * g;
                    const float sat = std::tanh (k * y);
                    y = (sat + 0.12f * cr * sat * sat) / k;
                    y = dc[c].process (y);
                    up[c][s] = (y * m + up[c][s] * (1.0f - m)) * og;
                }
            }

            for (int c = 0; c < 2; ++c)
                out[i * 2 + c] = toI16 (os[c].downsample (up[c][0], up[c][1]));
        }
    }

    int display (const char* key, char* buf, int len)
    {
        if (std::strcmp (key, "release_display") == 0)     fmtSec (buf, len, releaseMs (p.v[kRelease]) * 0.001f);
        else if (std::strcmp (key, "input_display") == 0)  snprintf (buf, len, "+%.1f dB", p.v[kInput]);
        else if (std::strcmp (key, "output_display") == 0) snprintf (buf, len, "%+.1f dB", p.v[kOutput]);
        else if (std::strcmp (key, "crunch_display") == 0) snprintf (buf, len, "%d %%", (int) std::lround (p.v[kCrunch]));
        else if (std::strcmp (key, "mix_display") == 0)    snprintf (buf, len, "%d %%", (int) std::lround (p.v[kMix]));
        else return 0;
        return (int) std::strlen (buf) + 1;
    }
};

void* create (const char*)                  { return new Listen4K(); }
void destroy (void* inst)                   { delete static_cast<Listen4K*> (inst); }
void midi (void*, const uint8_t*, int)      {}
void setParam (void* inst, const char* k, const char* val) { static_cast<Listen4K*> (inst)->p.set (k, val); }
int getParam (void* inst, const char* k, char* buf, int len)
{
    if (! inst || ! k || ! buf || len <= 0) return 0;
    auto* e = static_cast<Listen4K*> (inst);
    const int n = e->p.get (k, buf, len);
    return n > 0 ? n : e->display (k, buf, len);
}
void render (void*, int16_t* out, int frames) { std::memset (out, 0, sizeof (int16_t) * 2 * (size_t) frames); }
void process (void* inst, const int16_t* in, int16_t* out, int frames) { static_cast<Listen4K*> (inst)->process (in, out, frames); }

const mpc_engine_t kEngine = { create, destroy, midi, setParam, getParam, render, process, nullptr };
} // namespace

extern "C" const mpc_engine_t* mpc_engine (void) { return &kEngine; }
