#include "mpc_console.h"
#include <iostream>
#include <limits>
#include <cstdlib>

void require(bool ok, const char* message)
{
    if (!ok) { std::cerr << message << '\n'; std::exit(1); }
}

int main()
{
    using namespace mpc4k;
    for (int i = -32768; i <= 32767; ++i)
        require(toI16(fromI16((int16_t)i)) == i, "PCM round-trip");
    require(toI16(2) == 32767 && toI16(-2) == -32768, "PCM clipping");
    require(toI16(std::numeric_limits<float>::infinity()) == 0, "infinite PCM");
    const ParamDef defs[] = {{"gain", 0, -24, 12, false}, {"choice", 1, 0, 2, true}};
    ParamTable<2> p(defs);
    for (const char* bad : {"nan", "inf", "-inf", "", "junk", "2junk", "1e99"})
    {
        require(!p.set("gain", bad), "invalid parameter accepted");
        require(p.v[0] == 0, "invalid parameter changed state");
    }
    require(p.set("gain", "999") && p.v[0] == 12, "upper clamp");
    require(p.set("gain", "-999") && p.v[0] == -24, "lower clamp");
    p.set("choice", "1.7"); require(p.v[1] == 2, "choice rounding");
    p.restore("gain=nan;choice=1;unknown=5;gain=-3.125;");
    require(p.v[0] == -3.125f && p.v[1] == 1, "state validation");
    char state[256]; require(p.get("state", state, sizeof state) > 0, "save state");
    ParamTable<2> restored(defs); restored.restore(state);
    require(restored.v[0] == p.v[0] && restored.v[1] == p.v[1], "state round-trip");
    require(p.get("state", state, 1) == 0 && state[0] == 0, "truncated state accepted");
    require(p.get("gain", nullptr, 0) == 0, "empty buffer");
    require(!p.set(nullptr, "0") && !p.set("gain", nullptr), "null input");
    require(ecee::computeGR(-6, -18, 4, 0) == 9, "4:1 compression slope");
    require(ecee::computeGR(-30, -18, 4, 6) == 0, "below threshold");
    require(ecee::computeGR(0, -18, 1, 6) == 0, "1:1 unity");
    for (double fs : {44100., 48000., 96000.})
    {
        ecee::Biquad eq; eq.peak(1000, 1, 12, fs);
        double input = 0, output = 0;
        for (int n = 0; n < 24000; ++n)
        {
            float x = (float)std::sin(2 * 3.141592653589793 * 1000 * n / fs);
            float y = eq.process(x);
            require(std::isfinite(y), "EQ nonfinite");
            if (n > 12000) { input += x*x; output += y*y; }
        }
        require(std::abs(10*std::log10(output/input)-12) < .03, "EQ centre gain");
    }
    Oversampler2x os;
    float sum = 0;
    for (int n = 0; n < 2000; ++n)
    {
        float a,b; os.upsample(.25f,a,b); float y = os.downsample(a,b);
        if (n >= 1000) sum += y;
    }
    require(std::abs(sum/1000-.25f) < .0001, "oversampler DC gain");
    os.reset();
    float a,b; os.upsample(0,a,b);
    require(os.downsample(a,b) == 0, "oversampler reset");
    std::cout << "PCM, parameter/state safety, compressor, EQ and oversampling passed\n";
}
