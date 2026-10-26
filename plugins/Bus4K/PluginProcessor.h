#pragma once
#include "EceeProcessorBase.h"
#include "EceeDSP.h"

/** Bus-4K — G-series style quad/stereo bus compressor with the console's summing-amp drive. */
class Bus4KProcessor : public ecee::ProcessorBase
{
public:
    Bus4KProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    using ProcessorBase::processBlock;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    juce::AudioProcessorValueTreeState& apvts() override { return state; }

    juce::AudioProcessorValueTreeState state;
    std::atomic<float> meterGr { 0.0f };

    static juce::StringArray ratioNames()   { return { "2", "4", "10" }; }
    static juce::StringArray attackNames()  { return { ".1", ".3", "1", "3", "10", "30" }; }
    static juce::StringArray releaseNames() { return { ".1", ".3", ".6", "1.2", "AUTO" }; }
    static juce::StringArray hpfNames()     { return { "OFF", "60", "90", "150", "250" }; }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void updateFilters (int hpfIndex);

    std::atomic<float>* pThresh; std::atomic<float>* pRatio; std::atomic<float>* pAttack; std::atomic<float>* pRelease;
    std::atomic<float>* pMakeup; std::atomic<float>* pHpf; std::atomic<float>* pMix; std::atomic<float>* pDrive; std::atomic<float>* pIn;

    juce::dsp::Oversampling<float> oversampler { 2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true };
    double osRate = 88200.0;

    float env2 = 0.0f, gr = 0.0f, grAvg = 0.0f;
    ecee::Biquad scHpf[2];
    ecee::OnePole sumTilt[2];
    ecee::DCBlocker dc[2];
    juce::SmoothedValue<float> makeup, mix, thresh, drive;
    int lastHpf = -1;
};
