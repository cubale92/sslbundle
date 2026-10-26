#pragma once
// 8cee — shared vector UI: faceplates, knobs, VU meters, switches.
// Everything is drawn in code (no bitmaps), so the UI stays sharp at any size.
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "EceeProcessorBase.h"
#include <functional>
#include <cmath>

namespace ecee::ui
{
inline juce::FontOptions font (float h, bool bold = true, float kerning = 0.0f)
{
    return juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain).withKerningFactor (kerning);
}

inline juce::Point<float> polar (juce::Point<float> c, float r, float angle)
{
    return { c.x + r * std::sin (angle), c.y - r * std::cos (angle) };
}

/** Panel text with a subtle printed/engraved look. */
inline void panelText (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Colour c,
                       juce::FontOptions f, juce::Justification j = juce::Justification::centred)
{
    g.setFont (f);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawText (t, r.translated (0.0f, 1.0f), j, false);
    g.setColour (c);
    g.drawText (t, r, j, false);
}

inline void drawScrew (juce::Graphics& g, juce::Point<float> c, float r, float angle = 0.6f)
{
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillEllipse (c.x - r + 0.8f, c.y - r + 1.4f, r * 2, r * 2);
    juce::ColourGradient grad (juce::Colour (0xfff2f2f2), c.x - r * 0.7f, c.y - r * 0.7f,
                               juce::Colour (0xff5c5c5c), c.x + r * 0.8f, c.y + r * 0.8f, false);
    g.setGradientFill (grad);
    g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
    g.setColour (juce::Colour (0xff2e2e2e));
    g.drawEllipse (c.x - r, c.y - r, r * 2, r * 2, 0.7f);
    juce::Path p;
    p.addRectangle (-r * 0.72f, -r * 0.12f, r * 1.44f, r * 0.24f);
    p.addRectangle (-r * 0.12f, -r * 0.72f, r * 0.24f, r * 1.44f);
    g.setColour (juce::Colour (0xff262626));
    g.fillPath (p, juce::AffineTransform::rotation (angle).translated (c));
}

inline void drawLed (juce::Graphics& g, juce::Point<float> c, float r, juce::Colour col, bool on)
{
    if (on)
    {
        juce::ColourGradient glow (col.withAlpha (0.55f), c.x, c.y, col.withAlpha (0.0f), c.x + r * 3.2f, c.y, true);
        g.setGradientFill (glow);
        g.fillEllipse (c.x - r * 3.2f, c.y - r * 3.2f, r * 6.4f, r * 6.4f);
    }
    g.setColour (juce::Colour (0xff111111));
    g.fillEllipse (c.x - r * 1.35f, c.y - r * 1.35f, r * 2.7f, r * 2.7f);
    auto body = on ? col : col.darker (2.2f).withSaturation (0.5f);
    juce::ColourGradient grad (body.brighter (on ? 0.9f : 0.3f), c.x - r * 0.4f, c.y - r * 0.5f, body, c.x + r, c.y + r, true);
    g.setGradientFill (grad);
    g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
}

/** Brushed metal fill (deterministic so it does not shimmer between repaints). */
inline void brushed (juce::Graphics& g, juce::Rectangle<float> r, float amount)
{
    juce::Random rnd (7331);
    for (float y = r.getY(); y < r.getBottom(); y += 1.0f)
    {
        g.setColour ((rnd.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha (rnd.nextFloat() * amount));
        g.fillRect (r.getX(), y, r.getWidth(), 1.0f);
    }
}

/** A 19" rack unit: metal ears with slotted screws and a faceplate. Returns the usable panel area. */
inline juce::Rectangle<float> drawRackUnit (juce::Graphics& g, juce::Rectangle<float> b, juce::Colour top, juce::Colour bottom,
                                            juce::Colour ear, float brushAmount = 0.035f, float earW = 40.0f)
{
    g.setColour (juce::Colour (0xff0b0b0c));
    g.fillRect (b);

    auto earL = b.removeFromLeft (earW);
    auto earR = b.removeFromRight (earW);

    for (auto e : { earL, earR })
    {
        juce::ColourGradient eg (ear.brighter (0.25f), e.getX(), e.getY(), ear.darker (0.5f), e.getRight(), e.getBottom(), false);
        g.setGradientFill (eg);
        g.fillRect (e);
        brushed (g, e, 0.04f);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawRect (e, 1.0f);

        for (float fy : { 0.17f, 0.83f })
        {
            auto slot = juce::Rectangle<float> (e.getWidth() * 0.52f, 22.0f).withCentre ({ e.getCentreX(), e.getY() + e.getHeight() * fy });
            g.setColour (juce::Colour (0xff050505));
            g.fillRoundedRectangle (slot, slot.getWidth() * 0.5f);
            g.setColour (juce::Colours::white.withAlpha (0.12f));
            g.drawRoundedRectangle (slot.translated (0, 1), slot.getWidth() * 0.5f, 0.8f);
            drawScrew (g, slot.getCentre(), 7.5f, fy * 3.0f);
        }
    }

    juce::ColourGradient pg (top, b.getX(), b.getY(), bottom, b.getX(), b.getBottom(), false);
    g.setGradientFill (pg);
    g.fillRect (b);
    brushed (g, b, brushAmount);

    g.setColour (juce::Colours::white.withAlpha (0.16f));
    g.fillRect (b.getX(), b.getY(), b.getWidth(), 1.5f);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRect (b.getX(), b.getBottom() - 2.0f, b.getWidth(), 2.0f);
    g.fillRect (b.getX(), b.getY(), 1.5f, b.getHeight());
    g.fillRect (b.getRight() - 1.5f, b.getY(), 1.5f, b.getHeight());
    return b;
}

/** Recessed rectangle (meter windows, decks, etc). */
inline void drawRecess (juce::Graphics& g, juce::Rectangle<float> r, float corner, juce::Colour fill)
{
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (r.expanded (2.0f), corner + 2.0f);
    g.setColour (fill);
    g.fillRoundedRectangle (r, corner);
    juce::ColourGradient sh (juce::Colours::black.withAlpha (0.45f), r.getX(), r.getY(),
                             juce::Colours::transparentBlack, r.getX(), r.getY() + 10.0f, false);
    g.setGradientFill (sh);
    g.fillRoundedRectangle (r, corner);
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawRoundedRectangle (r.expanded (2.5f), corner + 2.5f, 1.0f);
}

//==============================================================================
class VintageLookAndFeel : public juce::LookAndFeel_V4
{
public:
    VintageLookAndFeel()
    {
        setColour (juce::BubbleComponent::backgroundColourId, juce::Colour (0xf0151515));
        setColour (juce::BubbleComponent::outlineColourId, juce::Colour (0xff6b5d3f));
        setColour (juce::TooltipWindow::textColourId, juce::Colour (0xfff1e4c3));
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff17171a));
        setColour (juce::PopupMenu::textColourId, juce::Colour (0xffe9e2cf));
        setColour (juce::PopupMenu::headerTextColourId, juce::Colour (0xffd7a84a));
        setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff6b5d3f));
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colour (0xfffff4d6));
    }

    juce::Font getPopupMenuFont() override { return juce::Font (font (14.0f, true)); }

    juce::Font getSliderPopupFont (juce::Slider&) override { return juce::Font (font (13.0f, true)); }
    int getSliderPopupPlacement (juce::Slider&) override   { return juce::BubbleComponent::above; }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider& s) override
    {
        const auto style = s.getProperties()["knob"].toString();
        const auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
        const auto c = b.getCentre();
        const float r = std::min (b.getWidth(), b.getHeight()) * 0.5f * 0.86f;
        const float angle = startAngle + pos * (endAngle - startAngle);

        if (style == "chicken")      drawChicken (g, c, r, angle);
        else if (style == "chrome")  drawRound (g, c, r, angle, true);
        else if (style == "console") drawConsole (g, c, r, angle, juce::Colour ((juce::uint32) (juce::int64) s.getProperties()["cap"]));
        else                         drawRound (g, c, r, angle, false);
    }

    /** Console-desk knob: dark ribbed body with a coloured cap insert (4000-series style). */
    static void drawConsole (juce::Graphics& g, juce::Point<float> c, float r, float angle, juce::Colour cap)
    {
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillEllipse (c.x - r + r * 0.08f, c.y - r + r * 0.16f, r * 2.0f, r * 2.0f);

        // skirt with fine ribs
        juce::ColourGradient sk (juce::Colour (0xff4a4c50), c.x - r, c.y - r, juce::Colour (0xff0d0e10), c.x + r, c.y + r, false);
        g.setGradientFill (sk);
        g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        for (int i = 0; i < 36; ++i)
        {
            const float a = angle + (float) i * juce::MathConstants<float>::twoPi / 36.0f;
            g.drawLine (juce::Line<float> (polar (c, r * 0.80f, a), polar (c, r * 0.98f, a)), r * 0.04f);
        }
        // pointer notch on the skirt
        g.setColour (juce::Colour (0xfff2f0ea));
        g.drawLine (juce::Line<float> (polar (c, r * 0.80f, angle), polar (c, r * 0.99f, angle)), r * 0.09f);

        // flat top
        const float tr = r * 0.78f;
        juce::ColourGradient top (juce::Colour (0xff3a3c40), c.x - tr * 0.7f, c.y - tr * 0.8f, juce::Colour (0xff121315), c.x + tr * 0.6f, c.y + tr * 0.9f, false);
        g.setGradientFill (top);
        g.fillEllipse (c.x - tr, c.y - tr, tr * 2, tr * 2);
        g.setColour (juce::Colours::black);
        g.drawEllipse (c.x - tr, c.y - tr, tr * 2, tr * 2, r * 0.03f);

        // coloured cap insert
        const float cr = r * 0.56f;
        juce::ColourGradient cg (cap.brighter (0.45f), c.x - cr * 0.6f, c.y - cr * 0.7f, cap.darker (0.55f), c.x + cr * 0.6f, c.y + cr * 0.8f, false);
        g.setGradientFill (cg);
        g.fillEllipse (c.x - cr, c.y - cr, cr * 2, cr * 2);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawEllipse (c.x - cr, c.y - cr, cr * 2, cr * 2, r * 0.025f);
        juce::ColourGradient spec (juce::Colours::white.withAlpha (0.28f), c.x, c.y - cr, juce::Colours::transparentWhite, c.x, c.y, false);
        g.setGradientFill (spec);
        g.fillEllipse (c.x - cr * 0.75f, c.y - cr * 0.92f, cr * 1.5f, cr * 0.95f);

        // pointer line across the cap
        const bool darkCap = cap.getPerceivedBrightness() > 0.62f;
        g.setColour (darkCap ? juce::Colour (0xff141414) : juce::Colour (0xfff7f5ef));
        juce::Path p;
        p.addRoundedRectangle (-r * 0.045f, -r * 0.78f, r * 0.09f, r * 0.62f, r * 0.04f);
        g.fillPath (p, juce::AffineTransform::rotation (angle).translated (c));
    }

private:
    static void drawRound (juce::Graphics& g, juce::Point<float> c, float r, float angle, bool chrome)
    {
        // drop shadow
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillEllipse (c.x - r * 1.02f + r * 0.07f, c.y - r * 1.02f + r * 0.14f, r * 2.04f, r * 2.04f);

        // skirt
        juce::ColourGradient sk (chrome ? juce::Colour (0xffe9e9e6) : juce::Colour (0xff3b3b3b), c.x - r, c.y - r,
                                 chrome ? juce::Colour (0xff6d6d6a) : juce::Colour (0xff050505), c.x + r, c.y + r, false);
        g.setGradientFill (sk);
        g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);

        // knurling, rotates with the knob
        g.setColour (juce::Colours::black.withAlpha (chrome ? 0.35f : 0.6f));
        for (int i = 0; i < 48; ++i)
        {
            const float a = angle + (float) i * juce::MathConstants<float>::twoPi / 48.0f;
            g.drawLine (juce::Line<float> (polar (c, r * 0.84f, a), polar (c, r * 0.99f, a)), r * 0.035f);
        }

        // cap
        const float cr = r * (chrome ? 0.70f : 0.80f);
        juce::ColourGradient cap (juce::Colour (0xff4d4d4d), c.x - cr * 0.7f, c.y - cr * 0.8f,
                                  juce::Colour (0xff070707), c.x + cr * 0.6f, c.y + cr * 0.9f, false);
        g.setGradientFill (cap);
        g.fillEllipse (c.x - cr, c.y - cr, cr * 2, cr * 2);
        g.setColour (juce::Colours::black);
        g.drawEllipse (c.x - cr, c.y - cr, cr * 2, cr * 2, r * 0.03f);

        // specular highlight
        juce::ColourGradient spec (juce::Colours::white.withAlpha (0.22f), c.x - cr * 0.35f, c.y - cr * 0.55f,
                                   juce::Colours::transparentWhite, c.x - cr * 0.35f, c.y + cr * 0.1f, false);
        g.setGradientFill (spec);
        g.fillEllipse (c.x - cr * 0.78f, c.y - cr * 0.86f, cr * 1.56f, cr * 1.0f);

        // pointer
        g.setColour (juce::Colour (0xfff4efe2));
        juce::Path p;
        p.addRoundedRectangle (-r * 0.05f, -r * 0.95f, r * 0.10f, r * 0.62f, r * 0.05f);
        g.fillPath (p, juce::AffineTransform::rotation (angle).translated (c));
    }

    static void drawChicken (juce::Graphics& g, juce::Point<float> c, float r, float angle)
    {
        juce::Path body;
        body.startNewSubPath (0.0f, -r);
        body.cubicTo (r * 0.18f, -r * 0.98f, r * 0.46f, -r * 0.25f, r * 0.44f, r * 0.15f);
        body.cubicTo (r * 0.42f, r * 0.62f, r * 0.24f, r * 0.72f, 0.0f, r * 0.72f);
        body.cubicTo (-r * 0.24f, r * 0.72f, -r * 0.42f, r * 0.62f, -r * 0.44f, r * 0.15f);
        body.cubicTo (-r * 0.46f, -r * 0.25f, -r * 0.18f, -r * 0.98f, 0.0f, -r);
        body.closeSubPath();

        const auto t = juce::AffineTransform::rotation (angle).translated (c);
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillPath (body, t.translated (r * 0.07f, r * 0.13f));

        // round base
        juce::ColourGradient base (juce::Colour (0xff3a3a3a), c.x - r * 0.6f, c.y - r * 0.6f,
                                   juce::Colour (0xff080808), c.x + r * 0.6f, c.y + r * 0.6f, false);
        g.setGradientFill (base);
        g.fillEllipse (c.x - r * 0.62f, c.y - r * 0.62f, r * 1.24f, r * 1.24f);

        juce::ColourGradient grad (juce::Colour (0xff4a4a4a), c.x - r * 0.5f, c.y - r * 0.8f,
                                   juce::Colour (0xff0a0a0a), c.x + r * 0.5f, c.y + r * 0.8f, false);
        g.setGradientFill (grad);
        g.fillPath (body, t);
        g.setColour (juce::Colours::black);
        g.strokePath (body, juce::PathStrokeType (r * 0.03f), t);

        g.setColour (juce::Colour (0xfff4efe2));
        juce::Path line;
        line.addRoundedRectangle (-r * 0.04f, -r * 0.94f, r * 0.08f, r * 0.62f, r * 0.04f);
        g.fillPath (line, t);
    }
};

//==============================================================================
/** Parameter-attached rotary knob with printed scale and title. */
class Knob : public juce::Component
{
public:
    Knob (juce::AudioProcessorValueTreeState& state, const juce::String& paramID, const juce::String& titleText,
          juce::StringArray scaleLabels = {}, const juce::String& style = "bakelite")
        : title (titleText), labels (scaleLabels)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.getProperties().set ("knob", style);
        slider.setMouseDragSensitivity (220);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        addAndMakeVisible (slider);
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, paramID, slider);

        if (auto* p = state.getParameter (paramID))
            slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
    }

    /** Clockwise = lower value (hardware style for attack/release "speed" knobs). */
    void setReversed()
    {
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 2.75f, juce::MathConstants<float>::pi * 1.25f, true);
        reversed = true;
    }

    void setColours (juce::Colour text, juce::Colour scale) { textColour = text; scaleColour = scale; repaint(); }
    void setPopupParent (juce::Component* parent)          { slider.setPopupDisplayEnabled (true, true, parent, 1200); }
    void setKnobScale (float s)                            { knobScale = s; resized(); }
    void setNumTicks (int n)                               { ticks = n; repaint(); }
    /** For the "console" style: colour of the cap insert. */
    void setCapColour (juce::Colour c)                     { slider.getProperties().set ("cap", (juce::int64) c.getARGB()); slider.repaint(); }
    /** Smaller title text, for dense console layouts. */
    void setTitleSize (float h)                            { titleSize = h; repaint(); }
    juce::Slider& getSlider()                              { return slider; }

    void paint (juce::Graphics& g) override
    {
        panelText (g, title, getLocalBounds().toFloat().removeFromTop (titleH), textColour, font (titleSize, true, 0.12f));

        const int n = labels.isEmpty() ? ticks : labels.size();
        const float a0 = juce::MathConstants<float>::pi * 1.25f, a1 = juce::MathConstants<float>::pi * 2.75f;
        g.setColour (scaleColour);
        for (int i = 0; i < n; ++i)
        {
            const float t = n > 1 ? (float) i / (float) (n - 1) : 0.5f;
            const float a = a0 + (reversed ? 1.0f - t : t) * (a1 - a0);
            g.drawLine (juce::Line<float> (polar (centre, ringR * 0.80f, a), polar (centre, ringR * 0.90f, a)), 1.6f);
        }

        if (! labels.isEmpty())
        {
            g.setFont (font (labelSize, true));
            for (int i = 0; i < n; ++i)
            {
                const float t = n > 1 ? (float) i / (float) (n - 1) : 0.5f;
                const float a = a0 + (reversed ? 1.0f - t : t) * (a1 - a0);
                auto p = polar (centre, ringR * 1.06f, a);
                g.drawText (labels[i], juce::Rectangle<float> (44.0f, 14.0f).withCentre (p), juce::Justification::centred, false);
            }
        }
    }

    void resized() override
    {
        auto b = getLocalBounds().toFloat();
        b.removeFromTop (titleH);
        const float side = std::min (b.getWidth(), b.getHeight());
        centre = b.getCentre();
        ringR = side * 0.5f * 0.80f;
        labelSize = juce::jlimit (9.0f, 12.0f, side * 0.085f);
        const float kr = ringR * 0.74f * knobScale;
        slider.setBounds (juce::Rectangle<float> (kr * 2.3f, kr * 2.3f).withCentre (centre).toNearestInt());
    }

private:
    juce::Slider slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    juce::String title;
    juce::StringArray labels;
    juce::Colour textColour { 0xffe9e2cf }, scaleColour { 0xffcfc8b4 };
    juce::Point<float> centre;
    float ringR = 40.0f, labelSize = 10.0f, knobScale = 1.0f, titleSize = 12.5f;
    const float titleH = 18.0f;
    int ticks = 11;
    bool reversed = false;
};

//==============================================================================
/** Row/column of latching push buttons driving one choice parameter (radio style). */
class ChoiceButtons : public juce::Component
{
public:
    ChoiceButtons (juce::RangedAudioParameter& p, juce::StringArray buttonLabels, bool isVertical,
                   juce::Colour litColour = juce::Colour (0xfff6e7b8))
        : labels (buttonLabels), vertical (isVertical), lit (litColour),
          attachment (p, [this] (float v) { current = juce::roundToInt (v); repaint(); }, nullptr)
    {
        attachment.sendInitialUpdate();
    }

    void paint (juce::Graphics& g) override
    {
        for (int i = 0; i < labels.size(); ++i)
        {
            auto r = cell (i).reduced (3.0f);
            const bool on = (i == current);
            g.setColour (juce::Colours::black.withAlpha (0.7f));
            g.fillRoundedRectangle (r.expanded (2.0f).translated (0, 1.5f), 4.0f);

            if (on)
            {
                juce::ColourGradient gl (lit.withAlpha (0.35f), r.getCentreX(), r.getCentreY(),
                                         lit.withAlpha (0.0f), r.getCentreX() + r.getWidth(), r.getCentreY(), true);
                g.setGradientFill (gl);
                g.fillRect (r.expanded (10.0f));
                juce::ColourGradient face (lit.brighter (0.3f), r.getX(), r.getY(), lit.darker (0.25f), r.getX(), r.getBottom(), false);
                g.setGradientFill (face);
            }
            else
            {
                juce::ColourGradient face (juce::Colour (0xff3a3a3a), r.getX(), r.getY(), juce::Colour (0xff141414), r.getX(), r.getBottom(), false);
                g.setGradientFill (face);
            }
            g.fillRoundedRectangle (r, 3.0f);
            g.setColour (juce::Colours::white.withAlpha (on ? 0.5f : 0.12f));
            g.drawLine (r.getX() + 3, r.getY() + 1, r.getRight() - 3, r.getY() + 1, 1.0f);

            g.setColour (on ? juce::Colour (0xff1b1408) : juce::Colour (0xffbdb6a3));
            g.setFont (font (std::min (13.0f, r.getHeight() * 0.5f), true, 0.05f));
            g.drawText (labels[i], r, juce::Justification::centred, false);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        for (int i = 0; i < labels.size(); ++i)
            if (cell (i).contains (e.position))
                attachment.setValueAsCompleteGesture ((float) i);
    }

private:
    juce::Rectangle<float> cell (int i) const
    {
        auto b = getLocalBounds().toFloat();
        const float n = (float) labels.size();
        return vertical ? juce::Rectangle<float> (b.getX(), b.getY() + b.getHeight() * (float) i / n, b.getWidth(), b.getHeight() / n)
                        : juce::Rectangle<float> (b.getX() + b.getWidth() * (float) i / n, b.getY(), b.getWidth() / n, b.getHeight());
    }

    juce::StringArray labels;
    bool vertical;
    juce::Colour lit;
    int current = 0;
    juce::ParameterAttachment attachment;
};

//==============================================================================
/** Chrome bat-handle toggle with 2 or 3 positions. Labels are given from lowest index (bottom) to highest (top). */
class BatSwitch : public juce::Component
{
public:
    BatSwitch (juce::RangedAudioParameter& p, juce::StringArray positionLabels, const juce::String& titleText,
               juce::Colour text = juce::Colour (0xffe9e2cf))
        : labels (positionLabels), title (titleText), textColour (text),
          attachment (p, [this] (float v) { current = juce::roundToInt (v); repaint(); }, nullptr)
    {
        attachment.sendInitialUpdate();
    }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        panelText (g, title, b.removeFromTop (18.0f), textColour, font (12.5f, true, 0.12f));

        auto sw = b.removeFromLeft (b.getWidth() * 0.42f);
        const auto c = sw.getCentre();
        const float L = std::min (sw.getHeight() * 0.36f, 26.0f);
        const int n = labels.size();

        // labels to the right, top = highest index
        g.setFont (font (11.5f, true, 0.06f));
        for (int i = 0; i < n; ++i)
        {
            const float y = labelY (i, c.y, L);
            g.setColour (i == current ? textColour : textColour.withAlpha (0.45f));
            g.drawText (labels[i], juce::Rectangle<float> (b.getX() + 2.0f, y - 8.0f, b.getWidth(), 16.0f), juce::Justification::centredLeft, false);
        }

        // hex nut + washer
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillEllipse (c.x - 13.0f + 1.0f, c.y - 13.0f + 2.0f, 26.0f, 26.0f);
        juce::ColourGradient wg (juce::Colour (0xffe0e0dc), c.x - 12.0f, c.y - 12.0f, juce::Colour (0xff5e5e5a), c.x + 12.0f, c.y + 12.0f, false);
        g.setGradientFill (wg);
        g.fillEllipse (c.x - 13.0f, c.y - 13.0f, 26.0f, 26.0f);
        juce::Path hex;
        hex.addPolygon (c, 6, 9.5f, 0.0f);
        juce::ColourGradient hg (juce::Colour (0xfff5f5f2), c.x - 8.0f, c.y - 8.0f, juce::Colour (0xff76766f), c.x + 8.0f, c.y + 8.0f, false);
        g.setGradientFill (hg);
        g.fillPath (hex);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.strokePath (hex, juce::PathStrokeType (0.8f));

        // lever
        const float t = n > 1 ? (float) current / (float) (n - 1) : 0.0f;
        const float dy = -(t * 2.0f - 1.0f) * L;
        const juce::Point<float> tip (c.x, c.y + dy);
        juce::Path lever;
        lever.startNewSubPath (c.x - 3.2f, c.y);
        lever.lineTo (tip.x - 4.0f, tip.y);
        lever.lineTo (tip.x + 4.0f, tip.y);
        lever.lineTo (c.x + 3.2f, c.y);
        lever.closeSubPath();
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillPath (lever, juce::AffineTransform::translation (2.0f, 3.0f));
        juce::ColourGradient lg (juce::Colour (0xffffffff), c.x - 4.0f, c.y, juce::Colour (0xff7a7a74), c.x + 4.0f, c.y, false);
        g.setGradientFill (lg);
        g.fillPath (lever);
        // a centred lever points straight at the viewer, so its ball tip looks bigger
        const float br = std::abs (dy) < 1.0f ? 7.5f : 5.5f;
        juce::ColourGradient bg (juce::Colour (0xffffffff), tip.x - br * 0.5f, tip.y - br * 0.6f, juce::Colour (0xff8a8a84), tip.x + br, tip.y + br, false);
        g.setGradientFill (bg);
        g.fillEllipse (tip.x - br, tip.y - br, br * 2.0f, br * 2.0f);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawEllipse (tip.x - br, tip.y - br, br * 2.0f, br * 2.0f, 0.7f);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        const int n = labels.size();
        if (n == 2) { attachment.setValueAsCompleteGesture (current == 0 ? 1.0f : 0.0f); return; }

        auto b = getLocalBounds().toFloat();
        b.removeFromTop (18.0f);
        const float cy = b.getCentreY();
        const float L = std::min (b.removeFromLeft (b.getWidth() * 0.42f).getHeight() * 0.36f, 26.0f);
        int best = 0; float bestD = 1.0e9f;
        for (int i = 0; i < n; ++i)
        {
            const float d = std::abs (e.position.y - labelY (i, cy, L));
            if (d < bestD) { bestD = d; best = i; }
        }
        attachment.setValueAsCompleteGesture ((float) best);
    }

private:
    float labelY (int i, float cy, float L) const
    {
        const int n = labels.size();
        const float t = n > 1 ? (float) i / (float) (n - 1) : 0.0f;
        return cy - (t * 2.0f - 1.0f) * L;
    }

    juce::StringArray labels;
    juce::String title;
    juce::Colour textColour;
    int current = 0;
    juce::ParameterAttachment attachment;
};

//==============================================================================
/** Backlit analog VU meter with real VU ballistics. */
class VUMeter : public juce::Component, private juce::Timer
{
public:
    enum class Mode { level, gainReduction };

    VUMeter (std::function<float()> source, Mode m, const juce::String& caption = "VU")
        : src (std::move (source)), mode (m), text (caption)
    {
        setOpaque (true);
        startTimerHz (60);
    }

    void setMode (Mode m)                   { mode = m; }
    void setCaption (const juce::String& c) { text = c; repaint(); }
    /** dBFS that reads 0 VU (level mode). */
    void setReference (float dbfs)          { referenceDb = dbfs; }
    void setLit (bool shouldBeLit)          { lit = shouldBeLit; repaint(); }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        g.fillAll (juce::Colour (0xff0c0c0c));

        auto face = b.reduced (7.0f);
        juce::ColourGradient bezel (juce::Colour (0xff3c3c3c), b.getX(), b.getY(), juce::Colour (0xff0a0a0a), b.getX(), b.getBottom(), false);
        g.setGradientFill (bezel);
        g.fillRoundedRectangle (b.reduced (1.0f), 5.0f);

        const auto paper = lit ? juce::Colour (0xfff3dfa6) : juce::Colour (0xff8f8468);
        juce::ColourGradient fg (paper.brighter (0.15f), face.getCentreX(), face.getBottom(),
                                 paper.darker (lit ? 0.35f : 0.2f), face.getX(), face.getY(), true);
        g.setGradientFill (fg);
        g.fillRoundedRectangle (face, 3.0f);

        const float fh = face.getHeight();
        const juce::Point<float> pivot (face.getCentreX(), face.getBottom() + fh * 0.14f);
        const float R = std::min (fh * 1.12f, face.getWidth() * 0.80f);
        const float span = 0.70f;

        // red zone 0..+3
        {
            juce::Path red;
            red.addCentredArc (pivot.x, pivot.y, R * 0.80f, R * 0.80f, 0.0f, angleFor (0.0f, span), angleFor (3.0f, span), true);
            g.setColour (juce::Colour (0xffb3261e));
            g.strokePath (red, juce::PathStrokeType (face.getHeight() * 0.045f));
        }
        juce::Path arc;
        arc.addCentredArc (pivot.x, pivot.y, R * 0.76f, R * 0.76f, 0.0f, angleFor (-20.0f, span), angleFor (3.0f, span), true);
        g.setColour (juce::Colour (0xff2a241a));
        g.strokePath (arc, juce::PathStrokeType (1.2f));

        const int marks[] = { -20, -10, -7, -5, -3, -2, -1, 0, 1, 2, 3 };
        g.setFont (font (fh * 0.095f, true));
        for (auto m : marks)
        {
            const float a = angleFor ((float) m, span);
            g.setColour (m > 0 ? juce::Colour (0xffb3261e) : juce::Colour (0xff2a241a));
            g.drawLine (juce::Line<float> (polar (pivot, R * 0.76f, a), polar (pivot, R * 0.84f, a)), 1.4f);
            const juce::String s = m > 0 ? "+" + juce::String (m) : juce::String (m);
            if (m == -7 || m == -2 || m == 1) continue; // keep the crowded part of the scale readable
            g.drawText (s, juce::Rectangle<float> (30.0f, 14.0f).withCentre (polar (pivot, R * 0.93f, a)), juce::Justification::centred, false);
        }

        g.setColour (juce::Colour (0xff2a241a));
        g.setFont (font (fh * 0.105f, true, 0.12f));
        g.drawText (text, face.withTrimmedTop (fh * 0.58f).withHeight (fh * 0.14f), juce::Justification::centred, false);
        g.setFont (font (fh * 0.065f, true, 0.3f));
        g.drawText ("8CEE", face.withTrimmedTop (fh * 0.73f).withHeight (fh * 0.1f), juce::Justification::centred, false);

        // needle
        const float a = angleFor (needle, span);
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.drawLine (juce::Line<float> (polar (pivot, R * 0.30f, a).translated (2.5f, 2.0f), polar (pivot, R * 0.90f, a).translated (2.5f, 2.0f)), 1.6f);
        g.setColour (juce::Colour (0xff141414));
        g.drawLine (juce::Line<float> (polar (pivot, R * 0.30f, a), polar (pivot, R * 0.90f, a)), 1.6f);

        // pivot cover
        g.setColour (juce::Colour (0xff141414));
        auto cover = juce::Rectangle<float> (face.getX(), face.getBottom() - face.getHeight() * 0.10f, face.getWidth(), face.getHeight() * 0.10f);
        g.fillRect (cover);

        // glass
        juce::ColourGradient glass (juce::Colours::white.withAlpha (0.20f), face.getX(), face.getY(),
                                    juce::Colours::transparentWhite, face.getX(), face.getCentreY(), false);
        g.setGradientFill (glass);
        g.fillRoundedRectangle (face.withHeight (face.getHeight() * 0.5f), 3.0f);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawRoundedRectangle (face, 3.0f, 1.5f);
    }

private:
    static float vuPos (float db)
    {
        const float lo = std::pow (10.0f, -20.0f / 20.0f), hi = std::pow (10.0f, 3.0f / 20.0f);
        return (std::pow (10.0f, juce::jlimit (-24.0f, 4.0f, db) / 20.0f) - lo) / (hi - lo);
    }
    static float angleFor (float db, float span) { return -span + vuPos (db) * span * 2.0f; }

    void timerCallback() override
    {
        float target;
        if (! lit)                              target = -24.0f;
        else if (mode == Mode::gainReduction)   target = -src();
        else                                    target = src() - referenceDb;

        // VU ballistics: ~300 ms rise time
        const float dt = 1.0f / 60.0f;
        const float k = 1.0f - std::exp (-dt / 0.065f);
        const float newNeedle = needle + (juce::jlimit (-24.0f, 4.0f, target) - needle) * k;
        if (std::abs (newNeedle - needle) > 0.005f) { needle = newNeedle; repaint(); }
    }

    std::function<float()> src;
    Mode mode;
    juce::String text;
    float needle = -24.0f, referenceDb = -18.0f;
    bool lit = true;
};

//==============================================================================
/** Amber preset display: < name > . Click the name for the full list. */
class PresetBar : public juce::Component, private juce::Timer
{
public:
    explicit PresetBar (ecee::ProcessorBase& p) : proc (p)
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        startTimerHz (10);
        timerCallback();
    }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        drawRecess (g, b.reduced (2.0f), 4.0f, juce::Colour (0xff0f0d0a));

        const juce::Colour amber (0xffffb547);
        auto arrows = b.reduced (2.0f);
        auto left = arrows.removeFromLeft (arrowW), right = arrows.removeFromRight (arrowW);
        for (auto [r, dir] : { std::pair { left, -1.0f }, std::pair { right, 1.0f } })
        {
            juce::Path tri;
            const auto c = r.getCentre();
            tri.addTriangle (c.x - 3.5f * dir, c.y - 5.0f, c.x - 3.5f * dir, c.y + 5.0f, c.x + 4.0f * dir, c.y);
            g.setColour (amber.withAlpha (hoverZone == (dir < 0 ? 1 : 3) ? 1.0f : 0.6f));
            g.fillPath (tri);
        }

        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.fillRect (left.getRight(), b.getY() + 6.0f, 1.0f, b.getHeight() - 12.0f);
        g.fillRect (right.getX() - 1.0f, b.getY() + 6.0f, 1.0f, b.getHeight() - 12.0f);

        // glowing LCD-style text
        const auto text = (number > 0 ? juce::String (number).paddedLeft ('0', 2) + "  " : juce::String()) + name + (modified ? " *" : "");
        g.setFont (font (13.5f, true, 0.06f));
        g.setColour (amber.withAlpha (0.25f));
        g.drawText (text, arrows.translated (0.0f, 0.5f).expanded (1.0f, 0.0f), juce::Justification::centred, true);
        g.setColour (hoverZone == 2 ? amber.brighter (0.25f) : amber);
        g.drawText (text, arrows, juce::Justification::centred, true);
    }

    void mouseMove (const juce::MouseEvent& e) override  { setHover (zoneAt (e.position.x)); }
    void mouseExit (const juce::MouseEvent&) override     { setHover (0); }

    void mouseDown (const juce::MouseEvent& e) override
    {
        const int n = (int) proc.getPresets().size();
        if (n == 0) return;
        const int current = proc.getCurrentProgram();
        const int zone = zoneAt (e.position.x);

        if (zone == 1)      proc.applyPreset ((current + n - 1) % n);
        else if (zone == 3) proc.applyPreset ((current + 1) % n);
        else                showMenu();
        timerCallback();
    }

    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override
    {
        const int n = (int) proc.getPresets().size();
        if (n == 0 || std::abs (w.deltaY) < 0.01f) return;
        proc.applyPreset ((proc.getCurrentProgram() + (w.deltaY < 0 ? 1 : n - 1)) % n);
        timerCallback();
    }

private:
    int zoneAt (float x) const { return x < arrowW + 2.0f ? 1 : (x > (float) getWidth() - arrowW - 2.0f ? 3 : 2); }
    void setHover (int z) { if (z != hoverZone) { hoverZone = z; repaint(); } }

    void showMenu()
    {
        juce::PopupMenu menu;
        menu.addSectionHeader ("FACTORY PRESETS");
        const auto& list = proc.getPresets();
        for (int i = 0; i < (int) list.size(); ++i)
            menu.addItem (i + 1, list[(size_t) i].name, true, i == proc.getCurrentProgram());

        if (proc.isPresetModified())
        {
            menu.addSeparator();
            menu.addItem (1000, "Revert to \"" + proc.getProgramName (proc.getCurrentProgram()) + "\"");
        }

        juce::Component::SafePointer<PresetBar> safe (this);
        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMinimumWidth (getWidth()),
                            [safe] (int result)
                            {
                                if (safe == nullptr || result == 0) return;
                                auto& p = safe->proc;
                                p.applyPreset (result == 1000 ? p.getCurrentProgram() : result - 1);
                                safe->timerCallback();
                            });
    }

    void timerCallback() override
    {
        const int idx = proc.getCurrentProgram();
        const auto newName = proc.getProgramName (idx).toUpperCase();
        const bool newModified = proc.isPresetModified();
        const int newNumber = proc.getPresets().empty() ? 0 : idx + 1;
        if (newName != name || newModified != modified || newNumber != number)
        {
            name = newName; modified = newModified; number = newNumber;
            repaint();
        }
    }

    ecee::ProcessorBase& proc;
    juce::String name;
    bool modified = false;
    int number = 0, hoverZone = 0;
    static constexpr float arrowW = 24.0f;
};

//==============================================================================
/** Editor base: draws a fixed-size panel and scales it to whatever size the host window is. */
class ScaledEditor : public juce::AudioProcessorEditor
{
public:
    explicit ScaledEditor (juce::AudioProcessor& p) : juce::AudioProcessorEditor (p) {}
    // The panel is a member of the derived editor, so it is destroyed before our look-and-feel. Nothing to undo here.

    void initScaling (juce::Component& panel, int w, int h)
    {
        content = &panel;
        baseW = w; baseH = h;
        panel.setLookAndFeel (&lnf);
        addAndMakeVisible (panel);
        setResizable (true, true);
        setResizeLimits (w / 2, h / 2, w * 2, h * 2);
        getConstrainer()->setFixedAspectRatio ((double) w / (double) h);
        setSize (w, h);
    }

    void resized() override
    {
        if (content == nullptr) return;
        content->setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) baseW));
        content->setBounds (0, 0, baseW, baseH);
    }

private:
    VintageLookAndFeel lnf;
    juce::Component* content = nullptr;
    int baseW = 1, baseH = 1;
};
} // namespace ecee::ui
