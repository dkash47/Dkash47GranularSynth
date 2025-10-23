#pragma once
#include <JuceHeader.h>
#include "GranularEngine.h"
#include "ParameterIDs.h"

class Dkash47GranularSynthAudioProcessor : public juce::AudioProcessor
{
public:
    Dkash47GranularSynthAudioProcessor();
    ~Dkash47GranularSynthAudioProcessor() override = default;

    // AudioProcessor overrides
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Dkash47 Granular Synthesizer"; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // File loading
    bool loadFile(const juce::File&);

    // Params
    juce::AudioProcessorValueTreeState apvts { *this, nullptr, "Params", createParameterLayout() };
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Accessors for UI
    const juce::AudioBuffer<float>* getSampleBuffer() const { return sampleBuffer.get(); }
    float getLastPeak() const { return lastPeak.load(); }
    int getMidiCounter() const { return midiCounter.load(); }
    float getPlayheadNorm() const { return engine.getPlayheadNorm(); }
    int getLastMidiNote() const { return lastMidiNote.load(); }
    int getLastMidiVel() const { return lastMidiVel.load(); }
    int getLastMidiChan() const { return lastMidiChan.load(); }
    juce::String getCurrentSamplePath() const { return currentSamplePath; }
    
    // Public access to engine for UI
    GranularEngine engine;

private:

    juce::AudioFormatManager formats;
    std::unique_ptr<juce::AudioBuffer<float>> sampleBuffer;

    // FX
    juce::Reverb reverb;
    juce::Reverb::Parameters reverbParams;
    juce::dsp::DelayLine<float> delay { 48000 }; // 1s max at 48k
    float delayFeedback = 0.5f;

    bool noteGate = false;
    double fileSampleRate = 44100.0;
    juce::String currentSamplePath;

    // Fallback tone + metering
    float tonePhase = 0.0f;
    float toneFreqHz = 220.0f;
    std::atomic<float> lastPeak { 0.0f };
    std::atomic<int> midiCounter { 0 };
    std::atomic<int> lastMidiNote { -1 };
    std::atomic<int> lastMidiVel  { -1 };
    std::atomic<int> lastMidiChan { -1 };

    void updateFromParams();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Dkash47GranularSynthAudioProcessor)
};
