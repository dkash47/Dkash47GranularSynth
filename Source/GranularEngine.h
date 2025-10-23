#pragma once
#include <JuceHeader.h>

// Professional Quanta-style granular synthesizer engine
// Polyphonic with advanced grain processing and smooth interpolation
class GranularVoice : public juce::SynthesiserVoice {
public:
    struct GranularParams {
        // Core granular parameters (Quanta-style)
        float grainSize = 100.0f;      // ms (10-2000)
        float density = 0.5f;          // 0-1 (grains density)
        float texture = 0.5f;          // 0-1 (position randomization)
        float pitch = 0.0f;            // -48 to +48 semitones
        float position = 0.5f;         // 0-1 playback position
        float reverse = 0.0f;          // 0-1 reverse probability
        
        // Ableton-style granular features
        float scan = 0.0f;             // 0-1 automatic position scanning
        float spray = 0.5f;            // 0-1 position randomization (enhanced texture)
        float jitter = 0.0f;           // 0-1 timing randomization
        float pitchJitter = 0.0f;      // 0-1 pitch randomization per grain
        float grainShape = 0.0f;       // 0=Hann, 1=Triangle, 2=Square, 3=Gauss
        float loopMode = 0.0f;         // 0=Forward, 1=Backward, 2=PingPong
        float glide = 0.0f;            // 0-1 portamento time
        
        // Advanced parameters
        float stereoWidth = 0.0f;      // 0-1 stereo spread
        float grainPitch = 0.0f;       // -24 to +24 individual grain pitch
        float freeze = 0.0f;           // 0-1 position freeze
        float filterCutoff = 1.0f;     // 0-1 filter cutoff
        float filterRes = 0.0f;        // 0-1 filter resonance
        float filterType = 0.0f;       // 0=LP, 1=HP, 2=BP, 3=Notch
        float formantShift = 0.0f;     // -24 to +24 semitones formant shifting
        float randomSpread = 0.0f;     // 0-1 stereo grain placement randomization
        float grainAmp = 0.0f;         // 0-1 per-grain amplitude variation
        
        // Envelope
        float attack = 10.0f;          // ms
        float decay = 50.0f;           // ms
        float sustain = 1.0f;          // 0-1
        float release = 200.0f;        // ms
        
        // Enhanced Modulation System
        float lfoRate = 1.0f;          // Hz
        float lfoAmount = 0.0f;        // 0-1
        float lfoTarget = 0.0f;        // 0=position, 1=pitch, 2=size, 3=filter, 4=amp
        float lfoShape = 0.0f;         // 0=Sine, 1=Triangle, 2=Square, 3=Saw, 4=Random
        
        // Second LFO for complex modulation
        float lfo2Rate = 0.5f;         // Hz
        float lfo2Amount = 0.0f;       // 0-1
        float lfo2Target = 1.0f;       // Same targets as LFO1
        float lfo2Shape = 0.0f;        // Same shapes as LFO1
        
        // Widening effects for constant granular sound
        float chorusAmount = 0.0f;     // 0-1 chorus effect amount
        float unisonVoices = 1.0f;     // 1-8 number of unison voices
    };
    
    bool canPlaySound(juce::SynthesiserSound*) override { return true; }
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int currentPitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int newPitchWheelValue) override { juce::ignoreUnused(newPitchWheelValue); }
    void controllerMoved(int controllerNumber, int newControllerValue) override { juce::ignoreUnused(controllerNumber, newControllerValue); }
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;
    bool isVoiceActive() const override { return isActive; }
    
    void setAudioSource(const juce::AudioBuffer<float>* source, double sourceSampleRate);
    void setParameters(const GranularParams& params) { parameters = params; updateInternalParams(); }
    void prepare(double sampleRate, int maximumBlockSize);
    float getCurrentLFOValue() const { return std::sin(lfoPhase); }
    
private:
    struct Grain {
        float position = 0.0f;         // Current position in source
        float startPosition = 0.0f;    // Starting position
        float increment = 1.0f;        // Playback speed
        float envelope = 0.0f;         // Current envelope value
        float envelopeInc = 0.0f;      // Envelope increment
        int samplesRemaining = 0;      // Samples left in grain
        int totalSamples = 0;          // Total grain length
        float panL = 1.0f, panR = 1.0f; // Stereo positioning
        bool reverse = false;          // Reverse playbook
        float filterState1 = 0.0f, filterState2 = 0.0f; // Filter states
        
        // Enhanced Ableton-style features
        float pitchOffset = 0.0f;      // Individual grain pitch randomization
        float ampMultiplier = 1.0f;    // Per-grain amplitude variation
        float jitterOffset = 0.0f;     // Timing jitter for this grain
        int shapeType = 0;             // Grain shape (0=Hann, 1=Triangle, 2=Square, 3=Gauss)
        float formantShift = 1.0f;     // Formant preservation ratio
        float stereoPosition = 0.5f;   // Random stereo placement (0=left, 1=right)
    };
    
    const juce::AudioBuffer<float>* audioSource = nullptr;
    double sourceSampleRate = 44100.0;
    double currentSampleRate = 44100.0;
    
    GranularParams parameters;
    std::vector<Grain> activeGrains;
    
    // Voice state
    bool isActive = false;
    float velocity = 1.0f;
    int midiNote = 60;
    float pitchBend = 0.0f;
    
    // Grain spawning
    float grainSpawnTimer = 0.0f;
    float grainSpawnInterval = 0.1f;
    
    // ADSR envelope
    juce::ADSR envelope;
    juce::ADSR::Parameters envelopeParams;
    
    // Enhanced LFO System
    float lfoPhase = 0.0f;
    float lfo2Phase = 0.0f;
    
    // Multi-mode Filter System
    juce::dsp::IIR::Filter<float> filterL, filterR;
    juce::dsp::IIR::Filter<float> hpFilterL, hpFilterR;  // High-pass filters
    juce::dsp::IIR::Filter<float> bpFilterL, bpFilterR;  // Band-pass filters
    
    // Scan and Motion System
    float scanPhase = 0.0f;        // Current scan position
    float scanDirection = 1.0f;    // Scan direction (1 = forward, -1 = backward)
    
    // Position tracking for UI
    float currentPlayPosition = 0.0f;
    float targetPlayPosition = 0.0f;  // For glide/portamento
    
    // Enhanced Timing and Jitter
    float jitterAccumulator = 0.0f;
    float nextGrainTime = 0.0f;
    
    // Random number generation
    juce::Random random;
    
    // Chorus effect for widening
    juce::dsp::DelayLine<float> chorusDelayL { 48000 };
    juce::dsp::DelayLine<float> chorusDelayR { 48000 };
    float chorusLFOPhase = 0.0f;
    
    void updateInternalParams();
    void spawnGrain();
    void updateGrains(juce::AudioBuffer<float>& buffer, int startSample, int numSamples);
    float getInterpolatedSample(int channel, float position) const;
    
    // Enhanced LFO System
    float generateLFO(int lfoIndex = 0);  // 0=LFO1, 1=LFO2
    float getLFOShape(float phase, int shapeType);
    
    // Advanced Granular Features
    void updateScanPosition();
    float calculateGrainPosition();  // Position with scan, spray, and jitter
    float calculateGrainPitch(const Grain& grain);
    float calculateGrainEnvelope(const Grain& grain);
    void applyGrainShape(Grain& grain, float& envelope);
    
    // Enhanced Filter System
    void processFilter(float& sampleL, float& sampleR);
    void updateFilterCoefficients();
    void processFormantShift(float& sampleL, float& sampleR, float shiftAmount);
    
    // Timing and Jitter
    bool shouldSpawnGrain();
    float calculateJitteredTiming();
    
    // Widening effects
    void processChorus(float& sampleL, float& sampleR);
};

// Quanta-style synthesizer sound
class GranularSound : public juce::SynthesiserSound {
public:
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

// Main granular engine class
class GranularEngine {
public:
    using Params = GranularVoice::GranularParams;

    void prepare(double sampleRate, int maximumBlockSize) {
        synthesizer.setCurrentPlaybackSampleRate(sampleRate);
        
        // Clear existing voices and sounds
        synthesizer.clearVoices();
        synthesizer.clearSounds();
        
        // Add polyphonic voices (reduced to 8 for better performance)
        for (int i = 0; i < 8; ++i) {
            auto voice = new GranularVoice();
            voice->prepare(sampleRate, maximumBlockSize);
            synthesizer.addVoice(voice);
        }
        
        // Add sound
        synthesizer.addSound(new GranularSound());
    }
    
    void reset() {
        synthesizer.allNotesOff(0, true);
    }
    
    void setSource(const juce::AudioBuffer<float>* source, double sourceRate) {
        for (int i = 0; i < synthesizer.getNumVoices(); ++i) {
            if (auto* granularVoice = dynamic_cast<GranularVoice*>(synthesizer.getVoice(i))) {
                granularVoice->setAudioSource(source, sourceRate);
            }
        }
        audioSource = source;
        sourceSampleRate = sourceRate;
    }
    
    void setParams(const Params& p) {
        currentParams = p;
        for (int i = 0; i < synthesizer.getNumVoices(); ++i) {
            if (auto* granularVoice = dynamic_cast<GranularVoice*>(synthesizer.getVoice(i))) {
                granularVoice->setParameters(p);
            }
        }
    }
    
    void noteOn(int midiNote, float velocity) {
        synthesizer.noteOn(1, midiNote, velocity);
    }
    
    void noteOff(int midiNote) {
        synthesizer.noteOff(1, midiNote, 0.0f, true);
    }
    
    void allNotesOff() {
        synthesizer.allNotesOff(0, true);
    }
    
    void render(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
        synthesizer.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
    }

    // UI feedback
    float getPlayheadNorm() const {
        // Get average position from all active voices
        float totalPosition = 0.0f;
        int activeVoices = 0;
        
        for (int i = 0; i < synthesizer.getNumVoices(); ++i) {
            if (auto* voice = dynamic_cast<GranularVoice*>(synthesizer.getVoice(i))) {
                if (voice->isVoiceActive()) {
                    // Would need to expose position from voice
                    activeVoices++;
                }
            }
        }
        
        return activeVoices > 0 ? (totalPosition / activeVoices) : currentParams.position;
    }
    
    float getCurrentLFOValue() const {
        // Get LFO value from first active voice
        for (int i = 0; i < synthesizer.getNumVoices(); ++i) {
            if (auto* voice = dynamic_cast<GranularVoice*>(synthesizer.getVoice(i))) {
                if (voice->isVoiceActive()) {
                    return voice->getCurrentLFOValue();
                }
            }
        }
        return 0.0f;
    }

private:
    juce::Synthesiser synthesizer;
    const juce::AudioBuffer<float>* audioSource = nullptr;
    double sourceSampleRate = 44100.0;
    Params currentParams;
};
