#pragma once
#include <JuceHeader.h>
#include "ParameterIDs.h"

class Dkash47GranularSynthAudioProcessor;

class Dkash47GranularSynthAudioProcessorEditor : public juce::AudioProcessorEditor,
                                                 public juce::FileDragAndDropTarget,
                                                 private juce::ChangeListener,
                                                 private juce::Timer
{
public:
    explicit Dkash47GranularSynthAudioProcessorEditor(Dkash47GranularSynthAudioProcessor&);
    ~Dkash47GranularSynthAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

    // Drag & drop
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int, int) override;

    // Thumbnail notifications
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    // Mouse for region selection
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void timerCallback() override;

private:
    Dkash47GranularSynthAudioProcessor& processor;

    // Enhanced Futuristic Look and Feel for Quanta-style reactiveness
    class FuturisticLNF : public juce::LookAndFeel_V4 {
    public:
        FuturisticLNF() { 
            setColour(juce::Slider::thumbColourId, juce::Colour::fromRGB(0, 255, 150)); 
            animationTime = 0.0f;
            midiIntensity = 0.0f;
        }
        
        void updateAnimation() { animationTime += 0.02f; if (animationTime > 1.0f) animationTime = 0.0f; }
        float getAnimationTime() const { return animationTime; }
        void setMidiIntensity(float intensity) { midiIntensity = intensity; }
        float getMidiIntensity() const { return midiIntensity; }
        
        void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float posProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& s) override
        {
            juce::ignoreUnused(s);
            const auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h).reduced(6.0f);
            auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
            auto centre = bounds.getCentre();
            auto ang = rotaryStartAngle + posProportional * (rotaryEndAngle - rotaryStartAngle);
            
            // Enhanced reactive glow based on MIDI activity and mouse interaction
            bool isMouseOver = s.isMouseOverOrDragging();
            float glowIntensity = 0.3f + (midiIntensity * 0.4f) + (isMouseOver ? 0.3f : 0.0f);
            float pulseEffect = 1.0f + std::sin(animationTime * 6.28f) * 0.1f * midiIntensity;
            
            // Outer glow for Quanta-style alive feel
            if (glowIntensity > 0.1f) {
                g.setColour(juce::Colour::fromRGB(255, 100, 100).withAlpha(glowIntensity * 0.6f));
                g.fillEllipse(bounds.expanded(4.0f * glowIntensity));
            }
            
            // Dark knob body with subtle gradient
            juce::ColourGradient knobGrad(
                juce::Colour::fromRGB(55, 55, 55), centre.x, centre.y - radius,
                juce::Colour::fromRGB(35, 35, 35), centre.x, centre.y + radius, false);
            g.setGradientFill(knobGrad);
            g.fillEllipse(bounds);
            
            // Reactive border that brightens with activity
            auto borderBrightness = 25 + (int)(glowIntensity * 40);
            g.setColour(juce::Colour::fromRGB(borderBrightness, borderBrightness, borderBrightness));
            g.drawEllipse(bounds, 1.5f);
            
            // Enhanced value arc with reactive effects
            juce::Path valuePath;
            auto arcRadius = radius - 6;
            valuePath.addArc(centre.x - arcRadius, centre.y - arcRadius, arcRadius * 2, arcRadius * 2, rotaryStartAngle, ang, true);
            
            // Main value arc with enhanced color
            auto arcColor = juce::Colour::fromRGB(255, 100, 100).withAlpha(0.9f + glowIntensity * 0.1f);
            g.setColour(arcColor);
            g.strokePath(valuePath, juce::PathStrokeType(3.5f * pulseEffect, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            
            // Subtle glow on the arc for alive feel
            if (midiIntensity > 0.1f) {
                g.setColour(arcColor.withAlpha(0.3f));
                g.strokePath(valuePath, juce::PathStrokeType(6.0f * pulseEffect, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
            
            // Center dot indicator
            auto dotRadius = 3.0f + (glowIntensity * 2.0f);
            g.setColour(juce::Colour::fromRGB(255, 150, 150).withAlpha(0.8f + glowIntensity * 0.2f));
            g.fillEllipse(centre.x - dotRadius/2, centre.y - dotRadius/2, dotRadius, dotRadius);
        }
        
        void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, const juce::Slider::SliderStyle style, juce::Slider& slider) override
        {
            juce::ignoreUnused(minSliderPos, maxSliderPos, style, slider);
            
            bool isMouseOver = slider.isMouseOverOrDragging();
            float reactiveIntensity = midiIntensity + (isMouseOver ? 0.4f : 0.0f);
            
            auto trackWidth = 8.0f + (reactiveIntensity * 2.0f);
            auto trackRect = juce::Rectangle<float>((float)x, (float)y + ((float)height - trackWidth) * 0.5f, (float)width, trackWidth);
            
            // Enhanced track background with gradient
            juce::ColourGradient trackGrad(
                juce::Colour::fromRGB(50, 50, 50), trackRect.getCentreX(), trackRect.getY(),
                juce::Colour::fromRGB(30, 30, 30), trackRect.getCentreX(), trackRect.getBottom(), false);
            g.setGradientFill(trackGrad);
            g.fillRoundedRectangle(trackRect, trackWidth * 0.5f);
            
            // Track border with reactive glow
            auto borderAlpha = 0.4f + (reactiveIntensity * 0.3f);
            g.setColour(juce::Colour::fromRGB(80, 80, 80).withAlpha(borderAlpha));
            g.drawRoundedRectangle(trackRect, trackWidth * 0.5f, 1.0f);
            
            // Enhanced filled portion with reactive effects
            auto fillRect = trackRect.withWidth((sliderPos - x));
            auto fillColor = juce::Colour::fromRGB(255, 100, 100).withAlpha(0.9f + reactiveIntensity * 0.1f);
            
            // Fill glow effect
            if (reactiveIntensity > 0.1f) {
                g.setColour(fillColor.withAlpha(0.3f));
                g.fillRoundedRectangle(fillRect.expanded(2.0f), trackWidth * 0.5f + 2.0f);
            }
            
            g.setColour(fillColor);
            g.fillRoundedRectangle(fillRect, trackWidth * 0.5f);
            
            // Enhanced thumb with reactive glow
            auto thumbSize = 14.0f + (reactiveIntensity * 4.0f);
            auto thumbRect = juce::Rectangle<float>(thumbSize, thumbSize).withCentre(juce::Point<float>(sliderPos, (float)y + (float)height * 0.5f));
            
            // Thumb glow
            if (reactiveIntensity > 0.1f) {
                g.setColour(juce::Colour::fromRGB(255, 150, 100).withAlpha(reactiveIntensity * 0.5f));
                g.fillEllipse(thumbRect.expanded(3.0f));
            }
            
            // Thumb gradient
            juce::ColourGradient thumbGrad(
                juce::Colour::fromRGB(255, 130, 130), thumbRect.getCentreX(), thumbRect.getY(),
                juce::Colour::fromRGB(255, 80, 80), thumbRect.getCentreX(), thumbRect.getBottom(), false);
            g.setGradientFill(thumbGrad);
            g.fillEllipse(thumbRect);
            
            // Thumb highlight
            g.setColour(juce::Colour::fromRGB(255, 200, 200).withAlpha(0.6f));
            g.fillEllipse(thumbRect.reduced(thumbSize * 0.3f));
        }
        
        void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown, int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& box) override
        {
            juce::ignoreUnused(isButtonDown, buttonX, buttonY, buttonW, buttonH, box);
            
            auto bounds = juce::Rectangle<float>(0, 0, (float)width, (float)height);
            
            // Simple dark background
            g.setColour(juce::Colour::fromRGB(45, 45, 45));
            g.fillRoundedRectangle(bounds, 4.0f);
            
            // Simple dark border
            g.setColour(juce::Colour::fromRGB(25, 25, 25));
            g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
            
            // Simple arrow
            auto arrowZone = bounds.removeFromRight(18.0f);
            juce::Path arrow;
            arrow.addTriangle(arrowZone.getCentreX() - 3, arrowZone.getCentreY() - 2,
                             arrowZone.getCentreX() + 3, arrowZone.getCentreY() - 2,
                             arrowZone.getCentreX(), arrowZone.getCentreY() + 2);
            g.setColour(juce::Colour::fromRGB(180, 180, 180));
            g.fillPath(arrow);
        }
        
        void positionComboBoxText(juce::ComboBox& box, juce::Label& label) override
        {
            label.setBounds(8, 1, box.getWidth() - 25, box.getHeight() - 2);
            label.setFont(juce::FontOptions(11.0f));
            label.setColour(juce::Label::textColourId, juce::Colour::fromRGB(180, 180, 180));
            label.setJustificationType(juce::Justification::centredLeft);
        }
        
    private:
        float animationTime;
        float midiIntensity;
    } futuristicLNF;

    // Core Granular Controls
    juce::Slider grainSize   { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider density     { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider texture     { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider pitch       { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider position    { juce::Slider::LinearHorizontal, juce::Slider::NoTextBox };
    juce::Slider reverse     { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    
    // Advanced Controls
    juce::Slider stereoWidth { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider grainPitch  { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider freeze      { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider filterCutoff{ juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider filterRes   { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    
    // Envelope Controls
    juce::Slider attack      { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider decay       { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider sustain     { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider release     { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    
    // Modulation Controls
    juce::Slider lfoRate     { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider lfoAmount   { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    juce::ComboBox lfoTarget;
    
    // Effects Controls
    juce::Slider reverbMix   { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider delayMix    { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    
    // Widening Controls (for constant granular sound)
    juce::Slider chorusAmount{ juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider unisonVoices{ juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    
    // Master Controls
    juce::Slider level       { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    
    std::unique_ptr<SliderAttachment> grainSizeA, densityA, textureA, pitchA, positionA, reverseA,
                                      stereoWidthA, grainPitchA, freezeA, filterCutoffA, filterResA,
                                      attackA, decayA, sustainA, releaseA,
                                      lfoRateA, lfoAmountA, reverbMixA, delayMixA,
                                      chorusAmountA, unisonVoicesA, levelA;
    std::unique_ptr<ComboBoxAttachment> lfoTargetA;
    std::unique_ptr<ButtonAttachment> testToneA;

    // Waveform
    juce::AudioThumbnailCache thumbCache { 5 };
    juce::AudioFormatManager thumbFormats;
    juce::AudioThumbnail thumbnail { 1024, thumbFormats, thumbCache };
    juce::File lastFile;

    // Region selection state
    juce::Rectangle<int> waveformBounds;
    bool dragging = false;

    // MIDI LED state
    int lastMidiCounter = 0;
    int ledTicks = 0; // frames remaining
    float midiActivity = 0.0f; // For MIDI-responsive animations

    // UI controls
    juce::ToggleButton testTone { "Test Tone" };
    juce::Label midiLabel;
    
    // Enhanced LFO Visualization with reactive effects
    class LFOVisualizer : public juce::Component {
    public:
        void paint(juce::Graphics& g) override {
            auto bounds = getLocalBounds().toFloat().reduced(1);
            
            // Enhanced background with gradient
            juce::ColourGradient bgGrad(
                juce::Colour::fromRGB(50, 50, 50), bounds.getCentreX(), bounds.getY(),
                juce::Colour::fromRGB(30, 30, 30), bounds.getCentreX(), bounds.getBottom(), false);
            g.setGradientFill(bgGrad);
            g.fillRoundedRectangle(bounds, 4.0f);
            
            // Reactive border with subtle glow
            auto borderAlpha = 0.6f + (midiIntensity * 0.4f);
            g.setColour(juce::Colour::fromRGB(120, 80, 80).withAlpha(borderAlpha));
            g.drawRoundedRectangle(bounds, 4.0f, 1.5f);
            
            // Enhanced sine wave with reactive thickness
            juce::Path wavePath;
            bool first = true;
            const int numPoints = 40; // More points for smoother wave
            
            for (int i = 0; i <= numPoints; ++i) {
                float x = bounds.getX() + 4 + ((bounds.getWidth() - 8) * (float)i / (float)numPoints);
                float phase = ((float)i / (float)numPoints) * 2.0f * juce::MathConstants<float>::pi + lfoPhase;
                float y = bounds.getCentreY() + std::sin(phase) * bounds.getHeight() * 0.35f;
                
                if (first) {
                    wavePath.startNewSubPath(x, y);
                    first = false;
                } else {
                    wavePath.lineTo(x, y);
                }
            }
            
            // Wave with reactive color and thickness
            auto waveAlpha = 0.8f + (midiIntensity * 0.2f);
            auto strokeWidth = 2.0f + (midiIntensity * 1.5f);
            auto waveColor = juce::Colour::fromRGB(255, 120, 120).withAlpha(waveAlpha);
            
            // Subtle wave glow during activity
            if (midiIntensity > 0.1f) {
                g.setColour(waveColor.withAlpha(midiIntensity * 0.3f));
                g.strokePath(wavePath, juce::PathStrokeType(strokeWidth + 2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
            
            g.setColour(waveColor);
            g.strokePath(wavePath, juce::PathStrokeType(strokeWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            
            // Enhanced current position indicator with glow
            float currentX = bounds.getCentreX();
            float currentY = bounds.getCentreY() + std::sin(lfoPhase) * bounds.getHeight() * 0.35f;
            
            auto dotSize = 4.0f + (midiIntensity * 2.0f);
            auto dotColor = juce::Colour::fromRGB(255, 180, 120);
            
            // Dot glow
            if (midiIntensity > 0.1f) {
                g.setColour(dotColor.withAlpha(midiIntensity * 0.5f));
                g.fillEllipse(currentX - dotSize, currentY - dotSize, dotSize * 2, dotSize * 2);
            }
            
            g.setColour(dotColor);
            g.fillEllipse(currentX - dotSize/2, currentY - dotSize/2, dotSize, dotSize);
        }
        
        void setLFOPhase(float phase) { lfoPhase = phase; repaint(); }
        void setMidiIntensity(float intensity) { midiIntensity = intensity; }
        
    private:
        float lfoPhase = 0.0f;
        float midiIntensity = 0.0f;
    } lfoVisualizer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Dkash47GranularSynthAudioProcessorEditor)
};
