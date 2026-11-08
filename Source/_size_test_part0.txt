#include "PluginEditor.h"
#include "dsp/TempoHelpers.h"
#include <algorithm>

namespace
{
constexpr int kMinHit = 44;
constexpr int kPreferredWidth = 920;
constexpr int kPreferredHeight = 780;
constexpr int kSectionTitleH = 22;
constexpr int kPanelPad = 10;

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
    setResizeLimits (720, 680, 1400, 1200);

    setupToggle (demoToggle,     "Demo",     "demoOn",       demoOnAttachment);
    setupToggle (chorusToggle,   "On",       "chorusOn",     chorusOnAttachment);
    setupToggle (delayToggle,    "On",       "delayOn",      delayOnAttachment);
    setupToggle (shimmerToggle,  "On",       "shimmerOn",    shimmerOnAttachment);
    setupToggle (syncToggle,     "Sync",     "tempoSync",    tempoSyncAttachment);
    setupToggle (musicalToggle,  "Musical",  "delayMusical", delayMusicalAttachment);
    setupToggle (stereoToggle,   "Stereo",   "delayStereo",  delayStereoAttachment);
    setupToggle (pingPongToggle, "Ping",     "delayPingPong", delayPingPongAttachment);
    setupToggle (reverseToggle,  "Reverse",  "delayReverse", delayReverseAttachment);
    setupToggle (etherealToggle, "Ethereal", "shimmerEthereal", shimmerEtherealAttachment);
    setupToggle (wavesToggle,    "Waves",    "wavesOn",         wavesOnAttachment);
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
               linSteps (40.0f, 240.0f, 10.0f),
               1.0);

    setupKnob (rateKnob, "Rate", "rate",
               [] (float v) { return juce::String (v, 2) + " Hz"; },
               { 0.1f, 0.25f, 0.5f, 0.7f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f },
               0.01);

    setupKnob (depthKnob, "Depth", "depth", formatPercent, percentSteps(), 0.01);

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
    delayKnob.title.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    addAndMakeVisible (delayKnob.title);
    delayKnob.value.setJustificationType (juce::Justification::centred);
    delayKnob.value.setColour (juce::Label::textColourId, kMuted);
    delayKnob.value.setFont (juce::FontOptions (12.0f));
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

    setupKnob (shimmerKnob, "Amount", "shimmer", formatPercent, percentSteps(), 0.01);
    setupKnob (pitchKnob, "Pitch", "shimmerPitch", formatPitch,
               { -12.0f, -5.0f, 0.0f, 7.0f, 12.0f, 19.0f, 24.0f }, 1.0);
    setupKnob (decayKnob, "Decay", "shimmerDecay",
               [] (float v) { return juce::String (v, v < 10.0f ? 2 : 1) + " s"; },
               { 0.4f, 1.0f, 2.0f, 3.5f, 5.0f, 8.0f, 12.0f }, 0.01);
    setupKnob (chorusLevelKnob,  "Level", "chorusLevel",  formatPercent, percentSteps(), 0.01);
    setupKnob (delayLevelKnob,   "Level", "delayLevel",   formatPercent, percentSteps(), 0.01);
    setupKnob (shimmerLevelKnob, "Level", "shimmerLevel", formatPercent, percentSteps(), 0.01);
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
