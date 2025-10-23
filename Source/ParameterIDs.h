#pragma once

namespace Params {
    // Core Quanta-style granular parameters
    static constexpr const char* GrainSize     = "grainSize";       // 0..1 -> 10-2000ms
    static constexpr const char* Density       = "density";         // 0..1 grain density
    static constexpr const char* Texture       = "texture";         // 0..1 position randomization
    static constexpr const char* Pitch         = "pitch";           // -48..+48 semitones
    static constexpr const char* Position      = "position";        // 0..1 playback position
    static constexpr const char* Reverse       = "reverse";         // 0..1 reverse probability
    
    // Ableton-style granular features
    static constexpr const char* Scan          = "scan";            // 0..1 automatic position scanning
    static constexpr const char* Spray         = "spray";           // 0..1 position randomization (alias for texture)
    static constexpr const char* Jitter        = "jitter";          // 0..1 timing randomization
    static constexpr const char* PitchJitter   = "pitchJitter";     // 0..1 pitch randomization per grain
    static constexpr const char* GrainShape    = "grainShape";      // 0=Hann, 1=Triangle, 2=Square, 3=Gauss
    static constexpr const char* LoopMode      = "loopMode";        // 0=Forward, 1=Backward, 2=PingPong
    static constexpr const char* Glide         = "glide";           // 0..1 portamento time
    
    // Advanced parameters
    static constexpr const char* StereoWidth   = "stereoWidth";     // 0..1 stereo spread
    static constexpr const char* GrainPitch    = "grainPitch";      // -24..+24 individual grain pitch
    static constexpr const char* Freeze        = "freeze";          // 0..1 position freeze
    static constexpr const char* FilterCutoff  = "filterCutoff";    // 0..1 lowpass filter cutoff
    static constexpr const char* FilterRes     = "filterRes";       // 0..1 filter resonance
    static constexpr const char* FilterType    = "filterType";      // 0=LP, 1=HP, 2=BP, 3=Notch
    static constexpr const char* FormantShift  = "formantShift";    // -24..+24 semitones formant shifting
    static constexpr const char* RandomSpread  = "randomSpread";    // 0..1 stereo grain placement randomization
    static constexpr const char* GrainAmp      = "grainAmp";        // 0..1 per-grain amplitude variation
    
    // Envelope (ADSR)
    static constexpr const char* Attack        = "attack";          // 1..2000ms
    static constexpr const char* Decay         = "decay";           // 1..2000ms
    static constexpr const char* Sustain       = "sustain";         // 0..1
    static constexpr const char* Release       = "release";         // 5..4000ms
    
    // Modulation (Enhanced LFO System)
    static constexpr const char* LFORate       = "lfoRate";         // 0.1..20 Hz
    static constexpr const char* LFOAmount     = "lfoAmount";       // 0..1
    static constexpr const char* LFOTarget     = "lfoTarget";       // 0=position, 1=pitch, 2=size, 3=filter, 4=amp
    static constexpr const char* LFOShape      = "lfoShape";        // 0=Sine, 1=Triangle, 2=Square, 3=Saw, 4=Random
    
    // Additional LFOs for complex modulation
    static constexpr const char* LFO2Rate      = "lfo2Rate";        // 0.1..20 Hz
    static constexpr const char* LFO2Amount    = "lfo2Amount";      // 0..1
    static constexpr const char* LFO2Target    = "lfo2Target";      // Same targets as LFO1
    static constexpr const char* LFO2Shape     = "lfo2Shape";       // Same shapes as LFO1
    
    // Effects (simplified from original)
    static constexpr const char* ReverbMix     = "reverbMix";       // 0..1
    static constexpr const char* DelayMix      = "delayMix";        // 0..1
    static constexpr const char* ChorusAmount  = "chorusAmount";    // 0..1 chorus widening
    static constexpr const char* UnisonVoices  = "unisonVoices";    // 1..8 unison voices
    
    // Legacy parameters for compatibility
    static constexpr const char* Mix           = "mix";             // 0..1 (now always 1.0 for granular)
    static constexpr const char* Level         = "level";           // 0..1 master level
    
    // Debug/diagnostic
    static constexpr const char* TestTone      = "testTone";        // bool
}
