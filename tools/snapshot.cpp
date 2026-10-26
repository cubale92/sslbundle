// Dev tool: renders a plug-in's editor to PNG and prints a quick DSP report.
// Usage: <tool> out.png
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "EceeProcessorBase.h"
#include <cmath>
#include <iostream>

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

namespace
{
constexpr double kFs = 48000.0;
constexpr int kBlock = 512;

double goertzel (const float* x, int n, double freq)
{
    const double w = 2.0 * juce::MathConstants<double>::pi * freq / kFs, c = 2.0 * std::cos (w);
    double s1 = 0, s2 = 0;
    for (int i = 0; i < n; ++i)
    {
        // Hann window
        const double win = 0.5 - 0.5 * std::cos (2.0 * juce::MathConstants<double>::pi * i / (n - 1));
        const double s = x[i] * win + c * s1 - s2;
        s2 = s1; s1 = s;
    }
    return std::sqrt (s1 * s1 + s2 * s2 - c * s1 * s2) * 4.0 / n; // amplitude (Hann gain 0.5)
}

struct Result { double outDb, thd; bool finite; };

Result runSine (juce::AudioProcessor& p, double freq, float levelDb, double seconds = 1.5)
{
    p.reset();
    p.prepareToPlay (kFs, kBlock);
    const int total = (int) (seconds * kFs);
    const int analyse = 16384;
    std::vector<float> tail ((size_t) total);
    juce::AudioBuffer<float> buf (2, kBlock);
    juce::MidiBuffer midi;
    const float amp = std::pow (10.0f, levelDb / 20.0f);
    bool finite = true;
    for (int pos = 0; pos < total; pos += kBlock)
    {
        const int n = std::min (kBlock, total - pos);
        buf.setSize (2, n, false, false, true);
        for (int i = 0; i < n; ++i)
        {
            const float v = amp * (float) std::sin (2.0 * juce::MathConstants<double>::pi * freq * (pos + i) / kFs);
            buf.setSample (0, i, v); buf.setSample (1, i, v);
        }
        p.processBlock (buf, midi);
        for (int i = 0; i < n; ++i)
        {
            const float v = buf.getSample (0, i);
            if (! std::isfinite (v)) finite = false;
            tail[(size_t) (pos + i)] = v;
        }
    }
    const float* x = tail.data() + total - analyse;
    const double f1 = goertzel (x, analyse, freq);
    double h = 0;
    for (int k = 2; k <= 7; ++k)
        if (freq * k < kFs * 0.45) { const double a = goertzel (x, analyse, freq * k); h += a * a; }
    return { 20.0 * std::log10 (std::max (f1, 1e-12)), 100.0 * std::sqrt (h) / std::max (f1, 1e-12), finite };
}

void setParam (juce::AudioProcessor& p, const juce::String& id, float value)
{
    for (auto* prm : p.getParameters())
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (prm))
            if (r->getParameterID() == id)
                r->setValueNotifyingHost (r->convertTo0to1 (value));
}
} // namespace

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    std::unique_ptr<juce::AudioProcessor> p (createPluginFilter());
    p->setPlayConfigDetails (2, 2, kFs, kBlock);
    std::cout << "== " << p->getName() << "  latency " << p->getLatencySamples() << " samples\n";

    const juce::String name = p->getName();
    auto report = [&] (const juce::String& label)
    {
        std::cout << label << "\n";
        for (float lvl : { -30.0f, -18.0f, -10.0f, -6.0f, 0.0f, 6.0f })
        {
            auto r = runSine (*p, 1000.0, lvl);
            std::cout << "  in " << juce::String (lvl, 1).paddedLeft (' ', 6) << " dBFS -> out "
                      << juce::String (r.outDb, 2).paddedLeft (' ', 7) << " dBFS  (gain " << juce::String (r.outDb - lvl, 2)
                      << " dB)  THD " << juce::String (r.thd, 3) << " %" << (r.finite ? "" : "  NaN!") << "\n";
        }
        auto lf = runSine (*p, 60.0, -18.0f), hf = runSine (*p, 12000.0, -18.0f), ref = runSine (*p, 1000.0, -18.0f);
        std::cout << "  response @-18: 60Hz " << juce::String (lf.outDb - ref.outDb, 2) << " dB, 12kHz "
                  << juce::String (hf.outDb - ref.outDb, 2) << " dB (re 1k)\n";
    };

    report ("defaults");
    if (name.contains ("FET"))
    {
        setParam (*p, "input", 20.0f);  setParam (*p, "output", -12.0f); report ("input +20, output -12");
        setParam (*p, "ratio", 4.0f);   report ("ALL buttons");
        setParam (*p, "ratio", 0.0f);   setParam (*p, "input", 0.0f); setParam (*p, "output", 0.0f);
    }
    else if (name.contains ("Channel"))
    {
        setParam (*p, "drive", 18.0f); report ("drive +18");
        setParam (*p, "drive", 0.0f);
        setParam (*p, "lfg", 10.0f); setParam (*p, "lff", 60.0f); report ("LF shelf +10 @60 (black)");
        setParam (*p, "eqtype", 0.0f); report ("LF shelf +10 @60 (brown)");
        setParam (*p, "lfg", 0.0f); setParam (*p, "eqtype", 1.0f);
        setParam (*p, "cratio", 4.0f); setParam (*p, "cthresh", -24.0f); report ("comp 4:1 @ -24");
        setParam (*p, "cratio", 1.0f); setParam (*p, "cthresh", 10.0f);
        setParam (*p, "grange", 30.0f); setParam (*p, "gthresh", -20.0f); setParam (*p, "gmode", 1.0f); report ("gate -20, range 30");
        setParam (*p, "grange", 0.0f); setParam (*p, "gmode", 0.0f);
        setParam (*p, "hpf", 120.0f); setParam (*p, "lpf", 6000.0f); report ("HPF 120 / LPF 6k");
        setParam (*p, "hpf", 16.0f); setParam (*p, "lpf", 22000.0f);
    }
    else if (name.contains ("Bus4K"))
    {
        setParam (*p, "threshold", -20.0f); setParam (*p, "makeup", 6.0f); report ("thr -20, makeup +6");
        setParam (*p, "drive", 100.0f); report ("console drive 100 %");
        setParam (*p, "threshold", 10.0f); setParam (*p, "makeup", 0.0f); setParam (*p, "drive", 0.0f);
    }
    else if (name.contains ("Listen"))
    {
        setParam (*p, "input", 24.0f); report ("input +24");
        setParam (*p, "crunch", 100.0f); report ("input +24, crunch 100");
        setParam (*p, "input", 10.0f); setParam (*p, "crunch", 30.0f);
    }
    else if (name.contains ("Bus"))
    {
        setParam (*p, "threshold", -20.0f); setParam (*p, "makeup", 6.0f); report ("thr -20, makeup +6");
        setParam (*p, "color", 0.0f); report ("clean");
        setParam (*p, "threshold", -10.0f); setParam (*p, "makeup", 0.0f); setParam (*p, "color", 1.0f);
    }
    else
    {
        setParam (*p, "input", 12.0f); report ("input +12 (auto gain)");
        setParam (*p, "speed", 0.0f); setParam (*p, "formula", 0.0f); report ("7.5 ips ferric, input +12");
        setParam (*p, "input", 0.0f); setParam (*p, "speed", 1.0f); setParam (*p, "formula", 1.0f);
    }

    // presets: every parameter name must exist, and each preset must produce sane audio
    if (auto* base = dynamic_cast<ecee::ProcessorBase*> (p.get()))
    {
        std::cout << "presets: " << base->getNumPrograms() << ", unknown parameter names: " << base->countUnknownPresetParameters() << "\n";
        juce::AudioBuffer<float> buf (2, kBlock);
        juce::MidiBuffer midi;
        for (int i = 0; i < base->getNumPrograms(); ++i)
        {
            base->applyPreset (i);
            p->reset();
            p->prepareToPlay (kFs, kBlock);
            juce::Random rnd (3);
            double inSq = 0, outSq = 0; float peak = 0; bool finite = true;
            int t = 0;
            for (int b = 0; b < 375; ++b)   // 4 s of a drum-ish loop at roughly -14 dBFS RMS
            {
                for (int k = 0; k < kBlock; ++k, ++t)
                {
                    const float envK = std::exp (-(float) (t % 12000) / 2500.0f);
                    const float v = 0.45f * envK * (float) std::sin (2.0 * juce::MathConstants<double>::pi * 60.0 * t / kFs)
                                  + 0.25f * (rnd.nextFloat() * 2.0f - 1.0f) * std::exp (-(float) ((t + 6000) % 12000) / 900.0f)
                                  + 0.08f * (float) std::sin (2.0 * juce::MathConstants<double>::pi * 330.0 * t / kFs);
                    buf.setSample (0, k, v); buf.setSample (1, k, v * 0.95f);
                    if (b >= 125) inSq += (double) v * v;
                }
                p->processBlock (buf, midi);
                if (b >= 125)
                    for (int k = 0; k < kBlock; ++k)
                    {
                        const float o = buf.getSample (0, k);
                        if (! std::isfinite (o)) finite = false;
                        outSq += (double) o * o;
                        peak = std::max (peak, std::abs (o));
                    }
            }
            const double change = 10.0 * std::log10 (std::max (outSq, 1e-20) / std::max (inSq, 1e-20));
            std::cout << "  " << juce::String (i + 1).paddedLeft ('0', 2) << " " << base->getProgramName (i).paddedRight (' ', 24)
                      << " level change " << juce::String (change, 1).paddedLeft (' ', 6) << " dB   peak "
                      << juce::String (20.0 * std::log10 (std::max (peak, 1e-9f)), 1).paddedLeft (' ', 6) << " dBFS"
                      << (finite ? "" : "   NaN!") << (base->isPresetModified() ? "   (modified?!)" : "") << "\n";
        }
        base->applyPreset (std::min (2, base->getNumPrograms() - 1)); // show a named preset in the snapshot
    }

    // feed something drum-like so the meters have something to show, then snapshot the editor
    if (argc > 1)
    {
        p->prepareToPlay (kFs, kBlock);
        std::unique_ptr<juce::AudioProcessorEditor> ed (p->createEditor());
        juce::AudioBuffer<float> buf (2, kBlock);
        juce::MidiBuffer midi;
        juce::Random rnd (1);
        int t = 0;
        for (int it = 0; it < 60; ++it)
        {
            for (int b = 0; b < 4; ++b)
            {
                for (int i = 0; i < kBlock; ++i, ++t)
                {
                    const float envK = std::exp (-(float) (t % 12000) / 2500.0f);
                    const float v = 0.7f * envK * (float) std::sin (2.0 * juce::MathConstants<double>::pi * 70.0 * t / kFs)
                                  + 0.15f * (rnd.nextFloat() * 2.0f - 1.0f) * std::exp (-(float) ((t + 6000) % 12000) / 900.0f)
                                  + 0.12f * (float) std::sin (2.0 * juce::MathConstants<double>::pi * 220.0 * t / kFs);
                    buf.setSample (0, i, v); buf.setSample (1, i, v * 0.9f);
                }
                p->processBlock (buf, midi);
            }
            juce::MessageManager::getInstance()->runDispatchLoopUntil (20);
        }
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.5f);
        const juce::File f { juce::String (argv[1]) };
        f.deleteFile();
        juce::FileOutputStream out (f);
        juce::PNGImageFormat().writeImageToStream (img, out);
        std::cout << "snapshot -> " << f.getFullPathName() << "\n";
        ed.reset();
    }
    p.reset();
    return 0;
}
