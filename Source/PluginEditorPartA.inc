#include "PluginEditor.h"
#include "dsp/TempoHelpers.h"
#include <algorithm>

namespace
{
constexpr int kMinHit = 52;
constexpr int kPreferredWidth = 980;
constexpr int kPreferredHeight = 720;
constexpr int kBigDialMin = 100;

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
    setResizeLimits (820, 620, 1400, 1100);

    setupToggle (demoToggle,      "Demo",     "demoOn",           demoOnAttachment);
    setupToggle (chorusToggle,    "On",       "chorusOn",         chorusOnAttachment);
    setupToggle (delayToggle,     "On",       "delayOn",          delayOnAttachment);
    setupToggle (shimmerToggle,   "On",       "shimmerOn",        shimmerOnAttachment);
    setupToggle (delay2Toggle,    "On",       "delay2On",         delay2OnAttachment);
    setupToggle (shimmer2Toggle,  "On",       "shimmer2On",       shimmer2OnAttachment);
    setupToggle (syncToggle,      "Sync",     "tempoSync",        tempoSyncAttachment);
    setupToggle (musicalToggle,   "Musical",  "delayMusical",     delayMusicalAttachment);
    setupToggle (musical2Toggle,  "Musical",  "delay2Musical",    delay2MusicalAttachment);
    setupToggle (stereoToggle,    "Stereo",   "delayStereo",      delayStereoAttachment);
    setupToggle (pingPongToggle,  "Ping",     "delayPingPong",    delayPingPongAttachment);
    setupToggle (reverseToggle,   "Reverse",  "delayReverse",     delayReverseAttachment);
    setupToggle (stereo2Toggle,   "Stereo",   "delay2Stereo",     delay2StereoAttachment);
    setupToggle (pingPong2Toggle, "Ping",     "delay2PingPong",   delay2PingPongAttachment);
    setupToggle (reverse2Toggle,  "Reverse",  "delay2Reverse",    delay2ReverseAttachment);
    setupToggle (etherealToggle,  "Ethereal", "shimmerEthereal",  shimmerEtherealAttachment);
    setupToggle (ethereal2Toggle, "Ethereal", "shimmer2Ethereal", shimmer2EtherealAttachment);
    setupToggle (wavesToggle,     "Waves",    "wavesOn",          wavesOnAttachment);
    refreshDemoLook();

    auto styleNav = [this] (juce::TextButton& button)
    {
        button.setColour (juce::TextButton::buttonColourId, kToggleOff);
        button.setColour (juce::TextButton::buttonOnColourId, kAccent);
        button.setColour (juce::TextButton::textColourOffId, kText);
        button.setColour (juce::TextButton::textColourOnId, kBgBottom);
        button.setConnectedEdges (0);
        addAndMakeVisible (button);
    };
    styleNav (presetPrevButton);
    styleNav (presetNextButton);
    presetPrevButton.onClick = [this]() { selectPrevPreset(); };
    presetNextButton.onClick = [this]() { selectNextPreset(); };

    for (auto* tab : { &pageChorusTab, &pageDelayTab, &pageShimmerTab,
                       &pageDelay2Tab, &pageShimmer2Tab })
    {
        tab->setClickingTogglesState (false);
        tab->setConnectedEdges (0);
        addAndMakeVisible (*tab);
    }
    pageChorusTab.onClick   = [this]() { setActivePage (Page::chorus); };
    pageDelayTab.onClick    = [this]() { setActivePage (Page::delay); };
    pageShimmerTab.onClick  = [this]() { setActivePage (Page::shimmer); };
    pageDelay2Tab.onClick   = [this]() { setActivePage (Page::delay2); };
    pageShimmer2Tab.onClick = [this]() { setActivePage (Page::shimmer2); };

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
    setActivePage (Page::chorus);
    startTimerHz (30);
}

void EasyChorusAudioProcessorEditor::setActivePage (Page page)
{
    activePage = page;
    updatePageVisibility();
    refreshPageTabLooks();
    resized();
    repaint();
}

void EasyChorusAudioProcessorEditor::updatePageVisibility()
{
    auto showKnob = [] (Knob& k, bool vis)
    {
        k.dialHost.setVisible (vis);
        k.title.setVisible (vis);
        k.value.setVisible (vis);
    };

    const bool c  = activePage == Page::chorus;
    const bool d  = activePage == Page::delay;
    const bool s  = activePage == Page::shimmer;
    const bool d2 = activePage == Page::delay2;
    const bool s2 = activePage == Page::shimmer2;

    chorusToggle.setVisible (c);
    showKnob (rateKnob, c);
    showKnob (depthKnob, c);
    showKnob (chorusLevelKnob, c);

    delayToggle.setVisible (d);
    stereoToggle.setVisible (d);
    pingPongToggle.setVisible (d);
    reverseToggle.setVisible (d);
    syncToggle.setVisible (d);
    musicalToggle.setVisible (d);
    showKnob (delayKnob, d);
    showKnob (feedbackKnob, d);
    showKnob (delayLevelKnob, d);
    showKnob (bpmKnob, d);

    shimmerToggle.setVisible (s);
    etherealToggle.setVisible (s);
    showKnob (shimmerKnob, s);
    showKnob (pitchKnob, s);
    showKnob (decayKnob, s);
    showKnob (shimmerLevelKnob, s);

    delay2Toggle.setVisible (d2);
    stereo2Toggle.setVisible (d2);
    pingPong2Toggle.setVisible (d2);
    reverse2Toggle.setVisible (d2);
    musical2Toggle.setVisible (d2);
    showKnob (delay2Knob, d2);
    showKnob (feedback2Knob, d2);
    showKnob (delay2LevelKnob, d2);

    shimmer2Toggle.setVisible (s2);
    ethereal2Toggle.setVisible (s2);
    showKnob (shimmer2Knob, s2);
    showKnob (pitch2Knob, s2);
    showKnob (decay2Knob, s2);
    showKnob (shimmer2LevelKnob, s2);

    wavesToggle.setVisible (true);
    showKnob (mixKnob, true);
    showKnob (masterKnob, true);
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

void EasyChorusAudioProcessorEditor::refreshPageTabLooks()
{
    auto paintTab = [this] (juce::TextButton& tab, Page page, bool stageOn)
    {
        const bool active = activePage == page;
        tab.setColour (juce::TextButton::buttonColourId,
                       active ? kAccent : (stageOn ? kToggleOff.brighter (0.08f) : kToggleOff));
        tab.setColour (juce::TextButton::textColourOffId,
                       active ? kBgBottom : (stageOn ? kText : kMuted));
        tab.setColour (juce::TextButton::textColourOnId, kBgBottom);
    };

    paintTab (pageChorusTab,   Page::chorus,   chorusToggle.getToggleState());
    paintTab (pageDelayTab,    Page::delay,    delayToggle.getToggleState());
    paintTab (pageShimmerTab,  Page::shimmer,  shimmerToggle.getToggleState());
    paintTab (pageDelay2Tab,   Page::delay2,   delay2Toggle.getToggleState());
    paintTab (pageShimmer2Tab, Page::shimmer2, shimmer2Toggle.getToggleState());
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
    knob.title.setFont (juce::FontOptions (16.0f, juce::Font::bold));
    addAndMakeVisible (knob.title);

    knob.value.setJustificationType (juce::Justification::centred);
    knob.value.setColour (juce::Label::textColourId, kMuted);
    knob.value.setFont (juce::FontOptions (14.0f));
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
    knob.title.setFont (juce::FontOptions (16.0f, juce::Font::bold));
    addAndMakeVisible (knob.title);
    knob.value.setJustificationType (juce::Justification::centred);
    knob.value.setColour (juce::Label::textColourId, kMuted);
    knob.value.setFont (juce::FontOptions (14.0f));
    addAndMakeVisible (knob.value);
}

void EasyChorusAudioProcessorEditor::configureDelayFreeSteps (Knob& knob)
{
    knob.steps.setSteps ({ 50.0f, 100.0f, 150.0f, 200.0f, 250.0f, 300.0f,
                           380.0f, 500.0f, 600.0f, 750.0f, 1000.0f });
    knob.slider.setRange (50.0, 1000.0, 1.0);
}
