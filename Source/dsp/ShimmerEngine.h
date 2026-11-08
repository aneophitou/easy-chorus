#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>

/**
 * Pitch-shifted ambient wash (Valhalla / Eventide-inspired).
 *
 * Dense FDN diffusion with feedback through a configurable pitch shifter
 * (default +12 semitones). Optional Ethereal mode lengthens the bloom,
 * lifts the air band, widens the image, and pushes more pitched feedback
 * for a spacier cloud.
 */
class ShimmerEngine
{
public:
    void prepare (double sampleRate, int maximumBlockSize, int numChannels);
    void reset();
    void process (juce::AudioBuffer<float>& buffer);

    void setAmount (float amount01);          // 0..1 wet wash amount
    void setPitchSemitones (float semitones); // typically -12..+24
    void setEthereal (bool enabled) noexcept;

private:
    static constexpr int   kNumDiffusers = 4;
    static constexpr int   kFdnSize = 8;
    static constexpr int   kNumGrains = 4;
    static constexpr float kGrainMs = 80.0f;
    static constexpr float kMinSemitones = -12.0f;
    static constexpr float kMaxSemitones = 24.0f;
    static constexpr float kMaxPitchRatio = 4.0f;

    // Normal mode
    static constexpr float kDecayNormal = 0.88f;
    static constexpr float kPitchFbNormal = 0.55f;
    static constexpr float kDampingHzNormal = 4200.0f;
    static constexpr float kInputHpfHzNormal = 180.0f;
    static constexpr float kWashHpfHzNormal = 250.0f;
    static constexpr float kPreDelayMsNormal = 35.0f;

    // Ethereal / spacey mode
    static constexpr float kDecayEthereal = 0.955f;
    static constexpr float kPitchFbEthereal = 0.78f;
    static constexpr float kDampingHzEthereal = 6800.0f; // brighter air in the loop
    static constexpr float kInputHpfHzEthereal = 240.0f;
    static constexpr float kWashHpfHzEthereal = 480.0f;  // thinner, floatier wash
    static constexpr float kPreDelayMsEthereal = 110.0f;
    static constexpr float kMaxPreDelayMs = 120.0f;

    struct Diffuser
    {
        std::vector<float> buffer;
        int size = 0;
        int index = 0;
        float z = 0.0f;
    };

    struct FdnLine
    {
        std::vector<float> buffer;
        int size = 0;
        int index = 0;
        float filterState = 0.0f;
    };

    struct Grain
    {
        float phase = 0.0f;
    };

    float processDiffuser (Diffuser& d, float input) noexcept;
    void  processFdn (float inL, float inR, float& outL, float& outR, float decay, float pitchFb) noexcept;
    float pitchShift (float input) noexcept;
    float grainRead (float readPos) const noexcept;
    void  updateModeTargets() noexcept;

    double sampleRate = 44100.0;
    int numChannels = 2;
    bool etherealEnabled = false;

    std::array<Diffuser, kNumDiffusers> diffL;
    std::array<Diffuser, kNumDiffusers> diffR;
    std::array<FdnLine, kFdnSize> fdn;

    std::vector<float> pitchDelay;
    int pitchDelaySize = 0;
    int pitchWrite = 0;
    float grainLengthSamples = 1.0f;
    std::array<Grain, kNumGrains> grains;
    float pitchLpState = 0.0f;

    std::array<float, 2> inputHpState { 0.0f, 0.0f };
    std::array<float, 2> washHpState { 0.0f, 0.0f };

    // Max-sized pre-delay; read length morphs with ethereal
    std::vector<float> preDelayL;
    std::vector<float> preDelayR;
    int preDelaySize = 0;
    int preDelayWrite = 0;

    // Slow stereo drift for ethereal width
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
