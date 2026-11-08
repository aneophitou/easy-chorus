#pragma once

#include <JuceHeader.h>
#include "dsp/ChorusEngine.h"
#include "dsp/DelayEngine.h"
#include "dsp/DemoToneGenerator.h"
#include "dsp/ShimmerEngine.h"

class EasyChorusAudioProcessor : public juce::AudioProcessor
{
public:
    EasyChorusAudioProcessor();
    ~EasyChorusAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    /** Effective BPM: host tempo when Sync is on and available, else manual BPM. */
    float getEffectiveBpm() const;
    float getCurrentDelayMs() const;
    bool  isTempoSyncEnabled() const;
    bool  isDelayMusical() const;

private:
    void updateEnginesFromParameters();
    float readHostBpm() const;

    juce::AudioProcessorValueTreeState apvts;
    ChorusEngine      chorusEngine;
    DelayEngine       delayEngine;
    ShimmerEngine     shimmerEngine;
    DemoToneGenerator demoTone;

    juce::AudioBuffer<float> dryBuffer;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoothed;
    float lastHostBpm = 120.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EasyChorusAudioProcessor)
};
