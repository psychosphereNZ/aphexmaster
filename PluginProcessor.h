#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>

#include "MasterEQ.h"
#include "MultibandComp.h"
#include "Exciter.h"
#include "Imager.h"
#include "Maximizer.h"
#include "LoudnessMeter.h"

// Lock-free FIFO that carries the mono output signal to the spectrum analyser.
struct SpectrumFifo
{
    static constexpr int size = 32768;
    juce::AbstractFifo fifo { size };
    std::vector<float> data = std::vector<float> ((size_t) size, 0.0f);

    void push (const float* src, int n)
    {
        int s1, z1, s2, z2;
        fifo.prepareToWrite (n, s1, z1, s2, z2);
        if (z1 > 0) std::copy (src, src + z1, data.begin() + s1);
        if (z2 > 0) std::copy (src + z1, src + z1 + z2, data.begin() + s2);
        fifo.finishedWrite (z1 + z2);
    }

    int pull (float* dst, int maxN)
    {
        int s1, z1, s2, z2;
        fifo.prepareToRead (maxN, s1, z1, s2, z2);
        if (z1 > 0) std::copy (data.begin() + s1, data.begin() + s1 + z1, dst);
        if (z2 > 0) std::copy (data.begin() + s2, data.begin() + s2 + z2, dst + z1);
        fifo.finishedRead (z1 + z2);
        return z1 + z2;
    }
};

class ApexMasterAudioProcessor : public juce::AudioProcessor
{
public:
    ApexMasterAudioProcessor();
    ~ApexMasterAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Apex Master"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    MasterEQ::Settings getEqSettings() const;
    double getCurrentSampleRate() const { return currentSampleRate.load(); }

    juce::AudioProcessorValueTreeState apvts;

    // --- meters (audio thread writes, GUI reads/clears) ---
    std::atomic<float> inPeak[2], outPeak[2], maxGr { 0.0f }, bandGr[3];
    std::atomic<float> lufsM { -100.0f }, lufsS { -100.0f }, lufsI { -100.0f }, correlation { 1.0f };
    std::atomic<bool> resetLoudnessRequest { false };
    SpectrumFifo spectrumFifo;

private:
    // cached raw parameter pointers (no string lookups on the audio thread)
    enum Pid
    {
        pInGain, pEqOn, pHpOn, pHpFreq, pLsFreq, pLsGain, pP1Freq, pP1Gain, pP1Q, pP2Freq, pP2Gain, pP2Q, pHsFreq, pHsGain,
        pMbOn, pX1, pX2,
        pLoThr, pLoRatio, pLoAtt, pLoRel, pLoMakeup,
        pMiThr, pMiRatio, pMiAtt, pMiRel, pMiMakeup,
        pHiThr, pHiRatio, pHiAtt, pHiRel, pHiMakeup,
        pExOn, pExMode, pExDrive, pExFreq, pExMix,
        pImOn, pImWidth, pImMono,
        pMaxOn, pMaxGain, pMaxCeiling, pMaxRelease,
        pCount
    };
    std::array<std::atomic<float>*, pCount> raw {};
    float p (Pid id) const  { return raw[(size_t) id]->load(); }
    bool  on (Pid id) const { return p (id) > 0.5f; }
    static void atomicMax (std::atomic<float>& a, float v)
    {
        float cur = a.load();
        while (v > cur && ! a.compare_exchange_weak (cur, v)) {}
    }

    MasterEQ eq;
    MultibandComp mbComp;
    Exciter exciter;
    Imager imager;
    Maximizer maximizer;
    LoudnessMeter loudness;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> inputGain;
    std::vector<float> monoScratch;
    double corrLR = 0.0, corrLL = 0.0, corrRR = 0.0;
    std::atomic<double> currentSampleRate { 44100.0 };
    int currentProgram = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ApexMasterAudioProcessor)
};
