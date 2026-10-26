#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;

namespace
{
juce::NormalisableRange<float> skewed (float lo, float hi, float centre, float step = 0.0f)
{
    juce::NormalisableRange<float> r (lo, hi, step);
    r.setSkewForCentre (centre);
    return r;
}

juce::String hz (float v)
{
    return v >= 1000.0f ? juce::String (v / 1000.0f, v >= 10000.0f ? 1 : 2) + " kHz" : juce::String (juce::roundToInt (v)) + " Hz";
}

constexpr float kHpfOff = 16.0f;     // at (or below) this the HPF is out
constexpr float kLpfOff = 21500.0f;  // at (or above) this the LPF is out
} // namespace

APVTS::ParameterLayout Channel4KProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    auto dB = juce::AudioParameterFloatAttributes().withLabel ("dB");
    auto freq = [] (const juce::String& offText, bool lowIsOff)
    {
        return juce::AudioParameterFloatAttributes().withStringFromValueFunction ([offText, lowIsOff] (float v, int)
        {
            if (lowIsOff ? v <= kHpfOff : v >= kLpfOff) return offText;
            return hz (v);
        });
    };
    auto hzAttr = juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return hz (v); });
    auto sec = juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
        { return v < 1.0f ? juce::String (juce::roundToInt (v * 1000.0f)) + " ms" : juce::String (v, 2) + " s"; });
    auto ratioAttr = juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
        { return v <= 1.01f ? juce::String ("1:1 (off)") : (v >= 19.9f ? juce::String ("LIMIT") : juce::String (v, 1) + ":1"); });
    auto pct = juce::AudioParameterFloatAttributes().withLabel ("%");

    auto addF = [&] (const char* id, const char* name, juce::NormalisableRange<float> r, float def, juce::AudioParameterFloatAttributes a)
    { p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name, r, def, a)); };
    auto addB = [&] (const char* id, const char* name, bool def)
    { p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, 1 }, name, def)); };

    addF ("drive",  "Drive",   juce::NormalisableRange<float> (0.0f, 24.0f, 0.1f), 0.0f, dB);
    addB ("phase",  "Polarity Invert", false);
    addF ("hpf",    "High-Pass",  skewed (kHpfOff, 400.0f, 70.0f), kHpfOff, freq ("OFF", true));
    addF ("lpf",    "Low-Pass",   skewed (3000.0f, 22000.0f, 9000.0f), 22000.0f, freq ("OFF", false));

    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "eqtype", 1 }, "EQ Type",
                    juce::StringArray { "Brown (E)", "Black (G)" }, 1));
    addB ("eqin",   "EQ In", true);

    addF ("hfg",    "HF Gain",  juce::NormalisableRange<float> (-16.0f, 16.0f, 0.1f), 0.0f, dB);
    addF ("hff",    "HF Freq",  skewed (1500.0f, 16000.0f, 6000.0f), 8000.0f, hzAttr);
    addB ("hfbell", "HF Bell", false);
    addF ("hmfg",   "HMF Gain", juce::NormalisableRange<float> (-16.0f, 16.0f, 0.1f), 0.0f, dB);
    addF ("hmff",   "HMF Freq", skewed (600.0f, 7000.0f, 2000.0f), 2500.0f, hzAttr);
    addF ("hmfq",   "HMF Q",    skewed (0.4f, 4.0f, 1.2f), 1.0f, {});
    addF ("lmfg",   "LMF Gain", juce::NormalisableRange<float> (-16.0f, 16.0f, 0.1f), 0.0f, dB);
    addF ("lmff",   "LMF Freq", skewed (200.0f, 2500.0f, 700.0f), 600.0f, hzAttr);
    addF ("lmfq",   "LMF Q",    skewed (0.4f, 4.0f, 1.2f), 1.0f, {});
    addF ("lfg",    "LF Gain",  juce::NormalisableRange<float> (-16.0f, 16.0f, 0.1f), 0.0f, dB);
    addF ("lff",    "LF Freq",  skewed (30.0f, 450.0f, 100.0f), 100.0f, hzAttr);
    addB ("lfbell", "LF Bell", false);

    addF ("cratio",  "Comp Ratio",     skewed (1.0f, 20.0f, 4.0f, 0.01f), 1.0f, ratioAttr);
    addF ("cthresh", "Comp Threshold", juce::NormalisableRange<float> (-40.0f, 10.0f, 0.1f), 10.0f, dB);
    addF ("crel",    "Comp Release",   skewed (0.05f, 4.0f, 0.4f), 0.3f, sec);
    addB ("cfast",   "Comp Fast Attack", false);

    addF ("grange",  "Gate Range",     juce::NormalisableRange<float> (0.0f, 40.0f, 0.1f), 0.0f, dB);
    addF ("gthresh", "Gate Threshold", juce::NormalisableRange<float> (-70.0f, 0.0f, 0.1f), -50.0f, dB);
    addF ("grel",    "Gate Release",   skewed (0.05f, 4.0f, 0.4f), 0.25f, sec);
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "gmode", 1 }, "Gate Mode",
                    juce::StringArray { "Expander", "Gate" }, 0));
    addB ("gfast",   "Gate Fast Attack", false);

    addB ("dynin",  "Dynamics In", true);
    addB ("dynpre", "Dynamics Pre-EQ", false);
    addF ("mix",    "Dynamics Mix", juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f, pct);
    addF ("output", "Output", juce::NormalisableRange<float> (-18.0f, 18.0f, 0.1f), 0.0f, dB);

    return { p.begin(), p.end() };
}

Channel4KProcessor::Channel4KProcessor()
    : state (*this, nullptr, "CHANNEL4K", createLayout())
{
    auto P = [this] (const char* id) { return state.getRawParameterValue (id); };
    pDrive = P ("drive"); pPhase = P ("phase"); pHpf = P ("hpf"); pLpf = P ("lpf");
    pEqType = P ("eqtype"); pEqIn = P ("eqin");
    pHfG = P ("hfg"); pHfF = P ("hff"); pHfBell = P ("hfbell");
    pHmfG = P ("hmfg"); pHmfF = P ("hmff"); pHmfQ = P ("hmfq");
    pLmfG = P ("lmfg"); pLmfF = P ("lmff"); pLmfQ = P ("lmfq");
    pLfG = P ("lfg"); pLfF = P ("lff"); pLfBell = P ("lfbell");
    pCRatio = P ("cratio"); pCThresh = P ("cthresh"); pCRelease = P ("crel"); pCFast = P ("cfast");
    pGRange = P ("grange"); pGThresh = P ("gthresh"); pGRelease = P ("grel"); pGMode = P ("gmode"); pGFast = P ("gfast");
    pDynIn = P ("dynin"); pDynPre = P ("dynpre"); pMix = P ("mix"); pOutput = P ("output");

    // eqtype: 0 brown (E), 1 black (G) | gmode: 0 expander, 1 gate. Levels are dBFS; the test loop sits around -14 dBFS RMS.
    setPresets ({
        { "Init", {} },
        { "Boom Bap Kick Thump",   { { "drive", 8 }, { "hpf", 28 }, { "lfg", 5 }, { "lff", 60 }, { "lfbell", 1 }, { "lmfg", -5 }, { "lmff", 380 }, { "lmfq", 1.4f },
                                     { "hmfg", 3.5f }, { "hmff", 3200 }, { "hmfq", 1.6f }, { "lpf", 12000 },
                                     { "cratio", 4 }, { "cthresh", -16 }, { "crel", 0.18f },
                                     { "grange", 14 }, { "gthresh", -42 }, { "grel", 0.12f } } },
        { "Boom Bap Snare Crack",  { { "drive", 10 }, { "hpf", 110 }, { "lmfg", 3 }, { "lmff", 210 }, { "lmfq", 1.2f },
                                     { "hmfg", 4.5f }, { "hmff", 4200 }, { "hmfq", 1.4f }, { "hfg", 3 }, { "hff", 10000 },
                                     { "cratio", 6 }, { "cthresh", -18 }, { "crel", 0.14f },
                                     { "grange", 22 }, { "gthresh", -34 }, { "grel", 0.16f }, { "gmode", 1 } } },
        { "Dusty Break Loop",      { { "drive", 12 }, { "hpf", 45 }, { "lpf", 11000 }, { "eqtype", 0 }, { "lfg", 3 }, { "lff", 90 },
                                     { "lmfg", -3 }, { "lmff", 420 }, { "hmfg", 2 }, { "hmff", 1800 }, { "hmfq", 0.8f },
                                     { "cratio", 4 }, { "cthresh", -14 }, { "crel", 0.3f }, { "mix", 75 } } },
        { "NY Parallel Drum Smash", { { "drive", 14 }, { "hpf", 35 }, { "lfg", 2.5f }, { "lff", 80 }, { "hmfg", 3 }, { "hmff", 2800 }, { "hmfq", 0.8f },
                                      { "cratio", 20 }, { "cthresh", -28 }, { "crel", 0.1f }, { "cfast", 1 }, { "mix", 45 } } },
        { "Hats & Shaker Tame",    { { "hpf", 260 }, { "hmfg", -2.5f }, { "hmff", 3600 }, { "hmfq", 1.8f }, { "hfg", 2 }, { "hff", 12000 },
                                     { "cratio", 3 }, { "cthresh", -14 }, { "crel", 0.08f }, { "cfast", 1 } } },
        { "Jazz Upright Bass Sample", { { "drive", 6 }, { "hpf", 35 }, { "lpf", 6000 }, { "eqtype", 0 }, { "lfg", 3 }, { "lff", 80 }, { "lfbell", 1 },
                                        { "lmfg", -3 }, { "lmff", 300 }, { "lmfq", 1.0f }, { "hmfg", 2.5f }, { "hmff", 900 }, { "hmfq", 1.2f },
                                        { "cratio", 4 }, { "cthresh", -16 }, { "crel", 0.4f } } },
        { "New Jack Synth Bass",   { { "drive", 9 }, { "hpf", 28 }, { "lfg", 4 }, { "lff", 55 }, { "lfbell", 1 }, { "lmfg", -2.5f }, { "lmff", 260 },
                                     { "hmfg", 3 }, { "hmff", 1200 }, { "hmfq", 1.4f }, { "cratio", 3 }, { "cthresh", -14 }, { "crel", 0.25f } } },
        { "New Jack Kick Punch",   { { "drive", 7 }, { "hpf", 30 }, { "lfg", 6 }, { "lff", 70 }, { "lfbell", 1 }, { "lmfg", -6 }, { "lmff", 420 }, { "lmfq", 1.6f },
                                     { "hmfg", 4 }, { "hmff", 3500 }, { "hmfq", 1.8f }, { "cratio", 4 }, { "cthresh", -14 }, { "crel", 0.12f },
                                     { "grange", 18 }, { "gthresh", -36 }, { "grel", 0.1f } } },
        { "New Jack Gated Snare",  { { "drive", 8 }, { "hpf", 120 }, { "hmfg", 5 }, { "hmff", 2000 }, { "hmfq", 1.0f }, { "hfg", 4 }, { "hff", 8000 },
                                     { "cratio", 8 }, { "cthresh", -20 }, { "crel", 0.12f }, { "cfast", 1 },
                                     { "grange", 40 }, { "gthresh", -22 }, { "grel", 0.11f }, { "gmode", 1 }, { "gfast", 1 } } },
        { "NY Rap Vocal Upfront",  { { "drive", 4 }, { "hpf", 95 }, { "lmfg", -3 }, { "lmff", 300 }, { "lmfq", 1.4f }, { "hmfg", 3 }, { "hmff", 3200 },
                                     { "hfg", 4 }, { "hff", 12000 }, { "cratio", 4 }, { "cthresh", -20 }, { "crel", 0.18f },
                                     { "grange", 10 }, { "gthresh", -48 }, { "grel", 0.3f } } },
        { "Conscious Vocal Warm",  { { "drive", 3 }, { "eqtype", 0 }, { "hpf", 80 }, { "lfg", 1.5f }, { "lff", 140 }, { "lmfg", -2 }, { "lmff", 420 },
                                     { "hmfg", 1.5f }, { "hmff", 2500 }, { "hmfq", 0.7f }, { "hfg", 2.5f }, { "hff", 10000 },
                                     { "cratio", 3 }, { "cthresh", -16 }, { "crel", 0.4f } } },
        { "R&B Lead Vocal Silk",   { { "drive", 2 }, { "hpf", 75 }, { "lmfg", -2.5f }, { "lmff", 260 }, { "lmfq", 1.2f }, { "hmfg", 2 }, { "hmff", 5000 }, { "hmfq", 0.7f },
                                     { "hfg", 5 }, { "hff", 14000 }, { "cratio", 3 }, { "cthresh", -18 }, { "crel", 0.3f } } },
        { "R&B Background Stack",  { { "hpf", 160 }, { "lmfg", -4 }, { "lmff", 350 }, { "hmfg", -1.5f }, { "hmff", 2600 }, { "hmfq", 1.0f },
                                     { "hfg", 4 }, { "hff", 12000 }, { "cratio", 6 }, { "cthresh", -22 }, { "crel", 0.25f } } },
        { "Rhodes & Keys Sample",  { { "drive", 5 }, { "eqtype", 0 }, { "hpf", 60 }, { "lpf", 9000 }, { "lmfg", 2 }, { "lmff", 300 }, { "lmfq", 0.8f },
                                     { "hmfg", -2 }, { "hmff", 1500 }, { "cratio", 2 }, { "cthresh", -14 }, { "crel", 0.6f } } },
        { "Soul Horn Stab Chop",   { { "drive", 7 }, { "hpf", 120 }, { "lpf", 10000 }, { "lmfg", -2 }, { "lmff", 400 }, { "hmfg", 3 }, { "hmff", 2000 }, { "hmfq", 0.9f },
                                     { "cratio", 4 }, { "cthresh", -16 }, { "crel", 0.2f } } },
        { "DJ Scratch Cuts",       { { "drive", 8 }, { "hpf", 200 }, { "hmfg", 4 }, { "hmff", 2500 }, { "hmfq", 1.2f },
                                     { "cratio", 6 }, { "cthresh", -18 }, { "crel", 0.1f }, { "cfast", 1 } } },
        { "Vinyl Sample Cleanup",  { { "hpf", 45 }, { "lpf", 14000 }, { "lmfg", -2 }, { "lmff", 250 }, { "hmfg", -2.5f }, { "hmff", 4500 }, { "hmfq", 2.2f },
                                     { "cratio", 2 }, { "cthresh", -12 }, { "crel", 0.5f } } },
    });
}

void Channel4KProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    oversampler.initProcessing ((size_t) samplesPerBlock);
    oversampler.reset();
    osRate = sampleRate * 2.0;
    setLatencySamples (juce::roundToInt (oversampler.getLatencyInSamples()));

    for (auto* f : { hpf2, lpf2, hf, hfDip, hmf, lmf, lf, lfDip })
        for (int c = 0; c < 2; ++c) f[c].reset();
    for (auto& f : hpf1) f.reset();
    for (auto& d : dc) { d.prepare (osRate); d.reset(); }
    eqValid = false;

    compEnv = compGr = gateEnv = gateHold = 0.0f;
    gateGain = 1.0f; gateOpen = false;

    drive.reset (osRate, 0.03);   drive.setCurrentAndTargetValue (ecee::dbToGain (raw (pDrive)));
    outGain.reset (osRate, 0.03); outGain.setCurrentAndTargetValue (ecee::dbToGain (raw (pOutput)));
    mix.reset (osRate, 0.03);     mix.setCurrentAndTargetValue (raw (pMix) * 0.01f);
    makeup.reset (osRate, 0.08);  makeup.setCurrentAndTargetValue (1.0f);
}

void Channel4KProcessor::updateFilters()
{
    EqSnapshot s;
    s.hpf = raw (pHpf); s.lpf = raw (pLpf);
    s.hfG = raw (pHfG); s.hfF = raw (pHfF); s.hfBell = raw (pHfBell) > 0.5f;
    s.hmfG = raw (pHmfG); s.hmfF = raw (pHmfF); s.hmfQ = raw (pHmfQ);
    s.lmfG = raw (pLmfG); s.lmfF = raw (pLmfF); s.lmfQ = raw (pLmfQ);
    s.lfG = raw (pLfG); s.lfF = raw (pLfF); s.lfBell = raw (pLfBell) > 0.5f;
    s.black = raw (pEqType) > 0.5f;
    if (eqValid && s == lastEq) return;
    lastEq = s; eqValid = true;

    const double fs = osRate;
    // Black (G) mids get narrower as you push them (proportional Q); Brown (E) keeps the set Q.
    auto qFor = [&] (float q, float g) { return s.black ? q * (0.55f + 1.25f * std::abs (g) / 16.0f) : q; };

    hpfOn = s.hpf > kHpfOff;
    lpfOn = s.lpf < kLpfOff;
    hfDipOn = s.black && ! s.hfBell && std::abs (s.hfG) > 0.05f;
    lfDipOn = s.black && ! s.lfBell && std::abs (s.lfG) > 0.05f;

    for (int c = 0; c < 2; ++c)
    {
        // 18 dB/oct high-pass (2nd-order with a little resonance + 1st-order), 12 dB/oct low-pass
        if (hpfOn) { hpf2[c].highPass (s.hpf, 0.9, fs); hpf1[c].setCutoff (s.hpf, fs); }
        if (lpfOn) lpf2[c].lowPass (s.lpf, 0.707, fs);

        if (s.hfBell) hf[c].peak (s.hfF, qFor (0.9f, s.hfG), s.hfG, fs);
        else          hf[c].highShelf (s.hfF, s.hfG, fs, s.black ? 1.0 : 0.6);
        if (hfDipOn)  hfDip[c].peak (s.hfF * 0.42, 1.1, -0.14 * s.hfG, fs);   // the black shelf's small dip/overshoot

        hmf[c].peak (s.hmfF, qFor (s.hmfQ, s.hmfG), s.hmfG, fs);
        lmf[c].peak (s.lmfF, qFor (s.lmfQ, s.lmfG), s.lmfG, fs);

        if (s.lfBell) lf[c].peak (s.lfF, qFor (0.9f, s.lfG), s.lfG, fs);
        else          lf[c].lowShelf (s.lfF, s.lfG, fs, s.black ? 1.0 : 0.6);
        if (lfDipOn)  lfDip[c].peak (std::min (s.lfF * 2.3, 1200.0), 1.1, -0.14 * s.lfG, fs);
    }
}

void Channel4KProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numCh = std::min (buffer.getNumChannels(), 2);
    const int n = buffer.getNumSamples();
    for (int c = getTotalNumInputChannels(); c < buffer.getNumChannels(); ++c)
        buffer.clear (c, 0, n);
    samplesProcessed += n;
    if (numCh == 0 || n == 0) return;

    updateFilters();
    drive.setTargetValue (ecee::dbToGain (raw (pDrive)));
    outGain.setTargetValue (ecee::dbToGain (raw (pOutput)));
    mix.setTargetValue (raw (pMix) * 0.01f);

    const bool  phase  = raw (pPhase) > 0.5f;
    const bool  eqIn   = raw (pEqIn) > 0.5f;
    const bool  dynIn  = raw (pDynIn) > 0.5f;
    const bool  dynPre = raw (pDynPre) > 0.5f;

    // compressor
    const float ratio   = raw (pCRatio);
    const float cThr    = raw (pCThresh);
    const bool  compOn  = dynIn && ratio > 1.01f;
    const bool  cFast   = raw (pCFast) > 0.5f;
    const float cAtk    = ecee::timeCoeff (cFast ? 0.3f : 5.0f, osRate);
    const float cRel    = ecee::timeCoeff (raw (pCRelease) * 1000.0f * 0.4f, osRate);
    const float detC    = ecee::timeCoeff (cFast ? 0.05f : 4.0f, osRate);   // peak in fast mode, short RMS otherwise
    const float knee    = ratio >= 19.9f ? 1.0f : 5.0f;
    const float effRatio = ratio >= 19.9f ? 100.0f : ratio;
    // automatic make-up, like the console: roughly half the reduction a hot (-10 dBFS) signal would get
    makeup.setTargetValue (compOn ? ecee::dbToGain (0.45f * ecee::computeGR (-10.0f, cThr, effRatio, knee)) : 1.0f);

    // expander / gate
    const float gRange  = raw (pGRange);
    const bool  gateOn  = dynIn && gRange > 0.1f;
    const float gThr    = raw (pGThresh);
    const bool  hardGate = raw (pGMode) > 0.5f;
    const float gAtk    = ecee::timeCoeff (raw (pGFast) > 0.5f ? 0.1f : 1.5f, osRate);
    const float gRel    = ecee::timeCoeff (raw (pGRelease) * 1000.0f * 0.4f, osRate);
    const float gDet    = ecee::timeCoeff (8.0f, osRate);
    const float holdLen = (float) (0.02 * osRate);

    juce::dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(), (size_t) numCh, (size_t) n);
    auto up = oversampler.processSamplesUp (block);
    const int N = (int) up.getNumSamples();
    float* ch[2] = { up.getChannelPointer (0), numCh > 1 ? up.getChannelPointer (1) : nullptr };

    constexpr float satA = 0.24f;
    float maxGr = 0.0f, maxGate = 0.0f, peakOut = 0.0f;

    auto eq = [&] (int c, float x)
    {
        if (! eqIn) return x;
        x = lf[c].process (x);
        if (lfDipOn) x = lfDip[c].process (x);
        x = lmf[c].process (x);
        x = hmf[c].process (x);
        x = hf[c].process (x);
        if (hfDipOn) x = hfDip[c].process (x);
        return x;
    };

    for (int i = 0; i < N; ++i)
    {
        const float dg = drive.getNextValue();
        const float k = satA * dg;
        float v[2] = { 0.0f, 0.0f };

        for (int c = 0; c < numCh; ++c)
        {
            float x = ch[c][i];
            if (phase) x = -x;

            // console preamp/line stage: clean at low level, rounds off and adds odd + a touch of even harmonics when driven
            const float s = std::tanh (k * x);
            x = (s + 0.045f * s * s) / k;
            x = dc[c].process (x);

            if (hpfOn) { x = hpf2[c].process (x); x = x - hpf1[c].process (x); }
            if (lpfOn) x = lpf2[c].process (x);
            if (! dynPre) x = eq (c, x);
            v[c] = x;
        }

        // linked dynamics
        float dynGain = 1.0f;
        if (compOn || gateOn)
        {
            float key = std::abs (v[0]);
            if (numCh > 1) key = std::max (key, std::abs (v[1]));

            if (compOn)
            {
                compEnv = detC * compEnv + (1.0f - detC) * key * key;
                const float lvl = ecee::gainToDb (std::sqrt (compEnv) * (cFast ? 1.0f : 1.4142f));
                const float t = ecee::computeGR (lvl, cThr, effRatio, knee);
                compGr = t > compGr ? cAtk * compGr + (1.0f - cAtk) * t : cRel * compGr + (1.0f - cRel) * t;
                dynGain *= ecee::dbToGain (-compGr);
                maxGr = std::max (maxGr, compGr);
            }

            if (gateOn)
            {
                gateEnv = std::max (key, gDet * gateEnv);
                const float lvl = ecee::gainToDb (gateEnv);
                float targetDb;
                if (hardGate)
                {
                    if (lvl > gThr)                { gateOpen = true; gateHold = holdLen; }
                    else if (lvl < gThr - 4.0f)    { if (gateHold > 0.0f) gateHold -= 1.0f; else gateOpen = false; }
                    targetDb = gateOpen ? 0.0f : -gRange;
                }
                else
                {
                    targetDb = -std::min (gRange, std::max (0.0f, gThr - lvl));   // 1:2 downward expansion
                }
                const float gDb = ecee::gainToDb (gateGain);
                const float coeff = targetDb > gDb ? gAtk : gRel;
                gateGain = ecee::dbToGain (coeff * gDb + (1.0f - coeff) * targetDb);
                dynGain *= gateGain;
                maxGate = std::max (maxGate, -ecee::gainToDb (gateGain));
            }
        }
        else
        {
            compGr = 0.0f; gateGain = 1.0f;
        }

        const float m  = mix.getNextValue();
        const float mk = makeup.getNextValue();
        const float og = outGain.getNextValue();
        for (int c = 0; c < numCh; ++c)
        {
            float x = v[c];
            const float wet = x * dynGain * mk;
            x = wet * m + x * (1.0f - m);
            if (dynPre) x = eq (c, x);
            x *= og;
            ch[c][i] = x;
            peakOut = std::max (peakOut, std::abs (x));
        }
    }

    oversampler.processSamplesDown (block);

    meterGr.store (maxGr);
    meterGate.store (maxGate);
    meterOut.store (ecee::gainToDb (peakOut));
}

juce::AudioProcessorEditor* Channel4KProcessor::createEditor() { return new Channel4KEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Channel4KProcessor(); }
