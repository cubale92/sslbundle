extern "C" {
#include "engine.h"
}
#include "mpc_console.h"
#include <vector>
#include <iostream>
#include <cstdlib>

namespace {
using mpc4k::ParamDef;
#include "param_defs.h"
}

void require(bool ok, const char* why)
{
    if (!ok) { std::cerr << why << '\n'; std::exit(1); }
}
int main()
{
    const auto* api = mpc_engine();
    void* a = api->create(nullptr);
    void* b = api->create(nullptr);
    require(a && b, "create");
    char state[4096], value[128];
    for (const auto& def : kDefs)
    {
        require(api->get_param(a, def.key, value, sizeof value) > 0, "missing parameter");
        require(std::abs(std::strtof(value,nullptr)-def.def) < .0001, "default mismatch");
        api->set_param(a, def.key, "nan");
        api->get_param(a, def.key, value, sizeof value);
        require(std::abs(std::strtof(value,nullptr)-def.def) < .0001, "NaN changed engine state");
    }
    require(api->get_param(a,"state",state,sizeof state) > 0, "save");
    api->set_param(b,"state",state);
    // Identical PCM processing regardless of host block boundaries.
    const int frames=4096;
    std::vector<int16_t> in(frames*2), outA(frames*2), outB(frames*2);
    for (int n=0;n<frames;++n)
        in[2*n]=in[2*n+1]=(int16_t)(8000*std::sin(n*.13));
    api->process(a,in.data(),outA.data(),frames);
    for (int n=0;n<frames;)
    {
        const int count=std::min(1+n%128,frames-n);
        api->process(b,in.data()+2*n,outB.data()+2*n,count); n+=count;
    }
    require(outA==outB,"block partition changed audio");
    long long energy=0;
    for (int n=0;n<frames;++n)
    {
        require(outA[2*n]==outA[2*n+1],"stereo symmetry");
        energy+=(long long)outA[2*n]*outA[2*n];
    }
    require(energy>100000,"silent engine");
    // Stress every valid endpoint and hostile state without poisoning subsequent audio.
    for (const auto& def : kDefs)
    {
        api->set_param(a,def.key,"1e30");
        api->get_param(a,def.key,value,sizeof value);
        require(std::strtof(value,nullptr)==def.maximum,"engine maximum");
        api->process(a,in.data(),outA.data(),frames);
        api->set_param(a,def.key,"-1e30");
        api->get_param(a,def.key,value,sizeof value);
        require(std::strtof(value,nullptr)==def.minimum,"engine minimum");
        api->process(a,in.data(),outA.data(),frames);
        api->set_param(a,"state",state);
    }
    api->destroy(a); api->destroy(b);
    std::cout << "Engine defaults, state, PCM, stereo, block partition and endpoints passed\n";
}
