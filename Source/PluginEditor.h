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
    struct Knob
    {
        juce::Component  dialHost;   // holds overlay + slider
        DialStepOverlay  steps;
        juce::Slider     slider;
        juce::Label      title;
        juce::Label      value;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void timerCallback() override;

    void layoutPresetBar (juce::Rectangle<int> bounds);
    void layoutChorusColumn (juce::Rectangle<int> bounds);
    void layoutDelayColumn (juce::Rectangle<int> bounds);
    void layoutShimmerColumn (juce::Rectangle<int> bounds);
    void layoutOutputRow (juce::Rectangle<int> bounds);
    void layoutToggleRow (juce::Rectangle<int> bounds,
                          juce::TextButton* const* toggles,
                          int count,
                          int gap);
    void layoutKnobStack (juce::Rectangle<int> bounds,
                          Knob* const* knobs,
                          int count);
    void layoutOneKnob (Knob& knob, juce::Rectangle<int> cell);

    void paintSectionPanel (juce::Graphics& g,
                            juce::Rectangle<float> bounds,
                            const juce::String& title) const;

    void refreshToggleLook (juce::TextButton& button, bool isOn);
    void refreshDemoLook();
    void refreshTempoLooks();
    void refreshDelayModeLooks();
    void refreshPresetLook();
    void rebindDelayKnob();
    void updateBpmInteraction();
    void updateDelayValueLabel();
    void configureDelayFreeSteps();
    void configureDelayMusicalSteps();
    void loadSelectedPreset();
    void selectPrevPreset();
    void selectNextPreset();
    void syncPresetComboFromProcessor();

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

    juce::TextButton demoToggle { "Demo" };
    juce::TextButton chorusToggle { "On" };
    juce::TextButton delayToggle  { "On" };
    juce::TextButton shimmerToggle { "On" };
    juce::TextButton syncToggle { "Sync" };
    juce::TextButton musicalToggle { "Musical" };
    juce::TextButton stereoToggle { "Stereo" };
    juce::TextButton pingPongToggle { "Ping" };
    juce::TextButton reverseToggle { "Reverse" };
    juce::TextButton etherealToggle { "Ethereal" };
    juce::TextButton wavesToggle { "Waves" };

    juce::TextButton presetPrevButton { "<" };
    juce::TextButton presetNextButton { ">" };
    juce::ComboBox   presetCombo;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> demoOnAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> chorusOnAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> delayOnAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> shimmerOnAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> tempoSyncAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> delayMusicalAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> delayStereoAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> delayPingPongAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> delayReverseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> shimmerEtherealAttachment;
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
    Knob mixKnob;
    Knob masterKnob;

    juce::Rectangle<float> chorusPanelBounds;
    juce::Rectangle<float> delayPanelBounds;
    juce::Rectangle<float> shimmerPanelBounds;
    juce::Rectangle<float> outputPanelBounds;

    bool delayKnobMusical = false;
    bool ignorePresetComboCallback = false;
    float lfoPhase = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EasyChorusAudioProcessorEditor)
};
