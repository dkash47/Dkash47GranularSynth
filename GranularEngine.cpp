#include "GranularEngine.h"
#include <cmath>

void GranularVoice::prepare(double sampleRate, int maximumBlockSize)
{
    currentSampleRate = sampleRate;
    
    // Initialize envelope
    envelope.setSampleRate(sampleRate);
    
    // Initialize filters
    filterL.prepare({ sampleRate, (juce::uint32)maximumBlockSize, 1 });
    filterR.prepare({ sampleRate, (juce::uint32)maximumBlockSize, 1 });
    
    // Set initial filter coefficients (lowpass)
    filterL.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 20000.0f, 0.7f);
    filterR.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 20000.0f, 0.7f);
    
    // Initialize chorus delay lines
    chorusDelayL.reset();
    chorusDelayR.reset();
    chorusLFOPhase = 0.0f;
}

void GranularVoice::setAudioSource(const juce::AudioBuffer<float>* source, double sourceRate)
{
    audioSource = source;
    this->sourceSampleRate = sourceRate;
}

void GranularVoice::startNote(int midiNoteNumber, float noteVelocity, juce::SynthesiserSound*, int currentPitchWheelPosition)
{
    juce::ignoreUnused(currentPitchWheelPosition);
    
    this->midiNote = midiNoteNumber;
    this->velocity = noteVelocity;
    this->isActive = true;
    
    // Start envelope
    envelope.noteOn();
    
    // Clear existing grains
    activeGrains.clear();
    
    // Reset grain spawn timer
    grainSpawnTimer = 0.0f;
    
    // Spawn fewer initial grains for better performance
    for (int i = 0; i < 2; ++i)
        spawnGrain();
}

void GranularVoice::stopNote(float noteOffVelocity, bool allowTailOff)
{
    juce::ignoreUnused(noteOffVelocity);
    
    if (allowTailOff)
    {
        envelope.noteOff();
    }
    else
    {
        envelope.reset();
        isActive = false;
        activeGrains.clear();
    }
}

void GranularVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (!isActive || !audioSource || audioSource->getNumSamples() == 0)
        return;
        
    // Update grain spawning
    updateGrains(outputBuffer, startSample, numSamples);
    
    // Check if envelope has finished
    if (!envelope.isActive() && activeGrains.empty())
    {
        isActive = false;
        clearCurrentNote();
    }
}

void GranularVoice::updateInternalParams()
{
    // Update envelope parameters
    envelopeParams.attack = parameters.attack / 1000.0f;
    envelopeParams.decay = parameters.decay / 1000.0f;
    envelopeParams.sustain = parameters.sustain;
    envelopeParams.release = parameters.release / 1000.0f;
    envelope.setParameters(envelopeParams);
    
    // Update grain spawn interval based on density
    grainSpawnInterval = 1.0f / juce::jmax(0.1f, parameters.density * 100.0f);
    
    // Update filter coefficients
    float cutoffHz = juce::jmap(parameters.filterCutoff, 0.0f, 1.0f, 80.0f, 20000.0f);
    float resonance = juce::jmap(parameters.filterRes, 0.0f, 1.0f, 0.5f, 10.0f);
    
    auto coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(currentSampleRate, cutoffHz, resonance);
    filterL.coefficients = coefficients;
    filterR.coefficients = coefficients;
}

void GranularVoice::updateGrains(juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    const float samplesPerSec = (float)currentSampleRate;
    const float grainSpawnRate = parameters.density * 50.0f; // Reduced from 100 to 50 for better performance
    const float spawnIncrement = grainSpawnRate / samplesPerSec;
    
    for (int sample = 0; sample < numSamples; ++sample)
    {
        const int bufferIndex = startSample + sample;
        
        // Spawn new grains based on density (with stricter CPU limit)
        if (activeGrains.size() < 16) // Further reduced from 24 to 16 for better CPU performance
        {
            grainSpawnTimer += spawnIncrement;
            while (grainSpawnTimer >= 1.0f && isActive)
            {
                spawnGrain();
                grainSpawnTimer -= 1.0f;
            }
        }
        
        // Process all active grains
        float outputL = 0.0f, outputR = 0.0f;
        
        for (int g = (int)activeGrains.size() - 1; g >= 0; --g)
        {
            auto& grain = activeGrains[g];
            
            if (grain.samplesRemaining <= 0)
            {
                activeGrains.erase(activeGrains.begin() + g);
                continue;
            }
            
            // Skip processing very quiet grains for performance
            float envelopePhase = 1.0f - (float)grain.samplesRemaining / (float)grain.totalSamples;
            float envelopeValue = 0.5f - 0.5f * std::cos(2.0f * juce::MathConstants<float>::pi * envelopePhase);
            if (envelopeValue < 0.02f) { // Increased threshold to skip more quiet grains
                --grain.samplesRemaining;
                continue; // Skip very quiet grains
            }
            
            // Get sample from audio source with interpolation
            float sampleL = getInterpolatedSample(0, grain.position);
            float sampleR = audioSource->getNumChannels() > 1 ? 
                           getInterpolatedSample(1, grain.position) : sampleL;
            
            // Use pre-calculated envelope value
            
            sampleL *= envelopeValue;
            sampleR *= envelopeValue;
            
            // Apply panning
            outputL += sampleL * grain.panL;
            outputR += sampleR * grain.panR;
            
            // Update grain state
            if (grain.reverse)
                grain.position -= grain.increment;
            else
                grain.position += grain.increment;
                
            // Handle looping/boundaries
            if (grain.position >= audioSource->getNumSamples())
                grain.position = 0.0f;
            else if (grain.position < 0.0f)
                grain.position = (float)(audioSource->getNumSamples() - 1);
                
            --grain.samplesRemaining;
        }
        
        // Apply voice envelope and velocity
        const float envelopeValue = envelope.getNextSample();
        outputL *= envelopeValue * velocity * 0.3f; // Scale down more for performance
        outputR *= envelopeValue * velocity * 0.3f;
        
        // Apply filter
        processFilter(outputL, outputR);
        
        // Apply chorus for widening
        processChorus(outputL, outputR);
        
        // Add to output buffer
        if (bufferIndex < buffer.getNumSamples())
        {
            buffer.addSample(0, bufferIndex, outputL);
            if (buffer.getNumChannels() > 1)
                buffer.addSample(1, bufferIndex, outputR);
        }
    }
}

void GranularVoice::spawnGrain()
{
    if (!audioSource || audioSource->getNumSamples() == 0)
        return;
        
    Grain newGrain;
    
    // CPU-Optimized LFO generation (use lookup tables or simple math)
    float lfoValue = generateLFO(0);  // LFO1
    float lfo2Value = generateLFO(1); // LFO2
    
    // Update scan position for Ableton-style motion
    updateScanPosition();
    
    // CPU-Optimized grain size calculation with enhanced modulation
    float grainSizeMs = juce::jmap(parameters.grainSize, 10.0f, 2000.0f);
    
    // Apply LFO modulation to grain size (CPU-optimized)
    if (parameters.lfoTarget == 2.0f && parameters.lfoAmount > 0.01f) // Size modulation
    {
        grainSizeMs *= (1.0f + lfoValue * parameters.lfoAmount * 0.5f);
        grainSizeMs = juce::jlimit(10.0f, 2000.0f, grainSizeMs);
    }
    
    // Apply jitter to grain size for Ableton-style variation
    if (parameters.jitter > 0.01f) {
        float jitterVariation = (random.nextFloat() * 2.0f - 1.0f) * parameters.jitter * 0.3f;
        grainSizeMs *= (1.0f + jitterVariation);
        grainSizeMs = juce::jlimit(10.0f, 2000.0f, grainSizeMs);
    }
    
    newGrain.totalSamples = (int)(grainSizeMs * currentSampleRate / 1000.0f);
    newGrain.samplesRemaining = newGrain.totalSamples;
    
    // Set grain shape (CPU-optimized - store as integer)
    newGrain.shapeType = (int)parameters.grainShape;
    
    // CPU-Optimized position calculation with Ableton-style features
    float basePosition = calculateGrainPosition() * (audioSource->getNumSamples() - 1);
    if (parameters.lfoTarget == 0.0f && parameters.lfoAmount > 0.01f) // Position modulation
    {
        basePosition += lfoValue * parameters.lfoAmount * audioSource->getNumSamples() * 0.3f;
    }
    
    // Apply scan motion (Ableton-style automatic movement)
    if (parameters.scan > 0.01f) {
        basePosition += scanPhase * audioSource->getNumSamples();
    }
    
    // Apply spray/texture (enhanced position randomization)
    float sprayAmount = juce::jmax(parameters.texture, parameters.spray);
    if (sprayAmount > 0.01f) {
        float jitter = (random.nextFloat() * 2.0f - 1.0f) * sprayAmount * audioSource->getNumSamples() * 0.2f;
        basePosition += jitter;
    }
    
    // Apply pitch jitter to individual grains
    if (parameters.pitchJitter > 0.01f) {
        newGrain.pitchOffset = (random.nextFloat() * 2.0f - 1.0f) * parameters.pitchJitter * 12.0f; // ±12 semitones
    }
    
    // Apply loop mode boundaries (CPU-optimized)
    int loopMode = (int)parameters.loopMode;
    if (loopMode == 1) { // Backward
        basePosition = audioSource->getNumSamples() - 1 - basePosition;
    }
    // PingPong mode will be handled in grain position update
    
    newGrain.startPosition = juce::jlimit(0.0f, (float)(audioSource->getNumSamples() - 1), basePosition);
    newGrain.position = newGrain.startPosition;
    
    // CPU-Optimized pitch calculation with enhanced modulation
    float midiPitch = (midiNote - 60) / 12.0f;
    float totalPitch = parameters.pitch + parameters.grainPitch + midiPitch * 12.0f;
    
    // Apply LFO pitch modulation (CPU-optimized)
    if (parameters.lfoTarget == 1.0f && parameters.lfoAmount > 0.01f) // Pitch modulation
    {
        totalPitch += lfoValue * parameters.lfoAmount * 12.0f; // ±1 octave pitch modulation
    }
    
    // Add individual grain pitch jitter
    totalPitch += newGrain.pitchOffset;
    
    newGrain.increment = std::pow(2.0f, totalPitch / 12.0f);
    
    // Account for source sample rate difference
    if (sourceSampleRate != currentSampleRate)
        newGrain.increment *= (float)(sourceSampleRate / currentSampleRate);
    
    // Reverse playback probability
    newGrain.reverse = random.nextFloat() < parameters.reverse;
    
    // Calculate stereo positioning
    float stereoPos = (random.nextFloat() * 2.0f - 1.0f) * parameters.stereoWidth;
    newGrain.panL = juce::jlimit(0.0f, 1.0f, 0.5f - stereoPos * 0.5f);
    newGrain.panR = juce::jlimit(0.0f, 1.0f, 0.5f + stereoPos * 0.5f);
    
    // Add the main grain
    activeGrains.push_back(newGrain);
    
    // Add unison grains for widening effect
    int numUnisonVoices = (int)parameters.unisonVoices;
    if (numUnisonVoices > 1 && activeGrains.size() < 12) // Limit total grains for performance
    {
        for (int i = 1; i < numUnisonVoices && activeGrains.size() < 16; ++i)
        {
            Grain unisonGrain = newGrain; // Copy the main grain
            
            // Detune slightly for chorus effect
            float detuneAmount = ((float)i / (float)(numUnisonVoices - 1) - 0.5f) * 0.1f; // ±5 cents max
            unisonGrain.increment *= std::pow(2.0f, detuneAmount / 12.0f);
            
            // Spread in stereo field
            float unisonStereoPos = ((float)i / (float)(numUnisonVoices - 1) - 0.5f) * parameters.stereoWidth;
            unisonGrain.panL = juce::jlimit(0.0f, 1.0f, 0.5f - unisonStereoPos * 0.5f);
            unisonGrain.panR = juce::jlimit(0.0f, 1.0f, 0.5f + unisonStereoPos * 0.5f);
            
            // Slightly different start position for texture
            float positionVariation = (random.nextFloat() * 2.0f - 1.0f) * 0.01f; // ±1% position variation
            unisonGrain.position = juce::jlimit(0.0f, (float)(audioSource->getNumSamples() - 1), 
                                               newGrain.position + positionVariation * audioSource->getNumSamples());
            
            activeGrains.push_back(unisonGrain);
        }
    }
    
    // Limit number of active grains for better CPU performance
    if (activeGrains.size() > 20) // Further reduced from 32 to 20
        activeGrains.erase(activeGrains.begin());
}

float GranularVoice::getInterpolatedSample(int channel, float position) const
{
    if (!audioSource || channel >= audioSource->getNumChannels() || audioSource->getNumSamples() == 0)
        return 0.0f;
        
    // Linear interpolation
    const int index0 = (int)position;
    const int index1 = juce::jmin(index0 + 1, audioSource->getNumSamples() - 1);
    const float fraction = position - index0;
    
    if (index0 < 0 || index0 >= audioSource->getNumSamples())
        return 0.0f;
        
    const float sample0 = audioSource->getSample(channel, index0);
    const float sample1 = audioSource->getSample(channel, index1);
    
    return juce::jmap(fraction, 0.0f, 1.0f, sample0, sample1);
}

// CPU-Optimized Enhanced LFO System
float GranularVoice::generateLFO(int lfoIndex)
{
    float* phase = (lfoIndex == 0) ? &lfoPhase : &lfo2Phase;
    float rate = (lfoIndex == 0) ? parameters.lfoRate : parameters.lfo2Rate;
    float shape = (lfoIndex == 0) ? parameters.lfoShape : parameters.lfo2Shape;
    
    // Generate LFO value based on shape (CPU-optimized)
    float lfoValue = getLFOShape(*phase, (int)shape);
    
    // Update LFO phase (CPU-optimized increment)
    float lfoRateHz = juce::jmap(rate, 0.1f, 20.0f);
    float phaseIncrement = 2.0f * juce::MathConstants<float>::pi * lfoRateHz / (float)currentSampleRate;
    *phase += phaseIncrement;
    
    // Keep phase in range (CPU-optimized)
    if (*phase > 2.0f * juce::MathConstants<float>::pi)
        *phase -= 2.0f * juce::MathConstants<float>::pi;
        
    return lfoValue;
}

// CPU-Optimized LFO shape generation
float GranularVoice::getLFOShape(float phase, int shapeType)
{
    switch (shapeType) {
        case 0: return std::sin(phase);  // Sine
        case 1: return 2.0f * (phase / (2.0f * juce::MathConstants<float>::pi)) - 1.0f;  // Triangle (approximation)
        case 2: return (phase < juce::MathConstants<float>::pi) ? 1.0f : -1.0f;  // Square
        case 3: return 2.0f * (phase / (2.0f * juce::MathConstants<float>::pi)) - 1.0f;  // Saw
        case 4: return (random.nextFloat() * 2.0f - 1.0f);  // Random
        default: return std::sin(phase);
    }
}

void GranularVoice::processFilter(float& sampleL, float& sampleR)
{
    if (parameters.filterCutoff < 1.0f)
    {
        sampleL = filterL.processSample(sampleL);
        sampleR = filterR.processSample(sampleR);
    }
}

// CPU-Optimized Ableton-style scan position update
void GranularVoice::updateScanPosition()
{
    if (parameters.scan > 0.01f) {
        float scanRate = parameters.scan * 0.5f; // Slower scan for musical results
        scanPhase += scanRate / (float)currentSampleRate;
        
        // Handle loop modes efficiently
        int loopMode = (int)parameters.loopMode;
        if (loopMode == 2) { // PingPong
            if (scanPhase >= 1.0f || scanPhase <= 0.0f) {
                scanDirection *= -1.0f;
            }
            scanPhase = juce::jlimit(0.0f, 1.0f, scanPhase);
        } else {
            // Forward or backward
            while (scanPhase >= 1.0f) scanPhase -= 1.0f;
            while (scanPhase < 0.0f) scanPhase += 1.0f;
        }
    }
}

// CPU-Optimized grain position calculation
float GranularVoice::calculateGrainPosition()
{
    float position = parameters.position;
    
    // Apply freeze effect (Ableton-style)
    if (parameters.freeze > 0.01f) {
        // Mix between current position and frozen position
        float frozenPos = targetPlayPosition;
        position = juce::jmap(parameters.freeze, position, frozenPos);
    }
    
    // Apply glide/portamento
    if (parameters.glide > 0.01f) {
        float glideRate = 1.0f - parameters.glide * 0.99f; // More glide = slower change
        currentPlayPosition += (position - currentPlayPosition) * glideRate;
        position = currentPlayPosition;
    }
    
    return position;
}

// CPU-Optimized grain pitch calculation
float GranularVoice::calculateGrainPitch(const Grain& grain)
{
    float pitch = parameters.pitch + parameters.grainPitch;
    pitch += grain.pitchOffset; // Individual grain jitter
    return pitch;
}

// CPU-Optimized grain envelope calculation
float GranularVoice::calculateGrainEnvelope(const Grain& grain)
{
    float envelopePhase = 1.0f - (float)grain.samplesRemaining / (float)grain.totalSamples;
    float grainEnvelope = 0.0f;
    
    // Apply grain shape (CPU-optimized)
    switch (grain.shapeType) {
        case 0: // Hann window (default)
            grainEnvelope = 0.5f - 0.5f * std::cos(2.0f * juce::MathConstants<float>::pi * envelopePhase);
            break;
        case 1: // Triangle
            grainEnvelope = (envelopePhase < 0.5f) ? (2.0f * envelopePhase) : (2.0f * (1.0f - envelopePhase));
            break;
        case 2: // Square (no envelope)
            grainEnvelope = 1.0f;
            break;
        case 3: // Gaussian (approximation)
            {
                float x = (envelopePhase - 0.5f) * 4.0f; // Scale to -2 to 2
                grainEnvelope = std::exp(-x * x);
            }
            break;
        default:
            grainEnvelope = 0.5f - 0.5f * std::cos(2.0f * juce::MathConstants<float>::pi * envelopePhase);
    }
    
    // Apply grain amplitude variation
    if (parameters.grainAmp > 0.01f) {
        grainEnvelope *= grain.ampMultiplier;
    }
    
    return grainEnvelope;
}

// CPU-Optimized jittered timing calculation
bool GranularVoice::shouldSpawnGrain()
{
    if (parameters.jitter > 0.01f) {
        nextGrainTime += calculateJitteredTiming();
        return nextGrainTime >= 1.0f;
    }
    return true; // Use regular timing
}

float GranularVoice::calculateJitteredTiming()
{
    float jitterAmount = (random.nextFloat() * 2.0f - 1.0f) * parameters.jitter;
    return 1.0f + jitterAmount * 0.5f; // ±50% timing variation
}

// Chorus processing for widening effect
void GranularVoice::processChorus(float& sampleL, float& sampleR)
{
    if (parameters.chorusAmount < 0.01f) return; // Skip if chorus disabled
    
    // Update chorus LFO phase
    const float chorusRate = 0.5f; // Fixed chorus rate for stability
    const float phaseIncrement = 2.0f * juce::MathConstants<float>::pi * chorusRate / (float)currentSampleRate;
    chorusLFOPhase += phaseIncrement;
    if (chorusLFOPhase > 2.0f * juce::MathConstants<float>::pi)
        chorusLFOPhase -= 2.0f * juce::MathConstants<float>::pi;
    
    // Create modulated delay times (3-15ms range for chorus effect)
    const float baseDelayMs = 8.0f;
    const float modulationDepthMs = 5.0f;
    
    float leftModulation = std::sin(chorusLFOPhase) * modulationDepthMs;
    float rightModulation = std::sin(chorusLFOPhase + juce::MathConstants<float>::pi * 0.5f) * modulationDepthMs; // 90° phase shift
    
    float leftDelayTime = (baseDelayMs + leftModulation) * currentSampleRate / 1000.0f;
    float rightDelayTime = (baseDelayMs + rightModulation) * currentSampleRate / 1000.0f;
    
    // Set delay times
    chorusDelayL.setDelay(juce::jlimit(1.0f, 720.0f, leftDelayTime)); // Max ~15ms at 48kHz
    chorusDelayR.setDelay(juce::jlimit(1.0f, 720.0f, rightDelayTime));
    
    // Process chorus
    float chorusL = chorusDelayL.popSample(0);
    float chorusR = chorusDelayR.popSample(0);
    
    chorusDelayL.pushSample(0, sampleL);
    chorusDelayR.pushSample(0, sampleR);
    
    // Mix with original signal
    float chorusMix = parameters.chorusAmount * 0.5f; // 50% max chorus mix
    sampleL = sampleL * (1.0f - chorusMix) + chorusL * chorusMix;
    sampleR = sampleR * (1.0f - chorusMix) + chorusR * chorusMix;
}
