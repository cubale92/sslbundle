#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;

APVTS::ParameterLayout Listen4KProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    auto dB = juce::AudioParameterFloatAttributes().withLabel ("dB");
    auto pct = juce::AudioParameterFloatAttributes().withLabel ("%");
    juce::NormalisableRange<float> rel (30.0f, 1200.0f, 1.0f);
    rel.setSkewForCentre (220.0f);

    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "input", 1 }, "Input",
                    juce::NormalisableRange<float> (0.0f, 30.0f, 0.1f), 10.0f, dB));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "release", 1 }, "Release", rel, 220.0f,
                    juce::AudioParameterFloatAttributes().withLabel ("ms")));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "tone", 1 }, "Tone", toneNames(), 1));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "crunch", 1 }, "Crunch",
                    juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 30.0f, pct));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "mix", 1 }, "Mix",
                    juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f, pct));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "output", 1 }, "Output",
                    juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f, dB));
    return { p.begin(), p.end() };
}

Listen4KProcessor::Listen4KProcessor()
    : state (*this, nullptr, "LISTEN4K", createLayout())
{
    pInput   = state.getRawParameterValue ("input");
    pRelease = state.getRawParameterValue ("release");
    pTone    = state.getRawParameterValue ("tone");
    pCrunch  = state.getRawParameterValue ("crunch");
    pMix     = state.getRawParameterValue ("mix");
    pOutput  = state.getRawParameterValue ("output");

    // tone: 0 full, 1 talkback, 2 dark
    setPresets ({
        { "Init", {} },
        { "Room Mic Explosion",     { { "input", 22 }, { "release", 300 }, { "tone", 1 }, { "crunch", 45 }, { "mix", 100 }, { "output", -4 } } },
        { "Boom Bap Break Crush",   { { "input", 16 }, { "release", 160 }, { "tone", 0 }, { "crunch", 40 }, { "mix", 45 } } },
        { "Parallel Kit Smash",     { { "input", 20 }, { "release", 120 }, { "tone", 0 }, { "crunch", 25 }, { "mix", 35 } } },
        { "Snare Room Bloom",       { { "input", 18 }, { "release", 420 }, { "tone", 1 }, { "crunch", 30 }, { "mix", 50 } } },
        { "Dusty Loop Pump",        { { "input", 14 }, { "release", 90 },  { "tone", 2 }, { "crunch", 55 }, { "mix", 60 } } },
        { "Hat Bleed Grit",         { { "input", 12 }, { "release", 60 },  { "tone", 1 }, { "crunch", 70 }, { "mix", 30 } } },
        { "Lo-Fi Telephone Vox",    { { "input", 14 }, { "release", 200 }, { "tone", 1 }, { "crunch", 60 }, { "mix", 100 }, { "output", -2 } } },
        { "Ad-Lib Thickener",       { { "input", 10 }, { "release", 150 }, { "tone", 1 }, { "crunch", 20 }, { "mix", 25 } } },
        { "New Jack Clap Slam",     { { "input", 18 }, { "release", 80 },  { "tone", 0 }, { "crunch", 35 }, { "mix", 40 } } },
        { "Dark Vinyl Kick Room",   { { "input", 16 }, { "release", 260 }, { "tone", 2 }, { "crunch", 30 }, { "mix", 40 } } },
        { "Gentle Room Lift",       { { "input", 8 },  { "release", 250 }, { "tone", 0 }, { "crunch", 10 }, { "mix", 20 } } },
    });
}

void Listen4KProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    oversampler.initProcessing ((size_t) samplesPerBlock);
    oversampler.reset();
    osRate = sampleRate * 2.0;
    setLatencySamples (juce::roundToInt (oversampler.getLatencyInSamples()));

    for (int c = 0; c < 2; ++c) { hp[c].reset(); bump[c].reset(); lp[c].reset(); dc[c].prepare (osRate); dc[c].reset(); }
    lastTone = -1;
    env = gr = 0.0f;
    inGain.reset (osRate, 0.03);  inGain.setCurrentAndTargetValue (ecee::dbToGain (raw (pInput)));
    outGain.reset (osRate, 0.03); outGain.setCurrentAndTargetValue (ecee::dbToGain (raw (pOutput)));
    mix.reset (osRate, 0.03);     mix.setCurrentAndTargetValue (raw (pMix) * 0.01f);
    crunch.reset (osRate, 0.03);  crunch.setCurrentAndTargetValue (raw (pCrunch) * 0.01f);
}

void Listen4KProcessor::updateTone (int tone)
{
    if (tone == lastTone) return;
    lastTone = tone;
    for (int c = 0; c < 2; ++c)
    {
        switch (tone)
        {
            case 1:  // talkback mic: thin bottom, honky presence, rolled-off top
                hp[c].highPass (140.0, 0.8, osRate); bump[c].peak (2400.0, 0.9, 6.0, osRate); lp[c].lowPass (7500.0, 0.8, osRate); break;
            case 2:  // dark: big and woolly
                hp[c].highPass (45.0, 0.7, osRate);  bump[c].peak (250.0, 0.8, 3.0, osRate);  lp[c].lowPass (3800.0, 0.7, osRate); break;
            default: // full range
                hp[c].highPass (25.0, 0.7, osRate);  bump[c].setIdentity();                    lp[c].lowPass (18000.0, 0.7, osRate); break;
        }
    }
}

void Listen4KProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numCh = std::min (buffer.getNumChannels(), 2);
    const int n = buffer.getNumSamples();
    for (int c = getTotalNumInputChannels(); c < buffer.getNumChannels(); ++c)
        buffer.clear (c, 0, n);
    samplesProcessed += n;
    if (numCh == 0 || n == 0) return;

    updateTone (juce::roundToInt (raw (pTone)));
    inGain.setTargetValue (ecee::dbToGain (raw (pInput)));
    outGain.setTargetValue (ecee::dbToGain (raw (pOutput)));
    mix.setTargetValue (raw (pMix) * 0.01f);
    crunch.setTargetValue (raw (pCrunch) * 0.01f);

    // fixed circuit: threshold, huge ratio, near-instant attack. Only the release is variable.
    constexpr float thrDb = -24.0f, ratio = 30.0f, knee = 3.0f, makeupDb = 12.0f;
    const float atkC = ecee::timeCoeff (0.15f, osRate);
    const float relC = ecee::timeCoeff (raw (pRelease) * 0.5f, osRate);
    const float detC = ecee::timeCoeff (1.0f, osRate);
    const float makeupG = ecee::dbToGain (makeupDb);

    juce::dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(), (size_t) numCh, (size_t) n);
    auto up = oversampler.processSamplesUp (block);
    const int N = (int) up.getNumSamples();
    float* ch[2] = { up.getChannelPointer (0), numCh > 1 ? up.getChannelPointer (1) : nullptr };
    float maxGr = 0.0f;

    for (int i = 0; i < N; ++i)
    {
        const float ig = inGain.getNextValue();
        const float m  = mix.getNextValue();
        const float cr = crunch.getNextValue();
        const float og = outGain.getNextValue();

        float v[2] = { 0.0f, 0.0f }, key = 0.0f;
        for (int c = 0; c < numCh; ++c)
        {
            float x = ch[c][i] * ig;
            x = lp[c].process (bump[c].process (hp[c].process (x)));
            v[c] = x;
            key = std::max (key, std::abs (x));
        }

        env = std::max (key, detC * env);
        const float t = ecee::computeGR (ecee::gainToDb (env), thrDb, ratio, knee);
        gr = t > gr ? atkC * gr + (1.0f - atkC) * t : relC * gr + (1.0f - relC) * t;
        maxGr = std::max (maxGr, gr);
        const float g = ecee::dbToGain (-gr) * makeupG;

        // crunch: the overdriven talkback amp
        const float k = 0.6f + 5.0f * cr * cr;
        for (int c = 0; c < numCh; ++c)
        {
            float y = v[c] * g;
            const float s = std::tanh (k * y);
            y = (s + 0.12f * cr * s * s) / k;
            y = dc[c].process (y);
            ch[c][i] = (y * m + ch[c][i] * (1.0f - m)) * og;
        }
    }

    oversampler.processSamplesDown (block);
    meterGr.store (maxGr);
}

juce::AudioProcessorEditor* Listen4KProcessor::createEditor() { return new Listen4KEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Listen4KProcessor(); }
