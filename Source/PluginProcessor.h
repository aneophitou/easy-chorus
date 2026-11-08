#pragma once

#include <JuceHeader.h>
#include "dsp/ChorusEngine.h"
#include "dsp/DelayEngine.h"
#include "dsp/DemoToneGenerator.h"
#include "dsp/ShimmerEngine.h"
#include "presets/PresetManager.h"
#include <vector>

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
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }
    PresetManager& getPresetManager() noexcept { return presetManager; }

    /** Load factory preset by index (also used by in-plugin prev/next/combo). */
    void loadPreset (int index);

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    /** Effective BPM: host tempo when Sync is on and available, else manual BPM. */
    float getEffectiveBpm() const;
    float getCurrentDelayMs() const;
    float getCurrentDelay2Ms() const;
    bool  isTempoSyncEnabled() const;
    bool  isDelayMusical() const;
    bool  isDelay2Musical() const;

private:
    void updateEnginesFromParameters();
    float readHostBpm() const;
    float resolveDelayMs (bool musical, const char* delayId, const char* divisionId) const;

    juce::AudioProcessorValueTreeState apvts;
    PresetManager     presetManager;
    ChorusEngine      chorusEngine;
    DelayEngine       delayEngine;
    DelayEngine       delayEngine2;
    ShimmerEngine     shimmerEngine;
    ShimmerEngine     shimmerEngine2;
    DemoToneGenerator demoTone;

    juce::AudioBuffer<float> dryBuffer;
    juce::AudioBuffer<float> stageBuffer;
    std::vector<float> wavesEnvelope;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> chorusLevelSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> delayLevelSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> shimmerLevelSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> delay2LevelSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> shimmer2LevelSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> masterSmoothed;
    float lastHostBpm = 120.0f;
    double currentSampleRate = 44100.0;
    float wavesPhase = 0.0f; // 0…1 cycle for delay+shimmer swell

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EasyChorusAudioProcessor)
};
