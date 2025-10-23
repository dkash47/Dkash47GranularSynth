#include "PluginEditor.h"
#include "PluginProcessor.h"

static juce::Colour accent()           { return juce::Colour::fromRGB(240, 80, 90); }
static juce::Colour accent2()          { return juce::Colour::fromRGB(120, 200, 255); }

Dkash47GranularSynthAudioProcessorEditor::Dkash47GranularSynthAudioProcessorEditor(Dkash47GranularSynthAudioProcessor& p)
    : juce::AudioProcessorEditor(&p), processor(p), thumbnail(1024, thumbFormats, thumbCache)
{
    setResizable(false, false);
    setSize(1200, 750);  // More compact size while keeping readability
    setLookAndFeel(&futuristicLNF);

    thumbFormats.registerBasicFormats();
    thumbnail.addChangeListener(this);
    startTimerHz(10); // Further reduced to 10Hz for better CPU performance

    // Setup all sliders with modern styling
    auto setupKnob = [](juce::Slider& s)
    {
        s.setColour(juce::Slider::rotarySliderFillColourId, accent());
        s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    };

    // Core controls
    setupKnob(grainSize);
    setupKnob(density);
    setupKnob(texture);
    setupKnob(pitch);
    setupKnob(reverse);
    
    // Advanced controls
    setupKnob(stereoWidth);
    setupKnob(grainPitch);
    setupKnob(freeze);
    setupKnob(filterCutoff);
    setupKnob(filterRes);
    
    // Envelope controls
    setupKnob(attack);
    setupKnob(decay);
    setupKnob(sustain);
    setupKnob(release);
    
    // Modulation controls
    setupKnob(lfoRate);
    setupKnob(lfoAmount);
    
    // Effects
    setupKnob(reverbMix);
    setupKnob(delayMix);
    
    // Widening effects (for constant granular sound)
    setupKnob(chorusAmount);
    setupKnob(unisonVoices);
    setupKnob(level);

    // Position slider special styling
    position.setColour(juce::Slider::trackColourId, accent().withAlpha(0.3f));
    position.setColour(juce::Slider::thumbColourId, accent());

    // LFO Target ComboBox
    lfoTarget.addItem("Position", 1);
    lfoTarget.addItem("Pitch", 2);
    lfoTarget.addItem("Size", 3);
    lfoTarget.setSelectedId(1);

    // Add all controls to editor
    for (auto* s : { &grainSize, &density, &texture, &pitch, &position, &reverse,
                     &stereoWidth, &grainPitch, &freeze, &filterCutoff, &filterRes,
                     &attack, &decay, &sustain, &release,
                     &lfoRate, &lfoAmount, &reverbMix, &delayMix, 
                     &chorusAmount, &unisonVoices, &level })
        addAndMakeVisible(s);
    
    addAndMakeVisible(lfoTarget);
    addAndMakeVisible(testTone);
    addAndMakeVisible(lfoVisualizer);
    
    midiLabel.setJustificationType(juce::Justification::centredLeft);
    midiLabel.setColour(juce::Label::textColourId, accent2());
    addAndMakeVisible(midiLabel);

    // Parameter attachments
    auto& apvts = processor.apvts;
    grainSizeA    = std::make_unique<SliderAttachment>(apvts, Params::GrainSize,    grainSize);
    densityA      = std::make_unique<SliderAttachment>(apvts, Params::Density,      density);
    textureA      = std::make_unique<SliderAttachment>(apvts, Params::Texture,      texture);
    pitchA        = std::make_unique<SliderAttachment>(apvts, Params::Pitch,        pitch);
    positionA     = std::make_unique<SliderAttachment>(apvts, Params::Position,     position);
    reverseA      = std::make_unique<SliderAttachment>(apvts, Params::Reverse,      reverse);
    
    stereoWidthA  = std::make_unique<SliderAttachment>(apvts, Params::StereoWidth,  stereoWidth);
    grainPitchA   = std::make_unique<SliderAttachment>(apvts, Params::GrainPitch,   grainPitch);
    freezeA       = std::make_unique<SliderAttachment>(apvts, Params::Freeze,       freeze);
    filterCutoffA = std::make_unique<SliderAttachment>(apvts, Params::FilterCutoff, filterCutoff);
    filterResA    = std::make_unique<SliderAttachment>(apvts, Params::FilterRes,    filterRes);
    
    attackA       = std::make_unique<SliderAttachment>(apvts, Params::Attack,       attack);
    decayA        = std::make_unique<SliderAttachment>(apvts, Params::Decay,        decay);
    sustainA      = std::make_unique<SliderAttachment>(apvts, Params::Sustain,      sustain);
    releaseA      = std::make_unique<SliderAttachment>(apvts, Params::Release,      release);
    
    lfoRateA      = std::make_unique<SliderAttachment>(apvts, Params::LFORate,      lfoRate);
    lfoAmountA    = std::make_unique<SliderAttachment>(apvts, Params::LFOAmount,    lfoAmount);
    lfoTargetA    = std::make_unique<ComboBoxAttachment>(apvts, Params::LFOTarget,  lfoTarget);
    
    reverbMixA    = std::make_unique<SliderAttachment>(apvts, Params::ReverbMix,    reverbMix);
    delayMixA     = std::make_unique<SliderAttachment>(apvts, Params::DelayMix,     delayMix);
    
    chorusAmountA = std::make_unique<SliderAttachment>(apvts, Params::ChorusAmount, chorusAmount);
    unisonVoicesA = std::make_unique<SliderAttachment>(apvts, Params::UnisonVoices, unisonVoices);
    levelA        = std::make_unique<SliderAttachment>(apvts, Params::Level,        level);
    
    testToneA     = std::make_unique<ButtonAttachment>(apvts, Params::TestTone,     testTone);
}

void Dkash47GranularSynthAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Enhanced background with subtle gradient for alive feel
    juce::ColourGradient bgGrad(
        juce::Colour::fromRGB(30, 30, 30), 0, 0,
        juce::Colour::fromRGB(20, 20, 20), 0, (float)getHeight(), false);
    
    // Add MIDI reactive background brightness
    if (midiActivity > 0.1f) {
        auto brightnessFactor = 1.0f + (midiActivity * 0.15f);
        bgGrad = juce::ColourGradient(
            juce::Colour::fromRGB((int)(30 * brightnessFactor), (int)(30 * brightnessFactor), (int)(30 * brightnessFactor)), 0, 0,
            juce::Colour::fromRGB((int)(20 * brightnessFactor), (int)(20 * brightnessFactor), (int)(20 * brightnessFactor)), 0, (float)getHeight(), false);
    }
    g.setGradientFill(bgGrad);
    g.fillAll();

    // Enhanced title with reactive glow
    auto titleBrightness = 200 + (int)(midiActivity * 55);
    g.setColour(juce::Colour::fromRGB(titleBrightness, titleBrightness, titleBrightness));
    g.setFont(juce::FontOptions(18.0f).withStyle("Bold"));
    
    // Add subtle title glow for alive feel
    if (midiActivity > 0.2f) {
        g.setColour(juce::Colour::fromRGB(255, 150, 150).withAlpha(midiActivity * 0.4f));
        g.drawText("Dkash47 Granular Synthesizer", 19, 14, getWidth()-38, 37, juce::Justification::centredLeft);
        g.drawText("Dkash47 Granular Synthesizer", 21, 16, getWidth()-42, 33, juce::Justification::centredLeft);
    }
    g.setColour(juce::Colour::fromRGB(titleBrightness, titleBrightness, titleBrightness));
    g.drawText("Dkash47 Granular Synthesizer", 20, 15, getWidth()-40, 35, juce::Justification::centredLeft);

    // Enhanced MIDI LED positioned above test tone area (moved from top-right)
    auto led = juce::Rectangle<int>(getWidth() - 120, 25, 12, 12);
    auto ledColor = ledTicks > 0 ? juce::Colour::fromRGB(255, 100, 100) : juce::Colour::fromRGB(60, 60, 60);
    
    // Add glow effect to MIDI LED when active
    if (ledTicks > 0) {
        g.setColour(ledColor.withAlpha(0.4f));
        g.fillEllipse(led.toFloat().expanded(3.0f));
        g.setColour(ledColor.withAlpha(0.7f));
        g.fillEllipse(led.toFloat().expanded(1.5f));
    }
    g.setColour(ledColor);
    g.fillEllipse(led.toFloat());
    
    // MIDI label next to LED
    g.setColour(juce::Colour::fromRGB(140, 140, 140));
    g.setFont(juce::FontOptions(10.0f));
    g.drawText("MIDI", led.getX() - 35, led.getY(), 30, led.getHeight(), juce::Justification::centredRight);

    // Enhanced waveform area with reactive border
    auto wf = juce::Rectangle<int>(20, 65, getWidth() - 60, 220); // Made narrower to leave space for meter
    waveformBounds = wf;
    
    // Enhanced waveform background with subtle gradient
    juce::ColourGradient waveGrad(
        juce::Colour::fromRGB(40, 40, 40), wf.getCentreX(), (float)wf.getY(),
        juce::Colour::fromRGB(30, 30, 30), wf.getCentreX(), (float)wf.getBottom(), false);
    g.setGradientFill(waveGrad);
    g.fillRoundedRectangle(wf.toFloat(), 8.0f);
    
    // Reactive border that responds to MIDI activity
    auto borderBrightness = 120 + (int)(midiActivity * 80);
    auto borderAlpha = 0.7f + (midiActivity * 0.3f);
    g.setColour(juce::Colour::fromRGB(255, borderBrightness, borderBrightness).withAlpha(borderAlpha));
    g.drawRoundedRectangle(wf.toFloat(), 8.0f, 2.0f);
    
    // Add subtle inner glow for alive feel
    if (midiActivity > 0.1f) {
        g.setColour(juce::Colour::fromRGB(255, 100, 100).withAlpha(midiActivity * 0.2f));
        g.drawRoundedRectangle(wf.toFloat().reduced(2.0f), 6.0f, 1.0f);
    }

    // Enhanced volume meter positioned to the RIGHT of waveform (not overlapping)
    auto meter = juce::Rectangle<int>(wf.getRight() + 10, wf.getY() + 15, 18, wf.getHeight() - 30);
    
    // Meter background with gradient
    juce::ColourGradient meterBgGrad(
        juce::Colour::fromRGB(15, 25, 35), meter.getCentreX(), (float)meter.getY(),
        juce::Colour::fromRGB(10, 20, 30), meter.getCentreX(), (float)meter.getBottom(), false);
    g.setGradientFill(meterBgGrad);
    g.fillRoundedRectangle(meter.toFloat(), 6.0f);
    
    // Enhanced meter border with reactive glow
    auto meterBorderAlpha = 0.6f + (midiActivity * 0.4f);
    g.setColour(juce::Colour::fromRGB(0, 255, 150).withAlpha(meterBorderAlpha));
    g.drawRoundedRectangle(meter.toFloat(), 6.0f, 1.5f);
    
    // Reactive meter fill with enhanced gradient
    auto peakLevel = juce::jlimit(0.0f, 1.0f, processor.getLastPeak() * 2.5f);
    int filled = (int)(meter.getHeight() * peakLevel);
    if (filled > 0) {
        auto fillRect = meter.withTop(meter.getBottom() - filled).toFloat();
        juce::ColourGradient meterGrad(
            juce::Colour::fromRGB(0, 255, 120), fillRect.getCentreX(), fillRect.getBottom(),
            juce::Colour::fromRGB(255, 220, 0), fillRect.getCentreX(), fillRect.getY(), false);
        
        // Add red zone at top for peak indication
        if (peakLevel > 0.8f) {
            meterGrad.addColour(0.8, juce::Colour::fromRGB(255, 150, 0));
            meterGrad.addColour(1.0, juce::Colour::fromRGB(255, 50, 50));
        }
        
        g.setGradientFill(meterGrad);
        g.fillRoundedRectangle(fillRect.reduced(2.0f), 4.0f);
        
        // Add subtle glow on high levels
        if (peakLevel > 0.7f) {
            g.setColour(juce::Colour::fromRGB(255, 200, 100).withAlpha(peakLevel * 0.3f));
            g.fillRoundedRectangle(fillRect.reduced(1.0f), 5.0f);
        }
    }
    
    // Meter label
    g.setColour(juce::Colour::fromRGB(120, 120, 120));
    g.setFont(juce::FontOptions(9.0f));
    g.drawText("OUT", meter.getX() - 2, meter.getBottom() + 5, meter.getWidth() + 4, 15, juce::Justification::centred);

    if (thumbnail.getNumChannels() > 0)
    {
        // Enhanced waveform with reactive colors
        auto waveArea = wf.reduced(12);
        auto waveAlpha = 0.8f + (midiActivity * 0.2f);
        auto waveColor = juce::Colour::fromRGB(255, 120, 120).withAlpha(waveAlpha);
        g.setColour(waveColor);
        thumbnail.drawChannels(g, waveArea, 0.0, thumbnail.getTotalLength(), 1.0f);
        
        // Add subtle waveform glow during MIDI activity
        if (midiActivity > 0.2f) {
            g.setColour(juce::Colour::fromRGB(255, 150, 150).withAlpha(midiActivity * 0.3f));
            thumbnail.drawChannels(g, waveArea.expanded(1), 0.0, thumbnail.getTotalLength(), 1.0f);
        }

        // Enhanced playhead with glow
        auto playX = wf.getX() + int(processor.getPlayheadNorm() * wf.getWidth());
        auto playheadColor = juce::Colour::fromRGB(255, 200, 100);
        
        // Playhead glow effect
        g.setColour(playheadColor.withAlpha(0.4f));
        g.drawLine((float)playX, (float)wf.getY(), (float)playX, (float)wf.getBottom(), 6.0f);
        g.setColour(playheadColor.withAlpha(0.7f));
        g.drawLine((float)playX, (float)wf.getY(), (float)playX, (float)wf.getBottom(), 3.0f);
        g.setColour(playheadColor);
        g.drawLine((float)playX, (float)wf.getY(), (float)playX, (float)wf.getBottom(), 1.5f);
    }
    else
    {
        // Enhanced drop zone with subtle animation
        auto dropTextAlpha = 0.6f + std::sin(futuristicLNF.getAnimationTime() * 3.14f) * 0.2f;
        g.setColour(juce::Colour::fromRGB(140, 140, 140).withAlpha(dropTextAlpha));
        g.setFont(juce::FontOptions(15.0f));
        g.drawFittedText("Drag & Drop Audio Files Here", wf, juce::Justification::centred, 2);
        
        // Add subtle border pulse
        g.setColour(juce::Colour::fromRGB(100, 100, 100).withAlpha(dropTextAlpha * 0.5f));
        g.drawRoundedRectangle(wf.toFloat().reduced(20), 4.0f, 1.0f);
    }

    // Enhanced section labels with Quanta-style reactive glow
    auto sectionBrightness = 160 + (int)(midiActivity * 40);
    auto sectionAlpha = 0.9f + (midiActivity * 0.1f);
    
    // Main section headers with glow effect
    g.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
    
    auto drawSectionLabel = [&](const juce::String& text, int x, int y, int w, int h) {
        // Subtle glow during MIDI activity
        if (midiActivity > 0.1f) {
            g.setColour(juce::Colour::fromRGB(255, 180, 100).withAlpha(midiActivity * 0.4f));
            g.drawText(text, x-1, y-1, w, h, juce::Justification::centredLeft);
            g.drawText(text, x+1, y+1, w, h, juce::Justification::centredLeft);
        }
        g.setColour(juce::Colour::fromRGB(sectionBrightness, sectionBrightness, sectionBrightness).withAlpha(sectionAlpha));
        g.drawText(text, x, y, w, h, juce::Justification::centredLeft);
    };
    
    drawSectionLabel("OSCILLATORS", 40, 360, 120, 20);
    drawSectionLabel("GRANULAR OSC", 180, 360, 120, 20);
    drawSectionLabel("NOISE", 320, 360, 120, 20);
    drawSectionLabel("FILTERS", 460, 360, 120, 20);
    drawSectionLabel("EFFECTS", 800, 360, 120, 20);
    
    // Add subtle section separators like Quanta
    auto separatorAlpha = 0.3f + (midiActivity * 0.2f);
    g.setColour(juce::Colour::fromRGB(100, 100, 100).withAlpha(separatorAlpha));
    
    // Vertical separators between sections
    g.drawLine(440.0f, 355.0f, 440.0f, 375.0f, 1.0f);
    g.drawLine(780.0f, 355.0f, 780.0f, 375.0f, 1.0f);
    
    // Simple clean parameter labels
    auto drawLabel = [&g](const juce::String& text, juce::Rectangle<int> bounds) {
        auto labelBounds = bounds.withY(bounds.getBottom() + 5).withHeight(15);
        g.setColour(juce::Colour::fromRGB(180, 180, 180));
        g.setFont(juce::FontOptions(10.0f));
        g.drawText(text, labelBounds, juce::Justification::centred, false);
    };
    
    // Clean parameter labels matching your UI style
    drawLabel("Size", grainSize.getBounds());
    drawLabel("Density", density.getBounds());
    drawLabel("Texture", texture.getBounds());
    drawLabel("Pitch", pitch.getBounds());
    drawLabel("Reverse", reverse.getBounds());
    
    // Advanced labels
    drawLabel("Width", stereoWidth.getBounds());
    drawLabel("G.Pitch", grainPitch.getBounds());
    drawLabel("Freeze", freeze.getBounds());
    
    // Envelope labels
    drawLabel("A", attack.getBounds());
    drawLabel("D", decay.getBounds());
    drawLabel("S", sustain.getBounds());
    drawLabel("R", release.getBounds());
    
    // Filter labels
    drawLabel("Cutoff", filterCutoff.getBounds());
    drawLabel("Res", filterRes.getBounds());
    
    // LFO labels (new)
    drawLabel("Rate", lfoRate.getBounds());
    drawLabel("Amount", lfoAmount.getBounds());
    
    // Effects labels
    drawLabel("Reverb", reverbMix.getBounds());
    drawLabel("Delay", delayMix.getBounds());
    drawLabel("Level", level.getBounds());
    
    // Widening labels
    drawLabel("Chorus", chorusAmount.getBounds());
    drawLabel("Unison", unisonVoices.getBounds());
    
    drawLabel("Position", position.getBounds());
}

bool Dkash47GranularSynthAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (auto& f : files)
        if (f.endsWithIgnoreCase(".wav") || f.endsWithIgnoreCase(".mp3")) return true;
    return false;
}

void Dkash47GranularSynthAudioProcessorEditor::filesDropped(const juce::StringArray& files, int, int)
{
    for (auto& p : files)
    {
        juce::File f(p);
        if (! f.existsAsFile()) continue;
        if (processor.loadFile(f))
        {
            lastFile = f;
            thumbnail.setSource(new juce::FileInputSource(f));
            repaint();
            break;
        }
    }
}

void Dkash47GranularSynthAudioProcessorEditor::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &thumbnail) repaint();
}

void Dkash47GranularSynthAudioProcessorEditor::timerCallback()
{
    bool needsRepaint = false;
    static int updateCounter = 0;
    updateCounter++;
    
    // Update animation for alive feel
    futuristicLNF.updateAnimation();
    
    // Check MIDI activity with reduced frequency
    const int c = processor.getMidiCounter();
    if (c != lastMidiCounter) { 
        lastMidiCounter = c; 
        ledTicks = 12; // Longer LED display time for better visibility
        midiActivity = 1.0f; // Trigger MIDI animation
        needsRepaint = true;
    }
    else if (ledTicks > 0) { 
        --ledTicks; 
        needsRepaint = true;
    }
    
    // Fade MIDI activity animation with smooth curve
    if (midiActivity > 0.0f) {
        midiActivity -= 0.06f; // Slower fade for smoother, more alive animation
        if (midiActivity < 0.0f) midiActivity = 0.0f;
        needsRepaint = true;
    }
    
    // Update LookAndFeel with current MIDI intensity for reactive controls
    futuristicLNF.setMidiIntensity(midiActivity);
    
    // Check if sample was loaded and update thumbnail (less frequently)
    if (updateCounter % 5 == 0) // Only check every 5 timer callbacks
    {
        if (processor.getSampleBuffer() != nullptr && thumbnail.getNumChannels() == 0)
        {
            auto currentPath = processor.getCurrentSamplePath();
            if (currentPath.isNotEmpty())
            {
                juce::File sampleFile(currentPath);
                if (sampleFile.existsAsFile())
                {
                    thumbnail.setSource(new juce::FileInputSource(sampleFile));
                    needsRepaint = true;
                }
            }
        }
    }
    
    // Update LFO visualizer with reactive effects
    float lfoValue = processor.engine.getCurrentLFOValue();
    lfoVisualizer.setLFOPhase(std::asin(lfoValue));
    lfoVisualizer.setMidiIntensity(midiActivity);
    
    // Trigger repaint more frequently for alive feel
    if (needsRepaint || midiActivity > 0.01f || updateCounter % 3 == 0) // Repaint every 3rd cycle for smooth animations
        repaint();
}

void Dkash47GranularSynthAudioProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
    if (waveformBounds.contains(e.getPosition())) {
        dragging = true;
        auto rel = (e.x - waveformBounds.getX()) / (float) waveformBounds.getWidth();
        rel = juce::jlimit(0.0f, 1.0f, rel);
        // Set position parameter when clicking on waveform
        processor.apvts.getParameter(Params::Position)->beginChangeGesture();
        processor.apvts.getParameterAsValue(Params::Position) = rel;
        repaint();
    }
}

void Dkash47GranularSynthAudioProcessorEditor::mouseDrag(const juce::MouseEvent& e)
{
    if (!dragging) return;
    auto rel = (e.x - waveformBounds.getX()) / (float) waveformBounds.getWidth();
    rel = juce::jlimit(0.0f, 1.0f, rel);
    // Update position parameter when dragging on waveform
    processor.apvts.getParameterAsValue(Params::Position) = rel;
    repaint();
}

void Dkash47GranularSynthAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    const int margin = 20;
    const int knobSize = 65;
    const int smallKnobSize = 45;
    const int spacing = 15;
    
    // Header area with title and controls
    auto headerArea = bounds.removeFromTop(60);
    testTone.setBounds(headerArea.removeFromRight(100).reduced(10));
    midiLabel.setBounds(headerArea.removeFromLeft(300).withTrimmedTop(35));
    
    // Waveform area (like Quanta's main display)
    auto waveformArea = bounds.removeFromTop(220).reduced(margin);
    waveformBounds = waveformArea;
    
    // Position slider below waveform (like Quanta)
    auto positionArea = bounds.removeFromTop(50).reduced(margin + 40, 10);
    position.setBounds(positionArea);
    
    bounds.removeFromTop(20); // spacing
    
    // Main control area organized like Quanta
    auto controlsArea = bounds.reduced(margin);
    controlsArea.removeFromTop(20); // space for section labels
    
    // === IMPROVED ORGANIZED LAYOUT ===
    
    // Row 1: Main controls organized in clear sections
    auto mainRow = controlsArea.removeFromTop(knobSize + 40);
    
    // Section 1: Granular Core (left) - 5 knobs
    auto granularSection = mainRow.removeFromLeft(400).reduced(spacing);
    auto granRow = granularSection;
    const int granSpacing = granRow.getWidth() / 5;
    
    grainSize.setBounds(granRow.removeFromLeft(granSpacing).withSizeKeepingCentre(knobSize, knobSize));
    density.setBounds(granRow.removeFromLeft(granSpacing).withSizeKeepingCentre(knobSize, knobSize));
    texture.setBounds(granRow.removeFromLeft(granSpacing).withSizeKeepingCentre(knobSize, knobSize));
    pitch.setBounds(granRow.removeFromLeft(granSpacing).withSizeKeepingCentre(knobSize, knobSize));
    reverse.setBounds(granRow.withSizeKeepingCentre(knobSize, knobSize));
    
    // Section 2: Filters (middle) - 2 knobs
    auto filterSection = mainRow.removeFromLeft(180).reduced(spacing);
    auto filterRow = filterSection;
    const int filterSpacing = filterRow.getWidth() / 2;
    
    filterCutoff.setBounds(filterRow.removeFromLeft(filterSpacing).withSizeKeepingCentre(knobSize, knobSize));
    filterRes.setBounds(filterRow.withSizeKeepingCentre(knobSize, knobSize));
    
    // Section 3: Effects (right) - 5 knobs in 2 rows
    auto effectsSection = mainRow.reduced(spacing);
    
    // Top row of effects: Reverb, Delay, Level
    auto effectsTopRow = effectsSection.removeFromTop(knobSize);
    const int effectTopSpacing = effectsTopRow.getWidth() / 3;
    
    reverbMix.setBounds(effectsTopRow.removeFromLeft(effectTopSpacing).withSizeKeepingCentre(knobSize, knobSize));
    delayMix.setBounds(effectsTopRow.removeFromLeft(effectTopSpacing).withSizeKeepingCentre(knobSize, knobSize));
    level.setBounds(effectsTopRow.withSizeKeepingCentre(knobSize, knobSize));
    
    // Bottom row of effects: Chorus, Unison (with proper spacing)
    effectsSection.removeFromTop(10); // Small gap
    auto effectsBottomRow = effectsSection;
    const int effectBottomSpacing = effectsBottomRow.getWidth() / 2;
    
    chorusAmount.setBounds(effectsBottomRow.removeFromLeft(effectBottomSpacing).withSizeKeepingCentre(knobSize, knobSize));
    unisonVoices.setBounds(effectsBottomRow.withSizeKeepingCentre(knobSize, knobSize));
    
    controlsArea.removeFromTop(30); // spacing
    
    // Row 2: Bottom controls organized properly
    auto bottomArea = controlsArea;
    
    // Section 1: Advanced granular controls (left) - 3 knobs
    auto advGranularSection = bottomArea.removeFromLeft(240).reduced(spacing);
    const int advSpacing = advGranularSection.getWidth() / 3;
    
    stereoWidth.setBounds(advGranularSection.removeFromLeft(advSpacing).withSizeKeepingCentre(smallKnobSize, smallKnobSize));
    grainPitch.setBounds(advGranularSection.removeFromLeft(advSpacing).withSizeKeepingCentre(smallKnobSize, smallKnobSize));
    freeze.setBounds(advGranularSection.withSizeKeepingCentre(smallKnobSize, smallKnobSize));
    
    // Section 2: Envelope (ADSR) - 4 knobs
    auto envelopeSection = bottomArea.removeFromLeft(320).reduced(spacing);
    const int envSpacing = envelopeSection.getWidth() / 4;
    
    attack.setBounds(envelopeSection.removeFromLeft(envSpacing).withSizeKeepingCentre(smallKnobSize, smallKnobSize));
    decay.setBounds(envelopeSection.removeFromLeft(envSpacing).withSizeKeepingCentre(smallKnobSize, smallKnobSize));
    sustain.setBounds(envelopeSection.removeFromLeft(envSpacing).withSizeKeepingCentre(smallKnobSize, smallKnobSize));
    release.setBounds(envelopeSection.withSizeKeepingCentre(smallKnobSize, smallKnobSize));
    
    // Section 3: LFO and visualization (right)
    auto lfoSection = bottomArea.reduced(spacing);
    
    // Top part: LFO knobs
    auto lfoKnobsArea = lfoSection.removeFromTop(smallKnobSize);
    const int lfoSpacing = lfoKnobsArea.getWidth() / 2;
    
    lfoRate.setBounds(lfoKnobsArea.removeFromLeft(lfoSpacing).withSizeKeepingCentre(smallKnobSize, smallKnobSize));
    lfoAmount.setBounds(lfoKnobsArea.withSizeKeepingCentre(smallKnobSize, smallKnobSize));
    
    // Middle part: LFO Target combo box
    lfoSection.removeFromTop(5); // small gap
    lfoTarget.setBounds(lfoSection.removeFromTop(25).reduced(5));
    
    // Bottom part: LFO Visualizer
    lfoSection.removeFromTop(5); // small gap
    lfoVisualizer.setBounds(lfoSection.reduced(2));
}
