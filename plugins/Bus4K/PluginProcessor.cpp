#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;

APVTS::ParameterLayout Bus4KProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    auto dB = juce::AudioParameterFloatAttributes().withLabel ("dB");
    auto pct = juce::AudioParameterFloatAttributes().withLabel ("%");

    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "threshold", 1 }, "Threshold",
                    juce::NormalisableRange<float> (-30.0f, 10.0f, 0.1f), -10.0f, dB));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "ratio", 1 }, "Ratio", ratioNames(), 1));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "attack", 1 }, "Attack (ms)", attackNames(), 5));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "release", 1 }, "Release (s)", releaseNames(), 4));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "makeup", 1 }, "Make-Up",
                    juce::NormalisableRange<float> (0.0f, 20.0f, 0.1f), 0.0f, dB));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "schpf", 1 }, "Sidechain HPF", hpfNames(), 0));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "mix", 1 }, "Mix",
                    juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f, pct));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "drive", 1 }, "Console Drive",
                    juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 20.0f, pct));
    p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "compin", 1 }, "Comp In", true));

    return { p.begin(), p.end() };
}

Bus4KProcessor::Bus4KProcessor()
    : state (*this, nullptr, "BUS4K", createLayout())
{
    pThresh  = state.getRawParameterValue ("threshold");
    pRatio   = state.getRawParameterValue ("ratio");
    pAttack  = state.getRawParameterValue ("attack");
    pRelease = state.getRawParameterValue ("release");
    pMakeup  = state.getRawParameterValue ("makeup");
    pHpf     = state.getRawParameterValue ("schpf");
    pMix     = state.getRawParameterValue ("mix");
    pDrive   = state.getRawParameterValue ("drive");
    pIn      = state.getRawParameterValue ("compin");

    // ratio: 0=2 1=4 2=10 | attack: 0=.1 1=.3 2=1 3=3 4=10 5=30 ms | release: 0=.1 1=.3 2=.6 3=1.2 s 4=AUTO | schpf: 0=off 1=60 2=90 3=150 4=250 Hz
    setPresets ({
        { "Init", {} },
        { "90s NY Mix Bus",          { { "threshold", -14 }, { "ratio", 1 }, { "attack", 5 }, { "release", 4 }, { "makeup", 3 },  { "schpf", 1 }, { "drive", 35 } } },
        { "Boom Bap Drum Bus",       { { "threshold", -20 }, { "ratio", 1 }, { "attack", 4 }, { "release", 0 }, { "makeup", 5 },  { "schpf", 2 }, { "drive", 45 } } },
        { "Breakbeat Pump",          { { "threshold", -24 }, { "ratio", 2 }, { "attack", 3 }, { "release", 0 }, { "makeup", 8 },  { "drive", 50 } } },
        { "NY Parallel Crush",       { { "threshold", -28 }, { "ratio", 2 }, { "attack", 0 }, { "release", 0 }, { "makeup", 13 }, { "mix", 40 }, { "drive", 60 } } },
        { "New Jack Drum Snap",      { { "threshold", -18 }, { "ratio", 1 }, { "attack", 3 }, { "release", 1 }, { "makeup", 4 },  { "schpf", 2 }, { "drive", 30 } } },
        { "New Jack Mix Shine",      { { "threshold", -12 }, { "ratio", 0 }, { "attack", 4 }, { "release", 4 }, { "makeup", 2 },  { "schpf", 1 }, { "drive", 25 } } },
        { "Conscious Mix Glue",      { { "threshold", -12 }, { "ratio", 0 }, { "attack", 5 }, { "release", 4 }, { "makeup", 2 },  { "schpf", 1 }, { "drive", 20 } } },
        { "R&B Vocal Bus",           { { "threshold", -14 }, { "ratio", 0 }, { "attack", 4 }, { "release", 1 }, { "makeup", 2 },  { "schpf", 3 }, { "drive", 15 } } },
        { "R&B Ballad Master",       { { "threshold", -8 },  { "ratio", 0 }, { "attack", 5 }, { "release", 4 }, { "makeup", 1 },  { "schpf", 1 }, { "drive", 10 } } },
        { "Sample Chop Glue",        { { "threshold", -16 }, { "ratio", 1 }, { "attack", 3 }, { "release", 1 }, { "makeup", 3 },  { "schpf", 3 }, { "drive", 40 } } },
        { "Music Bus (Under Vocal)", { { "threshold", -14 }, { "ratio", 0 }, { "attack", 5 }, { "release", 2 }, { "makeup", 2 },  { "schpf", 2 }, { "drive", 25 } } },
        { "Master Loud 96",          { { "threshold", -18 }, { "ratio", 1 }, { "attack", 4 }, { "release", 4 }, { "makeup", 6 },  { "schpf", 2 }, { "drive", 55 } } },
        { "Console Colour Only",     { { "compin", 0 }, { "drive", 60 } } },
    });
}

void Bus4KProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    oversampler.initProcessing ((size_t) samplesPerBlock);
    oversampler.reset();
    osRate = sampleRate * 2.0;
    setLatencySamples (juce::roundToInt (oversampler.getLatencyInSamples()));

    env2 = gr = grAvg = 0.0f;
    for (auto& d : dc) { d.prepare (osRate); d.reset(); }
    for (auto& f : scHpf) f.reset();
    for (auto& f : sumTilt) { f.setCutoff (9000.0, osRate); f.reset(); }
    lastHpf = -1;
    makeup.reset (osRate, 0.03); makeup.setCurrentAndTargetValue (ecee::dbToGain (raw (pMakeup)));
    mix.reset (osRate, 0.03);    mix.setCurrentAndTargetValue (raw (pMix) * 0.01f);
    thresh.reset (osRate, 0.05); thresh.setCurrentAndTargetValue (raw (pThresh));
    drive.reset (osRate, 0.05);  drive.setCurrentAndTargetValue (raw (pDrive) * 0.01f);
}

void Bus4KProcessor::updateFilters (int hpfIndex)
{
    if (hpfIndex == lastHpf) return;
    lastHpf = hpfIndex;
    const double freqs[] = { 0.0, 60.0, 90.0, 150.0, 250.0 };
    for (auto& f : scHpf)
    {
        if (hpfIndex <= 0) f.setIdentity();
        else               f.highPass (freqs[hpfIndex], 0.707, osRate);
    }
}

void Bus4KProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numCh = std::min (buffer.getNumChannels(), 2);
    const int n = buffer.getNumSamples();
    for (int c = getTotalNumInputChannels(); c < buffer.getNumChannels(); ++c)
        buffer.clear (c, 0, n);
    samplesProcessed += n;
    if (numCh == 0 || n == 0) return;

    updateFilters (juce::roundToInt (raw (pHpf)));
    makeup.setTargetValue (ecee::dbToGain (raw (pMakeup)));
    mix.setTargetValue (raw (pMix) * 0.01f);
    thresh.setTargetValue (raw (pThresh));
    drive.setTargetValue (raw (pDrive) * 0.01f);

    const float ratios[]   = { 2.0f, 4.0f, 10.0f };
    const float attacks[]  = { 0.1f, 0.3f, 1.0f, 3.0f, 10.0f, 30.0f };
    const float releases[] = { 100.0f, 300.0f, 600.0f, 1200.0f, -1.0f };

    const bool  compIn  = raw (pIn) > 0.5f;
    const float ratio   = ratios[juce::jlimit (0, 2, juce::roundToInt (raw (pRatio)))];
    const float atkMs   = attacks[juce::jlimit (0, 5, juce::roundToInt (raw (pAttack)))];
    const float relMs   = releases[juce::jlimit (0, 4, juce::roundToInt (raw (pRelease)))];
    const bool  autoRel = relMs < 0.0f;
    const float knee    = 4.0f;

    const float atkC    = ecee::timeCoeff (atkMs, osRate);
    const float relC    = ecee::timeCoeff (autoRel ? 600.0f : relMs, osRate);
    const float relFast = ecee::timeCoeff (60.0f, osRate);
    const float relSlow = ecee::timeCoeff (1000.0f, osRate);
    const float rmsC    = ecee::timeCoeff (2.0f, osRate);   // a touch faster detector than a pure RMS bus comp: more "grab"
    const float avgC    = ecee::timeCoeff (1500.0f, osRate);

    juce::dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(), (size_t) numCh, (size_t) n);
    auto up = oversampler.processSamplesUp (block);
    const int N = (int) up.getNumSamples();
    float* ch[2] = { up.getChannelPointer (0), numCh > 1 ? up.getChannelPointer (1) : nullptr };
    float maxGr = 0.0f;

    for (int i = 0; i < N; ++i)
    {
        const float thr = thresh.getNextValue();
        const float mk  = makeup.getNextValue();
        const float m   = mix.getNextValue();
        const float d   = drive.getNextValue();

        if (compIn)
        {
            float s = 0.0f;
            for (int c = 0; c < numCh; ++c)
                s = std::max (s, std::abs (scHpf[c].process (ch[c][i])));
            env2 = rmsC * env2 + (1.0f - rmsC) * s * s;
            const float levelDb = ecee::gainToDb (std::sqrt (env2) * 1.4142f);
            const float t = ecee::computeGR (levelDb, thr, ratio, knee);

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
        else
        {
            gr = 0.0f;
        }

        const float g = compIn ? ecee::dbToGain (-gr) * mk : 1.0f;

        // console summing amp: a little channel crosstalk, then gentle odd/even saturation that grows with DRIVE
        const float k = 0.15f + 1.6f * d * d;
        float in[2] = { ch[0][i], numCh > 1 ? ch[1][i] : 0.0f };
        for (int c = 0; c < numCh; ++c)
        {
            const float dry = in[c];
            float y = dry * g;
            if (numCh > 1) y += 0.006f * d * in[1 - c] * g;
            const float sat = std::tanh (k * y);
            y = (sat + 0.06f * d * sat * sat) / k;
            y = dc[c].process (y);
            // hot drive also dulls the very top a little, like a hard-working summing bus
            const float lp = sumTilt[c].process (y);
            y += (lp - y) * 0.25f * d;
            ch[c][i] = compIn ? y * m + dry * (1.0f - m) : y;
        }
        maxGr = std::max (maxGr, gr);
    }

    oversampler.processSamplesDown (block);
    meterGr.store (maxGr);
}

juce::AudioProcessorEditor* Bus4KProcessor::createEditor() { return new Bus4KEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Bus4KProcessor(); }
