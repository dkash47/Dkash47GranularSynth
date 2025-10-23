#include "PluginProcessor.h"
#include "PluginEditor.h"

Dkash47GranularSynthAudioProcessor::Dkash47GranularSynthAudioProcessor()
    : juce::AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    formats.registerBasicFormats();
}

void Dkash47GranularSynthAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    engine.prepare(sampleRate, samplesPerBlock);
    delay.reset();
    {
        const float delaySamples = (float) juce::jlimit(1, (int) sampleRate, (int) (300.0 / 1000.0 * sampleRate));
        delay.setDelay(delaySamples);
    }
    reverb.reset();
    updateFromParams();
}

bool Dkash47GranularSynthAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void Dkash47GranularSynthAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // Update MIDI counters for UI feedback
    for (const auto metadata : midi)
    {
        const auto msg = metadata.getMessage();
        if (msg.isNoteOn()) {
            const int note = msg.getNoteNumber();
            const int velocity = msg.getVelocity();
            toneFreqHz = 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
            midiCounter.fetch_add(1);
            lastMidiNote.store(note); 
            lastMidiVel.store(velocity); 
            lastMidiChan.store((int) msg.getChannel());
            noteGate = true;
        }
        if (msg.isNoteOff()) { 
            midiCounter.fetch_add(1); 
            lastMidiVel.store(0); 
            noteGate = false;
        }
    }

    // Update parameters
    updateFromParams();

    // Render granular synthesis
    engine.render(buffer, midi);

    // Calculate peak for metering
    float peak = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            peak = std::max(peak, std::abs(buffer.getReadPointer(ch)[i]));

    // Test tone / fallback (only if forced)
    const bool forceTone = apvts.getRawParameterValue(Params::TestTone)->load() > 0.5f;
    if (forceTone)
    {
        auto* l = buffer.getWritePointer(0);
        auto* r = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : l;
        const auto N = buffer.getNumSamples();
        const float twoPi = juce::MathConstants<float>::twoPi;
        for (int i = 0; i < N; ++i)
        {
            float s = 0.1f * std::sin(tonePhase);
            tonePhase += twoPi * toneFreqHz / (float)getSampleRate();
            if (tonePhase > twoPi) tonePhase -= twoPi;
            l[i] += s; if (r) r[i] += s;
        }
        peak = std::max(peak, 0.1f);
    }

    // Apply master level
    const float masterLevel = apvts.getRawParameterValue(Params::Level)->load();
    buffer.applyGain(masterLevel);

    // Simple effects
    auto totalCh = buffer.getNumChannels();
    auto numSamples = buffer.getNumSamples();

    // Simple delay
    const float delayMixAmount = apvts.getRawParameterValue(Params::DelayMix)->load();
    if (delayMixAmount > 0.0f)
    {
        for (int ch = 0; ch < totalCh; ++ch)
        {
            auto* d = buffer.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i)
            {
                float delayed = delay.popSample(0);
                float inp = d[i];
                delay.pushSample(0, inp + delayed * delayFeedback);
                d[i] = inp * (1.0f - delayMixAmount) + delayed * delayMixAmount;
            }
        }
    }

    // Simple reverb
    const float reverbAmount = apvts.getRawParameterValue(Params::ReverbMix)->load();
    if (reverbAmount > 0.0f)
    {
        reverbParams.wetLevel = reverbAmount;
        reverbParams.roomSize = 0.7f;
        reverb.setParameters(reverbParams);
        reverb.processStereo(buffer.getWritePointer(0), 
                           buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : buffer.getWritePointer(0), 
                           numSamples);
    }

    // Update peak meter
    lastPeak.store(peak);
}

void Dkash47GranularSynthAudioProcessor::updateFromParams()
{
    // Create CPU-Optimized Ableton-style parameters
    GranularEngine::Params p;
    
    // Core granular parameters
    p.grainSize     = apvts.getRawParameterValue(Params::GrainSize)->load();
    p.density       = apvts.getRawParameterValue(Params::Density)->load();
    p.texture       = apvts.getRawParameterValue(Params::Texture)->load();
    p.pitch         = apvts.getRawParameterValue(Params::Pitch)->load();
    p.position      = apvts.getRawParameterValue(Params::Position)->load();
    p.reverse       = apvts.getRawParameterValue(Params::Reverse)->load();
    
    // CPU-Optimized Ableton-style features
    p.scan          = apvts.getRawParameterValue(Params::Scan)->load();
    p.spray         = apvts.getRawParameterValue(Params::Spray)->load();
    p.jitter        = apvts.getRawParameterValue(Params::Jitter)->load();
    p.pitchJitter   = apvts.getRawParameterValue(Params::PitchJitter)->load();
    p.grainShape    = apvts.getRawParameterValue(Params::GrainShape)->load();
    p.loopMode      = apvts.getRawParameterValue(Params::LoopMode)->load();
    p.glide         = apvts.getRawParameterValue(Params::Glide)->load();
    
    // Advanced parameters
    p.stereoWidth   = apvts.getRawParameterValue(Params::StereoWidth)->load();
    p.grainPitch    = apvts.getRawParameterValue(Params::GrainPitch)->load();
    p.freeze        = apvts.getRawParameterValue(Params::Freeze)->load();
    p.filterCutoff  = apvts.getRawParameterValue(Params::FilterCutoff)->load();
    p.filterRes     = apvts.getRawParameterValue(Params::FilterRes)->load();
    p.filterType    = apvts.getRawParameterValue(Params::FilterType)->load();
    p.formantShift  = apvts.getRawParameterValue(Params::FormantShift)->load();
    p.randomSpread  = apvts.getRawParameterValue(Params::RandomSpread)->load();
    p.grainAmp      = apvts.getRawParameterValue(Params::GrainAmp)->load();
    
    // Envelope
    p.attack        = apvts.getRawParameterValue(Params::Attack)->load();
    p.decay         = apvts.getRawParameterValue(Params::Decay)->load();
    p.sustain       = apvts.getRawParameterValue(Params::Sustain)->load();
    p.release       = apvts.getRawParameterValue(Params::Release)->load();
    
    // CPU-Optimized Enhanced Modulation System
    p.lfoRate       = apvts.getRawParameterValue(Params::LFORate)->load();
    p.lfoAmount     = apvts.getRawParameterValue(Params::LFOAmount)->load();
    p.lfoTarget     = apvts.getRawParameterValue(Params::LFOTarget)->load();
    p.lfoShape      = apvts.getRawParameterValue(Params::LFOShape)->load();
    
    // Second LFO (CPU-optimized)
    p.lfo2Rate      = apvts.getRawParameterValue(Params::LFO2Rate)->load();
    p.lfo2Amount    = apvts.getRawParameterValue(Params::LFO2Amount)->load();
    p.lfo2Target    = apvts.getRawParameterValue(Params::LFO2Target)->load();
    p.lfo2Shape     = apvts.getRawParameterValue(Params::LFO2Shape)->load();
    
    // New widening effects
    p.chorusAmount  = apvts.getRawParameterValue(Params::ChorusAmount)->load();
    p.unisonVoices  = apvts.getRawParameterValue(Params::UnisonVoices)->load();
    
    engine.setParams(p);

    // Setup simple delay
    const float delaySamples = (float) juce::jlimit(1, (int) getSampleRate(), (int) (200.0 / 1000.0 * getSampleRate()));
    delay.setDelay(delaySamples);
    delayFeedback = 0.4f;

    // Set audio source
    if (sampleBuffer)
        engine.setSource(sampleBuffer.get(), fileSampleRate);
}

bool Dkash47GranularSynthAudioProcessor::loadFile(const juce::File& f)
{
    std::unique_ptr<juce::AudioFormatReader> r (formats.createReaderFor(f));
    if (! r) return false;
    auto newBuf = std::make_unique<juce::AudioBuffer<float>>((int) juce::jmax(1u, r->numChannels), (int) r->lengthInSamples);
    r->read(newBuf.get(), 0, (int) r->lengthInSamples, 0, true, true);
    sampleBuffer = std::move(newBuf);
    fileSampleRate = r->sampleRate;
    currentSamplePath = f.getFullPathName(); // Store path for state persistence
    engine.setSource(sampleBuffer.get(), fileSampleRate);
    return true;
}

juce::AudioProcessorEditor* Dkash47GranularSynthAudioProcessor::createEditor()
{
    return new Dkash47GranularSynthAudioProcessorEditor(*this);
}

void Dkash47GranularSynthAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    
    // Add sample file path to state
    if (currentSamplePath.isNotEmpty())
    {
        xml->setAttribute("samplePath", currentSamplePath);
        xml->setAttribute("sampleRate", fileSampleRate);
    }
    
    copyXmlToBinary(*xml, destData);
}

void Dkash47GranularSynthAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
    {
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
        
        // Restore sample file if path exists
        juce::String samplePath = xml->getStringAttribute("samplePath");
        if (samplePath.isNotEmpty())
        {
            juce::File sampleFile(samplePath);
            if (sampleFile.existsAsFile())
            {
                loadFile(sampleFile); // This will restore the sample
            }
        }
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout Dkash47GranularSynthAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    
    // Debug
    p.push_back(std::make_unique<juce::AudioParameterBool>(Params::TestTone, "Test Tone", false));

    // Core Quanta-style granular parameters
    p.push_back(std::make_unique<juce::AudioParameterFloat>(Params::GrainSize, "Grain Size", 
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.1f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(Params::Density, "Density", 
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(Params::Texture, "Texture", 
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.2f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(Params::Pitch, "Pitch", 
        juce::NormalisableRange<float>(-48.0f, 48.0f, 0.01f), 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(Params::Position, "Position", 
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(Params::Reverse, "Reverse", 
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
    
    // CPU-Optimized Ableton-style granular features
    p.push_back(std::make_unique<juce::AudioParameterFloat>(Params::Scan, "Scan", 
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(Params::Spray, "Spray", 
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.1f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(Params::Jitter, "Jitter", 
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(Params::PitchJitter, "Pitch Jitter", 
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(Params::GrainShape, "Grain Shape", 
        juce::NormalisableRange<float>(0.0f, 3.0f, 1.0f), 0.0f)); // 0=Hann, 1=Triangle, 2=Square, 3=Gauss
    p.push_back(std::make_unique<juce::AudioParameterFloat>(Params::LoopMode, "Loop Mode", 
        juce::NormalisableRange<float>(0.0f, 2.0f, 1.0f), 0.0f)); // 0=Forward, 1=Backward, 2=PingPong
    p.push_back(std::make_unique<juce::AudioParameterFloat>(Params::Glide, "Glide", 
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
    
    // Advanced parameters
    p.push_back(std::make_unique<juce::AudioParameterFloat>(Params::StereoWidth, "Stereo Width", 
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(Params::GrainPitch, "Grain Pitch", 
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f), 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(Params::Freeze, "Freeze", 
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(Params::FilterCutoff, "Filter Cutoff", 
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 1.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(Params::FilterRes, "Filter Resonance", 
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
    
    // CPU-Optimized Advanced Filter & Effects
    p.push_back(std::make_unique<juce::AudioParameterFloat>(Params::FilterType, "Filter Type", 
        juce::NormalisableRange<float>(0.0f, 3.0f, 1.0f), 0.0f)); // 0=LP, 1=HP, 2=BP, 3=Notch
    p.push_back(std::make_unique<juce::AudioParameterFloat>(Params::FormantShift, "Formant Shift", 
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(Params::RandomSpread, "Random Spread", 
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(Params::GrainAmp, "Grain Amp Variation", 
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
    
    // Envelope (ADSR)
    p.push_back(std::make_unique<juce::AudioParameterFloat>(Params::Attack, "Attack", 
        juce::NormalisableRange<float>(1.0f, 2000.0f, 0.01f, 0.3f), 10.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(Params::Decay, "Decay", 
        juce::NormalisableRange<float>(1.0f, 2000.0f, 0.01f, 0.3f), 50.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(Params::Sustain, "Sustain", 
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 1.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(Params::Release, "Release", 
        juce::NormalisableRange<float>(5.0f, 4000.0f, 0.01f, 0.3f), 200.0f));
    
    // CPU-Optimized Enhanced Modulation System
    p.push_back(std::make_unique<juce::AudioParameterFloat>(Params::LFORate, "LFO1 Rate", 
        juce::NormalisableRange<float>(0.1f, 20.0f, 0.01f, 0.3f), 1.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(Params::LFOAmount, "LFO1 Amount", 
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f)); // Default 0.0f for constant sound
    p.push_back(std::make_unique<juce::AudioParameterFloat>(Params::LFOTarget, "LFO1 Target", 
        juce::NormalisableRange<float>(0.0f, 4.0f, 1.0f), 0.0f)); // Extended targets
    p.push_back(std::make_unique<juce::AudioParameterFloat>(Params::LFOShape, "LFO1 Shape", 
        juce::NormalisableRange<float>(0.0f, 4.0f, 1.0f), 0.0f)); // 0=Sine, 1=Triangle, 2=Square, 3=Saw, 4=Random
    
    // Second LFO for complex modulation (CPU-optimized)
    p.push_back(std::make_unique<juce::AudioParameterFloat>(Params::LFO2Rate, "LFO2 Rate", 
        juce::NormalisableRange<float>(0.1f, 20.0f, 0.01f, 0.3f), 0.5f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(Params::LFO2Amount, "LFO2 Amount", 
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(Params::LFO2Target, "LFO2 Target", 
        juce::NormalisableRange<float>(0.0f, 4.0f, 1.0f), 1.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(Params::LFO2Shape, "LFO2 Shape", 
        juce::NormalisableRange<float>(0.0f, 4.0f, 1.0f), 0.0f));
    
    // Effects
    p.push_back(std::make_unique<juce::AudioParameterFloat>(Params::ReverbMix, "Reverb", 
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(Params::DelayMix, "Delay", 
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
    
    // Widening effects for constant granular sound
    p.push_back(std::make_unique<juce::AudioParameterFloat>(Params::ChorusAmount, "Chorus", 
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(Params::UnisonVoices, "Unison", 
        juce::NormalisableRange<float>(1.0f, 8.0f, 1.0f), 1.0f));
    
    // Legacy/Utility
    p.push_back(std::make_unique<juce::AudioParameterFloat>(Params::Mix, "Mix", 
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 1.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(Params::Level, "Level", 
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.8f));

    return { p.begin(), p.end() };
}

// Factory function required by JUCE VST3 wrapper
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Dkash47GranularSynthAudioProcessor();
}
