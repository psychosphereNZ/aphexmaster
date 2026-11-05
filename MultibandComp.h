#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>

// Stereo-linked feed-forward compressor with soft knee (smoothing in the dB domain).
class BandCompressor
{
public:
    void prepare (double fs) { sampleRate = fs; grDb = 0.0f; }

    void setParams (float thresholdDb, float ratio_, float attackMs, float releaseMs, float makeupDb)
    {
        threshold = thresholdDb;
        ratio = juce::jmax (1.0f, ratio_);
        attCoeff = std::exp (-1.0f / (juce::jmax (0.05f, attackMs) * 0.001f * (float) sampleRate));
        relCoeff = std::exp (-1.0f / (juce::jmax (1.0f, releaseMs) * 0.001f * (float) sampleRate));
        makeup = juce::Decibels::decibelsToGain (makeupDb);
    }

    // channels: array of write pointers, processed in place. Returns max gain reduction (dB, positive).
    float process (float* const* ch, int numCh, int n)
    {
        float maxGr = 0.0f;
        for (int i = 0; i < n; ++i)
        {
            float peak = 0.0f;
            for (int c = 0; c < numCh; ++c) peak = juce::jmax (peak, std::abs (ch[c][i]));

            const float lvl = juce::Decibels::gainToDecibels (peak, -120.0f);
            const float target = staticCurve (lvl) - lvl; // <= 0
            const float coeff = target < grDb ? attCoeff : relCoeff;
            grDb = target + coeff * (grDb - target);

            const float g = juce::Decibels::decibelsToGain (grDb) * makeup;
            for (int c = 0; c < numCh; ++c) ch[c][i] *= g;
            maxGr = juce::jmax (maxGr, -grDb);
        }
        return maxGr;
    }

private:
    float staticCurve (float x) const
    {
        const float knee = 6.0f;
        const float over = x - threshold;
        if (2.0f * over < -knee) return x;
        if (2.0f * std::abs (over) <= knee)
        {
            const float t = over + knee * 0.5f;
            return x + (1.0f / ratio - 1.0f) * t * t / (2.0f * knee);
        }
        return threshold + over / ratio;
    }

    double sampleRate = 44100.0;
    float threshold = 0.0f, ratio = 1.0f, attCoeff = 0.0f, relCoeff = 0.0f, makeup = 1.0f;
    float grDb = 0.0f;
};

// 3-band compressor with phase-coherent Linkwitz-Riley (LR4) crossovers.
class MultibandComp
{
public:
    struct BandParams { float threshold, ratio, attack, release, makeup; bool solo = false; };

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        numCh = (int) spec.numChannels;
        for (auto* f : { &lr1, &lr2, &ap2 }) f->prepare (spec);
        lr1.setType (juce::dsp::LinkwitzRileyFilterType::lowpass);
        lr2.setType (juce::dsp::LinkwitzRileyFilterType::lowpass);
        ap2.setType (juce::dsp::LinkwitzRileyFilterType::allpass);
        for (auto& b : bands) b.setSize (numCh, (int) spec.maximumBlockSize);
        for (auto& c : comps) c.prepare (spec.sampleRate);
        nyq = (float) spec.sampleRate * 0.45f;
    }

    void reset() { lr1.reset(); lr2.reset(); ap2.reset(); }

    void setCrossovers (float f1, float f2)
    {
        f1 = juce::jlimit (20.0f, nyq, f1);
        f2 = juce::jlimit (f1 * 1.5f, nyq, f2);
        lr1.setCutoffFrequency (f1);
        lr2.setCutoffFrequency (f2);
        ap2.setCutoffFrequency (f2);
    }

    void setBand (int i, const BandParams& p)
    {
        comps[(size_t) i].setParams (p.threshold, p.ratio, p.attack, p.release, p.makeup);
    }

    void process (juce::AudioBuffer<float>& buffer)
    {
        const int n = buffer.getNumSamples();
        const int chs = juce::jmin (numCh, buffer.getNumChannels());

        for (int c = 0; c < chs; ++c)
        {
            const float* in = buffer.getReadPointer (c);
            float* lo = bands[0].getWritePointer (c);
            float* mi = bands[1].getWritePointer (c);
            float* hi = bands[2].getWritePointer (c);
            for (int i = 0; i < n; ++i)
            {
                float low, rest, mid, high;
                lr1.processSample (c, in[i], low, rest);
                lr2.processSample (c, rest, mid, high);
                lo[i] = ap2.processSample (c, low);
                mi[i] = mid;
                hi[i] = high;
            }
        }

        for (size_t b = 0; b < 3; ++b)
            gainReduction[b] = comps[b].process (bands[b].getArrayOfWritePointers(), chs, n);

        for (int c = 0; c < chs; ++c)
        {
            float* out = buffer.getWritePointer (c);
            const float* lo = bands[0].getReadPointer (c);
            const float* mi = bands[1].getReadPointer (c);
            const float* hi = bands[2].getReadPointer (c);
            for (int i = 0; i < n; ++i) out[i] = lo[i] + mi[i] + hi[i];
        }
    }

    std::array<float, 3> gainReduction { 0, 0, 0 };

private:
    int numCh = 2;
    float nyq = 20000.0f;
    juce::dsp::LinkwitzRileyFilter<float> lr1, lr2, ap2;
    std::array<juce::AudioBuffer<float>, 3> bands;
    std::array<BandCompressor, 3> comps;
};
