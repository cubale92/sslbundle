#pragma once
#include "EceeProcessorBase.h"
#include "EceeDSP.h"

/** Channel-4K — E/G-series console channel strip: drive, HPF/LPF, 4-band brown/black EQ,
    compressor/limiter and expander/gate, dynamics pre/post EQ. */
class Channel4KProcessor : public ecee::ProcessorBase
{
public:
    Channel4KProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    using ProcessorBase::processBlock;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    juce::AudioProcessorValueTreeState& apvts() override { return state; }

    juce::AudioProcessorValueTreeState state;
    std::atomic<float> meterGr { 0.0f }, meterGate { 0.0f }, meterOut { -100.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void updateFilters();

    struct EqSnapshot
    {
        float hpf = 0, lpf = 0, hfG = 0, hfF = 0, hmfG = 0, hmfF = 0, hmfQ = 0, lmfG = 0, lmfF = 0, lmfQ = 0, lfG = 0, lfF = 0;
        bool hfBell = false, lfBell = false, black = false;
        auto tie() const noexcept { return std::tie (hpf, lpf, hfG, hfF, hmfG, hmfF, hmfQ, lmfG, lmfF, lmfQ, lfG, lfF, hfBell, lfBell, black); }
        bool operator== (const EqSnapshot& o) const noexcept { return tie() == o.tie(); }
    };

    std::atomic<float>* pDrive; std::atomic<float>* pPhase; std::atomic<float>* pHpf; std::atomic<float>* pLpf;
    std::atomic<float>* pEqType; std::atomic<float>* pEqIn;
    std::atomic<float>* pHfG; std::atomic<float>* pHfF; std::atomic<float>* pHfBell;
    std::atomic<float>* pHmfG; std::atomic<float>* pHmfF; std::atomic<float>* pHmfQ;
    std::atomic<float>* pLmfG; std::atomic<float>* pLmfF; std::atomic<float>* pLmfQ;
    std::atomic<float>* pLfG; std::atomic<float>* pLfF; std::atomic<float>* pLfBell;
    std::atomic<float>* pCRatio; std::atomic<float>* pCThresh; std::atomic<float>* pCRelease; std::atomic<float>* pCFast;
    std::atomic<float>* pGRange; std::atomic<float>* pGThresh; std::atomic<float>* pGRelease; std::atomic<float>* pGMode; std::atomic<float>* pGFast;
    std::atomic<float>* pDynIn; std::atomic<float>* pDynPre; std::atomic<float>* pMix; std::atomic<float>* pOutput;

    juce::dsp::Oversampling<float> oversampler { 2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true };
    double osRate = 88200.0;

    ecee::Biquad hpf2[2], lpf2[2], hf[2], hfDip[2], hmf[2], lmf[2], lf[2], lfDip[2];
    ecee::OnePole hpf1[2];
    bool hpfOn = false, lpfOn = false, hfDipOn = false, lfDipOn = false;
    ecee::DCBlocker dc[2];
    EqSnapshot lastEq;
    bool eqValid = false;

    float compEnv = 0.0f, compGr = 0.0f, gateEnv = 0.0f, gateGain = 1.0f, gateHold = 0.0f;
    bool gateOpen = false;
    juce::SmoothedValue<float> drive, outGain, mix, makeup;
};
