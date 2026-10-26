#include "PluginEditor.h"

using namespace ecee::ui;
namespace C = ecee::ui::console;

Listen4KPanel::Listen4KPanel (Listen4KProcessor& p)
    : input   (p.state, "input",   "INPUT",   { "0", "", "10", "", "20", "", "30" }, "console"),
      release (p.state, "release", "RELEASE", { "30", "", "220", "", "1.2k" }, "console"),
      tone    (p.state, "tone",    "TONE",    { "FULL", "TALK", "DARK" }, "console"),
      crunch  (p.state, "crunch",  "CRUNCH",  { "0", "", "50", "", "100" }, "console"),
      mixKnob (p.state, "mix",     "MIX",     { "DRY", "", "", "", "WET" }, "console"),
      output  (p.state, "output",  "OUTPUT",  { "-24", "", "-6", "", "+12" }, "console"),
      vu ([&p] { return p.meterGr.load(); }, VUMeter::Mode::gainReduction, "LISTEN GR"),
      presetBar (p)
{
    struct K { Knob* k; juce::Colour cap; };
    const K knobs[] = { { &input, C::kRed }, { &release, C::kCream }, { &tone, C::kBlue },
                        { &crunch, C::kOrange }, { &mixKnob, C::kGrey }, { &output, C::kGrey } };
    for (auto& [k, cap] : knobs)
    {
        k->setColours (C::kInk, C::kScale);
        k->setCapColour (cap);
        k->setPopupParent (this);
        addAndMakeVisible (*k);
    }
    addAndMakeVisible (vu);
    addAndMakeVisible (presetBar);
}

void Listen4KPanel::paint (juce::Graphics& g)
{
    C::drawDeskPanel (g, getLocalBounds().toFloat(), "LISTEN-4K", "TALKBACK COMPRESSOR");
    C::drawSection (g, { 56.0f, 82.0f, 240.0f, 236.0f }, "METER", C::kGrey);
    C::drawSection (g, { 308.0f, 82.0f, 536.0f, 236.0f }, "LISTEN MIC COMPRESSOR", C::kRed);
    C::drawSection (g, { 856.0f, 82.0f, 128.0f, 236.0f }, "OUTPUT", C::kGrey);

    // the circuit's fixed specs, printed like a desk legend
    panelText (g, "FIXED THRESHOLD  /  30:1  /  FAST ATTACK", { 56.0f, 286.0f, 240.0f, 16.0f },
               C::kInkDim, font (9.0f, true, 0.12f));
}

void Listen4KPanel::resized()
{
    presetBar.setBounds (470, 14, 300, 28);

    vu.setBounds (68, 100, 216, 170);

    input.setBounds   (316, 96, 150, 184);
    release.setBounds (474, 108, 118, 140);
    tone.setBounds    (596, 108, 118, 140);
    crunch.setBounds  (718, 108, 118, 140);

    mixKnob.setBounds (864, 94, 112, 106);
    output.setBounds  (864, 206, 112, 106);
}
