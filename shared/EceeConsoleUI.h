#pragma once
// 8cee Console 4K — console-desk widgets shared by Channel-4K, Bus-4K and Listen-4K:
// illuminated push keys, LED ladders, activity LEDs and the slate desk panel.
#include "EceeUI.h"

namespace ecee::ui
{
namespace console
{
const juce::Colour kInk    { 0xfff1efe8 };   // white silkscreen
const juce::Colour kInkDim { 0xffaeb0b2 };
const juce::Colour kScale  { 0xffc9cbcc };
const juce::Colour kRed    { 0xffc7362c };
const juce::Colour kGreen  { 0xff3f8f4f };
const juce::Colour kBlue   { 0xff3c6fb0 };
const juce::Colour kBrown  { 0xff7a5232 };
const juce::Colour kBlack  { 0xff1d1d1f };
const juce::Colour kGrey   { 0xff8d9094 };
const juce::Colour kCream  { 0xffe7e1d0 };
const juce::Colour kYellow { 0xffd9b43a };
const juce::Colour kOrange { 0xffd8742a };

/** Slate desk panel in a rack frame, with the 4K header and returns the usable area. */
inline juce::Rectangle<float> drawDeskPanel (juce::Graphics& g, juce::Rectangle<float> bounds, const juce::String& model,
                                             const juce::String& subtitle)
{
    auto panel = drawRackUnit (g, bounds, juce::Colour (0xff4a4e55), juce::Colour (0xff2f3237), juce::Colour (0xff5f6166), 0.02f);

    // header strip
    auto head = juce::Rectangle<float> (panel.getX(), panel.getY(), panel.getWidth(), 56.0f);
    g.setColour (juce::Colour (0xff1c1d20));
    g.fillRect (head);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.fillRect (head.getX(), head.getBottom() - 1.0f, head.getWidth(), 1.0f);

    panelText (g, model, { panel.getX() + 22.0f, 11.0f, 240.0f, 34.0f }, kInk, font (26.0f, true, 0.06f), juce::Justification::centredLeft);
    const float mw = juce::GlyphArrangement::getStringWidth (juce::Font (font (26.0f, true, 0.06f)), model);
    panelText (g, subtitle, { panel.getX() + 34.0f + mw, 11.0f, 360.0f, 34.0f }, kInkDim, font (11.5f, true, 0.32f), juce::Justification::centredLeft);
    panelText (g, "8cee", { panel.getRight() - 150.0f, 11.0f, 126.0f, 34.0f }, kInk, font (22.0f, true), juce::Justification::centredRight);

    // the colour band under the header
    const juce::Colour band[] = { kRed, kGreen, kBlue, kBrown };
    const float bw = (panel.getWidth() - 40.0f) / 4.0f;
    for (int i = 0; i < 4; ++i)
    {
        g.setColour (band[i].withAlpha (0.9f));
        g.fillRect (panel.getX() + 20.0f + bw * (float) i, 58.0f, bw - 2.0f, 2.5f);
    }
    return panel;
}

/** Section frame with a printed label tab on top. */
inline void drawSection (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& label, juce::Colour accent = kInkDim)
{
    g.setColour (juce::Colours::black.withAlpha (0.18f));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.drawRoundedRectangle (r, 6.0f, 1.2f);
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.drawRoundedRectangle (r.translated (0.0f, 1.0f), 6.0f, 1.0f);

    if (label.isNotEmpty())
    {
        const auto f = font (10.5f, true, 0.3f);
        const float w = juce::GlyphArrangement::getStringWidth (juce::Font (f), label) + 18.0f;
        auto tab = juce::Rectangle<float> (w, 15.0f).withCentre ({ r.getCentreX(), r.getY() });
        g.setColour (juce::Colour (0xff1c1d20));
        g.fillRoundedRectangle (tab, 3.0f);
        g.setColour (accent);
        g.fillRect (tab.getX() + 4.0f, tab.getBottom() - 2.0f, tab.getWidth() - 8.0f, 1.5f);
        g.setColour (kInk);
        g.setFont (f);
        g.drawText (label, tab, juce::Justification::centred, false);
    }
}
} // namespace console

//==============================================================================
/** Square illuminated push key driving a bool (or 2-choice) parameter. */
class LitKey : public juce::Component
{
public:
    LitKey (juce::RangedAudioParameter& p, const juce::String& labelText, juce::Colour ledColour = juce::Colour (0xffff4a3a),
            bool inverted = false)
        : label (labelText), led (ledColour), invert (inverted),
          attachment (p, [this] (float v) { on = (v > 0.5f) != invert; repaint(); }, nullptr)
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        attachment.sendInitialUpdate();
    }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        auto keyArea = b.removeFromTop (b.getHeight() - 15.0f);
        const float side = std::min (keyArea.getWidth(), keyArea.getHeight());
        auto key = juce::Rectangle<float> (side * 1.25f, side).withCentre (keyArea.getCentre()).reduced (2.0f);

        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.fillRoundedRectangle (key.expanded (2.5f).translated (0.0f, 1.5f), 4.0f);
        juce::ColourGradient face (juce::Colour (on ? 0xffd9d8d2 : 0xffbdbcb6), key.getX(), key.getY(),
                                   juce::Colour (on ? 0xff9a9993 : 0xff7d7c77), key.getX(), key.getBottom(), false);
        g.setGradientFill (face);
        g.fillRoundedRectangle (key, 3.0f);
        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.drawLine (key.getX() + 3.0f, key.getY() + 1.0f, key.getRight() - 3.0f, key.getY() + 1.0f, 1.0f);

        // lens
        auto lens = key.reduced (key.getWidth() * 0.3f, key.getHeight() * 0.3f);
        if (on)
        {
            juce::ColourGradient glow (led.withAlpha (0.5f), lens.getCentreX(), lens.getCentreY(), led.withAlpha (0.0f),
                                       lens.getCentreX() + lens.getWidth() * 1.6f, lens.getCentreY(), true);
            g.setGradientFill (glow);
            g.fillEllipse (lens.expanded (lens.getWidth() * 0.9f));
        }
        g.setColour (juce::Colour (0xff141414));
        g.fillRoundedRectangle (lens.expanded (1.5f), 2.5f);
        juce::ColourGradient lg ((on ? led.brighter (0.7f) : led.darker (2.4f)), lens.getX(), lens.getY(),
                                 (on ? led : led.darker (3.0f)), lens.getRight(), lens.getBottom(), false);
        g.setGradientFill (lg);
        g.fillRoundedRectangle (lens, 2.0f);

        panelText (g, label, b, console::kInk, font (10.5f, true, 0.12f));
    }

    void mouseDown (const juce::MouseEvent&) override
    {
        attachment.setValueAsCompleteGesture ((on != invert) ? 0.0f : 1.0f);
    }

private:
    juce::String label;
    juce::Colour led;
    bool invert, on = false;
    juce::ParameterAttachment attachment;
};

//==============================================================================
/** Small activity LED (e.g. gate open). */
class ActivityLed : public juce::Component, private juce::Timer
{
public:
    ActivityLed (std::function<bool()> source, juce::Colour colour, const juce::String& labelText)
        : src (std::move (source)), col (colour), label (labelText)
    {
        startTimerHz (30);
    }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        const float r = 4.5f;
        drawLed (g, { b.getX() + r * 2.0f, b.getCentreY() }, r, col, lit);
        panelText (g, label, b.withTrimmedLeft (r * 4.2f), console::kInk, font (10.0f, true, 0.15f), juce::Justification::centredLeft);
    }

private:
    void timerCallback() override
    {
        const bool now = src();
        if (now != lit) { lit = now; repaint(); }
    }

    std::function<bool()> src;
    juce::Colour col;
    juce::String label;
    bool lit = false;
};

//==============================================================================
/** Vertical LED ladder. Gain-reduction mode lights from the top down (0..maxDb). */
class LedLadder : public juce::Component, private juce::Timer
{
public:
    LedLadder (std::function<float()> source, std::vector<float> stepsDb, const juce::String& captionText)
        : src (std::move (source)), steps (std::move (stepsDb)), caption (captionText)
    {
        startTimerHz (30);
    }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        panelText (g, caption, b.removeFromBottom (14.0f), console::kInk, font (10.0f, true, 0.2f));
        auto well = b.reduced (2.0f);
        drawRecess (g, well, 3.0f, juce::Colour (0xff101113));

        auto inner = well.reduced (5.0f, 6.0f);
        const int n = (int) steps.size();
        const float h = inner.getHeight() / (float) n;
        g.setFont (font (8.5f, true));
        for (int i = 0; i < n; ++i)
        {
            const bool on = value >= steps[(size_t) i] - 0.01f && value > 0.05f;
            const auto c = i < n / 2 ? juce::Colour (0xff4cdb5a) : (i < n - 2 ? juce::Colour (0xffffc23a) : juce::Colour (0xffff3a2e));
            auto seg = juce::Rectangle<float> (inner.getX(), inner.getY() + h * (float) i + 1.0f, inner.getWidth() * 0.45f, h - 2.5f);
            if (on)
            {
                g.setColour (c.withAlpha (0.25f));
                g.fillRoundedRectangle (seg.expanded (2.0f), 2.0f);
            }
            g.setColour (on ? c : c.darker (2.6f).withSaturation (0.4f));
            g.fillRoundedRectangle (seg, 1.5f);
            g.setColour (console::kInkDim);
            g.drawText (juce::String (juce::roundToInt (steps[(size_t) i])), seg.withX (seg.getRight() + 2.0f).withWidth (inner.getWidth() * 0.55f),
                        juce::Justification::centredLeft, false);
        }
    }

private:
    void timerCallback() override
    {
        const float target = std::max (0.0f, src());
        const float v = target > value ? target : value + (target - value) * 0.25f;   // fast on, slow off
        if (std::abs (v - value) > 0.02f) { value = v; repaint(); }
    }

    std::function<float()> src;
    std::vector<float> steps;
    juce::String caption;
    float value = 0.0f;
};
} // namespace ecee::ui
