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
    void timerCallback() override;
    void layoutSectionToggles (juce::Rectangle<int> bounds);
    void layoutTempoRow (juce::Rectangle<int> bounds);
    void layoutDelayModeRow (juce::Rectangle<int> bounds);
    void layoutKnobs (juce::Rectangle<int> bounds);
    void refreshToggleLook (juce::TextButton& button, bool isOn);
    void refreshDemoLook();
    void refreshTempoLooks();
    void refreshDelayModeLooks();
    void rebindDelayKnob();
    void updateBpmInteraction();
    void updateDelayValueLabel();
    void configureDelayFreeSteps();
    void configureDelayMusicalSteps();

    struct Knob
    {
        juce::Component  dialHost;   // holds overlay + slider
        DialStepOverlay  steps;
        juce::Slider     slider;
        juce::Label      title;
        juce::Label      value;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void layoutOneKnob (Knob& knob, juce::Rectangle<int> cell);

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
    juce::TextButton chorusToggle { "Chorus" };
    juce::TextButton delayToggle  { "Delay" };
    juce::TextButton shimmerToggle { "Shimmer" };
    juce::TextButton syncToggle { "Sync" };
    juce::TextButton musicalToggle { "Musical" };
    juce::TextButton stereoToggle { "Stereo" };
    juce::TextButton pingPongToggle { "Ping" };
    juce::TextButton reverseToggle { "Reverse" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> demoOnAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> chorusOnAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> delayOnAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> shimmerOnAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> tempoSyncAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> delayMusicalAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> delayStereoAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> delayPingPongAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> delayReverseAttachment;

    Knob bpmKnob;
    Knob rateKnob;
    Knob depthKnob;
    Knob delayKnob;
    Knob feedbackKnob;
    Knob shimmerKnob;
    Knob pitchKnob;
    Knob mixKnob;

    bool delayKnobMusical = false;
    float lfoPhase = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EasyChorusAudioProcessorEditor)
};
