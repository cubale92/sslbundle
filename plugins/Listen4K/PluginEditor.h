#pragma once
#include "PluginProcessor.h"
#include "EceeConsoleUI.h"

class Listen4KPanel : public juce::Component
{
public:
    explicit Listen4KPanel (Listen4KProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int W = 1040, H = 340;

private:
    ecee::ui::Knob input, release, tone, crunch, mixKnob, output;
    ecee::ui::VUMeter vu;
    ecee::ui::PresetBar presetBar;
};

class Listen4KEditor : public ecee::ui::ScaledEditor
{
public:
    explicit Listen4KEditor (Listen4KProcessor& p) : ScaledEditor (p), panel (p) { initScaling (panel, Listen4KPanel::W, Listen4KPanel::H); }

private:
    Listen4KPanel panel;
};
