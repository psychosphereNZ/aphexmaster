#pragma once
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <cmath>
#include "Biquad.h"

// Harmonic exciter: isolates the band above "freq", saturates it at 2x oversampling
// and blends only the generated harmonics back onto the (latency-aligned) dry signal.
class Exciter
{
public:
    enum Mode { Tape = 0, Tube, Warm };

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        numCh = (int) spec.numChannels;
        sampleRate = spec.sampleRate;

        oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
            (size_t) numCh, 1, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
        oversampler->initProcessing (spec.maximumBlockSize);
        latency = (int) std::lround (oversampler->getLatencyInSamples());

        band.setSize (numCh, (int) spec.maximumBlockSize);
        hp.assign ((size_t) numCh, Biquad());
        dcX.assign ((size_t) numCh, 0.0f);
        dcY.assign ((size_t) numCh, 0.0f);

        dryDelay.prepare (spec);
        dryDelay.setMaximumDelayInSamples (juce::jmax (1, latency + 1));
        dryDelay.setDelay ((float) latency);
        lastFreq = -1.0f;
    }

    int getLatency() const { return latency; }

    void reset()
    {
        if (oversampler) oversampler->reset();
        dryDelay.reset();
        for (auto& f : hp) f.reset();
        std::fill (dcX.begin(), dcX.end(), 0.0f);
        std::fill (dcY.begin(), dcY.end(), 0.0f);
    }

    void process (juce::AudioBuffer<float>& buffer, bool active, float driveDb, float mix, float freq, int mode)
    {
        const int n = buffer.getNumSamples();
        const int chs = juce::jmin (numCh, buffer.getNumChannels());

        if (freq != lastFreq)
        {
            lastFreq = freq;
            for (auto& f : hp) f.setHighPass (sampleRate, juce::jlimit (200.0, sampleRate * 0.45, (double) freq));
        }

        // isolate excitation band (before delaying the dry path)
        for (int c = 0; c < chs; ++c)
        {
            const float* in = buffer.getReadPointer (c);
            float* b = band.getWritePointer (c);
            for (int i = 0; i < n; ++i) b[i] = (float) hp[(size_t) c].process (in[i]);
        }

        // dry path is always delayed so plugin latency stays constant
        {
            juce::dsp::AudioBlock<float> blk (buffer.getArrayOfWritePointers(), (size_t) chs, (size_t) n);
            juce::dsp::ProcessContextReplacing<float> ctx (blk);
            dryDelay.process (ctx);
        }

        if (! active || mix <= 0.0001f) return;

        const float d = juce::Decibels::decibelsToGain (driveDb);
        juce::dsp::AudioBlock<float> bandBlk (band.getArrayOfWritePointers(), (size_t) chs, (size_t) n);
        auto up = oversampler->processSamplesUp (bandBlk);

        for (size_t c = 0; c < up.getNumChannels(); ++c)
        {
            float* s = up.getChannelPointer (c);
            for (size_t i = 0; i < up.getNumSamples(); ++i)
            {
                const float x = s[i];
                s[i] = shape (d * x, mode) / d - x; // harmonics only
            }
        }

        oversampler->processSamplesDown (bandBlk);

        for (int c = 0; c < chs; ++c)
        {
            float* out = buffer.getWritePointer (c);
            const float* h = band.getReadPointer (c);
            float& x1 = dcX[(size_t) c];
            float& y1 = dcY[(size_t) c];
            for (int i = 0; i < n; ++i)
            {
                const float y = h[i] - x1 + 0.9995f * y1; // DC blocker (tube mode is asymmetric)
                x1 = h[i]; y1 = y;
                out[i] += mix * y;
            }
        }
    }

private:
    static float shape (float x, int mode)
    {
        switch (mode)
        {
            case Tube: { const float a = x + 0.3f * x * x; return std::tanh (a); }
            case Warm: return x / (1.0f + std::abs (x));
            default:   return std::tanh (x);
        }
    }

    int numCh = 2, latency = 0;
    double sampleRate = 44100.0;
    float lastFreq = -1.0f;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    juce::AudioBuffer<float> band;
    std::vector<Biquad> hp;
    std::vector<float> dcX, dcY;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> dryDelay { 64 };
};
