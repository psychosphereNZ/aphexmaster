#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "Biquad.h"

// Mid/side stereo imager with "mono bass" (side signal high-passed below a frequency).
class Imager
{
public:
    void prepare (double fs)
    {
        sampleRate = fs;
        width.reset (fs, 0.05);
        width.setCurrentAndTargetValue (1.0f);
        sideHp1.reset(); sideHp2.reset();
        lastMono = -1.0f;
    }

    void process (juce::AudioBuffer<float>& buffer, float widthPct, float monoFreq)
    {
        if (buffer.getNumChannels() < 2) return;
        width.setTargetValue (widthPct * 0.01f);

        const bool monoOn = monoFreq > 20.5f;
        if (monoOn && monoFreq != lastMono)
        {
            lastMono = monoFreq;
            sideHp1.setHighPass (sampleRate, monoFreq, 0.5411961); // 4th-order Butterworth
            sideHp2.setHighPass (sampleRate, monoFreq, 1.3065630);
        }

        float* L = buffer.getWritePointer (0);
        float* R = buffer.getWritePointer (1);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float m = 0.5f * (L[i] + R[i]);
            float s = 0.5f * (L[i] - R[i]);
            if (monoOn) s = (float) sideHp2.process (sideHp1.process (s));
            s *= width.getNextValue();
            L[i] = m + s;
            R[i] = m - s;
        }
    }

private:
    double sampleRate = 44100.0;
    float lastMono = -1.0f;
    juce::SmoothedValue<float> width;
    Biquad sideHp1, sideHp2;
};
