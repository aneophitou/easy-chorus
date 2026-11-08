#include "PluginEditor.h"
#include "dsp/TempoHelpers.h"
#include <algorithm>

namespace
{
constexpr int kMinHit = 40;
constexpr int kPreferredWidth = 980;
constexpr int kPreferredHeight = 900;
constexpr int kSectionTitleH = 22;
constexpr int kPanelPad = 8;

const juce::Colour kBgTop      { 0xff1a1c1e };
const juce::Colour kBgBottom   { 0xff101214 };
const juce::Colour kPanel      { 0xff24282c };
const juce::Colour kSection    { 0xff181b1e };
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

juce::String formatDecay (float v)
{
    return juce::String (v, v < 10.0f ? 2 : 1) + " s";
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
    setResizeLimits (820, 760, 1500, 1400);

    setupToggle (demoToggle,      "Demo",     "demoOn",          demoOnAttachment);
    setupToggle (chorusToggle,    "On",       "chorusOn",        chorusOnAttachment);
    setupToggle (delayToggle,     "On",       "delayOn",         delayOnAttachment);
    setupToggle (shimmerToggle,   "On",       "shimmerOn",       shimmerOnAttachment);
    setupToggle (delay2Toggle,    "On",       "delay2On",        delay2OnAttachment);
    setupToggle (shimmer2Toggle,  "On",       "shimmer2On",      shimmer2OnAttachment);
    setupToggle (syncToggle,      "Sync",     "tempoSync",       tempoSyncAttachment);
    setupToggle (musicalToggle,   "Musical",  "delayMusical",    delayMusicalAttachment);
    setupToggle (musical2Toggle,  "Musical",  "delay2Musical",   delay2MusicalAttachment);
    setupToggle (stereoToggle,    "Stereo",   "delayStereo",     delayStereoAttachment);
    setupToggle (pingPongToggle,  "Ping",     "delayPingPong",   delayPingPongAttachment);
    setupToggle (reverseToggle,   "Reverse",  "delayReverse",    delayReverseAttachment);
    setupToggle (stereo2Toggle,   "Stereo",   "delay2Stereo",    delay2StereoAttachment);
    setupToggle (pingPong2Toggle, "Ping",     "delay2PingPong",  delay2PingPongAttachment);
    setupToggle (reverse2Toggle,  "Reverse",  "delay2Reverse",   delay2ReverseAttachment);
    setupToggle (etherealToggle,  "Ethereal", "shimmerEthereal", shimmerEtherealAttachment);
    setupToggle (ethereal2Toggle, "Ethereal", "shimmer2Ethereal", shimmer2EtherealAttachment);
    setupToggle (wavesToggle,     "Waves",    "wavesOn",         wavesOnAttachment);
    refreshDemoLook();

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
               linSteps (40.0f, 240.0f, 10.0f), 1.0);
    setupKnob (rateKnob, "Rate", "rate",
               [] (float v) { return juce::String (v, 2) + " Hz"; },
               { 0.1f, 0.25f, 0.5f, 0.7f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f }, 0.01);
    setupKnob (depthKnob, "Depth", "depth", formatPercent, percentSteps(), 0.01);

    setupDelayKnobShell (delayKnob);
    setupDelayKnobShell (delay2Knob);
    rebindDelayKnob (false);
    rebindDelayKnob (true);

    auto fbSteps = percentSteps();
    fbSteps.erase (std::remove_if (fbSteps.begin(), fbSteps.end(),
                                   [] (float v) { return v > 0.85f; }),
                   fbSteps.end());
    if (fbSteps.empty() || fbSteps.back() < 0.85f)
        fbSteps.push_back (0.85f);
    setupKnob (feedbackKnob,  "Feedback", "feedback",  formatPercent, fbSteps, 0.01);
    setupKnob (feedback2Knob, "Feedback", "feedback2", formatPercent, fbSteps, 0.01);

    setupKnob (shimmerKnob,  "Amount", "shimmer",  formatPercent, percentSteps(), 0.01);
    setupKnob (shimmer2Knob, "Amount", "shimmer2", formatPercent, percentSteps(), 0.01);
    setupKnob (pitchKnob,  "Pitch", "shimmerPitch",  formatPitch,
               { -12.0f, -5.0f, 0.0f, 7.0f, 12.0f, 19.0f, 24.0f }, 1.0);
    setupKnob (pitch2Knob, "Pitch", "shimmer2Pitch", formatPitch,
               { -12.0f, -5.0f, 0.0f, 7.0f, 12.0f, 19.0f, 24.0f }, 1.0);
    setupKnob (decayKnob,  "Decay", "shimmerDecay",  formatDecay,
               { 0.4f, 1.0f, 2.0f, 3.5f, 5.0f, 8.0f, 12.0f }, 0.01);
    setupKnob (decay2Knob, "Decay", "shimmer2Decay", formatDecay,
               { 0.4f, 1.0f, 2.0f, 3.5f, 5.0f, 8.0f, 12.0f }, 0.01);

    setupKnob (chorusLevelKnob,   "Level", "chorusLevel",   formatPercent, percentSteps(), 0.01);
    setupKnob (delayLevelKnob,    "Level", "delayLevel",    formatPercent, percentSteps(), 0.01);
    setupKnob (shimmerLevelKnob,  "Level", "shimmerLevel",  formatPercent, percentSteps(), 0.01);
    setupKnob (delay2LevelKnob,   "Level", "delay2Level",   formatPercent, percentSteps(), 0.01);
    setupKnob (shimmer2LevelKnob, "Level", "shimmer2Level", formatPercent, percentSteps(), 0.01);
    setupKnob (mixKnob, "Mix", "mix", formatPercent, percentSteps(), 0.01);
    setupKnob (masterKnob, "Master", "master",
               [] (float v) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; },
               { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f, 1.25f, 1.5f }, 0.01);

    syncToggle.onClick = [this]()
    {
        refreshToggleLook (syncToggle, syncToggle.getToggleState());
        updateBpmInteraction();
        updateDelayValueLabel (false);
        updateDelayValueLabel (true);
    };
    musicalToggle.onClick = [this]()
    {
        refreshToggleLook (musicalToggle, musicalToggle.getToggleState());
        rebindDelayKnob (false);
    };
    musical2Toggle.onClick = [this]()
    {
        refreshToggleLook (musical2Toggle, musical2Toggle.getToggleState());
        rebindDelayKnob (true);
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

    knob.steps.setTargetSlider (&knob.slider);
    knob.steps.setRotarySpan (kStartAngle, kEndAngle);
    knob.steps.setColours (kMuted, kAccent);
    knob.steps.setSteps (std::move (stepValues));
    knob.dialHost.addAndMakeVisible (knob.steps);
    knob.steps.toFront (false);

    knob.title.setText (name, juce::dontSendNotification);
    knob.title.setJustificationType (juce::Justification::centred);
    knob.title.setColour (juce::Label::textColourId, kText);
    knob.title.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    addAndMakeVisible (knob.title);

    knob.value.setJustificationType (juce::Justification::centred);
    knob.value.setColour (juce::Label::textColourId, kMuted);
    knob.value.setFont (juce::FontOptions (11.0f));
    addAndMakeVisible (knob.value);

    knob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), paramId, knob.slider);

    if (snapInterval > 0.0)
        knob.slider.setRange (knob.slider.getMinimum(), knob.slider.getMaximum(), snapInterval);

    knob.slider.onValueChange = [&knob, formatter]()
    {
        knob.value.setText (formatter ((float) knob.slider.getValue()), juce::dontSendNotification);
        knob.steps.repaint();
    };

    knob.slider.onValueChange();
}

void EasyChorusAudioProcessorEditor::setupDelayKnobShell (Knob& knob)
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
    knob.steps.setTargetSlider (&knob.slider);
    knob.steps.setRotarySpan (kStartAngle, kEndAngle);
    knob.steps.setColours (kMuted, kAccent);
    knob.dialHost.addAndMakeVisible (knob.steps);
    knob.steps.toFront (false);
    knob.title.setJustificationType (juce::Justification::centred);
    knob.title.setColour (juce::Label::textColourId, kText);
    knob.title.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    addAndMakeVisible (knob.title);
    knob.value.setJustificationType (juce::Justification::centred);
    knob.value.setColour (juce::Label::textColourId, kMuted);
    knob.value.setFont (juce::FontOptions (11.0f));
    addAndMakeVisible (knob.value);
}

void EasyChorusAudioProcessorEditor::configureDelayFreeSteps (Knob& knob)
{
    knob.steps.setSteps ({ 50.0f, 100.0f, 150.0f, 200.0f, 250.0f, 300.0f,
                           380.0f, 500.0f, 600.0f, 750.0f, 1000.0f });
    knob.slider.setRange (50.0, 1000.0, 1.0);
}

void EasyChorusAudioProcessorEditor::configureDelayMusicalSteps (Knob& knob)
{
    const int n = TempoHelpers::delayDivisionLabels().size();
    std::vector<float> idxs;
    idxs.reserve ((size_t) n);
    for (int i = 0; i < n; ++i)
        idxs.push_back ((float) i);
    knob.steps.setSteps (std::move (idxs));
    knob.slider.setRange (0.0, (double) n - 1.0, 1.0);
}

void EasyChorusAudioProcessorEditor::rebindDelayKnob (bool isSecond)
{
    Knob& knob = isSecond ? delay2Knob : delayKnob;
    const bool musical = isSecond ? audioProcessor.isDelay2Musical()
                                  : audioProcessor.isDelayMusical();
    if (isSecond)
        delay2KnobMusical = musical;
    else
        delayKnobMusical = musical;

    knob.attachment.reset();

    if (musical)
    {
        knob.title.setText ("Note", juce::dontSendNotification);
        configureDelayMusicalSteps (knob);
        knob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            audioProcessor.getAPVTS(), isSecond ? "delay2Division" : "delayDivision", knob.slider);
    }
    else
    {
        knob.title.setText ("Time", juce::dontSendNotification);
        configureDelayFreeSteps (knob);
        knob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            audioProcessor.getAPVTS(), isSecond ? "delay2" : "delay", knob.slider);
    }

    knob.slider.onValueChange = [this, isSecond]()
    {
        updateDelayValueLabel (isSecond);
        (isSecond ? delay2Knob : delayKnob).steps.repaint();
    };

    updateDelayValueLabel (isSecond);
    knob.steps.repaint();
}

void EasyChorusAudioProcessorEditor::updateDelayValueLabel (bool isSecond)
{
    Knob& knob = isSecond ? delay2Knob : delayKnob;
    const bool musical = isSecond ? audioProcessor.isDelay2Musical()
                                  : audioProcessor.isDelayMusical();
    if (musical)
    {
        const int idx = juce::jlimit (0, TempoHelpers::delayDivisionLabels().size() - 1,
                                      juce::roundToInt (knob.slider.getValue()));
        const float ms = isSecond ? audioProcessor.getCurrentDelay2Ms()
                                  : audioProcessor.getCurrentDelayMs();
        knob.value.setText (TempoHelpers::delayDivisionLabels()[idx]
                                + "  (" + juce::String (juce::roundToInt (ms)) + " ms)",
                            juce::dontSendNotification);
    }
    else
    {
        knob.value.setText (juce::String (juce::roundToInt (knob.slider.getValue())) + " ms",
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
    refreshToggleLook (musical2Toggle, musical2Toggle.getToggleState());
    updateBpmInteraction();

    if (audioProcessor.isTempoSyncEnabled())
    {
        const float bpm = audioProcessor.getEffectiveBpm();
        bpmKnob.value.setText (juce::String (bpm, 1) + " host", juce::dontSendNotification);
    }
}

void EasyChorusAudioProcessorEditor::refreshModeLooks()
{
    refreshToggleLook (stereoToggle, stereoToggle.getToggleState());
    refreshToggleLook (pingPongToggle, pingPongToggle.getToggleState());
    refreshToggleLook (reverseToggle, reverseToggle.getToggleState());
    refreshToggleLook (stereo2Toggle, stereo2Toggle.getToggleState());
    refreshToggleLook (pingPong2Toggle, pingPong2Toggle.getToggleState());
    refreshToggleLook (reverse2Toggle, reverse2Toggle.getToggleState());
    refreshToggleLook (etherealToggle, etherealToggle.getToggleState());
    refreshToggleLook (ethereal2Toggle, ethereal2Toggle.getToggleState());
    refreshToggleLook (wavesToggle, wavesToggle.getToggleState());
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
    rebindDelayKnob (false);
    rebindDelayKnob (true);
    updateBpmInteraction();
    refreshModeLooks();
    refreshTempoLooks();
}

void EasyChorusAudioProcessorEditor::selectPrevPreset()
{
    const int idx = audioProcessor.getPresetManager().prevIndex();
    audioProcessor.loadPreset (idx);
    syncPresetComboFromProcessor();
    rebindDelayKnob (false);
    rebindDelayKnob (true);
    updateBpmInteraction();
    refreshModeLooks();
    refreshTempoLooks();
}

void EasyChorusAudioProcessorEditor::selectNextPreset()
{
    const int idx = audioProcessor.getPresetManager().nextIndex();
    audioProcessor.loadPreset (idx);
    syncPresetComboFromProcessor();
    rebindDelayKnob (false);
    rebindDelayKnob (true);
    updateBpmInteraction();
    refreshModeLooks();
    refreshTempoLooks();
}

void EasyChorusAudioProcessorEditor::timerCallback()
{
    const float rate = audioProcessor.getAPVTS().getRawParameterValue ("rate")->load();
    lfoPhase += rate / 30.0f;
    if (lfoPhase >= 1.0f)
        lfoPhase -= 1.0f;

    refreshDemoLook();
    refreshToggleLook (chorusToggle,   chorusToggle.getToggleState());
    refreshToggleLook (delayToggle,    delayToggle.getToggleState());
    refreshToggleLook (shimmerToggle,  shimmerToggle.getToggleState());
    refreshToggleLook (delay2Toggle,   delay2Toggle.getToggleState());
    refreshToggleLook (shimmer2Toggle, shimmer2Toggle.getToggleState());
    refreshTempoLooks();
    refreshModeLooks();
    refreshPresetLook();

    if (delayKnobMusical != audioProcessor.isDelayMusical())
        rebindDelayKnob (false);
    else if (audioProcessor.isDelayMusical())
        updateDelayValueLabel (false);

    if (delay2KnobMusical != audioProcessor.isDelay2Musical())
        rebindDelayKnob (true);
    else if (audioProcessor.isDelay2Musical())
        updateDelayValueLabel (true);

    for (Knob* k : { &bpmKnob, &rateKnob, &depthKnob, &delayKnob, &feedbackKnob,
                     &shimmerKnob, &pitchKnob, &decayKnob, &chorusLevelKnob,
                     &delayLevelKnob, &shimmerLevelKnob, &delay2Knob, &feedback2Knob,
                     &delay2LevelKnob, &shimmer2Knob, &pitch2Knob, &decay2Knob,
                     &shimmer2LevelKnob, &mixKnob, &masterKnob })
        k->steps.repaint();

    repaint (getLocalBounds().removeFromTop (56));
}

void EasyChorusAudioProcessorEditor::paintSectionPanel (juce::Graphics& g,
                                                        juce::Rectangle<float> bounds,
                                                        const juce::String& title) const
{
    g.setColour (kSection);
    g.fillRoundedRectangle (bounds, 12.0f);
    g.setColour (kAccent.withAlpha (0.28f));
    g.drawRoundedRectangle (bounds, 12.0f, 1.2f);

    auto titleArea = bounds.removeFromTop ((float) kSectionTitleH + 8.0f);
    g.setColour (kAccent.withAlpha (0.12f));
    g.fillRoundedRectangle (titleArea.reduced (1.0f, 1.0f), 11.0f);

    g.setColour (kAccent);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText (title,
                juce::roundToInt (titleArea.getX() + 12.0f),
                juce::roundToInt (titleArea.getY() + 4.0f),
                juce::roundToInt (titleArea.getWidth() - 24.0f),
                kSectionTitleH,
                juce::Justification::centredLeft);
}

void EasyChorusAudioProcessorEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setGradientFill (juce::ColourGradient (kBgTop, 0.0f, 0.0f,
                                             kBgBottom, 0.0f, bounds.getHeight(), false));
    g.fillAll();

    auto panel = bounds.reduced (14.0f, 12.0f);
    g.setColour (kPanel);
    g.fillRoundedRectangle (panel, 14.0f);
    g.setColour (kMuted.withAlpha (0.25f));
    g.drawRoundedRectangle (panel, 14.0f, 1.2f);

    g.setColour (kText);
    g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    g.drawText ("Easy Chorus", 24, 16, 200, 28, juce::Justification::centredLeft);

    g.setColour (kMuted);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("chorus  \u00b7  delay  \u00b7  shimmer  \u00d72", 24, 40, 260, 16, juce::Justification::centredLeft);

    const float wobble = std::sin (lfoPhase * juce::MathConstants<float>::twoPi) * 6.0f;
    g.setColour (kAccent.withAlpha (0.85f));
    g.fillEllipse (250.0f + wobble, 26.0f, 6.0f, 6.0f);

    paintSectionPanel (g, chorusPanelBounds,   "Chorus");
    paintSectionPanel (g, delayPanelBounds,    "Delay");
    paintSectionPanel (g, shimmerPanelBounds,  "Shimmer");
    paintSectionPanel (g, delay2PanelBounds,   "Delay 2");
    paintSectionPanel (g, shimmer2PanelBounds, "Shimmer 2");
    paintSectionPanel (g, outputPanelBounds,   "Output");
}

void EasyChorusAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (18, 12);

    auto brandRow = area.removeFromTop (46);
    demoToggle.setBounds (brandRow.removeFromRight (100).withSizeKeepingCentre (100, 42));
    brandRow.removeFromRight (8);
    layoutPresetBar (brandRow.removeFromRight (juce::jmin (300, brandRow.getWidth())));

    area.removeFromTop (8);

    const int gap = 8;
    const int rowH = (area.getHeight() - gap) / 2;
    auto topRow = area.removeFromTop (rowH);
    area.removeFromTop (gap);
    auto bottomRow = area;

    const int colWTop = (topRow.getWidth() - gap * 2) / 3;
    auto chorusArea = topRow.removeFromLeft (colWTop);
    topRow.removeFromLeft (gap);
    auto delayArea = topRow.removeFromLeft (colWTop);
    topRow.removeFromLeft (gap);
    auto shimmerArea = topRow;

    const int colWBot = (bottomRow.getWidth() - gap * 2) / 3;
    auto delay2Area = bottomRow.removeFromLeft (colWBot);
    bottomRow.removeFromLeft (gap);
    auto shimmer2Area = bottomRow.removeFromLeft (colWBot);
    bottomRow.removeFromLeft (gap);
    auto outputArea = bottomRow;

    chorusPanelBounds   = chorusArea.toFloat();
    delayPanelBounds    = delayArea.toFloat();
    shimmerPanelBounds  = shimmerArea.toFloat();
    delay2PanelBounds   = delay2Area.toFloat();
    shimmer2PanelBounds = shimmer2Area.toFloat();
    outputPanelBounds   = outputArea.toFloat();

    layoutChorusColumn (chorusArea.reduced (kPanelPad, kPanelPad));
    layoutDelayColumn (delayArea.reduced (kPanelPad, kPanelPad), false);
    layoutShimmerColumn (shimmerArea.reduced (kPanelPad, kPanelPad), false);
    layoutDelayColumn (delay2Area.reduced (kPanelPad, kPanelPad), true);
    layoutShimmerColumn (shimmer2Area.reduced (kPanelPad, kPanelPad), true);
    layoutOutputColumn (outputArea.reduced (kPanelPad, kPanelPad));
}

void EasyChorusAudioProcessorEditor::layoutPresetBar (juce::Rectangle<int> bounds)
{
    const int navW = 40;
    const int gap = 6;
    const int h = juce::jmax (38, juce::jmin (42, bounds.getHeight()));

    presetPrevButton.setBounds (bounds.removeFromLeft (navW).withSizeKeepingCentre (navW, h));
    bounds.removeFromLeft (gap);
    presetNextButton.setBounds (bounds.removeFromRight (navW).withSizeKeepingCentre (navW, h));
    bounds.removeFromRight (gap);
    presetCombo.setBounds (bounds.withSizeKeepingCentre (bounds.getWidth(), h));
}

void EasyChorusAudioProcessorEditor::layoutToggleRow (juce::Rectangle<int> bounds,
                                                      juce::TextButton* const* toggles,
                                                      int count,
                                                      int gap)
{
    if (count <= 0)
        return;

    const int cellW = (bounds.getWidth() - gap * (count - 1)) / count;
    const int height = juce::jmax (kMinHit - 4, juce::jmin (34, bounds.getHeight()));

    for (int i = 0; i < count; ++i)
    {
        auto cell = juce::Rectangle<int> (bounds.getX() + i * (cellW + gap),
                                          bounds.getCentreY() - height / 2,
                                          cellW,
                                          height);
        toggles[i]->setBounds (cell);
    }
}

void EasyChorusAudioProcessorEditor::layoutKnobStack (juce::Rectangle<int> bounds,
                                                      Knob* const* knobs,
                                                      int count)
{
    if (count <= 0)
        return;

    const int rowH = bounds.getHeight() / count;
    for (int i = 0; i < count; ++i)
    {
        auto cell = juce::Rectangle<int> (bounds.getX(),
                                          bounds.getY() + i * rowH,
                                          bounds.getWidth(),
                                          rowH).reduced (2, 1);
        layoutOneKnob (*knobs[i], cell);
    }
}

void EasyChorusAudioProcessorEditor::layoutChorusColumn (juce::Rectangle<int> bounds)
{
    bounds.removeFromTop (kSectionTitleH + 4);
    auto onRow = bounds.removeFromTop (32);
    chorusToggle.setBounds (onRow.withSizeKeepingCentre (juce::jmin (110, onRow.getWidth()), 30));
    bounds.removeFromTop (4);
    Knob* knobs[3] = { &rateKnob, &depthKnob, &chorusLevelKnob };
    layoutKnobStack (bounds, knobs, 3);
}

void EasyChorusAudioProcessorEditor::layoutDelayColumn (juce::Rectangle<int> bounds, bool isSecond)
{
    bounds.removeFromTop (kSectionTitleH + 4);

    auto& onToggle = isSecond ? delay2Toggle : delayToggle;
    auto& musical = isSecond ? musical2Toggle : musicalToggle;
    auto& stereo = isSecond ? stereo2Toggle : stereoToggle;
    auto& ping = isSecond ? pingPong2Toggle : pingPongToggle;
    auto& reverse = isSecond ? reverse2Toggle : reverseToggle;
    auto& timeKnob = isSecond ? delay2Knob : delayKnob;
    auto& fbKnob = isSecond ? feedback2Knob : feedbackKnob;
    auto& levelKnob = isSecond ? delay2LevelKnob : delayLevelKnob;

    auto onRow = bounds.removeFromTop (30);
    onToggle.setBounds (onRow.withSizeKeepingCentre (juce::jmin (90, onRow.getWidth()), 28));
    bounds.removeFromTop (3);

    juce::TextButton* modes[3] = { &stereo, &ping, &reverse };
    layoutToggleRow (bounds.removeFromTop (30), modes, 3, 4);
    bounds.removeFromTop (3);

    if (! isSecond)
    {
        juce::TextButton* tempo[2] = { &syncToggle, &musical };
        layoutToggleRow (bounds.removeFromTop (30), tempo, 2, 4);
        bounds.removeFromTop (4);

        const int tempoRowH = 72;
        auto stack = bounds;
        auto tempoRow = stack.removeFromBottom (tempoRowH);
        stack.removeFromBottom (2);
        Knob* knobs[3] = { &timeKnob, &fbKnob, &levelKnob };
        layoutKnobStack (stack, knobs, 3);
        layoutOneKnob (bpmKnob, tempoRow.reduced (2, 0));
    }
    else
    {
        juce::TextButton* tempo[1] = { &musical };
        layoutToggleRow (bounds.removeFromTop (30), tempo, 1, 0);
        bounds.removeFromTop (4);
        Knob* knobs[3] = { &timeKnob, &fbKnob, &levelKnob };
        layoutKnobStack (bounds, knobs, 3);
    }
}

void EasyChorusAudioProcessorEditor::layoutShimmerColumn (juce::Rectangle<int> bounds, bool isSecond)
{
    bounds.removeFromTop (kSectionTitleH + 4);

    auto& onToggle = isSecond ? shimmer2Toggle : shimmerToggle;
    auto& ethereal = isSecond ? ethereal2Toggle : etherealToggle;
    auto& amount = isSecond ? shimmer2Knob : shimmerKnob;
    auto& pitch = isSecond ? pitch2Knob : pitchKnob;
    auto& decay = isSecond ? decay2Knob : decayKnob;
    auto& level = isSecond ? shimmer2LevelKnob : shimmerLevelKnob;

    auto onRow = bounds.removeFromTop (30);
    onToggle.setBounds (onRow.withSizeKeepingCentre (juce::jmin (90, onRow.getWidth()), 28));
    bounds.removeFromTop (3);

    juce::TextButton* modes[1] = { &ethereal };
    layoutToggleRow (bounds.removeFromTop (30), modes, 1, 0);
    bounds.removeFromTop (4);

    Knob* knobs[4] = { &amount, &pitch, &decay, &level };
    layoutKnobStack (bounds, knobs, 4);
}

void EasyChorusAudioProcessorEditor::layoutOutputColumn (juce::Rectangle<int> bounds)
{
    bounds.removeFromTop (kSectionTitleH + 4);

    auto wavesRow = bounds.removeFromTop (34);
    wavesToggle.setBounds (wavesRow.withSizeKeepingCentre (juce::jmin (140, wavesRow.getWidth()), 32));
    bounds.removeFromTop (6);

    Knob* knobs[2] = { &mixKnob, &masterKnob };
    layoutKnobStack (bounds, knobs, 2);
}

void EasyChorusAudioProcessorEditor::layoutOneKnob (Knob& knob, juce::Rectangle<int> cell)
{
    auto titleArea = cell.removeFromTop (16);
    auto valueArea = cell.removeFromBottom (14);
    auto dialArea  = cell;

    const int dialSize = juce::jmax (kMinHit + 4, juce::jmin (dialArea.getWidth(), dialArea.getHeight()));
    auto dial = juce::Rectangle<int> (dialSize, dialSize).withCentre (dialArea.getCentre());

    knob.title.setBounds (titleArea);
    knob.dialHost.setBounds (dial);
    knob.steps.setBounds (knob.dialHost.getLocalBounds());

    const int inset = juce::jmax (10, dialSize / 8);
    knob.slider.setBounds (knob.dialHost.getLocalBounds().reduced (inset));
    knob.value.setBounds (valueArea);
}
