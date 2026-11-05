#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>

// 5-band mastering EQ: high-pass, low shelf, two bells, high shelf.
class MasterEQ
{
public:
    struct Settings
    {
        bool  hpOn   = false; float hpFreq = 25.0f;
        float lsFreq = 100.0f;  float lsGain = 0.0f;
        float p1Freq = 400.0f;  float p1Gain = 0.0f; float p1Q = 1.0f;
        float p2Freq = 3000.0f; float p2Gain = 0.0f; float p2Q = 1.0f;
        float hsFreq = 10000.0f; float hsGain = 0.0f;

        bool operator== (const Settings& o) const
        {
            return hpOn == o.hpOn && hpFreq == o.hpFreq && lsFreq == o.lsFreq && lsGain == o.lsGain
                && p1Freq == o.p1Freq && p1Gain == o.p1Gain && p1Q == o.p1Q
                && p2Freq == o.p2Freq && p2Gain == o.p2Gain && p2Q == o.p2Q
                && hsFreq == o.hsFreq && hsGain == o.hsGain;
        }
        bool operator!= (const Settings& o) const { return ! (*this == o); }
    };

    using Coeffs = juce::dsp::IIR::Coefficients<float>;
    using CoeffArray = std::array<Coeffs::Ptr, 5>;

    static CoeffArray makeCoeffs (const Settings& s, double fs)
    {
        const float nyq = (float) fs * 0.49f;
        auto clampF = [nyq] (float f) { return juce::jlimit (10.0f, nyq, f); };
        CoeffArray c;
        c[0] = Coeffs::makeHighPass  (fs, clampF (s.hpFreq), 0.7071f);
        c[1] = Coeffs::makeLowShelf  (fs, clampF (s.lsFreq), 0.7071f, juce::Decibels::decibelsToGain (s.lsGain));
        c[2] = Coeffs::makePeakFilter (fs, clampF (s.p1Freq), s.p1Q, juce::Decibels::decibelsToGain (s.p1Gain));
        c[3] = Coeffs::makePeakFilter (fs, clampF (s.p2Freq), s.p2Q, juce::Decibels::decibelsToGain (s.p2Gain));
        c[4] = Coeffs::makeHighShelf (fs, clampF (s.hsFreq), 0.7071f, juce::Decibels::decibelsToGain (s.hsGain));
        return c;
    }

    // Magnitude (dB) of the whole EQ at one frequency - used by the GUI curve.
    static float magnitudeDb (const CoeffArray& c, bool hpOn, double freq, double fs)
    {
        double mag = 1.0;
        for (size_t i = hpOn ? 0 : 1; i < c.size(); ++i)
            mag *= c[i]->getMagnitudeForFrequency (freq, fs);
        return (float) juce::Decibels::gainToDecibels (mag, -100.0);
    }

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        for (auto& f : filters) f.prepare (spec);
        firstUpdate = true;
        reset();
    }

    void reset() { for (auto& f : filters) f.reset(); }

    void setSettings (const Settings& s)
    {
        if (! firstUpdate && s == current) return;
        firstUpdate = false;
        current = s;
        auto c = makeCoeffs (s, sampleRate);
        for (size_t i = 0; i < filters.size(); ++i)
            *filters[i].state = *c[i];
    }

    void process (juce::dsp::ProcessContextReplacing<float>& ctx)
    {
        if (current.hpOn) filters[0].process (ctx);
        if (current.lsGain != 0.0f) filters[1].process (ctx);
        if (current.p1Gain != 0.0f) filters[2].process (ctx);
        if (current.p2Gain != 0.0f) filters[3].process (ctx);
        if (current.hsGain != 0.0f) filters[4].process (ctx);
    }

private:
    using Dup = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, Coeffs>;
    std::array<Dup, 5> filters;
    Settings current;
    bool firstUpdate = true;
    double sampleRate = 44100.0;
};
