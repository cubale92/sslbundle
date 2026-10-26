#pragma once
#include "PluginProcessor.h"
#include "EceeConsoleUI.h"

class Bus4KPanel : public juce::Component
{
public:
    explicit Bus4KPanel (Bus4KProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int W = 1120, H = 360;

private:
    ecee::ui::Knob threshold, ratio, attack, release, makeup, hpf, mixKnob, drive;
    ecee::ui::LitKey compIn;
    ecee::ui::VUMeter vu;
    ecee::ui::LedLadder ladder;
    ecee::ui::PresetBar presetBar;
};

class Bus4KEditor : public ecee::ui::ScaledEditor
{
public:
    explicit Bus4KEditor (Bus4KProcessor& p) : ScaledEditor (p), panel (p) { initScaling (panel, Bus4KPanel::W, Bus4KPanel::H); }

private:
    Bus4KPanel panel;
};
