#include "PluginEditor.h"
#include "dsp/TempoHelpers.h"
#include <algorithm>

namespace
{
constexpr int kMinHit = 56;
constexpr int kPreferredWidth = 780;
constexpr int kPreferredHeight = 780;

const juce::Colour kBgTop      { 0xff1a1c1e };
const juce::Colour kBgBottom   { 0xff101214 };
const juce::Colour kPanel      { 0xff24282c };
const juce::Colour kAccent     { 0xffc4a574 };
const juce::Colour kDemoAccent { 0xff8fbc8f };
const juce::Colour kText       { 0xffe6e2da };
const juce::Colour kMuted      { 0xff9a9590 };
const juce::Colour kToggleOff  { 0xff1c1f22 };

constexpr float kStartAngle = juce::MathConstants<float>::pi * 225.0f / 180.0f;
constexpr float kEndAngle   = juce::MathConstants<float>::pi * 495.0f / 180.0f;

juce::String formatPercent (float v)
{
    return juce::String (juce::roundToInt (v * 100.0f)) + " %";
}

juce::String formatPitch (float v)
{
    const int semis = juce::roundToInt (v);
    return (semis > 0 ? "+" : "") + juce::String (semis) + " st";
}
} // namespace

std::vector<float> EasyChorusAudioProcessorEditor::percentSteps()
{
    return { 0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f };
}

std::vector<float> EasyChorusAudioProcessorEditor::linSteps (float minV, float maxV, float step)
{
    std::vector<float> out;
    for (float v = minV; v <= maxV + 0.001f; v += step)
        out.push_back (juce::jlimit (minV, maxV, v));
    if (out.empty() || out.back() < maxV - 0.001f)
        out.push_back (maxV);
    return out;
}

EasyChorusAudioProcessorEditor::EasyChorusAudioProcessorEditor (EasyChorusAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (kPreferredWidth, kPreferredHeight);
    setResizable (true, true);
    setResizeLimits (560, 640, 1400, 1400);

    setupToggle (demoToggle,     "Demo",     "demoOn",       demoOnAttachment);
    setupToggle (chorusToggle,   "Chorus",   "chorusOn",     chorusOnAttachment);
    setupToggle (delayToggle,    "Delay",    "delayOn",      delayOnAttachment);
    setupToggle (shimmerToggle,  "Shimmer",  "shimmerOn",    shimmerOnAttachment);
    setupToggle (syncToggle,     "Sync",     "tempoSync",    tempoSyncAttachment);
    setupToggle (musicalToggle,  "Musical",  "delayMusical", delayMusicalAttachment);
    setupToggle (stereoToggle,   "Stereo",   "delayStereo",  delayStereoAttachment);
    setupToggle (pingPongToggle, "Ping",     "delayPingPong", delayPingPongAttachment);
    setupToggle (reverseToggle,  "Reverse",  "delayReverse", delayReverseAttachment);
    setupToggle (etherealToggle, "Ethereal", "shimmerEthereal", shimmerEtherealAttachment);
    refreshDemoLook();

    // In-plugin preset navigator (no Options menu required)
    auto styleNavButton = [this] (juce::TextButton& button)
    {
        button.setColour (juce::TextButton::buttonColourId, kToggleOff);
        button.setColour (juce::TextButton::buttonOnColourId, kAccent);
        button.setColour (juce::TextButton::textColourOffId, kText);
        button.setColour (juce::TextButton::textColourOnId, kBgBottom);
        button.setConnectedEdges (0);
        addAndMakeVisible (button);
    };
    styleNavButton (presetPrevButton);
    styleNavButton (presetNextButton);
    presetPrevButton.onClick = [this]() { selectPrevPreset(); };
    presetNextButton.onClick = [this]() { selectNextPreset(); };

    presetCombo.setColour (juce::ComboBox::backgroundColourId, kToggleOff);
    presetCombo.setColour (juce::ComboBox::outlineColourId, kMuted.withAlpha (0.35f));
    presetCombo.setColour (juce::ComboBox::textColourId, kText);
    presetCombo.setColour (juce::ComboBox::arrowColourId, kAccent);
    presetCombo.setTextWhenNothingSelected ("Preset");
    addAndMakeVisible (presetCombo);
    {
        const auto names = audioProcessor.getPresetManager().getPresetNames();
        for (int i = 0; i < names.size(); ++i)
            presetCombo.addItem (names[i], i + 1);
    }
    syncPresetComboFromProcessor();
    presetCombo.onChange = [this]()
    {
        if (ignorePresetComboCallback)
            return;
        loadSelectedPreset();
    };

    setupKnob (bpmKnob, "BPM", "bpm",
               [] (float v) { return juce::String (v, 0); },
               linSteps (40.0f, 240.0f, 10.0f),
               1.0);

    setupKnob (rateKnob, "Rate", "rate",
               [] (float v) { return juce::String (v, 2) + " Hz"; },
               { 0.1f, 0.25f, 0.5f, 0.7f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f },
               0.01);

    setupKnob (depthKnob, "Depth", "depth", formatPercent, percentSteps(), 0.01);

    // Delay dial host prepared here; binding depends on Musical mode
    delayKnob.dialHost.setInterceptsMouseClicks (false, true);
    addAndMakeVisible (delayKnob.dialHost);
    delayKnob.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    delayKnob.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    delayKnob.slider.setRotaryParameters (kStartAngle, kEndAngle, true);
    delayKnob.slider.setColour (juce::Slider::rotarySliderFillColourId, kAccent);
    delayKnob.slider.setColour (juce::Slider::rotarySliderOutlineColourId, kMuted.withAlpha (0.55f));
    delayKnob.slider.setColour (juce::Slider::thumbColourId, kText);
    delayKnob.dialHost.addAndMakeVisible (delayKnob.slider);
    delayKnob.steps.setTargetSlider (&delayKnob.slider);
    delayKnob.steps.setRotarySpan (kStartAngle, kEndAngle);
    delayKnob.steps.setColours (kMuted, kAccent);
    delayKnob.dialHost.addAndMakeVisible (delayKnob.steps);
    delayKnob.steps.toFront (false);
    delayKnob.title.setJustificationType (juce::Justification::centred);
    delayKnob.title.setColour (juce::Label::textColourId, kText);
    delayKnob.title.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    addAndMakeVisible (delayKnob.title);
    delayKnob.value.setJustificationType (juce::Justification::centred);
    delayKnob.value.setColour (juce::Label::textColourId, kMuted);
    delayKnob.value.setFont (juce::FontOptions (13.0f));
    addAndMakeVisible (delayKnob.value);
    delayKnob.slider.onValueChange = [this]()
    {
        updateDelayValueLabel();
        delayKnob.steps.repaint();
    };
    rebindDelayKnob();

    {
        auto fbSteps = percentSteps();
        fbSteps.erase (std::remove_if (fbSteps.begin(), fbSteps.end(),
                                       [] (float v) { return v > 0.85f; }),
                       fbSteps.end());
        if (fbSteps.empty() || fbSteps.back() < 0.85f)
            fbSteps.push_back (0.85f);
        setupKnob (feedbackKnob, "Feedback", "feedback", formatPercent, std::move (fbSteps), 0.01);
    }

    setupKnob (shimmerKnob, "Shimmer", "shimmer", formatPercent, percentSteps(), 0.01);
    setupKnob (pitchKnob, "Pitch", "shimmerPitch", formatPitch,
               { -12.0f, -5.0f, 0.0f, 7.0f, 12.0f, 19.0f, 24.0f }, 1.0);
    setupKnob (chorusLevelKnob,  "Ch Level",  "chorusLevel",  formatPercent, percentSteps(), 0.01);
    setupKnob (delayLevelKnob,   "Dly Level", "delayLevel",   formatPercent, percentSteps(), 0.01);
    setupKnob (shimmerLevelKnob, "Shim Level","shimmerLevel", formatPercent, percentSteps(), 0.01);
    setupKnob (mixKnob, "Mix", "mix", formatPercent, percentSteps(), 0.01);
    setupKnob (masterKnob, "Master", "master",
               [] (float v) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; },
               { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f, 1.25f, 1.5f }, 0.01);

    syncToggle.onClick = [this]()
    {
        refreshToggleLook (syncToggle, syncToggle.getToggleState());
        updateBpmInteraction();
        updateDelayValueLabel();
    };
    musicalToggle.onClick = [this]()
    {
        refreshToggleLook (musicalToggle, musicalToggle.getToggleState());
        rebindDelayKnob();
    };

    updateBpmInteraction();
    startTimerHz (30);
}

void EasyChorusAudioProcessorEditor::setupToggle (
    juce::TextButton& button,
    const juce::String& label,
    const juce::String& paramId,
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>& attachment)
{
    button.setButtonText (label);
    button.setClickingTogglesState (true);
    button.setColour (juce::TextButton::textColourOffId, kMuted);
    button.setColour (juce::TextButton::textColourOnId, kBgBottom);
    button.setColour (juce::TextButton::buttonColourId, kToggleOff);
    button.setColour (juce::TextButton::buttonOnColourId, kAccent);
    button.setConnectedEdges (0);
    addAndMakeVisible (button);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        audioProcessor.getAPVTS(), paramId, button);

    button.onClick = [this, &button]()
    {
        if (&button == &demoToggle)
            refreshDemoLook();
        else
            refreshToggleLook (button, button.getToggleState());
    };

    refreshToggleLook (button, button.getToggleState());
}

void EasyChorusAudioProcessorEditor::refreshToggleLook (juce::TextButton& button, bool isOn)
{
    button.setColour (juce::TextButton::buttonColourId, isOn ? kAccent : kToggleOff);
    button.setColour (juce::TextButton::buttonOnColourId, kAccent);
    button.setColour (juce::TextButton::textColourOffId, isOn ? kBgBottom : kMuted);
    button.setColour (juce::TextButton::textColourOnId, kBgBottom);
}

void EasyChorusAudioProcessorEditor::refreshDemoLook()
{
    const bool isOn = demoToggle.getToggleState();
    demoToggle.setColour (juce::TextButton::buttonColourId, isOn ? kDemoAccent : kToggleOff);
    demoToggle.setColour (juce::TextButton::buttonOnColourId, kDemoAccent);
    demoToggle.setColour (juce::TextButton::textColourOffId, isOn ? kBgBottom : kMuted);
    demoToggle.setColour (juce::TextButton::textColourOnId, kBgBottom);
}

void EasyChorusAudioProcessorEditor::setupKnob (Knob& knob,
                                                const juce::String& name,
                                                const juce::String& paramId,
                                                std::function<juce::String (float)> formatter,
                                                std::vector<float> stepValues,
                                                double snapInterval)
{
    knob.dialHost.setInterceptsMouseClicks (false, true);
    addAndMakeVisible (knob.dialHost);

    knob.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    knob.slider.setRotaryParameters (kStartAngle, kEndAngle, true);
    knob.slider.setColour (juce::Slider::rotarySliderFillColourId, kAccent);
    knob.slider.setColour (juce::Slider::rotarySliderOutlineColourId, kMuted.withAlpha (0.55f));
    knob.slider.setColour (juce::Slider::thumbColourId, kText);
    knob.dialHost.addAndMakeVisible (knob.slider);

    // Step dots above the dial; hitTest only claims clicks on the marks
    knob.steps.setTargetSlider (&knob.slider);
    knob.steps.setRotarySpan (kStartAngle, kEndAngle);
    knob.steps.setColours (kMuted, kAccent);
    knob.steps.setSteps (std::move (stepValues));
    knob.dialHost.addAndMakeVisible (knob.steps);
    knob.steps.toFront (false);

    knob.title.setText (name, juce::dontSendNotification);
    knob.title.setJustificationType (juce::Justification::centred);
    knob.title.setColour (juce::Label::textColourId, kText);
    knob.title.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    addAndMakeVisible (knob.title);

    knob.value.setJustificationType (juce::Justification::centred);
    knob.value.setColour (juce::Label::textColourId, kMuted);
    knob.value.setFont (juce::FontOptions (13.0f));
    addAndMakeVisible (knob.value);

    knob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), paramId, knob.slider);

    // Keep drag resolution fine, but ticks jump to exact steps
    if (snapInterval > 0.0)
        knob.slider.setRange (knob.slider.getMinimum(), knob.slider.getMaximum(), snapInterval);

    knob.slider.onValueChange = [&knob, formatter]()
    {
        knob.value.setText (formatter ((float) knob.slider.getValue()), juce::dontSendNotification);
        knob.steps.repaint();
    };

    knob.slider.onValueChange();
}

void EasyChorusAudioProcessorEditor::configureDelayFreeSteps()
{
    // Common delay times including BFMV ~380 ms
    delayKnob.steps.setSteps ({ 50.0f, 100.0f, 150.0f, 200.0f, 250.0f, 300.0f,
                                380.0f, 500.0f, 600.0f, 750.0f, 1000.0f });
    delayKnob.slider.setRange (50.0, 1000.0, 1.0);
}

void EasyChorusAudioProcessorEditor::configureDelayMusicalSteps()
{
    const int n = TempoHelpers::delayDivisionLabels().size();
    std::vector<float> idxs;
    idxs.reserve ((size_t) n);
    for (int i = 0; i < n; ++i)
        idxs.push_back ((float) i);
    delayKnob.steps.setSteps (std::move (idxs));
    delayKnob.slider.setRange (0.0, (double) n - 1.0, 1.0);
}

void EasyChorusAudioProcessorEditor::rebindDelayKnob()
{
    const bool musical = audioProcessor.isDelayMusical();
    delayKnobMusical = musical;
    delayKnob.attachment.reset();

    if (musical)
    {
        delayKnob.title.setText ("Note", juce::dontSendNotification);
        configureDelayMusicalSteps();
        delayKnob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            audioProcessor.getAPVTS(), "delayDivision", delayKnob.slider);
    }
    else
    {
        delayKnob.title.setText ("Delay", juce::dontSendNotification);
        configureDelayFreeSteps();
        delayKnob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            audioProcessor.getAPVTS(), "delay", delayKnob.slider);
    }

    delayKnob.slider.onValueChange = [this]()
    {
        updateDelayValueLabel();
        delayKnob.steps.repaint();
    };

    updateDelayValueLabel();
    delayKnob.steps.repaint();
}

void EasyChorusAudioProcessorEditor::updateDelayValueLabel()
{
    if (audioProcessor.isDelayMusical())
    {
        const int idx = juce::jlimit (0, TempoHelpers::delayDivisionLabels().size() - 1,
                                      juce::roundToInt (delayKnob.slider.getValue()));
        const float ms = audioProcessor.getCurrentDelayMs();
        delayKnob.value.setText (TempoHelpers::delayDivisionLabels()[idx]
                                     + "  (" + juce::String (juce::roundToInt (ms)) + " ms)",
                                 juce::dontSendNotification);
    }
    else
    {
        delayKnob.value.setText (juce::String (juce::roundToInt (delayKnob.slider.getValue())) + " ms",
                                 juce::dontSendNotification);
    }
}

void EasyChorusAudioProcessorEditor::updateBpmInteraction()
{
    const bool sync = audioProcessor.isTempoSyncEnabled();
    bpmKnob.slider.setEnabled (! sync);
    bpmKnob.slider.setAlpha (sync ? 0.45f : 1.0f);
    bpmKnob.steps.setEnabled (! sync);
    bpmKnob.steps.setAlpha (sync ? 0.35f : 1.0f);
    bpmKnob.title.setColour (juce::Label::textColourId, sync ? kMuted : kText);
}

void EasyChorusAudioProcessorEditor::refreshTempoLooks()
{
    refreshToggleLook (syncToggle, syncToggle.getToggleState());
    refreshToggleLook (musicalToggle, musicalToggle.getToggleState());
    updateBpmInteraction();

    if (audioProcessor.isTempoSyncEnabled())
    {
        const float bpm = audioProcessor.getEffectiveBpm();
        bpmKnob.value.setText (juce::String (bpm, 1) + " host", juce::dontSendNotification);
    }
}

void EasyChorusAudioProcessorEditor::refreshDelayModeLooks()
{
    refreshToggleLook (stereoToggle, stereoToggle.getToggleState());
    refreshToggleLook (pingPongToggle, pingPongToggle.getToggleState());
    refreshToggleLook (reverseToggle, reverseToggle.getToggleState());
    refreshToggleLook (etherealToggle, etherealToggle.getToggleState());
}

void EasyChorusAudioProcessorEditor::refreshPresetLook()
{
    const int expectedId = audioProcessor.getPresetManager().getCurrentIndex() + 1;
    if (presetCombo.getSelectedId() != expectedId)
        syncPresetComboFromProcessor();
}

void EasyChorusAudioProcessorEditor::syncPresetComboFromProcessor()
{
    ignorePresetComboCallback = true;
    const int idx = audioProcessor.getPresetManager().getCurrentIndex();
    presetCombo.setSelectedId (idx + 1, juce::dontSendNotification);
    ignorePresetComboCallback = false;
}

void EasyChorusAudioProcessorEditor::loadSelectedPreset()
{
    const int id = presetCombo.getSelectedId();
    if (id <= 0)
        return;
    audioProcessor.loadPreset (id - 1);
    rebindDelayKnob();
    updateBpmInteraction();
    refreshDelayModeLooks();
    refreshTempoLooks();
}

void EasyChorusAudioProcessorEditor::selectPrevPreset()
{
    const int idx = audioProcessor.getPresetManager().prevIndex();
    audioProcessor.loadPreset (idx);
    syncPresetComboFromProcessor();
    rebindDelayKnob();
    updateBpmInteraction();
    refreshDelayModeLooks();
    refreshTempoLooks();
}

void EasyChorusAudioProcessorEditor::selectNextPreset()
{
    const int idx = audioProcessor.getPresetManager().nextIndex();
    audioProcessor.loadPreset (idx);
    syncPresetComboFromProcessor();
    rebindDelayKnob();
    updateBpmInteraction();
    refreshDelayModeLooks();
    refreshTempoLooks();
}

void EasyChorusAudioProcessorEditor::timerCallback()
{
    const float rate = audioProcessor.getAPVTS().getRawParameterValue ("rate")->load();
    lfoPhase += rate / 30.0f;
    if (lfoPhase >= 1.0f)
        lfoPhase -= 1.0f;

    refreshDemoLook();
    refreshToggleLook (chorusToggle,  chorusToggle.getToggleState());
    refreshToggleLook (delayToggle,   delayToggle.getToggleState());
    refreshToggleLook (shimmerToggle, shimmerToggle.getToggleState());
    refreshTempoLooks();
    refreshDelayModeLooks();
    refreshPresetLook();

    if (delayKnobMusical != audioProcessor.isDelayMusical())
        rebindDelayKnob();
    else if (audioProcessor.isDelayMusical())
        updateDelayValueLabel();

    // Keep active step highlight in sync while dragging
    bpmKnob.steps.repaint();
    rateKnob.steps.repaint();
    depthKnob.steps.repaint();
    delayKnob.steps.repaint();
    feedbackKnob.steps.repaint();
    shimmerKnob.steps.repaint();
    pitchKnob.steps.repaint();
    chorusLevelKnob.steps.repaint();
    delayLevelKnob.steps.repaint();
    shimmerLevelKnob.steps.repaint();
    mixKnob.steps.repaint();
    masterKnob.steps.repaint();

    repaint (getLocalBounds().removeFromTop (56));
}

void EasyChorusAudioProcessorEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setGradientFill (juce::ColourGradient (kBgTop, 0.0f, 0.0f,
                                             kBgBottom, 0.0f, bounds.getHeight(), false));
    g.fillAll();

    auto panel = bounds.reduced (18.0f, 14.0f);
    g.setColour (kPanel);
    g.fillRoundedRectangle (panel, 14.0f);
    g.setColour (kMuted.withAlpha (0.25f));
    g.drawRoundedRectangle (panel, 14.0f, 1.2f);

    g.setColour (kText);
    g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    g.drawText ("Easy Chorus", 28, 18, 200, 28, juce::Justification::centredLeft);

    g.setColour (kMuted);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("chorus  ·  delay  ·  shimmer", 28, 42, 200, 16, juce::Justification::centredLeft);

    const float wobble = std::sin (lfoPhase * juce::MathConstants<float>::twoPi) * 6.0f;
    g.setColour (kAccent.withAlpha (0.85f));
    g.fillEllipse (210.0f + wobble, 28.0f, 6.0f, 6.0f);
}

void EasyChorusAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (28, 18);

    auto brandRow = area.removeFromTop (52);
    demoToggle.setBounds (brandRow.removeFromRight (120).withSizeKeepingCentre (120, juce::jmax (kMinHit - 4, 48)));
    brandRow.removeFromRight (10);
    layoutPresetBar (brandRow.removeFromRight (juce::jmin (320, brandRow.getWidth())));

    area.removeFromTop (4);
    layoutSectionToggles (area.removeFromTop (52));
    area.removeFromTop (6);
    layoutTempoRow (area.removeFromTop (100));
    area.removeFromTop (4);
    layoutDelayModeRow (area.removeFromTop (48));
    area.removeFromTop (6);
    layoutKnobs (area);
}

void EasyChorusAudioProcessorEditor::layoutPresetBar (juce::Rectangle<int> bounds)
{
    const int navW = 44;
    const int gap = 6;
    const int h = juce::jmax (40, juce::jmin (48, bounds.getHeight()));

    presetPrevButton.setBounds (bounds.removeFromLeft (navW).withSizeKeepingCentre (navW, h));
    bounds.removeFromLeft (gap);
    presetNextButton.setBounds (bounds.removeFromRight (navW).withSizeKeepingCentre (navW, h));
    bounds.removeFromRight (gap);
    presetCombo.setBounds (bounds.withSizeKeepingCentre (bounds.getWidth(), h));
}

void EasyChorusAudioProcessorEditor::layoutSectionToggles (juce::Rectangle<int> bounds)
{
    juce::TextButton* toggles[3] = { &chorusToggle, &delayToggle, &shimmerToggle };
    const int gap = 12;
    const int cellW = (bounds.getWidth() - gap * 2) / 3;
    const int height = juce::jmax (kMinHit - 4, bounds.getHeight());

    for (int i = 0; i < 3; ++i)
    {
        auto cell = juce::Rectangle<int> (bounds.getX() + i * (cellW + gap),
                                          bounds.getCentreY() - height / 2,
                                          cellW,
                                          height);
        toggles[i]->setBounds (cell);
    }
}

void EasyChorusAudioProcessorEditor::layoutTempoRow (juce::Rectangle<int> bounds)
{
    const int gap = 10;
    const int toggleW = juce::jmax (100, bounds.getWidth() / 5);
    const int toggleH = juce::jmax (kMinHit - 4, 48);

    auto left = bounds.removeFromLeft (toggleW * 2 + gap);
    syncToggle.setBounds (left.removeFromLeft (toggleW).withSizeKeepingCentre (toggleW, toggleH));
    left.removeFromLeft (gap);
    musicalToggle.setBounds (left.withSizeKeepingCentre (toggleW, toggleH));

    bounds.removeFromLeft (gap);
    layoutOneKnob (bpmKnob, bounds);
}

void EasyChorusAudioProcessorEditor::layoutDelayModeRow (juce::Rectangle<int> bounds)
{
    juce::TextButton* toggles[4] = { &stereoToggle, &pingPongToggle, &reverseToggle, &etherealToggle };
    const int gap = 8;
    const int cellW = (bounds.getWidth() - gap * 3) / 4;
    const int height = juce::jmax (kMinHit - 8, bounds.getHeight());

    for (int i = 0; i < 4; ++i)
    {
        auto cell = juce::Rectangle<int> (bounds.getX() + i * (cellW + gap),
                                          bounds.getCentreY() - height / 2,
                                          cellW,
                                          height);
        toggles[i]->setBounds (cell);
    }
}

void EasyChorusAudioProcessorEditor::layoutOneKnob (Knob& knob, juce::Rectangle<int> cell)
{
    auto titleArea = cell.removeFromTop (20);
    auto valueArea = cell.removeFromBottom (18);
    auto dialArea  = cell;

    const int dialSize = juce::jmax (kMinHit + 16, juce::jmin (dialArea.getWidth(), dialArea.getHeight()));
    auto dial = juce::Rectangle<int> (dialSize, dialSize).withCentre (dialArea.getCentre());

    knob.title.setBounds (titleArea);
    knob.dialHost.setBounds (dial);
    knob.steps.setBounds (knob.dialHost.getLocalBounds());

    // Slider inset so step dots sit around the rim and remain clickable
    const int inset = juce::jmax (14, dialSize / 8);
    knob.slider.setBounds (knob.dialHost.getLocalBounds().reduced (inset));
    knob.value.setBounds (valueArea);
}

void EasyChorusAudioProcessorEditor::layoutKnobs (juce::Rectangle<int> bounds)
{
    // Row 0: Rate Depth Delay
    // Row 1: Feedback Shimmer Pitch
    // Row 2: Ch Level / Dly Level / Shim Level
    // Row 3: Mix / Master (centered pair)
    Knob* row0[3] = { &rateKnob, &depthKnob, &delayKnob };
    Knob* row1[3] = { &feedbackKnob, &shimmerKnob, &pitchKnob };
    Knob* row2[3] = { &chorusLevelKnob, &delayLevelKnob, &shimmerLevelKnob };

    constexpr int cols = 3;
    constexpr int rows = 4;
    const int rowH = bounds.getHeight() / rows;
    const int cellW = bounds.getWidth() / cols;

    auto placeRow = [&] (Knob* row[3], int rowIndex)
    {
        for (int col = 0; col < cols; ++col)
        {
            auto cell = juce::Rectangle<int> (bounds.getX() + col * cellW,
                                              bounds.getY() + rowIndex * rowH,
                                              cellW,
                                              rowH).reduced (6);
            layoutOneKnob (*row[col], cell);
        }
    };

    placeRow (row0, 0);
    placeRow (row1, 1);
    placeRow (row2, 2);

    auto mixCell = juce::Rectangle<int> (bounds.getX() + cellW / 2,
                                         bounds.getY() + rowH * 3,
                                         cellW,
                                         rowH).reduced (6);
    layoutOneKnob (mixKnob, mixCell);

    auto masterCell = juce::Rectangle<int> (bounds.getX() + cellW + cellW / 2,
                                            bounds.getY() + rowH * 3,
                                            cellW,
                                            rowH).reduced (6);
    layoutOneKnob (masterKnob, masterCell);
}
