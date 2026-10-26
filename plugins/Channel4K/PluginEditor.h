#pragma once
#include "PluginProcessor.h"
#include "EceeConsoleUI.h"

class Channel4KPanel : public juce::Component
{
public:
    explicit Channel4KPanel (Channel4KProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int W = 1320, H = 440;

private:
    ecee::ui::Knob drive, hpf, lpf,
                   hfG, hfF, hmfG, hmfF, hmfQ, lmfG, lmfF, lmfQ, lfG, lfF,
                   cRatio, cThresh, cRel, gRange, gThresh, gRel, mixKnob, output;
    ecee::ui::LitKey phase, hfBell, lfBell, cFast, gMode, gFast, eqIn, dynIn, dynPre, eqType;
    ecee::ui::VUMeter vu;
    ecee::ui::ActivityLed gateLed;
    ecee::ui::PresetBar presetBar;
    juce::ParameterAttachment eqTypeWatch;
};

class Channel4KEditor : public ecee::ui::ScaledEditor
{
public:
    explicit Channel4KEditor (Channel4KProcessor& p) : ScaledEditor (p), panel (p) { initScaling (panel, Channel4KPanel::W, Channel4KPanel::H); }

private:
    Channel4KPanel panel;
};
