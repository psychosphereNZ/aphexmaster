#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <cmath>

// Look-ahead brickwall limiter.
// Gain envelope = min-hold over (L+1) samples -> release smoothing -> box-car average over L samples.
// Audio is delayed by L, so the gain is fully down by the time a peak arrives (no overshoot).
class Maximizer
{
public:
    void prepare (double fs, int numChannels)
    {
        sampleRate = fs;
        numCh = numChannels;
        L = juce::jmax (1, (int) std::lround (fs * 0.002)); // 2 ms look-ahead
        holdRing.assign ((size_t) L + 1, 1.0f);
        boxRing.assign ((size_t) L, 1.0);
        delay.assign ((size_t) numCh, std::vector<float> ((size_t) L, 0.0f));
        reset();
    }

    int getLatency() const { return L; }

    void reset()
    {
        std::fill (holdRing.begin(), holdRing.end(), 1.0f);
        std::fill (boxRing.begin(), boxRing.end(), 1.0);
        for (auto& d : delay) std::fill (d.begin(), d.end(), 0.0f);
        boxSum = (double) L;
        env = 1.0f;
        holdPos = boxPos = delayPos = 0;
        inGain.reset (sampleRate, 0.05);
        inGain.setCurrentAndTargetValue (1.0f);
    }

    // returns max gain reduction in dB (positive)
    float process (juce::AudioBuffer<float>& buffer, bool active, float gainDb, float ceilingDb, float releaseMs)
    {
        const int n = buffer.getNumSamples();
        const int chs = juce::jmin (numCh, buffer.getNumChannels());
        const float ceiling = juce::Decibels::decibelsToGain (ceilingDb);
        const float rel = std::exp (-1.0f / (juce::jmax (1.0f, releaseMs) * 0.001f * (float) sampleRate));
        inGain.setTargetValue (active ? juce::Decibels::decibelsToGain (gainDb) : 1.0f);

        float minGain = 1.0f;
        float* const* ch = buffer.getArrayOfWritePointers();

        for (int i = 0; i < n; ++i)
        {
            const float g = inGain.getNextValue();
            float peak = 0.0f;
            for (int c = 0; c < chs; ++c)
            {
                ch[c][i] *= g;
                peak = juce::jmax (peak, std::abs (ch[c][i]));
            }

            float target = (active && peak > ceiling) ? ceiling / peak : 1.0f;

            holdRing[(size_t) holdPos] = target;
            holdPos = (holdPos + 1) % (L + 1);
            float held = 1.0f;
            for (float v : holdRing) held = juce::jmin (held, v);

            env = held < env ? held : held + rel * (env - held);

            boxSum += (double) env - boxRing[(size_t) boxPos];
            boxRing[(size_t) boxPos] = env;
            boxPos = (boxPos + 1) % L;
            const float gain = juce::jmin (1.0f, (float) (boxSum / (double) L));
            minGain = juce::jmin (minGain, gain);

            for (int c = 0; c < chs; ++c)
            {
                auto& d = delay[(size_t) c];
                const float delayed = d[(size_t) delayPos];
                d[(size_t) delayPos] = ch[c][i];
                float y = delayed * gain;
                if (active) y = juce::jlimit (-ceiling, ceiling, y); // safety net
                ch[c][i] = y;
            }
            delayPos = (delayPos + 1) % L;
        }

        // keep the running sum from drifting
        double s = 0.0; for (double v : boxRing) s += v; boxSum = s;

        return -juce::Decibels::gainToDecibels (minGain, -60.0f);
    }

private:
    double sampleRate = 44100.0;
    int numCh = 2, L = 88;
    std::vector<float> holdRing;
    std::vector<double> boxRing;
    std::vector<std::vector<float>> delay;
    double boxSum = 0.0;
    float env = 1.0f;
    int holdPos = 0, boxPos = 0, delayPos = 0;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> inGain;
};
