#include "PluginEditor.h"

using namespace ecee::ui;
namespace C = ecee::ui::console;

Bus4KPanel::Bus4KPanel (Bus4KProcessor& p)
    : threshold (p.state, "threshold", "THRESHOLD", { "-30", "-20", "-10", "0", "+10" }, "console"),
      ratio     (p.state, "ratio",     "RATIO",     Bus4KProcessor::ratioNames(),   "console"),
      attack    (p.state, "attack",    "ATTACK ms", Bus4KProcessor::attackNames(),  "console"),
      release   (p.state, "release",   "RELEASE s", Bus4KProcessor::releaseNames(), "console"),
      makeup    (p.state, "makeup",    "MAKE-UP",   { "0", "5", "10", "15", "20" }, "console"),
      hpf       (p.state, "schpf",     "S/C HPF",   Bus4KProcessor::hpfNames(),     "console"),
      mixKnob   (p.state, "mix",       "MIX",       { "DRY", "", "", "", "WET" }, "console"),
      drive     (p.state, "drive",     "DRIVE",     { "0", "", "50", "", "100" }, "console"),
      compIn    (*p.state.getParameter ("compin"), "COMP IN", juce::Colour (0xff4dff6a)),
      vu ([&p] { return p.meterGr.load(); }, VUMeter::Mode::gainReduction, "COMPRESSION"),
      ladder ([&p] { return p.meterGr.load(); }, { 1.0f, 2.0f, 3.0f, 4.0f, 6.0f, 8.0f, 10.0f, 14.0f, 20.0f }, "GR dB"),
      presetBar (p)
{
    struct K { Knob* k; juce::Colour cap; };
    const K knobs[] = { { &threshold, C::kRed }, { &ratio, C::kCream }, { &attack, C::kCream }, { &release, C::kCream },
                        { &makeup, C::kGrey }, { &hpf, C::kBlue }, { &mixKnob, C::kGrey }, { &drive, C::kOrange } };
    for (auto& [k, cap] : knobs)
    {
        k->setColours (C::kInk, C::kScale);
        k->setCapColour (cap);
        k->setPopupParent (this);
        addAndMakeVisible (*k);
    }
    addAndMakeVisible (compIn);
    addAndMakeVisible (vu);
    addAndMakeVisible (ladder);
    addAndMakeVisible (presetBar);
}

void Bus4KPanel::paint (juce::Graphics& g)
{
    C::drawDeskPanel (g, getLocalBounds().toFloat(), "BUS-4K", "CENTRE SECTION BUS COMPRESSOR");
    C::drawSection (g, { 56.0f, 82.0f, 280.0f, 256.0f }, "METER", C::kGrey);
    C::drawSection (g, { 348.0f, 82.0f, 520.0f, 256.0f }, "BUS COMPRESSOR", C::kRed);
    C::drawSection (g, { 880.0f, 82.0f, 184.0f, 256.0f }, "CONSOLE", C::kOrange);
    panelText (g, "SUMMING DRIVE", { 880.0f, 300.0f, 184.0f, 16.0f }, C::kInkDim, font (10.0f, true, 0.25f));
}

void Bus4KPanel::resized()
{
    presetBar.setBounds (520, 14, 300, 28);

    vu.setBounds (68, 100, 200, 140);
    ladder.setBounds (276, 98, 50, 230);
    compIn.setBounds (128, 258, 80, 62);

    threshold.setBounds (356, 98, 130, 148);
    ratio.setBounds     (490, 102, 92, 112);
    attack.setBounds    (584, 102, 92, 112);
    release.setBounds   (678, 102, 92, 112);
    makeup.setBounds    (772, 98, 92, 120);
    hpf.setBounds       (536, 222, 92, 108);
    mixKnob.setBounds   (650, 222, 92, 108);

    drive.setBounds (900, 104, 144, 176);
}
