#pragma once
// 8cee — shared DSP helpers (header-only, realtime safe)
#include <cmath>
#include <algorithm>
#include <cstdint>

namespace ecee
{
constexpr float kPi = 3.14159265358979f;

inline float dbToGain (float db) noexcept   { return std::exp (db * 0.11512925465f); }
inline float gainToDb (float g) noexcept    { return 20.0f * std::log10 (std::max (g, 1.0e-9f)); }
inline float clamp01 (float x) noexcept     { return std::min (1.0f, std::max (0.0f, x)); }

/** One-pole smoothing coefficient for a time constant in milliseconds. */
inline float timeCoeff (float ms, double fs) noexcept
{
    if (ms <= 0.0f) return 0.0f;
    return (float) std::exp (-1.0 / (0.001 * (double) ms * fs));
}

/** Soft-knee gain computer. Returns gain reduction in dB (>= 0). */
inline float computeGR (float inDb, float thrDb, float ratio, float kneeDb) noexcept
{
    const float over  = inDb - thrDb;
    const float slope = 1.0f - 1.0f / std::max (1.0f, ratio);

    if (kneeDb > 0.0f && 2.0f * over > -kneeDb && 2.0f * over < kneeDb)
    {
        const float x = over + kneeDb * 0.5f;
        return slope * x * x / (2.0f * kneeDb);
    }
    return over <= 0.0f ? 0.0f : slope * over;
}

/** RBJ biquad, transposed direct form II. */
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    float z1 = 0, z2 = 0;

    void reset() noexcept { z1 = z2 = 0; }
    void setIdentity() noexcept { b0 = 1; b1 = b2 = a1 = a2 = 0; }

    inline float process (float x) noexcept
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }

    void set (double B0, double B1, double B2, double A0, double A1, double A2) noexcept
    {
        b0 = (float) (B0 / A0); b1 = (float) (B1 / A0); b2 = (float) (B2 / A0);
        a1 = (float) (A1 / A0); a2 = (float) (A2 / A0);
    }

    static double safeFreq (double f, double fs) { return std::min (f, fs * 0.45); }

    void lowPass (double f, double q, double fs)
    {
        f = safeFreq (f, fs);
        const double w = 2.0 * 3.141592653589793 * f / fs, c = std::cos (w), al = std::sin (w) / (2.0 * q);
        set ((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void highPass (double f, double q, double fs)
    {
        f = safeFreq (f, fs);
        const double w = 2.0 * 3.141592653589793 * f / fs, c = std::cos (w), al = std::sin (w) / (2.0 * q);
        set ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void peak (double f, double q, double gainDb, double fs)
    {
        f = safeFreq (f, fs);
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w = 2.0 * 3.141592653589793 * f / fs, c = std::cos (w), al = std::sin (w) / (2.0 * q);
        set (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
    }
    void lowShelf (double f, double gainDb, double fs, double S = 0.8)
    {
        f = safeFreq (f, fs);
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w = 2.0 * 3.141592653589793 * f / fs, c = std::cos (w), s = std::sin (w);
        const double al = s / 2.0 * std::sqrt ((A + 1 / A) * (1 / S - 1) + 2), sa = 2 * std::sqrt (A) * al;
        set (A * ((A + 1) - (A - 1) * c + sa), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - sa),
             (A + 1) + (A - 1) * c + sa, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - sa);
    }
    void highShelf (double f, double gainDb, double fs, double S = 0.8)
    {
        f = safeFreq (f, fs);
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w = 2.0 * 3.141592653589793 * f / fs, c = std::cos (w), s = std::sin (w);
        const double al = s / 2.0 * std::sqrt ((A + 1 / A) * (1 / S - 1) + 2), sa = 2 * std::sqrt (A) * al;
        set (A * ((A + 1) + (A - 1) * c + sa), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - sa),
             (A + 1) - (A - 1) * c + sa, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - sa);
    }
};

/** First-order DC blocker. */
struct DCBlocker
{
    float x1 = 0, y1 = 0, r = 0.9995f;
    void prepare (double fs) noexcept { r = (float) std::exp (-2.0 * 3.141592653589793 * 8.0 / fs); }
    void reset() noexcept { x1 = y1 = 0; }
    inline float process (float x) noexcept
    {
        const float y = x - x1 + r * y1;
        x1 = x; y1 = y;
        return y;
    }
};

/** One-pole low-pass. */
struct OnePole
{
    float a = 0, z = 0;
    void setCutoff (double hz, double fs) noexcept { a = (float) std::exp (-2.0 * 3.141592653589793 * hz / fs); }
    void reset (float v = 0) noexcept { z = v; }
    inline float process (float x) noexcept { z = x + a * (z - x); return z; }
};

/** Cheap realtime-safe white noise in [-1, 1]. */
struct Noise
{
    uint32_t s = 0x9E3779B9u;
    explicit Noise (uint32_t seed = 0x9E3779B9u) : s (seed ? seed : 1u) {}
    inline float next() noexcept
    {
        s ^= s << 13; s ^= s >> 17; s ^= s << 5;
        return (float) (int32_t) s * (1.0f / 2147483648.0f);
    }
};
} // namespace ecee
