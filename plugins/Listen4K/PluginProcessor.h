#pragma once
#include "EceeProcessorBase.h"
#include "EceeDSP.h"

/** Listen-4K — the console's talkback ("listen mic") compressor: a fixed, brutal, very fast compressor
    with the talkback mic voicing, made famous as a room/drum crusher. */
class Listen4KProcessor : public ecee::ProcessorBase
{
public:
    Listen4KProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    using ProcessorBase::processBlock;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    juce::AudioProcessorValueTreeState& apvts() override { return state; }

    juce::AudioProcessorValueTreeState state;
    std::atomic<float> meterGr { 0.0f };

    static juce::StringArray toneNames() { return { "FULL", "TALKBACK", "DARK" }; }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void updateTone (int tone);

    std::atomic<float>* pInput; std::atomic<float>* pRelease; std::atomic<float>* pTone;
    std::atomic<float>* pCrunch; std::atomic<float>* pMix; std::atomic<float>* pOutput;

    juce::dsp::Oversampling<float> oversampler { 2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true };
    double osRate = 88200.0;

    ecee::Biquad hp[2], bump[2], lp[2];
    ecee::DCBlocker dc[2];
    float env = 0.0f, gr = 0.0f;
    int lastTone = -1;
    juce::SmoothedValue<float> inGain, outGain, mix, crunch;
};
