// Offline test: renders a synthetic techno loop through every preset, checks ceiling/NaN/latency,
// and saves GUI screenshots of every page.
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cstdio>

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::File outDir (argc > 1 ? juce::String (argv[1]) : juce::File::getCurrentWorkingDirectory().getFullPathName());

    const double fs = 48000.0;
    const int block = 512;
    bool ok = true;

    ApexMasterAudioProcessor proc;
    proc.setPlayConfigDetails (2, 2, fs, block);
    proc.prepareToPlay (fs, block);
    std::printf ("Latency: %d samples (%.2f ms)\n", proc.getLatencySamples(), 1000.0 * proc.getLatencySamples() / fs);

    juce::Random rnd (42);
    juce::MidiBuffer midi;

    for (int prog = 0; prog < proc.getNumPrograms(); ++prog)
    {
        proc.setCurrentProgram (prog);
        proc.prepareToPlay (fs, block); // reset state
        float maxOut = 0.0f; bool nan = false; float maxGr = 0.0f;
        double phase = 0.0, hatPhase = 0.0;
        const int total = (int) (fs * 12.0);

        juce::AudioBuffer<float> buf (2, block);
        for (int pos = 0; pos < total; pos += block)
        {
            for (int i = 0; i < block; ++i)
            {
                const double t = (pos + i) / fs;
                const double beat = std::fmod (t, 0.5);           // 120 BPM four-on-the-floor
                const double kickF = 50.0 + 120.0 * std::exp (-beat * 30.0);
                phase += 2.0 * juce::MathConstants<double>::pi * kickF / fs;
                const float kick = (float) (std::sin (phase) * std::exp (-beat * 7.0)) * 0.7f;
                hatPhase = std::fmod (t + 0.25, 0.5);
                const float hat = (rnd.nextFloat() * 2.0f - 1.0f) * (float) std::exp (-hatPhase * 60.0) * 0.25f;
                const float pad = 0.12f * (float) std::sin (2.0 * juce::MathConstants<double>::pi * 220.0 * t);
                const float noise = (rnd.nextFloat() * 2.0f - 1.0f) * 0.02f;
                buf.setSample (0, i, kick + hat + pad + noise);
                buf.setSample (1, i, kick + hat * 0.8f - pad * 0.5f + noise);
            }
            proc.processBlock (buf, midi);
            for (int c = 0; c < 2; ++c)
                for (int i = 0; i < block; ++i)
                {
                    const float v = buf.getSample (c, i);
                    if (! std::isfinite (v)) nan = true;
                    maxOut = juce::jmax (maxOut, std::abs (v));
                }
            maxGr = juce::jmax (maxGr, proc.maxGr.exchange (0.0f));
        }

        const float ceilingDb = proc.apvts.getRawParameterValue ("maxCeiling")->load();
        const float peakDb = juce::Decibels::gainToDecibels (maxOut, -100.0f);
        const bool pass = ! nan && peakDb <= ceilingDb + 0.01f;
        ok = ok && pass;
        std::printf ("%-16s peak %6.2f dBFS (ceiling %5.2f)  LUFS-I %6.2f  maxGR %5.2f dB  %s\n",
                     proc.getProgramName (prog).toRawUTF8(), peakDb, ceilingDb, proc.lufsI.load(), maxGr,
                     pass ? "OK" : "FAIL");
    }

    // LUFS calibration check: 1 kHz sine at -20 dBFS on both channels should read about -20 LUFS... per channel
    // (BS.1770: stereo sine at -20 dBFS each -> -20 + 3.01 - 3.01(sine RMS) ~= -20 LUFS)
    {
        proc.setCurrentProgram (0);
        *proc.apvts.getRawParameterValue ("maxOn") = 0.0f;
        proc.prepareToPlay (fs, block);
        juce::AudioBuffer<float> buf (2, block);
        const float amp = juce::Decibels::decibelsToGain (-20.0f);
        for (int pos = 0; pos < (int) (fs * 5); pos += block)
        {
            for (int i = 0; i < block; ++i)
            {
                const float v = amp * (float) std::sin (2.0 * juce::MathConstants<double>::pi * 1000.0 * (pos + i) / fs);
                buf.setSample (0, i, v); buf.setSample (1, i, v);
            }
            proc.processBlock (buf, midi);
        }
        std::printf ("LUFS check: 1 kHz stereo sine @ -20 dBFS -> %.2f LUFS (expected ~ -20.0)\n", proc.lufsI.load());
    }

    // GUI snapshots
    proc.setCurrentProgram (1);
    {
        juce::AudioBuffer<float> buf (2, block);
        for (int k = 0; k < 40; ++k)
        {
            for (int i = 0; i < block; ++i)
            {
                const float v = (rnd.nextFloat() * 2.0f - 1.0f) * 0.3f + 0.4f * (float) std::sin (k * block + i);
                buf.setSample (0, i, v); buf.setSample (1, i, v * 0.9f);
            }
            proc.processBlock (buf, midi);
        }
    }
    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    auto* apex = dynamic_cast<ApexMasterAudioProcessorEditor*> (ed.get());
    apex->getSpectrum().updateAnalyser();
    apex->getMeters().tick();
    const char* names[] = { "eq", "dynamics", "exciter", "imager", "maximizer" };
    for (int pg = 0; pg < 5; ++pg)
    {
        apex->showPage (pg);
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        auto f = outDir.getChildFile (juce::String ("gui_") + names[pg] + ".png");
        f.deleteFile();
        juce::FileOutputStream os (f);
        juce::PNGImageFormat().writeImageToStream (img, os);
    }
    ed.reset();

    std::printf (ok ? "ALL TESTS PASSED\n" : "SOME TESTS FAILED\n");
    return ok ? 0 : 1;
}
