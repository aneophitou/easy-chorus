#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <functional>

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
    void layoutToggles (juce::Rectangle<int> bounds);
    void layoutKnobs (juce::Rectangle<int> bounds);
    void refreshToggleLook (juce::TextButton& button, bool isOn);
    void refreshDemoLook();

    struct Knob
    {
        juce::Slider slider;
        juce::Label  title;
        juce::Label  value;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void setupKnob (Knob& knob,
                    const juce::String& name,
                    const juce::String& paramId,
                    std::function<juce::String (float)> formatter);

    void setupToggle (juce::TextButton& button,
                      const juce::String& label,
                      const juce::String& paramId,
                      std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>& attachment);

    EasyChorusAudioProcessor& audioProcessor;

    juce::TextButton demoToggle { "Demo" };
    juce::TextButton chorusToggle { "Chorus" };
    juce::TextButton delayToggle  { "Delay" };
    juce::TextButton shimmerToggle { "Shimmer" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> demoOnAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> chorusOnAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> delayOnAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> shimmerOnAttachment;

    Knob rateKnob;
    Knob depthKnob;
    Knob delayKnob;
    Knob feedbackKnob;
    Knob shimmerKnob;
    Knob mixKnob;

    float lfoPhase = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EasyChorusAudioProcessorEditor)
};
