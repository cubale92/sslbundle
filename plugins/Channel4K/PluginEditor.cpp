#include "PluginEditor.h"

using namespace ecee::ui;
namespace C = ecee::ui::console;

namespace
{
const juce::Colour kLedRed   { 0xffff3b2f };
const juce::Colour kLedGreen { 0xff4dff6a };
const juce::Colour kLedAmber { 0xffffb03a };
}

Channel4KPanel::Channel4KPanel (Channel4KProcessor& p)
    : drive   (p.state, "drive",  "DRIVE",  { "0", "", "12", "", "24" }, "console"),
      hpf     (p.state, "hpf",    "HPF",    { "OFF", "", "", "", "400" }, "console"),
      lpf     (p.state, "lpf",    "LPF",    { "3k", "", "", "", "OFF" }, "console"),
      hfG     (p.state, "hfg",    "HF",     { "-16", "-8", "0", "+8", "+16" }, "console"),
      hfF     (p.state, "hff",    "kHz",    { "1.5", "", "", "", "16" }, "console"),
      hmfG    (p.state, "hmfg",   "HMF",    { "-16", "-8", "0", "+8", "+16" }, "console"),
      hmfF    (p.state, "hmff",   "kHz",    { ".6", "", "", "", "7" }, "console"),
      hmfQ    (p.state, "hmfq",   "Q",      { ".4", "", "", "", "4" }, "console"),
      lmfG    (p.state, "lmfg",   "LMF",    { "-16", "-8", "0", "+8", "+16" }, "console"),
      lmfF    (p.state, "lmff",   "kHz",    { ".2", "", "", "", "2.5" }, "console"),
      lmfQ    (p.state, "lmfq",   "Q",      { ".4", "", "", "", "4" }, "console"),
      lfG     (p.state, "lfg",    "LF",     { "-16", "-8", "0", "+8", "+16" }, "console"),
      lfF     (p.state, "lff",    "Hz",     { "30", "", "", "", "450" }, "console"),
      cRatio  (p.state, "cratio", "RATIO",  { "1", "", "4", "", "LIM" }, "console"),
      cThresh (p.state, "cthresh","THRESH", { "-40", "", "", "", "+10" }, "console"),
      cRel    (p.state, "crel",   "RELEASE",{ ".05", "", ".4", "", "4" }, "console"),
      gRange  (p.state, "grange", "RANGE",  { "0", "", "20", "", "40" }, "console"),
      gThresh (p.state, "gthresh","THRESH", { "-70", "", "", "", "0" }, "console"),
      gRel    (p.state, "grel",   "RELEASE",{ ".05", "", ".4", "", "4" }, "console"),
      mixKnob (p.state, "mix",    "DYN MIX",{ "DRY", "", "", "", "WET" }, "console"),
      output  (p.state, "output", "OUTPUT", { "-18", "", "0", "", "+18" }, "console"),
      phase   (*p.state.getParameter ("phase"),  "PHASE",   kLedRed),
      hfBell  (*p.state.getParameter ("hfbell"), "BELL",    kLedAmber),
      lfBell  (*p.state.getParameter ("lfbell"), "BELL",    kLedAmber),
      cFast   (*p.state.getParameter ("cfast"),  "F.ATK",   kLedAmber),
      gMode   (*p.state.getParameter ("gmode"),  "GATE",    kLedGreen),
      gFast   (*p.state.getParameter ("gfast"),  "F.ATK",   kLedAmber),
      eqIn    (*p.state.getParameter ("eqin"),   "EQ IN",   kLedGreen),
      dynIn   (*p.state.getParameter ("dynin"),  "DYN IN",  kLedGreen),
      dynPre  (*p.state.getParameter ("dynpre"), "DYN>EQ",  kLedRed),
      eqType  (*p.state.getParameter ("eqtype"), "BLACK",   kLedAmber),
      vu ([&p] { return p.meterGr.load(); }, VUMeter::Mode::gainReduction, "COMP GR"),
      gateLed ([&p] { return p.meterGate.load() > 2.0f; }, kLedGreen, "GATING"),
      presetBar (p),
      eqTypeWatch (*p.state.getParameter ("eqtype"), [this] (float v)
                   {
                       const auto cap = v > 0.5f ? C::kBlack : C::kBrown;
                       lfG.setCapColour (cap);
                       lfF.setCapColour (cap);
                   }, nullptr)
{
    struct K { Knob* k; juce::Colour cap; };
    const K knobs[] = {
        { &drive, C::kCream }, { &hpf, C::kGrey }, { &lpf, C::kGrey },
        { &hfG, C::kRed }, { &hfF, C::kRed }, { &hmfG, C::kGreen }, { &hmfF, C::kGreen }, { &hmfQ, C::kGreen },
        { &lmfG, C::kBlue }, { &lmfF, C::kBlue }, { &lmfQ, C::kBlue }, { &lfG, C::kBlack }, { &lfF, C::kBlack },
        { &cRatio, C::kCream }, { &cThresh, C::kCream }, { &cRel, C::kCream },
        { &gRange, C::kGreen }, { &gThresh, C::kGreen }, { &gRel, C::kGreen },
        { &mixKnob, C::kGrey }, { &output, C::kRed },
    };
    for (auto& [k, cap] : knobs)
    {
        k->setColours (C::kInk, C::kScale);
        k->setCapColour (cap);
        k->setTitleSize (11.0f);
        k->setPopupParent (this);
        addAndMakeVisible (*k);
    }
    for (auto* k : { &hfF, &hmfF, &hmfQ, &lmfF, &lmfQ, &lfF })
        k->setKnobScale (0.9f);

    for (auto* key : { &phase, &hfBell, &lfBell, &cFast, &gMode, &gFast, &eqIn, &dynIn, &dynPre, &eqType })
        addAndMakeVisible (*key);

    addAndMakeVisible (vu);
    addAndMakeVisible (gateLed);
    addAndMakeVisible (presetBar);
    eqTypeWatch.sendInitialUpdate();
}

void Channel4KPanel::paint (juce::Graphics& g)
{
    C::drawDeskPanel (g, getLocalBounds().toFloat(), "CHANNEL-4K", "E / G SERIES CHANNEL STRIP");

    C::drawSection (g, { 56.0f, 82.0f, 190.0f, 150.0f }, "INPUT", C::kCream);
    C::drawSection (g, { 56.0f, 250.0f, 190.0f, 168.0f }, "FILTERS", C::kGrey);
    C::drawSection (g, { 258.0f, 82.0f, 480.0f, 336.0f }, "EQUALISER", C::kRed);
    C::drawSection (g, { 750.0f, 82.0f, 330.0f, 160.0f }, "COMPRESSOR / LIMITER", C::kCream);
    C::drawSection (g, { 750.0f, 258.0f, 330.0f, 160.0f }, "EXPANDER / GATE", C::kGreen);
    C::drawSection (g, { 1092.0f, 82.0f, 172.0f, 336.0f }, "OUTPUT", C::kRed);

    // thin column rules inside the EQ
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    for (int i = 1; i < 4; ++i)
        g.fillRect (258.0f + 120.0f * (float) i, 96.0f, 1.0f, 308.0f);
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    for (int i = 1; i < 4; ++i)
        g.fillRect (259.0f + 120.0f * (float) i, 96.0f, 1.0f, 308.0f);
}

void Channel4KPanel::resized()
{
    presetBar.setBounds (610, 14, 300, 28);

    drive.setBounds (62, 100, 100, 118);
    phase.setBounds (166, 122, 72, 66);
    hpf.setBounds (60, 274, 92, 112);
    lpf.setBounds (150, 274, 92, 112);

    auto col = [] (int i) { return 258 + 120 * i; };
    hfG.setBounds  (col (0) + 8, 92, 104, 112);   hfF.setBounds  (col (0) + 14, 206, 92, 102);  hfBell.setBounds (col (0) + 26, 330, 68, 62);
    hmfG.setBounds (col (1) + 8, 92, 104, 112);   hmfF.setBounds (col (1) + 14, 206, 92, 102);  hmfQ.setBounds (col (1) + 14, 308, 92, 102);
    lmfG.setBounds (col (2) + 8, 92, 104, 112);   lmfF.setBounds (col (2) + 14, 206, 92, 102);  lmfQ.setBounds (col (2) + 14, 308, 92, 102);
    lfG.setBounds  (col (3) + 8, 92, 104, 112);   lfF.setBounds  (col (3) + 14, 206, 92, 102);  lfBell.setBounds (col (3) + 26, 330, 68, 62);

    cRatio.setBounds  (754, 104, 84, 106);
    cThresh.setBounds (836, 104, 84, 106);
    cRel.setBounds    (918, 104, 84, 106);
    cFast.setBounds   (1002, 128, 70, 64);

    gRange.setBounds  (754, 276, 84, 106);
    gThresh.setBounds (836, 276, 84, 106);
    gRel.setBounds    (918, 276, 84, 106);
    gMode.setBounds   (1002, 270, 70, 64);
    gFast.setBounds   (1002, 340, 70, 64);
    gateLed.setBounds (764, 398, 90, 16);

    vu.setBounds (1100, 94, 156, 104);
    eqIn.setBounds   (1100, 206, 76, 54);
    dynIn.setBounds  (1180, 206, 76, 54);
    eqType.setBounds (1100, 264, 76, 54);
    dynPre.setBounds (1180, 264, 76, 54);
    mixKnob.setBounds (1096, 322, 82, 94);
    output.setBounds  (1178, 322, 82, 94);
}
