#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ui/DialStepOverlay.h"
#include <functional>
#include <vector>

class EasyChorusAudioProcessorEditor : public juce::AudioProcessorEditor,
                                       private juce::Timer
{
public:
    explicit EasyChorusAudioProcessorEditor (EasyChorusAudioProcessor&);
    ~EasyChorusAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    enum class Page
    {
        chorus = 0,
        delay,
        shimmer,
        delay2,
        shimmer2
    };

    struct Knob
    {
        juce::Component  dialHost;
        DialStepOverlay  steps;
        juce::Slider     slider;
        juce::Label      title;
        juce::Label      value;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void timerCallback() override;

    void setActivePage (Page page);
    void updatePageVisibility();

    void layoutPresetBar (juce::Rectangle<int> bounds);
    void layoutPageTabs (juce::Rectangle<int> bounds);
    void layoutActivePage (juce::Rectangle<int> bounds);
    void layoutChorusPage (juce::Rectangle<int> bounds);
    void layoutDelayPage (juce::Rectangle<int> bounds, bool isSecond);
    void layoutShimmerPage (juce::Rectangle<int> bounds, bool isSecond);
    void layoutOutputBar (juce::Rectangle<int> bounds);
    void layoutToggleRow (juce::Rectangle<int> bounds,
                          juce::TextButton* const* toggles,
                          int count,
                          int gap);
    void layoutKnobRow (juce::Rectangle<int> bounds,
                        Knob* const* knobs,
                        int count);
    void layoutOneKnob (Knob& knob, juce::Rectangle<int> cell);

    void refreshToggleLook (juce::TextButton& button, bool isOn);
    void refreshDemoLook();
    void refreshTempoLooks();
    void refreshModeLooks();
    void refreshPresetLook();
    void refreshPageTabLooks();
    void rebindDelayKnob (bool isSecond);
    void updateBpmInteraction();
    void updateDelayValueLabel (bool isSecond);
    void configureDelayFreeSteps (Knob& knob);
    void configureDelayMusicalSteps (Knob& knob);
    void loadSelectedPreset();
    void selectPrevPreset();
    void selectNextPreset();
    void syncPresetComboFromProcessor();
    void setupDelayKnobShell (Knob& knob);

    void setupKnob (Knob& knob,
                    const juce::String& name,
                    const juce::String& paramId,
                    std::function<juce::String (float)> formatter,
                    std::vector<float> stepValues,
                    double snapInterval);

    void setupToggle (juce::TextButton& button,
                      const juce::String& label,
                      const juce::String& paramId,
                      std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>& attachment);

    static std::vector<float> percentSteps();
    static std::vector<float> linSteps (float minV, float maxV, float step);

    EasyChorusAudioProcessor& audioProcessor;

    Page activePage = Page::chorus;

    juce::TextButton demoToggle { "Demo" };
    juce::TextButton pageChorusTab { "Chorus" };
    juce::TextButton pageDelayTab { "Delay" };
    juce::TextButton pageShimmerTab { "Shimmer" };
    juce::TextButton pageDelay2Tab { "Delay 2" };
    juce::TextButton pageShimmer2Tab { "Shimmer 2" };

    juce::TextButton chorusToggle { "On" };
    juce::TextButton delayToggle  { "On" };
    juce::TextButton shimmerToggle { "On" };
    juce::TextButton delay2Toggle { "On" };
    juce::TextButton shimmer2Toggle { "On" };
    juce::TextButton syncToggle { "Sync" };
    juce::TextButton musicalToggle { "Musical" };
    juce::TextButton musical2Toggle { "Musical" };
    juce::TextButton stereoToggle { "Stereo" };
    juce::TextButton pingPongToggle { "Ping" };
    juce::TextButton reverseToggle { "Reverse" };
    juce::TextButton stereo2Toggle { "Stereo" };
    juce::TextButton pingPong2Toggle { "Ping" };
    juce::TextButton reverse2Toggle { "Reverse" };
    juce::TextButton etherealToggle { "Ethereal" };
    juce::TextButton ethereal2Toggle { "Ethereal" };
    juce::TextButton wavesToggle { "Waves" };

    juce::TextButton presetPrevButton { "<" };
    juce::TextButton presetNextButton { ">" };
    juce::ComboBox   presetCombo;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> demoOnAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> chorusOnAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> delayOnAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> shimmerOnAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> delay2OnAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> shimmer2OnAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> tempoSyncAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> delayMusicalAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> delay2MusicalAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> delayStereoAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> delayPingPongAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> delayReverseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> delay2StereoAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> delay2PingPongAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> delay2ReverseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> shimmerEtherealAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> shimmer2EtherealAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> wavesOnAttachment;

    Knob bpmKnob;
    Knob rateKnob;
    Knob depthKnob;
    Knob delayKnob;
    Knob feedbackKnob;
    Knob shimmerKnob;
    Knob pitchKnob;
    Knob decayKnob;
    Knob chorusLevelKnob;
    Knob delayLevelKnob;
    Knob shimmerLevelKnob;
    Knob delay2Knob;
    Knob feedback2Knob;
    Knob delay2LevelKnob;
    Knob shimmer2Knob;
    Knob pitch2Knob;
    Knob decay2Knob;
    Knob shimmer2LevelKnob;
    Knob mixKnob;
    Knob masterKnob;

    juce::Rectangle<float> contentPanelBounds;
    juce::Rectangle<float> outputPanelBounds;

    bool delayKnobMusical = false;
    bool delay2KnobMusical = false;
    bool ignorePresetComboCallback = false;
    float lfoPhase = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EasyChorusAudioProcessorEditor)
};
