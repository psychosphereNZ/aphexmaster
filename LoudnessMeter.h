#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <vector>
#include <cmath>
#include "Biquad.h"

// ITU-R BS.1770 / EBU R128 loudness: momentary (400 ms), short-term (3 s), integrated (gated).
// Integrated loudness uses a fixed histogram so it is real-time safe (no allocation).
class LoudnessMeter
{
public:
    void prepare (double fs, int numChannels)
    {
        numCh = juce::jmin (2, numChannels);
        blockLen = juce::jmax (1, (int) std::lround (fs * 0.1));

        // K-weighting stage 1 (high shelf) and stage 2 (RLB high-pass), any sample rate
        const double pi = 3.141592653589793;
        {
            const double f0 = 1681.974450955533, G = 3.999843853973347, Q = 0.7071752369554196;
            const double K = std::tan (pi * f0 / fs);
            const double Vh = std::pow (10.0, G / 20.0);
            const double Vb = std::pow (Vh, 0.4996667741545416);
            const double a0 = 1.0 + K / Q + K * K;
            shelf.b0 = (Vh + Vb * K / Q + K * K) / a0;
            shelf.b1 = 2.0 * (K * K - Vh) / a0;
            shelf.b2 = (Vh - Vb * K / Q + K * K) / a0;
            shelf.a1 = 2.0 * (K * K - 1.0) / a0;
            shelf.a2 = (1.0 - K / Q + K * K) / a0;
        }
        {
            const double f0 = 38.13547087602444, Q = 0.5003270373238773;
            const double K = std::tan (pi * f0 / fs);
            const double a0 = 1.0 + K / Q + K * K;
            rlb.b0 = 1.0; rlb.b1 = -2.0; rlb.b2 = 1.0;
            rlb.a1 = 2.0 * (K * K - 1.0) / a0;
            rlb.a2 = (1.0 - K / Q + K * K) / a0;
        }
        for (int c = 0; c < 2; ++c) { st1[c] = shelf; st2[c] = rlb; }
        resetAll();
    }

    void resetAll()
    {
        for (int c = 0; c < 2; ++c) { st1[c].reset(); st2[c].reset(); }
        ring.fill (0.0); ringPos = 0; filled = 0; acc = 0.0; count = 0;
        momentary = shortTerm = -100.0f;
        resetIntegrated();
    }

    void resetIntegrated()
    {
        std::fill (histCount.begin(), histCount.end(), 0u);
        std::fill (histEnergy.begin(), histEnergy.end(), 0.0);
        integrated = -100.0f;
    }

    void process (const juce::AudioBuffer<float>& buffer)
    {
        const int n = buffer.getNumSamples();
        const int chs = juce::jmin (numCh, buffer.getNumChannels());
        for (int i = 0; i < n; ++i)
        {
            for (int c = 0; c < chs; ++c)
            {
                const double y = st2[c].process (st1[c].process ((double) buffer.getSample (c, i)));
                acc += y * y;
            }
            if (++count >= blockLen) { finishBlock (acc / blockLen); acc = 0.0; count = 0; }
        }
    }

    float momentary = -100.0f, shortTerm = -100.0f, integrated = -100.0f;

private:
    static float toLufs (double ms) { return ms > 1e-12 ? (float) (-0.691 + 10.0 * std::log10 (ms)) : -100.0f; }

    void finishBlock (double ms)
    {
        ring[(size_t) ringPos] = ms;
        ringPos = (ringPos + 1) % (int) ring.size();
        filled = juce::jmin (filled + 1, (int) ring.size());

        auto meanLast = [this] (int k)
        {
            k = juce::jmin (k, filled);
            double s = 0.0;
            for (int j = 1; j <= k; ++j) s += ring[(size_t) ((ringPos - j + (int) ring.size()) % (int) ring.size())];
            return k > 0 ? s / k : 0.0;
        };

        const double m400 = meanLast (4);
        momentary = toLufs (m400);
        shortTerm = toLufs (meanLast (30));

        if (filled >= 4)
        {
            const float l = toLufs (m400);
            if (l > -70.0f)
            {
                const int bin = juce::jlimit (0, numBins - 1, (int) ((l + 70.0f) * 10.0f));
                histCount[(size_t) bin]++;
                histEnergy[(size_t) bin] += m400;
            }
            computeIntegrated();
        }
    }

    void computeIntegrated()
    {
        double e = 0.0; unsigned long long cnt = 0;
        for (int b = 0; b < numBins; ++b) { e += histEnergy[(size_t) b]; cnt += histCount[(size_t) b]; }
        if (cnt == 0) { integrated = -100.0f; return; }

        const float gate = toLufs (e / (double) cnt) - 10.0f;
        const int startBin = juce::jlimit (0, numBins - 1, (int) ((gate + 70.0f) * 10.0f));
        e = 0.0; cnt = 0;
        for (int b = startBin; b < numBins; ++b) { e += histEnergy[(size_t) b]; cnt += histCount[(size_t) b]; }
        integrated = cnt > 0 ? toLufs (e / (double) cnt) : -100.0f;
    }

    static constexpr int numBins = 800; // -70 .. +10 LUFS in 0.1 dB steps
    int numCh = 2, blockLen = 4410, ringPos = 0, filled = 0, count = 0;
    double acc = 0.0;
    Biquad shelf, rlb, st1[2], st2[2];
    std::array<double, 30> ring {};
    std::array<unsigned int, numBins> histCount {};
    std::array<double, numBins> histEnergy {};
};
