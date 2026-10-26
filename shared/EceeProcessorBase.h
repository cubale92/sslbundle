#pragma once
// 8cee — boilerplate shared by every processor, including the factory preset system
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

namespace ecee
{
/** A factory preset: a name plus parameter values in real units (dB, ms, %, choice index, 0/1).
    Parameters a preset doesn't mention are set to their defaults. */
struct Preset
{
    juce::String name;
    std::vector<std::pair<juce::String, float>> values;
};

class ProcessorBase : public juce::AudioProcessor
{
public:
    ProcessorBase()
        : juce::AudioProcessor (BusesProperties()
                                    .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                    .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
    {}

    using juce::AudioProcessor::processBlock;
    virtual juce::AudioProcessorValueTreeState& apvts() = 0;

    const juce::String getName() const override        { return JucePlugin_Name; }
    bool acceptsMidi() const override                  { return false; }
    bool producesMidi() const override                 { return false; }
    bool isMidiEffect() const override                 { return false; }
    double getTailLengthSeconds() const override       { return 0.0; }
    bool hasEditor() const override                    { return true; }
    void releaseResources() override                   {}

    //==============================================================================
    // Presets (exposed to the host as programs, and to the editor's preset bar)
    int getNumPrograms() override                      { return std::max (1, (int) presets.size()); }
    int getCurrentProgram() override                   { return currentPreset.load(); }
    void setCurrentProgram (int index) override        { applyPreset (index); }
    void changeProgramName (int, const juce::String&) override {}

    const juce::String getProgramName (int index) override
    {
        return juce::isPositiveAndBelow (index, (int) presets.size()) ? presets[(size_t) index].name : juce::String ("Default");
    }

    const std::vector<Preset>& getPresets() const      { return presets; }

    void applyPreset (int index)
    {
        if (! juce::isPositiveAndBelow (index, (int) presets.size()))
            return;

        const auto& preset = presets[(size_t) index];
        for (auto* p : getParameters())
        {
            if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (p))
            {
                const float target = targetFor (*r, preset);
                if (std::abs (r->getValue() - target) > 1.0e-6f)
                {
                    r->beginChangeGesture();
                    r->setValueNotifyingHost (target);
                    r->endChangeGesture();
                }
            }
        }
        currentPreset = index;
        apvts().state.setProperty ("preset", index, nullptr);
    }

    /** True when any parameter differs from the current preset (the UI shows a '*'). */
    bool isPresetModified()
    {
        const int index = currentPreset.load();
        if (! juce::isPositiveAndBelow (index, (int) presets.size()))
            return false;

        for (auto* p : getParameters())
            if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (p))
                if (std::abs (r->getValue() - targetFor (*r, presets[(size_t) index])) > 0.002f)
                    return true;
        return false;
    }

    /** Number of preset entries that name a parameter that doesn't exist (should be 0; checked by the test tool). */
    int countUnknownPresetParameters() const
    {
        int unknown = 0;
        for (const auto& preset : presets)
            for (const auto& [id, value] : preset.values)
                if (apvtsConst().getParameter (id) == nullptr)
                    ++unknown;
        return unknown;
    }

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& dest) override
    {
        if (auto xml = apvts().copyState().createXml())
            copyXmlToBinary (*xml, dest);
    }

    void setStateInformation (const void* data, int size) override
    {
        if (auto xml = getXmlFromBinary (data, size))
            if (xml->hasTagName (apvts().state.getType()))
            {
                apvts().replaceState (juce::ValueTree::fromXml (*xml));
                const int index = apvts().state.getProperty ("preset", 0);
                currentPreset = juce::isPositiveAndBelow (index, (int) presets.size()) ? index : 0;
            }
    }

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override
    {
        const auto out = layouts.getMainOutputChannelSet();
        if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
            return false;
        return out == layouts.getMainInputChannelSet();
    }

    /** Counts processed samples, so the UI can tell whether audio is running (tape reels). */
    std::atomic<juce::int64> samplesProcessed { 0 };

protected:
    static float raw (std::atomic<float>* p) noexcept { return p->load (std::memory_order_relaxed); }

    /** Call from the derived constructor, after the parameter state exists. */
    void setPresets (std::vector<Preset> list)
    {
        presets = std::move (list);
        jassert (countUnknownPresetParameters() == 0);
    }

private:
    juce::AudioProcessorValueTreeState& apvtsConst() const { return const_cast<ProcessorBase*> (this)->apvts(); }

    static float targetFor (juce::RangedAudioParameter& r, const Preset& preset)
    {
        for (const auto& [id, value] : preset.values)
            if (id == r.getParameterID())
                return r.convertTo0to1 (value);
        return r.getDefaultValue();
    }

    std::vector<Preset> presets;
    std::atomic<int> currentPreset { 0 };
};
} // namespace ecee
