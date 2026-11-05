#include "PluginEditor.h"

using namespace juce;

//==============================================================================
// LookAndFeel
ApexLookAndFeel::ApexLookAndFeel()
{
    setColour (Slider::textBoxTextColourId, Theme::text);
    setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    setColour (Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    setColour (Label::textColourId, Theme::textDim);
    setColour (ComboBox::backgroundColourId, Theme::panelHi);
    setColour (ComboBox::textColourId, Theme::text);
    setColour (ComboBox::outlineColourId, Theme::line);
    setColour (ComboBox::arrowColourId, Theme::accent);
    setColour (PopupMenu::backgroundColourId, Theme::panel);
    setColour (PopupMenu::textColourId, Theme::text);
    setColour (PopupMenu::highlightedBackgroundColourId, Theme::accent.withAlpha (0.25f));
    setColour (PopupMenu::highlightedTextColourId, Theme::text);
    setColour (TextButton::buttonColourId, Theme::panel);
    setColour (TextButton::buttonOnColourId, Theme::panelHi);
    setColour (TextButton::textColourOffId, Theme::textDim);
    setColour (TextButton::textColourOnId, Theme::accent);
    setColour (TextEditor::backgroundColourId, Theme::panelHi);
    setColour (TextEditor::textColourId, Theme::text);
    setColour (TextEditor::highlightColourId, Theme::accent.withAlpha (0.3f));
    setColour (CaretComponent::caretColourId, Theme::accent);
}

void ApexLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                                        float start, float end, Slider& s)
{
    auto colour = s.findColour (Slider::rotarySliderFillColourId);
    const float r = jmin (w, h) * 0.5f - 4.0f;
    if (r < 4.0f) return;
    const float cx = x + w * 0.5f, cy = y + h * 0.5f;
    const float angle = start + pos * (end - start);

    Path track;
    track.addCentredArc (cx, cy, r, r, 0.0f, start, end, true);
    g.setColour (Theme::line);
    g.strokePath (track, PathStrokeType (3.0f, PathStrokeType::curved, PathStrokeType::rounded));

    // bipolar parameters (range symmetric around 0) draw from the centre
    const auto range = s.getRange();
    const bool bipolar = range.getStart() < 0.0 && std::abs (range.getStart() + range.getEnd()) < 1e-6;
    const float from = bipolar ? (start + end) * 0.5f : start;

    Path value;
    value.addCentredArc (cx, cy, r, r, 0.0f, jmin (from, angle), jmax (from, angle), true);
    g.setColour (s.isEnabled() ? colour : Theme::textDim);
    g.strokePath (value, PathStrokeType (3.0f, PathStrokeType::curved, PathStrokeType::rounded));

    const float kr = r - 6.0f;
    g.setGradientFill (ColourGradient (Theme::panelHi.brighter (0.15f), cx, cy - kr, Theme::panel, cx, cy + kr, false));
    g.fillEllipse (cx - kr, cy - kr, kr * 2, kr * 2);
    g.setColour (Theme::line.brighter (0.2f));
    g.drawEllipse (cx - kr, cy - kr, kr * 2, kr * 2, 1.0f);

    Path pointer;
    pointer.addRoundedRectangle (-1.5f, -kr + 3.0f, 3.0f, kr * 0.45f, 1.5f);
    g.setColour (Theme::text);
    g.fillPath (pointer, AffineTransform::rotation (angle).translated (cx, cy));
}

void ApexLookAndFeel::drawToggleButton (Graphics& g, ToggleButton& b, bool highlighted, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const bool onState = b.getToggleState();
    g.setColour (onState ? Theme::accent.withAlpha (0.18f) : Theme::panelHi);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (onState ? Theme::accent : (highlighted ? Theme::textDim : Theme::line));
    g.drawRoundedRectangle (r, 4.0f, 1.0f);

    auto dot = r.removeFromLeft (r.getHeight()).reduced (r.getHeight() * 0.3f);
    g.setColour (onState ? Theme::accent : Theme::textDim.withAlpha (0.5f));
    g.fillEllipse (dot);

    g.setColour (onState ? Theme::text : Theme::textDim);
    g.setFont (FontOptions (12.0f, Font::bold));
    g.drawText (b.getButtonText(), r.withTrimmedRight (6), Justification::centred);
}

void ApexLookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool onState = b.getToggleState();
    g.setColour (onState ? Theme::panelHi : (highlighted || down ? Theme::panelHi.darker (0.2f) : Theme::panel));
    g.fillRoundedRectangle (r, 4.0f);
    if (onState)
    {
        g.setColour (Theme::accent);
        g.fillRoundedRectangle (r.removeFromBottom (2.0f).reduced (8.0f, 0.0f), 1.0f);
    }
    else
    {
        g.setColour (Theme::line);
        g.drawRoundedRectangle (r, 4.0f, 1.0f);
    }
}

void ApexLookAndFeel::drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, ComboBox& box)
{
    auto r = Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (box.findColour (ComboBox::backgroundColourId));
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (box.findColour (ComboBox::outlineColourId));
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
    Path arrow;
    const float ax = w - 16.0f, ay = h * 0.5f;
    arrow.addTriangle (ax - 4, ay - 2, ax + 4, ay - 2, ax, ay + 3);
    g.setColour (box.findColour (ComboBox::arrowColourId));
    g.fillPath (arrow);
}

//==============================================================================
Knob::Knob (AudioProcessorValueTreeState& s, const String& paramId, const String& title, Colour colour)
{
    slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (Slider::TextBoxBelow, false, 80, 16);
    slider.setColour (Slider::rotarySliderFillColourId, colour);
    slider.setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
    addAndMakeVisible (slider);
    attachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (s, paramId, slider);
    if (auto* prm = s.getParameter (paramId))
        slider.setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));

    label.setText (title.toUpperCase(), dontSendNotification);
    label.setJustificationType (Justification::centred);
    label.setFont (FontOptions (11.0f, Font::bold));
    addAndMakeVisible (label);
}

void Knob::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromTop (14));
    slider.setBounds (slider.getSliderStyle() == Slider::LinearBar ? r.reduced (0, 2) : r);
    if (slider.getSliderStyle() != Slider::LinearBar)
        slider.setTextBoxStyle (Slider::TextBoxBelow, false, jmax (40, r.getWidth()), 16);
}

PowerButton::PowerButton (AudioProcessorValueTreeState& s, const String& paramId, const String& text)
    : ToggleButton (text)
{
    attachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (s, paramId, *this);
}

//==============================================================================
// Spectrum
SpectrumView::SpectrumView (ApexMasterAudioProcessor& p) : proc (p)
{
    nodes = {
        { "hpFreq", nullptr,  nullptr, Colour (0xff9aa5b1), "HP" },
        { "lsFreq", "lsGain", nullptr, Colour (0xff6ea8ff), "LS" },
        { "p1Freq", "p1Gain", "p1Q",   Theme::accent2,      "1" },
        { "p2Freq", "p2Gain", "p2Q",   Theme::accent,       "2" },
        { "hsFreq", "hsGain", nullptr, Colour (0xffff7ab8), "HS" },
    };
    startTimerHz (30);
}

float SpectrumView::freqToX (float f) const
{
    return (float) getWidth() * std::log (jmax (20.0f, f) / 20.0f) / std::log (1000.0f);
}
float SpectrumView::xToFreq (float x) const
{
    return 20.0f * std::pow (1000.0f, jlimit (0.0f, 1.0f, x / (float) getWidth()));
}
float SpectrumView::eqDbToY (float db) const
{
    const float h = (float) getHeight() - 16.0f;
    return 8.0f + h * (0.5f - db / 36.0f);
}
float SpectrumView::yToEqDb (float y) const
{
    const float h = (float) getHeight() - 16.0f;
    return (0.5f - (y - 8.0f) / h) * 36.0f;
}
float SpectrumView::specDbToY (float db) const
{
    return jmap (db, -84.0f, 0.0f, (float) getHeight(), 0.0f);
}

void SpectrumView::rebuildGrid()
{
    if (getWidth() <= 0 || getHeight() <= 0) return;
    gridImage = Image (Image::ARGB, getWidth(), getHeight(), true);
    Graphics g (gridImage);
    g.setColour (Theme::panel);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);

    g.setFont (FontOptions (10.0f));
    for (float f : { 30.f, 50.f, 100.f, 200.f, 300.f, 500.f, 1000.f, 2000.f, 3000.f, 5000.f, 10000.f, 20000.f })
    {
        const float x = freqToX (f);
        const bool major = (f == 100.f || f == 1000.f || f == 10000.f);
        g.setColour (Theme::line.withAlpha (major ? 0.9f : 0.45f));
        g.drawVerticalLine ((int) x, 0.0f, (float) getHeight());
        g.setColour (Theme::textDim);
        const String t = f >= 1000.f ? String ((int) (f / 1000)) + "k" : String ((int) f);
        g.drawText (t, (int) x + 3, getHeight() - 14, 30, 12, Justification::left);
    }
    for (float db : { -12.f, -6.f, 0.f, 6.f, 12.f })
    {
        const float y = eqDbToY (db);
        g.setColour (Theme::line.withAlpha (db == 0.f ? 0.9f : 0.45f));
        g.drawHorizontalLine ((int) y, 0.0f, (float) getWidth());
        g.setColour (Theme::textDim);
        g.drawText ((db > 0 ? "+" : "") + String ((int) db), getWidth() - 30, (int) y - 12, 26, 12, Justification::right);
    }
}

void SpectrumView::updateAnalyser()
{
    const int n = proc.spectrumFifo.pull (pullBuf.data(), (int) pullBuf.size());
    if (n > 0)
    {
        if (n >= fftSize)
            std::copy (pullBuf.begin() + (n - fftSize), pullBuf.begin() + n, history.begin());
        else
        {
            std::move (history.begin() + n, history.end(), history.begin());
            std::copy (pullBuf.begin(), pullBuf.begin() + n, history.end() - n);
        }
    }

    std::fill (fftData.begin(), fftData.end(), 0.0f);
    std::copy (history.begin(), history.end(), fftData.begin());
    window.multiplyWithWindowingTable (fftData.data(), (size_t) fftSize);
    fft.performFrequencyOnlyForwardTransform (fftData.data());

    const float norm = 4.0f / (float) fftSize;
    for (int b = 0; b < numBins; ++b)
    {
        const float db = Decibels::gainToDecibels (fftData[(size_t) b] * norm, -120.0f);
        float& sm = smoothedDb[(size_t) b];
        sm = db > sm ? db : sm + 0.25f * (db - sm);
    }
}

Point<float> SpectrumView::nodePosition (int i) const
{
    const auto& nd = nodes[(size_t) i];
    const float f = proc.apvts.getRawParameterValue (nd.freqId)->load();
    const float g = nd.gainId ? proc.apvts.getRawParameterValue (nd.gainId)->load() : 0.0f;
    return { freqToX (f), eqDbToY (g) };
}

int SpectrumView::hitNode (Point<float> pt) const
{
    int best = -1; float bestD = 14.0f;
    for (int i = 0; i < (int) nodes.size(); ++i)
    {
        const float d = nodePosition (i).getDistanceFrom (pt);
        if (d < bestD) { bestD = d; best = i; }
    }
    return best;
}

void SpectrumView::setParam (const char* id, float value)
{
    if (auto* prm = proc.apvts.getParameter (id))
        prm->setValueNotifyingHost (prm->convertTo0to1 (value));
}

void SpectrumView::paint (Graphics& g)
{
    if (gridImage.isNull()) rebuildGrid();
    g.drawImageAt (gridImage, 0, 0);

    const double fs = jmax (1.0, proc.getCurrentSampleRate());
    const float w = (float) getWidth(), h = (float) getHeight();

    // --- spectrum (with +4.5 dB/oct tilt so a balanced mix reads roughly flat)
    Path spec;
    spec.startNewSubPath (0, h);
    for (float x = 0; x <= w; x += 2.0f)
    {
        const float f0 = xToFreq (x), f1 = xToFreq (x + 2.0f);
        int b0 = (int) (f0 / fs * fftSize), b1 = (int) (f1 / fs * fftSize);
        b0 = jlimit (1, numBins - 1, b0); b1 = jlimit (b0, numBins - 1, b1);
        float db = -120.0f;
        for (int b = b0; b <= b1; ++b) db = jmax (db, smoothedDb[(size_t) b]);
        db += 4.5f * std::log2 (jmax (20.0f, f0) / 1000.0f);
        spec.lineTo (x, jlimit (0.0f, h, specDbToY (db)));
    }
    spec.lineTo (w, h);
    spec.closeSubPath();
    g.setGradientFill (ColourGradient (Theme::accent2.withAlpha (0.35f), 0, 0, Theme::accent2.withAlpha (0.03f), 0, h, false));
    g.fillPath (spec);
    g.setColour (Theme::accent2.withAlpha (0.7f));
    g.strokePath (spec, PathStrokeType (1.0f));

    // --- EQ curve
    const auto settings = proc.getEqSettings();
    const auto coeffs = MasterEQ::makeCoeffs (settings, fs);
    const bool eqActive = proc.apvts.getRawParameterValue ("eqOn")->load() > 0.5f;
    Path curve;
    for (float x = 0; x <= w; x += 2.0f)
    {
        const float db = MasterEQ::magnitudeDb (coeffs, settings.hpOn, xToFreq (x), fs);
        const float y = jlimit (-10.0f, h + 10.0f, eqDbToY (db));
        if (x == 0) curve.startNewSubPath (x, y); else curve.lineTo (x, y);
    }
    g.setColour (eqActive ? Theme::accent : Theme::textDim);
    g.strokePath (curve, PathStrokeType (2.0f, PathStrokeType::curved, PathStrokeType::rounded));

    // --- nodes
    for (int i = 0; i < (int) nodes.size(); ++i)
    {
        const auto pt = nodePosition (i);
        const bool disabled = (i == 0 && ! settings.hpOn) || ! eqActive;
        const float r = (i == dragNode || i == hoverNode) ? 9.0f : 7.0f;
        g.setColour (nodes[(size_t) i].colour.withAlpha (disabled ? 0.35f : 1.0f));
        g.fillEllipse (pt.x - r, pt.y - r, r * 2, r * 2);
        g.setColour (Theme::bg);
        g.setFont (FontOptions (9.0f, Font::bold));
        g.drawText (nodes[(size_t) i].name, Rectangle<float> (pt.x - r, pt.y - r, r * 2, r * 2), Justification::centred);
    }

    // --- readout for dragged/hovered node
    const int shown = dragNode >= 0 ? dragNode : hoverNode;
    if (shown >= 0)
    {
        const auto& nd = nodes[(size_t) shown];
        String t = proc.apvts.getParameter (nd.freqId)->getCurrentValueAsText();
        if (nd.gainId) t << "   " << proc.apvts.getParameter (nd.gainId)->getCurrentValueAsText();
        if (nd.qId)    t << "   Q " << proc.apvts.getParameter (nd.qId)->getCurrentValueAsText();
        g.setColour (Theme::text);
        g.setFont (FontOptions (12.0f));
        g.drawText (t, 10, 6, 300, 16, Justification::left);
    }
    else
    {
        g.setColour (Theme::textDim);
        g.setFont (FontOptions (11.0f));
        g.drawText ("Drag nodes  |  wheel = Q  |  double-click = reset / HP on-off", 10, 6, 420, 16, Justification::left);
    }
}

void SpectrumView::mouseDown (const MouseEvent& e)
{
    dragNode = hitNode (e.position);
    if (dragNode < 0) return;
    const auto& nd = nodes[(size_t) dragNode];
    proc.apvts.getParameter (nd.freqId)->beginChangeGesture();
    if (nd.gainId) proc.apvts.getParameter (nd.gainId)->beginChangeGesture();
    if (dragNode == 0 && proc.apvts.getRawParameterValue ("hpOn")->load() < 0.5f) setParam ("hpOn", 1.0f);
}

void SpectrumView::mouseDrag (const MouseEvent& e)
{
    if (dragNode < 0) return;
    const auto& nd = nodes[(size_t) dragNode];
    setParam (nd.freqId, xToFreq (e.position.x));
    if (nd.gainId) setParam (nd.gainId, jlimit (-12.0f, 12.0f, yToEqDb (e.position.y)));
    repaint();
}

void SpectrumView::mouseUp (const MouseEvent&)
{
    if (dragNode < 0) return;
    const auto& nd = nodes[(size_t) dragNode];
    proc.apvts.getParameter (nd.freqId)->endChangeGesture();
    if (nd.gainId) proc.apvts.getParameter (nd.gainId)->endChangeGesture();
    dragNode = -1;
    repaint();
}

void SpectrumView::mouseMove (const MouseEvent& e)
{
    const int h = hitNode (e.position);
    if (h != hoverNode) { hoverNode = h; repaint(); }
    setMouseCursor (h >= 0 ? MouseCursor::DraggingHandCursor : MouseCursor::NormalCursor);
}

void SpectrumView::mouseDoubleClick (const MouseEvent& e)
{
    const int i = hitNode (e.position);
    if (i < 0) return;
    if (i == 0) setParam ("hpOn", proc.apvts.getRawParameterValue ("hpOn")->load() > 0.5f ? 0.0f : 1.0f);
    else setParam (nodes[(size_t) i].gainId, 0.0f);
}

void SpectrumView::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& wheel)
{
    const int i = hitNode (e.position);
    if (i < 0 || nodes[(size_t) i].qId == nullptr) return;
    auto* prm = proc.apvts.getParameter (nodes[(size_t) i].qId);
    const float q = prm->convertFrom0to1 (prm->getValue()) * (wheel.deltaY > 0 ? 1.12f : 1.0f / 1.12f);
    prm->beginChangeGesture();
    prm->setValueNotifyingHost (prm->convertTo0to1 (jlimit (0.2f, 8.0f, q)));
    prm->endChangeGesture();
}

//==============================================================================
// Meters
MeterPanel::MeterPanel (ApexMasterAudioProcessor& p) : proc (p)
{
    resetButton.onClick = [this] { proc.resetLoudnessRequest.store (true); };
    resetButton.setTooltip ("Reset integrated loudness");
    addAndMakeVisible (resetButton);
    startTimerHz (30);
}

void MeterPanel::resized()
{
    resetButton.setBounds (getWidth() - 62, getHeight() - 30, 54, 22);
}

void MeterPanel::tick()
{
    auto update = [this] (std::atomic<float>& a, float& v, float& hold, int& counter)
    {
        const float db = Decibels::gainToDecibels (a.exchange (0.0f), -100.0f);
        v = jmax (db, v - 1.5f);
        if (db >= hold) { hold = db; counter = 45; }
        else if (--counter <= 0) hold = jmax (-100.0f, hold - 0.8f);
    };
    for (int c = 0; c < 2; ++c)
    {
        update (proc.inPeak[c], in[c], inHold[c], holdCounter[c]);
        update (proc.outPeak[c], out[c], outHold[c], holdCounter[2 + c]);
    }
    gr = jmax (proc.maxGr.exchange (0.0f), gr * 0.85f);
    corr = corr + 0.2f * (proc.correlation.load() - corr);
    m = proc.lufsM.load(); s = proc.lufsS.load(); i = proc.lufsI.load();
}

void MeterPanel::drawBar (Graphics& g, Rectangle<float> r, float db, float hold, Colour c)
{
    g.setColour (Theme::bg);
    g.fillRoundedRectangle (r, 2.0f);
    auto norm = [] (float d) { return jlimit (0.0f, 1.0f, (d + 60.0f) / 60.0f); };
    auto fill = r.withTop (r.getBottom() - r.getHeight() * norm (db));
    g.setGradientFill (ColourGradient (c, 0, r.getBottom(), db > -0.5f ? Theme::danger : c.brighter (0.4f), 0, r.getY(), false));
    g.fillRect (fill);
    const float hy = r.getBottom() - r.getHeight() * norm (hold);
    g.setColour (hold > -0.1f ? Theme::danger : Theme::text);
    g.fillRect (r.getX(), hy - 1.0f, r.getWidth(), 2.0f);
}

void MeterPanel::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (Theme::panel);
    g.fillRoundedRectangle (r, 6.0f);
    r.reduce (10, 10);

    auto lufsText = [] (float v) { return v <= -99.0f ? String ("-inf") : String (v, 1); };

    // header labels
    g.setFont (FontOptions (10.0f, Font::bold));
    g.setColour (Theme::textDim);
    auto labels = r.removeFromTop (14);
    const float colW = r.getWidth() / 3.0f;
    g.drawText ("IN",  labels.removeFromLeft (colW), Justification::centred);
    g.drawText ("OUT", labels.removeFromLeft (colW), Justification::centred);
    g.drawText ("GR",  labels, Justification::centred);

    auto bars = r.removeFromTop (220);
    auto colIn = bars.removeFromLeft (colW), colOut = bars.removeFromLeft (colW), colGr = bars;
    auto twoBars = [&] (Rectangle<float> col, const float* v, const float* hold, Colour c)
    {
        auto b = col.withSizeKeepingCentre (30, col.getHeight());
        drawBar (g, b.removeFromLeft (14), v[0], hold[0], c);
        b.removeFromLeft (2);
        drawBar (g, b.removeFromLeft (14), v[1], hold[1], c);
    };
    twoBars (colIn, in, inHold, Theme::accent2);
    twoBars (colOut, out, outHold, Theme::accent);

    {
        auto b = colGr.withSizeKeepingCentre (14, colGr.getHeight());
        g.setColour (Theme::bg);
        g.fillRoundedRectangle (b, 2.0f);
        g.setColour (Theme::danger.withAlpha (0.85f));
        g.fillRect (b.withHeight (b.getHeight() * jlimit (0.0f, 1.0f, gr / 12.0f)));
    }

    // numeric peaks
    g.setFont (FontOptions (11.0f));
    auto nums = r.removeFromTop (18);
    g.setColour (Theme::text);
    g.drawText (lufsText (jmax (inHold[0], inHold[1])), nums.removeFromLeft (colW), Justification::centred);
    const float oh = jmax (outHold[0], outHold[1]);
    g.setColour (oh > -0.1f ? Theme::danger : Theme::text);
    g.drawText (lufsText (oh), nums.removeFromLeft (colW), Justification::centred);
    g.setColour (Theme::text);
    g.drawText (gr > 0.05f ? "-" + String (gr, 1) : String ("0.0"), nums, Justification::centred);

    r.removeFromTop (10);
    g.setColour (Theme::line);
    g.drawHorizontalLine ((int) r.getY(), r.getX(), r.getRight());
    r.removeFromTop (8);

    // loudness
    auto drawLufs = [&] (const String& name, float v, float size, Colour c)
    {
        g.setColour (Theme::textDim);
        g.setFont (FontOptions (10.0f, Font::bold));
        g.drawText (name, r.removeFromTop (12), Justification::left);
        g.setColour (c);
        g.setFont (FontOptions (size, Font::bold));
        auto row = r.removeFromTop (size + 4);
        g.drawText (lufsText (v), row, Justification::left);
        g.setFont (FontOptions (10.0f));
        g.setColour (Theme::textDim);
        g.drawText ("LUFS", row, Justification::right);
        r.removeFromTop (4);
    };
    drawLufs ("INTEGRATED", i, 24.0f, Theme::accent);
    drawLufs ("SHORT-TERM", s, 18.0f, Theme::text);
    drawLufs ("MOMENTARY", m, 14.0f, Theme::text);

    // correlation
    g.setColour (Theme::textDim);
    g.setFont (FontOptions (10.0f, Font::bold));
    g.drawText ("CORRELATION", r.removeFromTop (14), Justification::left);
    auto cb = r.removeFromTop (8);
    g.setColour (Theme::bg);
    g.fillRoundedRectangle (cb, 3.0f);
    const float cx = cb.getCentreX(), px = jmap (jlimit (-1.0f, 1.0f, corr), -1.0f, 1.0f, cb.getX(), cb.getRight());
    g.setColour (corr < 0.0f ? Theme::danger : Theme::accent2);
    g.fillRect (Rectangle<float>::leftTopRightBottom (jmin (cx, px), cb.getY(), jmax (cx, px), cb.getBottom()));
    g.setColour (Theme::textDim);
    g.setFont (FontOptions (9.0f));
    auto lbl = r.removeFromTop (12);
    g.drawText ("-1", lbl, Justification::left);
    g.drawText ("0", lbl, Justification::centred);
    g.drawText ("+1", lbl, Justification::right);
}

//==============================================================================
// Module pages
ModulePage::ModulePage (const String& t, AudioProcessorValueTreeState& s, const String& powerId)
    : state (s), title (t), power (s, powerId, "ACTIVE")
{
    addAndMakeVisible (power);
}

Knob* ModulePage::addKnob (int column, const String& id, const String& name, Colour c)
{
    auto k = std::make_unique<Knob> (state, id, name, c);
    auto* raw = k.get();
    addItem (column, raw);
    owned.push_back (std::move (k));
    return raw;
}

void ModulePage::addItem (int column, Component* item)
{
    if ((int) columns.size() <= column) columns.resize ((size_t) column + 1);
    columns[(size_t) column].items.push_back (item);
    addAndMakeVisible (item);
}

Component* ModulePage::addOwnedItem (int column, std::unique_ptr<Component> item)
{
    auto* raw = item.get();
    addItem (column, raw);
    owned.push_back (std::move (item));
    return raw;
}

void ModulePage::setColumnTitle (int column, const String& t)
{
    if ((int) columns.size() <= column) columns.resize ((size_t) column + 1);
    columns[(size_t) column].title = t;
}

void ModulePage::paint (Graphics& g)
{
    g.setColour (Theme::panel);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);
    g.setColour (Theme::text);
    g.setFont (FontOptions (14.0f, Font::bold));
    g.drawText (title, 14, 8, 300, 20, Justification::left);

    if (columns.empty()) return;
    const float colW = content.getWidth() / (float) columns.size();
    g.setFont (FontOptions (10.0f, Font::bold));
    for (size_t c = 0; c < columns.size(); ++c)
    {
        const float x = content.getX() + colW * (float) c;
        if (c > 0)
        {
            g.setColour (Theme::line);
            g.drawVerticalLine ((int) x, (float) content.getY(), (float) content.getBottom());
        }
        g.setColour (Theme::accent.withAlpha (0.8f));
        g.drawText (columns[c].title, (int) x, content.getY(), (int) colW, 14, Justification::centred);
    }
}

void ModulePage::resized()
{
    auto r = getLocalBounds().reduced (10, 6);
    auto top = r.removeFromTop (26);
    power.setBounds (top.removeFromRight (88).reduced (0, 2));
    content = r;

    if (columns.empty()) return;
    const int colW = r.getWidth() / (int) columns.size();
    int knobH = 120; // uniform knob size across columns
    for (auto& column : columns)
    {
        int fixed = 0, flexible = 0;
        for (auto* it : column.items) { if (dynamic_cast<Knob*> (it)) ++flexible; else fixed += 30; }
        if (flexible > 0) knobH = jmin (knobH, (r.getHeight() - 16 - fixed) / flexible);
    }
    for (size_t c = 0; c < columns.size(); ++c)
    {
        auto col = Rectangle<int> (r.getX() + colW * (int) c, r.getY() + 16, colW, r.getHeight() - 16);
        auto& items = columns[c].items;
        int fixed = 0, flexible = 0;
        for (auto* it : items) { if (dynamic_cast<Knob*> (it)) ++flexible; else fixed += 30; }
        const int total = fixed + knobH * flexible;
        col = col.withSizeKeepingCentre (col.getWidth(), total);
        for (auto* it : items)
        {
            if (dynamic_cast<Knob*> (it)) it->setBounds (col.removeFromTop (knobH).withSizeKeepingCentre (jmin (colW - 8, 96), knobH));
            else it->setBounds (col.removeFromTop (30).withSizeKeepingCentre (jmin (colW - 16, 110), 24));
        }
    }
}

DynamicsPage::DynamicsPage (ApexMasterAudioProcessor& p)
    : ModulePage ("MULTIBAND DYNAMICS", p.apvts, "mbOn"), proc (p)
{
    x1 = std::make_unique<Knob> (p.apvts, "xover1", "Low / Mid", Theme::accent2);
    x2 = std::make_unique<Knob> (p.apvts, "xover2", "Mid / High", Theme::accent2);
    addAndMakeVisible (*x1);
    addAndMakeVisible (*x2);

    const char* pre[] = { "lo", "mi", "hi" };
    const char* suf[] = { "Thr", "Ratio", "Att", "Rel", "Makeup" };
    const char* nm[]  = { "Threshold", "Ratio", "Attack", "Release", "Makeup" };
    for (int b = 0; b < 3; ++b)
        for (int k = 0; k < 5; ++k)
        {
            bandKnobs.push_back (std::make_unique<Knob> (p.apvts, String (pre[b]) + suf[k], b == 0 ? nm[k] : "", Theme::accent));
            addAndMakeVisible (*bandKnobs.back());
        }
    startTimerHz (30);
}

void DynamicsPage::timerCallback()
{
    for (int b = 0; b < 3; ++b) grDisplay[b] = jmax (proc.bandGr[b].exchange (0.0f), grDisplay[b] * 0.85f);
    repaint();
}

void DynamicsPage::resized()
{
    ModulePage::resized();
    auto r = content;
    auto left = r.removeFromLeft (110).withTrimmedTop (8);
    const int kh = jmin (110, left.getHeight() / 2);
    x1->setBounds (left.removeFromTop (kh).reduced (6, 0));
    x2->setBounds (left.removeFromTop (kh).reduced (6, 0));

    r.removeFromLeft (10);
    r.removeFromLeft (60); // band names
    auto grArea = r.removeFromRight (150);
    juce::ignoreUnused (grArea);
    const int rowH = r.getHeight() / 3;
    for (int b = 0; b < 3; ++b)
    {
        auto row = Rectangle<int> (r.getX(), r.getY() + rowH * b, r.getWidth(), rowH);
        rows[b] = Rectangle<int> (content.getX() + 120, row.getY(), content.getWidth() - 120, rowH);
        const int kw = row.getWidth() / 5;
        for (int k = 0; k < 5; ++k)
        {
            auto* knob = bandKnobs[(size_t) (b * 5 + k)].get();
            auto cell = row.removeFromLeft (kw);
            knob->setBounds (b == 0 ? cell : cell.withTrimmedTop (2));
        }
    }
}

void DynamicsPage::paint (Graphics& g)
{
    ModulePage::paint (g);
    const char* names[] = { "LOW", "MID", "HIGH" };
    for (int b = 0; b < 3; ++b)
    {
        auto row = rows[b];
        if (b > 0)
        {
            g.setColour (Theme::line);
            g.drawHorizontalLine (row.getY(), (float) row.getX(), (float) row.getRight());
        }
        g.setColour (Theme::accent);
        g.setFont (FontOptions (12.0f, Font::bold));
        g.drawText (names[b], row.getX(), row.getY(), 60, row.getHeight(), Justification::centredLeft);

        auto bar = Rectangle<float> ((float) row.getRight() - 140.0f, row.getCentreY() - 6.0f, 110.0f, 12.0f);
        g.setColour (Theme::bg);
        g.fillRoundedRectangle (bar, 3.0f);
        g.setColour (Theme::danger.withAlpha (0.85f));
        g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * jlimit (0.0f, 1.0f, grDisplay[b] / 12.0f)), 3.0f);
        g.setColour (Theme::textDim);
        g.setFont (FontOptions (10.0f));
        g.drawText ("GR " + String (-grDisplay[b], 1) + " dB", bar.translated (0, 12).toNearestInt(), Justification::centredLeft);
    }
    g.setColour (Theme::line);
    g.drawVerticalLine (content.getX() + 112, (float) content.getY(), (float) content.getBottom());
    g.setColour (Theme::accent2.withAlpha (0.8f));
    g.setFont (FontOptions (10.0f, Font::bold));
    g.drawText ("CROSSOVER", content.getX(), content.getY() - 14, 110, 12, Justification::centred);
}

//==============================================================================
// Editor
ApexMasterAudioProcessorEditor::ApexMasterAudioProcessorEditor (ApexMasterAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p), spectrum (p), meters (p)
{
    setLookAndFeel (&lnf);

    for (int i = 0; i < p.getNumPrograms(); ++i) presetBox.addItem (p.getProgramName (i), i + 1);
    presetBox.setSelectedId (p.getCurrentProgram() + 1, dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);

    inputKnob = std::make_unique<Knob> (p.apvts, "inGain", "Input", Theme::accent2);
    inputKnob->slider.setSliderStyle (Slider::LinearBar);
    inputKnob->slider.setColour (Slider::trackColourId, Theme::accent2.withAlpha (0.45f));
    inputKnob->slider.setColour (Slider::backgroundColourId, Theme::panelHi);
    addAndMakeVisible (*inputKnob);

    addAndMakeVisible (spectrum);
    addAndMakeVisible (meters);

    // --- pages
    auto eqPage = std::make_unique<ModulePage> ("EQUALIZER", p.apvts, "eqOn");
    eqPage->setColumnTitle (0, "LOW CUT");
    eqPage->addOwnedItem (0, std::make_unique<PowerButton> (p.apvts, "hpOn", "HP"));
    eqPage->addKnob (0, "hpFreq", "Freq", Colour (0xff9aa5b1));
    eqPage->setColumnTitle (1, "LOW SHELF");
    eqPage->addKnob (1, "lsFreq", "Freq", Colour (0xff6ea8ff));
    eqPage->addKnob (1, "lsGain", "Gain", Colour (0xff6ea8ff));
    eqPage->setColumnTitle (2, "BELL 1");
    eqPage->addKnob (2, "p1Freq", "Freq", Theme::accent2);
    eqPage->addKnob (2, "p1Gain", "Gain", Theme::accent2);
    eqPage->addKnob (2, "p1Q", "Q", Theme::accent2);
    eqPage->setColumnTitle (3, "BELL 2");
    eqPage->addKnob (3, "p2Freq", "Freq");
    eqPage->addKnob (3, "p2Gain", "Gain");
    eqPage->addKnob (3, "p2Q", "Q");
    eqPage->setColumnTitle (4, "HIGH SHELF");
    eqPage->addKnob (4, "hsFreq", "Freq", Colour (0xffff7ab8));
    eqPage->addKnob (4, "hsGain", "Gain", Colour (0xffff7ab8));
    pages.push_back (std::move (eqPage));

    pages.push_back (std::make_unique<DynamicsPage> (p));

    auto exPage = std::make_unique<ModulePage> ("HARMONIC EXCITER", p.apvts, "exOn");
    auto* modeBox = static_cast<ComboBox*> (exPage->addOwnedItem (0, std::make_unique<ComboBox>()));
    modeBox->addItemList ({ "Tape", "Tube", "Warm" }, 1);
    exPage->setColumnTitle (0, "MODE");
    modeAttachment = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, "exMode", *modeBox);
    exPage->setColumnTitle (1, "DRIVE");  exPage->addKnob (1, "exDrive", "Drive");
    exPage->setColumnTitle (2, "BAND");   exPage->addKnob (2, "exFreq", "Above", Theme::accent2);
    exPage->setColumnTitle (3, "AMOUNT"); exPage->addKnob (3, "exMix", "Mix");
    pages.push_back (std::move (exPage));

    auto imPage = std::make_unique<ModulePage> ("STEREO IMAGER", p.apvts, "imOn");
    imPage->setColumnTitle (0, "WIDTH");     imPage->addKnob (0, "imWidth", "Width", Theme::accent2);
    imPage->setColumnTitle (1, "MONO BASS"); imPage->addKnob (1, "imMono", "Below");
    pages.push_back (std::move (imPage));

    auto maxPage = std::make_unique<ModulePage> ("MAXIMIZER", p.apvts, "maxOn");
    maxPage->setColumnTitle (0, "GAIN");    maxPage->addKnob (0, "maxGain", "Push");
    maxPage->setColumnTitle (1, "CEILING"); maxPage->addKnob (1, "maxCeiling", "Ceiling", Theme::danger);
    maxPage->setColumnTitle (2, "RELEASE"); maxPage->addKnob (2, "maxRelease", "Release", Theme::accent2);
    pages.push_back (std::move (maxPage));

    const char* tabNames[] = { "EQ", "DYNAMICS", "EXCITER", "IMAGER", "MAXIMIZER" };
    for (int t = 0; t < 5; ++t)
    {
        auto* b = tabs.add (new TextButton (tabNames[t]));
        b->setRadioGroupId (1001);
        b->setClickingTogglesState (true);
        b->onClick = [this, t] { showPage (t); };
        addAndMakeVisible (b);
        addChildComponent (*pages[(size_t) t]);
    }

    setResizable (true, true);
    setResizeLimits (960, 620, 1600, 1000);
    setSize (1080, 660);
    showPage (0);
}

ApexMasterAudioProcessorEditor::~ApexMasterAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void ApexMasterAudioProcessorEditor::showPage (int index)
{
    currentPage = index;
    for (size_t i = 0; i < pages.size(); ++i) pages[i]->setVisible ((int) i == index);
    tabs[index]->setToggleState (true, dontSendNotification);
}

void ApexMasterAudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll (Theme::bg);
    g.setFont (FontOptions (22.0f, Font::bold));
    g.setColour (Theme::accent);
    g.drawText ("APEX", 16, 12, 70, 28, Justification::left);
    g.setColour (Theme::text);
    g.drawText ("MASTER", 76, 12, 110, 28, Justification::left);
    g.setColour (Theme::textDim);
    g.setFont (FontOptions (10.0f));
    g.drawText ("EQ  >  DYNAMICS  >  EXCITER  >  IMAGER  >  MAXIMIZER", 190, 20, 320, 14, Justification::left);
}

void ApexMasterAudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced (12);
    auto header = r.removeFromTop (40);
    inputKnob->setBounds (header.removeFromRight (150).withTrimmedTop (0));
    header.removeFromRight (16);
    presetBox.setBounds (header.removeFromRight (240).withSizeKeepingCentre (240, 26));

    r.removeFromTop (8);
    meters.setBounds (r.removeFromRight (180));
    r.removeFromRight (10);

    const int specH = jmax (200, (int) (r.getHeight() * 0.42f));
    spectrum.setBounds (r.removeFromTop (specH));
    r.removeFromTop (8);

    auto tabRow = r.removeFromTop (30);
    const int tw = tabRow.getWidth() / 5;
    for (int t = 0; t < 5; ++t) tabs[t]->setBounds (tabRow.removeFromLeft (tw).reduced (2, 0));
    r.removeFromTop (6);
    for (auto& pg : pages) pg->setBounds (r);
}
