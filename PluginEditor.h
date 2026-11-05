#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "PluginProcessor.h"

namespace Theme
{
    const juce::Colour bg        { 0xff0e1116 };
    const juce::Colour panel     { 0xff161b22 };
    const juce::Colour panelHi   { 0xff1f2630 };
    const juce::Colour line      { 0xff2a3340 };
    const juce::Colour text      { 0xffd7dde5 };
    const juce::Colour textDim   { 0xff7d8895 };
    const juce::Colour accent    { 0xffffb547 }; // amber
    const juce::Colour accent2   { 0xff3fd0c9 }; // teal
    const juce::Colour danger    { 0xffff5a5f };
}

//==============================================================================
class ApexLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ApexLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float start, float end, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
};

//==============================================================================
class Knob : public juce::Component
{
public:
    Knob (juce::AudioProcessorValueTreeState& s, const juce::String& paramId, const juce::String& title,
          juce::Colour colour = Theme::accent);
    void resized() override;

    juce::Slider slider;
    juce::Label label;
private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

class PowerButton : public juce::ToggleButton
{
public:
    PowerButton (juce::AudioProcessorValueTreeState& s, const juce::String& paramId, const juce::String& text);
private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
};

//==============================================================================
// Spectrum analyser with EQ curve and draggable band nodes.
class SpectrumView : public juce::Component, private juce::Timer
{
public:
    explicit SpectrumView (ApexMasterAudioProcessor&);
    ~SpectrumView() override { stopTimer(); }
    void paint (juce::Graphics&) override;
    void resized() override { rebuildGrid(); }
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    void updateAnalyser(); // public so the test harness can drive it without a message loop

private:
    void timerCallback() override { updateAnalyser(); repaint(); }
    void rebuildGrid();
    float freqToX (float f) const;
    float xToFreq (float x) const;
    float eqDbToY (float db) const;
    float yToEqDb (float y) const;
    float specDbToY (float db) const;

    struct Node { const char* freqId; const char* gainId; const char* qId; juce::Colour colour; const char* name; };
    juce::Point<float> nodePosition (int i) const;
    int hitNode (juce::Point<float>) const;
    void setParam (const char* id, float value);

    ApexMasterAudioProcessor& proc;
    static constexpr int fftOrder = 12, fftSize = 1 << fftOrder, numBins = fftSize / 2;
    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window { (size_t) fftSize, juce::dsp::WindowingFunction<float>::hann };
    std::vector<float> history = std::vector<float> ((size_t) fftSize, 0.0f);
    std::vector<float> fftData = std::vector<float> ((size_t) fftSize * 2, 0.0f);
    std::vector<float> smoothedDb = std::vector<float> ((size_t) numBins, -120.0f);
    std::vector<float> pullBuf = std::vector<float> ((size_t) SpectrumFifo::size, 0.0f);

    std::vector<Node> nodes;
    int dragNode = -1, hoverNode = -1;
    juce::Image gridImage;
};

//==============================================================================
class MeterPanel : public juce::Component, private juce::Timer
{
public:
    explicit MeterPanel (ApexMasterAudioProcessor&);
    ~MeterPanel() override { stopTimer(); }
    void paint (juce::Graphics&) override;
    void resized() override;
    void tick(); // public for the test harness

private:
    void timerCallback() override { tick(); repaint(); }
    void drawBar (juce::Graphics&, juce::Rectangle<float>, float db, float hold, juce::Colour);

    ApexMasterAudioProcessor& proc;
    float in[2] { -100, -100 }, out[2] { -100, -100 }, inHold[2] { -100, -100 }, outHold[2] { -100, -100 };
    int holdCounter[4] {};
    float gr = 0.0f, corr = 1.0f, m = -100, s = -100, i = -100;
    juce::TextButton resetButton { "RESET" };
};

//==============================================================================
// Generic module page: columns of controls.
class ModulePage : public juce::Component
{
public:
    ModulePage (const juce::String& title, juce::AudioProcessorValueTreeState& s, const juce::String& powerId);
    void paint (juce::Graphics&) override;
    void resized() override;

    struct Column { juce::String title; std::vector<juce::Component*> items; };
    Knob* addKnob (int column, const juce::String& id, const juce::String& name, juce::Colour c = Theme::accent);
    void addItem (int column, juce::Component* item);
    juce::Component* addOwnedItem (int column, std::unique_ptr<juce::Component> item);
    void setColumnTitle (int column, const juce::String& t);

protected:
    juce::AudioProcessorValueTreeState& state;
    juce::String title;
    PowerButton power;
    std::vector<Column> columns;
    std::vector<std::unique_ptr<juce::Component>> owned;
    juce::Rectangle<int> content;
};

class DynamicsPage : public ModulePage, private juce::Timer
{
public:
    DynamicsPage (ApexMasterAudioProcessor& p);
    ~DynamicsPage() override { stopTimer(); }
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    ApexMasterAudioProcessor& proc;
    std::vector<std::unique_ptr<Knob>> bandKnobs;
    std::unique_ptr<Knob> x1, x2;
    float grDisplay[3] {};
    juce::Rectangle<int> rows[3];
};

//==============================================================================
class ApexMasterAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit ApexMasterAudioProcessorEditor (ApexMasterAudioProcessor&);
    ~ApexMasterAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

    void showPage (int index);
    SpectrumView& getSpectrum() { return spectrum; }
    MeterPanel& getMeters() { return meters; }

private:
    ApexMasterAudioProcessor& proc;
    ApexLookAndFeel lnf;

    juce::ComboBox presetBox;
    std::unique_ptr<Knob> inputKnob;
    SpectrumView spectrum;
    MeterPanel meters;

    juce::OwnedArray<juce::TextButton> tabs;
    std::vector<std::unique_ptr<juce::Component>> pages;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment;
    int currentPage = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ApexMasterAudioProcessorEditor)
};
