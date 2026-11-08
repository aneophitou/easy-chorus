#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>

/**
 * Valhalla Shimmer–inspired pitch-shifted reverb wash (original DSP).
 *
 * Dense FDN diffusion with feedback through a configurable pitch shifter
 * (default +12 semitones). Optional Ethereal mode lengthens the bloom,
 * lifts the air band, and widens the image — kept stable with soft limiting.
 */
class ShimmerEngine
{
public:
    ShimmerEngine() = default;

    void prepare (double sampleRate, int maximumBlockSize, int numChannels);
    void reset();

    void setAmount (float amount01);
    /** Pitch shift in the feedback path, in semitones (−12 … +24). */
    void setPitchSemitones (float semitones);
    /** When enabled, longer / airier / wider wash (smoothly morphs). */
    void setEthereal (bool enabled) noexcept;

    void process (juce::AudioBuffer<float>& buffer);

private:
    static constexpr int kNumDiffusers = 4;
    static constexpr int kFdnSize = 8;
    static constexpr int kNumGrains = 3;
    static constexpr float kGrainMs = 90.0f;
    static constexpr float kMinSemitones = -12.0f;
    static constexpr float kMaxSemitones = 24.0f;
    static constexpr float kMaxPitchRatio = 4.0f;
    static constexpr float kWashMakeup = 1.85f; // audible bloom at modest Amount settings

    // Normal mode
    static constexpr float kDecayNormal = 0.88f;
    static constexpr float kPitchFbNormal = 0.48f;
    static constexpr float kDampingHzNormal = 4200.0f;
    static constexpr float kInputHpfHzNormal = 180.0f;
    static constexpr float kWashHpfHzNormal = 220.0f;
    static constexpr float kPreDelayMsNormal = 35.0f;

    // Ethereal / spacey mode — longer & wider, but feedback-stable
    static constexpr float kDecayEthereal = 0.925f;
    static constexpr float kPitchFbEthereal = 0.58f;
    static constexpr float kDampingHzEthereal = 5600.0f;
    static constexpr float kInputHpfHzEthereal = 210.0f;
    static constexpr float kWashHpfHzEthereal = 320.0f;
    static constexpr float kPreDelayMsEthereal = 95.0f;
    static constexpr float kMaxPreDelayMs = 120.0f;

    struct Diffuser
    {
        std::vector<float> buffer;
        int index = 0;
        int size = 1;
        float z = 0.0f;
    };

    struct FdnLine
    {
        std::vector<float> buffer;
        int index = 0;
        int size = 1;
        float filterState = 0.0f;
    };

    struct Grain
    {
        float phase = 0.0f;
    };

    static float softClip (float x) noexcept;
    float processDiffuser (Diffuser& d, float input) noexcept;
    void  processFdn (float inL, float inR, float& outL, float& outR, float decay, float pitchFb) noexcept;
    float pitchShift (float input) noexcept;
    float grainRead (float readPos) const noexcept;
    void  updateModeTargets() noexcept;

    double sampleRate = 44100.0;
    int numChannels = 2;
    bool etherealEnabled = false;

    std::array<Diffuser, kNumDiffusers> diffL {};
    std::array<Diffuser, kNumDiffusers> diffR {};
    std::array<FdnLine, kFdnSize> fdn {};

    std::vector<float> pitchDelay;
    int pitchDelaySize = 0;
    int pitchWrite = 0;
    float grainLengthSamples = 1.0f;
    std::array<Grain, kNumGrains> grains {};

    float pitchLpState = 0.0f;
    std::array<float, 2> inputHpState { 0.0f, 0.0f };
    std::array<float, 2> washHpState { 0.0f, 0.0f };

    std::vector<float> preDelayL;
    std::vector<float> preDelayR;
    int preDelaySize = 0;
    int preDelayWrite = 0;

    float spaceLfoPhase = 0.0f;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> amountSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> pitchRatioSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> etherealSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> decaySmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> pitchFbSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> dampCoeffSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> inputHpCoeffSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> washHpCoeffSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> preDelaySamplesSmoothed;
};
