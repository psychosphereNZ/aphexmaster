#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <map>

namespace
{
    using APF = juce::AudioParameterFloat;

    juce::NormalisableRange<float> freqRange (float lo, float hi, float centre)
    {
        juce::NormalisableRange<float> r (lo, hi);
        r.setSkewForCentre (centre);
        return r;
    }

    juce::NormalisableRange<float> skewRange (float lo, float hi, float centre, float step = 0.0f)
    {
        juce::NormalisableRange<float> r (lo, hi, step);
        r.setSkewForCentre (centre);
        return r;
    }

    juce::AudioParameterFloatAttributes unit (const juce::String& suffix, int decimals)
    {
        return juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction ([suffix, decimals] (float v, int)
            {
                if (suffix == " Hz" && v >= 1000.0f) return juce::String (v / 1000.0f, 2) + " kHz";
                return juce::String (v, decimals) + suffix;
            })
            .withValueFromStringFunction ([suffix] (const juce::String& t)
            {
                auto v = t.getFloatValue();
                if (suffix == " Hz" && t.containsIgnoreCase ("k")) v *= 1000.0f;
                return v;
            });
    }

    void addFloat (juce::AudioProcessorValueTreeState::ParameterLayout& l, const char* id, const char* name,
                   juce::NormalisableRange<float> range, float def, const juce::String& suffix, int decimals)
    {
        l.add (std::make_unique<APF> (juce::ParameterID { id, 1 }, name, range, def, unit (suffix, decimals)));
    }

    void addBool (juce::AudioProcessorValueTreeState::ParameterLayout& l, const char* id, const char* name, bool def)
    {
        l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, 1 }, name, def));
    }

    // ----------------------------------------------------------------- presets
    struct Preset { const char* name; std::map<juce::String, float> values; };

    const std::vector<Preset>& presets()
    {
        static const std::vector<Preset> list
        {
            { "Init (neutral)", {} },
            { "Techno Punch", {
                { "lsFreq", 70 }, { "lsGain", 1.5f }, { "p1Freq", 320 }, { "p1Gain", -1.5f }, { "p1Q", 1.0f },
                { "p2Freq", 3500 }, { "p2Gain", 0.8f }, { "hsFreq", 11000 }, { "hsGain", 1.5f }, { "hpOn", 1 }, { "hpFreq", 25 },
                { "mbOn", 1 }, { "xover1", 150 }, { "xover2", 2800 },
                { "loThr", -18 }, { "loRatio", 3.0f }, { "loAtt", 25 }, { "loRel", 120 }, { "loMakeup", 1.0f },
                { "miThr", -20 }, { "miRatio", 2.0f }, { "miAtt", 12 }, { "miRel", 150 }, { "miMakeup", 1.0f },
                { "hiThr", -22 }, { "hiRatio", 2.0f }, { "hiAtt", 5 },  { "hiRel", 90 },  { "hiMakeup", 0.5f },
                { "exOn", 1 }, { "exMode", 1 }, { "exDrive", 6 }, { "exFreq", 4000 }, { "exMix", 25 },
                { "imOn", 1 }, { "imWidth", 115 }, { "imMono", 120 },
                { "maxOn", 1 }, { "maxGain", 6 }, { "maxCeiling", -0.3f }, { "maxRelease", 60 } } },
            { "EDM Loud", {
                { "hpOn", 1 }, { "hpFreq", 28 }, { "lsFreq", 60 }, { "lsGain", 2.0f }, { "p2Freq", 2500 }, { "p2Gain", 1.0f }, { "p2Q", 0.7f },
                { "hsFreq", 9000 }, { "hsGain", 2.0f },
                { "mbOn", 1 }, { "xover1", 120 }, { "xover2", 3500 },
                { "loThr", -20 }, { "loRatio", 4.0f }, { "loAtt", 20 }, { "loRel", 100 }, { "loMakeup", 2.0f },
                { "miThr", -22 }, { "miRatio", 3.0f }, { "miAtt", 10 }, { "miRel", 120 }, { "miMakeup", 2.0f },
                { "hiThr", -24 }, { "hiRatio", 3.0f }, { "hiAtt", 3 },  { "hiRel", 80 },  { "hiMakeup", 1.5f },
                { "exOn", 1 }, { "exMode", 0 }, { "exDrive", 9 }, { "exFreq", 5000 }, { "exMix", 30 },
                { "imOn", 1 }, { "imWidth", 125 }, { "imMono", 150 },
                { "maxOn", 1 }, { "maxGain", 10 }, { "maxCeiling", -0.5f }, { "maxRelease", 40 } } },
            { "Warm Analog", {
                { "lsFreq", 90 }, { "lsGain", 1.0f }, { "p1Freq", 250 }, { "p1Gain", 0.7f }, { "p1Q", 0.7f },
                { "hsFreq", 12000 }, { "hsGain", -0.5f },
                { "mbOn", 1 }, { "xover1", 200 }, { "xover2", 3000 },
                { "loThr", -16 }, { "loRatio", 2.0f }, { "loAtt", 30 }, { "loRel", 200 }, { "loMakeup", 0.5f },
                { "miThr", -18 }, { "miRatio", 1.8f }, { "miAtt", 20 }, { "miRel", 220 }, { "miMakeup", 0.5f },
                { "hiThr", -20 }, { "hiRatio", 1.5f }, { "hiAtt", 10 }, { "hiRel", 150 }, { "hiMakeup", 0.0f },
                { "exOn", 1 }, { "exMode", 2 }, { "exDrive", 8 }, { "exFreq", 1500 }, { "exMix", 35 },
                { "imOn", 1 }, { "imWidth", 105 }, { "imMono", 100 },
                { "maxOn", 1 }, { "maxGain", 4 }, { "maxCeiling", -0.3f }, { "maxRelease", 120 } } },
            { "Gentle Polish", {
                { "hpOn", 1 }, { "hpFreq", 20 }, { "p2Freq", 4000 }, { "p2Gain", 0.5f }, { "p2Q", 0.6f }, { "hsFreq", 14000 }, { "hsGain", 1.0f },
                { "exOn", 1 }, { "exMode", 0 }, { "exDrive", 4 }, { "exFreq", 6000 }, { "exMix", 15 },
                { "imOn", 1 }, { "imWidth", 108 }, { "imMono", 80 },
                { "maxOn", 1 }, { "maxGain", 3 }, { "maxCeiling", -1.0f }, { "maxRelease", 150 } } },
            { "Deep / Dub", {
                { "hpOn", 1 }, { "hpFreq", 22 }, { "lsFreq", 55 }, { "lsGain", 2.5f }, { "p1Freq", 450 }, { "p1Gain", -2.0f }, { "p1Q", 0.8f },
                { "hsFreq", 8000 }, { "hsGain", -1.0f },
                { "mbOn", 1 }, { "xover1", 110 }, { "xover2", 2500 },
                { "loThr", -14 }, { "loRatio", 2.5f }, { "loAtt", 40 }, { "loRel", 250 }, { "loMakeup", 1.0f },
                { "miThr", -20 }, { "miRatio", 2.0f }, { "miAtt", 15 }, { "miRel", 180 }, { "miMakeup", 1.0f },
                { "hiThr", -20 }, { "hiRatio", 2.0f }, { "hiAtt", 8 },  { "hiRel", 120 }, { "hiMakeup", 0.0f },
                { "imOn", 1 }, { "imWidth", 110 }, { "imMono", 180 },
                { "maxOn", 1 }, { "maxGain", 5 }, { "maxCeiling", -0.3f }, { "maxRelease", 100 } } },
        };
        return list;
    }
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout ApexMasterAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;

    addFloat (l, "inGain", "Input Gain", { -24.0f, 24.0f, 0.01f }, 0.0f, " dB", 1);

    // EQ
    addBool  (l, "eqOn", "EQ On", true);
    addBool  (l, "hpOn", "EQ HP On", false);
    addFloat (l, "hpFreq", "EQ HP Freq", freqRange (10, 500, 60), 25, " Hz", 0);
    addFloat (l, "lsFreq", "EQ Low Shelf Freq", freqRange (20, 1000, 150), 100, " Hz", 0);
    addFloat (l, "lsGain", "EQ Low Shelf Gain", { -12.0f, 12.0f, 0.01f }, 0.0f, " dB", 1);
    addFloat (l, "p1Freq", "EQ Bell 1 Freq", freqRange (40, 8000, 500), 400, " Hz", 0);
    addFloat (l, "p1Gain", "EQ Bell 1 Gain", { -12.0f, 12.0f, 0.01f }, 0.0f, " dB", 1);
    addFloat (l, "p1Q", "EQ Bell 1 Q", skewRange (0.2f, 8.0f, 1.0f), 1.0f, "", 2);
    addFloat (l, "p2Freq", "EQ Bell 2 Freq", freqRange (200, 18000, 3000), 3000, " Hz", 0);
    addFloat (l, "p2Gain", "EQ Bell 2 Gain", { -12.0f, 12.0f, 0.01f }, 0.0f, " dB", 1);
    addFloat (l, "p2Q", "EQ Bell 2 Q", skewRange (0.2f, 8.0f, 1.0f), 1.0f, "", 2);
    addFloat (l, "hsFreq", "EQ High Shelf Freq", freqRange (1000, 20000, 6000), 10000, " Hz", 0);
    addFloat (l, "hsGain", "EQ High Shelf Gain", { -12.0f, 12.0f, 0.01f }, 0.0f, " dB", 1);

    // Multiband dynamics
    addBool  (l, "mbOn", "Dynamics On", false);
    addFloat (l, "xover1", "Crossover Low/Mid", freqRange (40, 1000, 200), 150, " Hz", 0);
    addFloat (l, "xover2", "Crossover Mid/High", freqRange (800, 12000, 3000), 3000, " Hz", 0);
    for (auto b : { std::pair<const char*, const char*> { "lo", "Low" }, { "mi", "Mid" }, { "hi", "High" } })
    {
        const juce::String pre (b.first), nm (b.second);
        auto id = [&] (const char* s) { return pre + s; };
        l.add (std::make_unique<APF> (juce::ParameterID { id ("Thr"), 1 },    nm + " Threshold", juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f), -12.0f, unit (" dB", 1)));
        l.add (std::make_unique<APF> (juce::ParameterID { id ("Ratio"), 1 },  nm + " Ratio",     skewRange (1.0f, 20.0f, 4.0f, 0.01f), 2.0f, unit (":1", 1)));
        l.add (std::make_unique<APF> (juce::ParameterID { id ("Att"), 1 },    nm + " Attack",    skewRange (0.1f, 200.0f, 20.0f), 15.0f, unit (" ms", 1)));
        l.add (std::make_unique<APF> (juce::ParameterID { id ("Rel"), 1 },    nm + " Release",   skewRange (10.0f, 2000.0f, 200.0f), 150.0f, unit (" ms", 0)));
        l.add (std::make_unique<APF> (juce::ParameterID { id ("Makeup"), 1 }, nm + " Makeup",    juce::NormalisableRange<float> (-12.0f, 12.0f, 0.01f), 0.0f, unit (" dB", 1)));
    }

    // Exciter
    addBool  (l, "exOn", "Exciter On", false);
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "exMode", 1 }, "Exciter Mode",
                                                         juce::StringArray { "Tape", "Tube", "Warm" }, 0));
    addFloat (l, "exDrive", "Exciter Drive", { 0.0f, 24.0f, 0.01f }, 6.0f, " dB", 1);
    addFloat (l, "exFreq", "Exciter Freq", freqRange (500, 12000, 3000), 3000, " Hz", 0);
    addFloat (l, "exMix", "Exciter Mix", { 0.0f, 100.0f, 0.1f }, 20.0f, " %", 0);

    // Imager
    addBool  (l, "imOn", "Imager On", false);
    addFloat (l, "imWidth", "Stereo Width", { 0.0f, 200.0f, 0.1f }, 100.0f, " %", 0);
    addFloat (l, "imMono", "Mono Bass", freqRange (20, 400, 120), 20, " Hz", 0);

    // Maximizer
    addBool  (l, "maxOn", "Maximizer On", true);
    addFloat (l, "maxGain", "Maximizer Gain", { 0.0f, 24.0f, 0.01f }, 0.0f, " dB", 1);
    addFloat (l, "maxCeiling", "Ceiling", { -12.0f, 0.0f, 0.01f }, -0.3f, " dB", 1);
    addFloat (l, "maxRelease", "Release", skewRange (5.0f, 1000.0f, 100.0f), 100.0f, " ms", 0);

    return l;
}

//==============================================================================
ApexMasterAudioProcessor::ApexMasterAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "APEX", createLayout())
{
    static const char* ids[pCount] = {
        "inGain", "eqOn", "hpOn", "hpFreq", "lsFreq", "lsGain", "p1Freq", "p1Gain", "p1Q", "p2Freq", "p2Gain", "p2Q", "hsFreq", "hsGain",
        "mbOn", "xover1", "xover2",
        "loThr", "loRatio", "loAtt", "loRel", "loMakeup",
        "miThr", "miRatio", "miAtt", "miRel", "miMakeup",
        "hiThr", "hiRatio", "hiAtt", "hiRel", "hiMakeup",
        "exOn", "exMode", "exDrive", "exFreq", "exMix",
        "imOn", "imWidth", "imMono",
        "maxOn", "maxGain", "maxCeiling", "maxRelease" };
    for (int i = 0; i < pCount; ++i)
    {
        raw[(size_t) i] = apvts.getRawParameterValue (ids[i]);
        jassert (raw[(size_t) i] != nullptr);
    }

    for (auto& a : inPeak) a.store (0.0f);
    for (auto& a : outPeak) a.store (0.0f);
    for (auto& a : bandGr) a.store (0.0f);
}

bool ApexMasterAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono()) return false;
    return out == layouts.getMainInputChannelSet();
}

void ApexMasterAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate.store (sampleRate);
    const auto chs = (juce::uint32) juce::jmax (1, getTotalNumOutputChannels());
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, chs };

    eq.prepare (spec);
    mbComp.prepare (spec);
    exciter.prepare (spec);
    imager.prepare (sampleRate);
    maximizer.prepare (sampleRate, (int) chs);
    loudness.prepare (sampleRate, (int) chs);

    inputGain.reset (sampleRate, 0.05);
    inputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (p (pInGain)));
    monoScratch.assign ((size_t) samplesPerBlock, 0.0f);
    corrLR = corrLL = corrRR = 0.0;

    setLatencySamples (exciter.getLatency() + maximizer.getLatency());
}

MasterEQ::Settings ApexMasterAudioProcessor::getEqSettings() const
{
    MasterEQ::Settings s;
    s.hpOn = on (pHpOn);  s.hpFreq = p (pHpFreq);
    s.lsFreq = p (pLsFreq); s.lsGain = p (pLsGain);
    s.p1Freq = p (pP1Freq); s.p1Gain = p (pP1Gain); s.p1Q = p (pP1Q);
    s.p2Freq = p (pP2Freq); s.p2Gain = p (pP2Gain); s.p2Q = p (pP2Q);
    s.hsFreq = p (pHsFreq); s.hsGain = p (pHsGain);
    return s;
}

void ApexMasterAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numIn = getTotalNumInputChannels();
    const int numCh = getTotalNumOutputChannels();
    const int n = buffer.getNumSamples();
    for (int c = numIn; c < numCh; ++c) buffer.clear (c, 0, n);
    if (n == 0) return;

    if ((int) monoScratch.size() < n) return; // host broke its block-size promise; pass through

    if (resetLoudnessRequest.exchange (false)) loudness.resetIntegrated();

    // ---- input gain + input meter
    inputGain.setTargetValue (juce::Decibels::decibelsToGain (p (pInGain)));
    for (int i = 0; i < n; ++i)
    {
        const float g = inputGain.getNextValue();
        for (int c = 0; c < numCh; ++c) buffer.getWritePointer (c)[i] *= g;
    }
    for (int c = 0; c < juce::jmin (2, numCh); ++c) atomicMax (inPeak[c], buffer.getMagnitude (c, 0, n));

    // ---- EQ
    if (on (pEqOn))
    {
        eq.setSettings (getEqSettings());
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        eq.process (ctx);
    }

    // ---- multiband dynamics
    if (on (pMbOn))
    {
        mbComp.setCrossovers (p (pX1), p (pX2));
        for (int b = 0; b < 3; ++b)
        {
            const int o = pLoThr + b * 5;
            mbComp.setBand (b, { p ((Pid) o), p ((Pid) (o + 1)), p ((Pid) (o + 2)), p ((Pid) (o + 3)), p ((Pid) (o + 4)) });
        }
        mbComp.process (buffer);
        for (int b = 0; b < 3; ++b) atomicMax (bandGr[b], mbComp.gainReduction[(size_t) b]);
    }

    // ---- exciter (always runs its delay line to keep latency constant)
    exciter.process (buffer, on (pExOn), p (pExDrive), p (pExMix) * 0.01f, p (pExFreq), (int) p (pExMode));

    // ---- imager
    if (on (pImOn) && numCh >= 2) imager.process (buffer, p (pImWidth), p (pImMono));

    // ---- maximizer (always runs its look-ahead delay)
    const float gr = maximizer.process (buffer, on (pMaxOn), p (pMaxGain), p (pMaxCeiling), p (pMaxRelease));
    atomicMax (maxGr, gr);

    // ---- output meters
    for (int c = 0; c < juce::jmin (2, numCh); ++c) atomicMax (outPeak[c], buffer.getMagnitude (c, 0, n));
    loudness.process (buffer);
    lufsM.store (loudness.momentary);
    lufsS.store (loudness.shortTerm);
    lufsI.store (loudness.integrated);

    const float* L = buffer.getReadPointer (0);
    const float* R = buffer.getReadPointer (numCh > 1 ? 1 : 0);
    const double k = 1.0 - std::exp (-1.0 / (0.3 * getSampleRate()));
    for (int i = 0; i < n; ++i)
    {
        corrLR += k * ((double) L[i] * R[i] - corrLR);
        corrLL += k * ((double) L[i] * L[i] - corrLL);
        corrRR += k * ((double) R[i] * R[i] - corrRR);
        monoScratch[(size_t) i] = 0.5f * (L[i] + R[i]);
    }
    const double den = std::sqrt (corrLL * corrRR);
    correlation.store (den > 1e-10 ? (float) (corrLR / den) : 1.0f);

    spectrumFifo.push (monoScratch.data(), n);
}

//==============================================================================
int ApexMasterAudioProcessor::getNumPrograms() { return (int) presets().size(); }

const juce::String ApexMasterAudioProcessor::getProgramName (int index)
{
    return juce::isPositiveAndBelow (index, (int) presets().size()) ? presets()[(size_t) index].name : "";
}

void ApexMasterAudioProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) presets().size())) return;
    currentProgram = index;
    const auto& preset = presets()[(size_t) index];

    for (auto* param : getParameters())
    {
        auto* withId = dynamic_cast<juce::RangedAudioParameter*> (param);
        if (withId == nullptr) continue;
        const auto id = withId->getParameterID();
        if (id == "inGain") continue; // keep the user's input trim

        auto it = preset.values.find (id);
        const float norm = it != preset.values.end() ? withId->convertTo0to1 (it->second)
                                                     : withId->getDefaultValue();
        withId->setValueNotifyingHost (norm);
    }
    resetLoudnessRequest.store (true);
}

void ApexMasterAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void ApexMasterAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            currentProgram = tree.getProperty ("program", 0);
            apvts.replaceState (tree);
        }
}

juce::AudioProcessorEditor* ApexMasterAudioProcessor::createEditor()
{
    return new ApexMasterAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ApexMasterAudioProcessor();
}
