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
    /** Approximate wash RT60 in seconds (0.4 … 12). */
    void setDecaySeconds (float seconds);
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
    static constexpr float kWashMakeup = 1.55f; // audible bloom at modest Amount settings
    static constexpr float kMinDecaySec = 0.4f;
    static constexpr float kMaxDecaySec = 12.0f;
    static constexpr float kDefaultDecaySec = 3.5f;
    // Mean FDN line length used to map RT60 → per-sample feedback
    static constexpr float kAvgFdnDelayMs = 67.3f;
    static constexpr float kMaxFeedback = 0.94f; // hard stability ceiling

    static constexpr float kPitchFbNormal = 0.34f;
    static constexpr float kDampingHzNormal = 3800.0f;
    static constexpr float kInputHpfHzNormal = 180.0f;
    static constexpr float kWashHpfHzNormal = 200.0f;
    static constexpr float kPreDelayMsNormal = 35.0f;
    // One-pole on pitched grains for FDN inject: higher = brighter
    static constexpr float kPitchLpCoeffNormal = 0.28f;

    // Ethereal / spacey mode — wider/airier; decay stays user-controlled
    static constexpr float kPitchFbEthereal = 0.40f;
    static constexpr float kDampingHzEthereal = 5200.0f;
    static constexpr float kInputHpfHzEthereal = 200.0f;
    static constexpr float kWashHpfHzEthereal = 280.0f;
    static constexpr float kPreDelayMsEthereal = 95.0f;
    static constexpr float kPitchLpCoeffEthereal = 0.38f;
    static constexpr float kMaxPreDelayMs = 120.0f;
    static constexpr float kEtherealDecayScale = 1.12f; // mild RT stretch when Ethereal is on
    static constexpr float kSparkleMixEthereal = 0.04f; // whisper of glitter only
    static constexpr float kSparkleMixNormal = 0.02f;
    static constexpr float kMaxSafePitchFb = 0.40f;
    // Final wash air cut — stops +24 feedback from going ice-pick
    static constexpr float kWashAirHzNormal = 6500.0f;
    static constexpr float kWashAirHzEthereal = 7800.0f;

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
    float feedbackFromDecaySeconds (float seconds) const noexcept;
    void  refreshDecayTarget() noexcept;
    float processDiffuser (Diffuser& d, float input) noexcept;
    void  processFdn (float inL, float inR, float& outL, float& outR,
                      float& sparkleL, float& sparkleR,
                      float decay, float pitchFb, float pitchLpCoeff) noexcept;
    float pitchShift (float input) noexcept;
    float grainRead (float readPos) const noexcept;
    void  updateModeTargets() noexcept;

    double sampleRate = 44100.0;
    int numChannels = 2;
    bool etherealEnabled = false;
    float decaySeconds = kDefaultDecaySec;

    std::array<Diffuser, kNumDiffusers> diffL {};
    std::array<Diffuser, kNumDiffusers> diffR {};
    std::array<FdnLine, kFdnSize> fdn {};

    std::vector<float> pitchDelay;
    int pitchDelaySize = 0;
    int pitchWrite = 0;
    float grainLengthSamples = 1.0f;
    std::array<Grain, kNumGrains> grains {};

    float pitchLpState = 0.0f;
    float sparkleHpState = 0.0f;
    float sparkleAirState = 0.0f; // gentle LP on sparkle tap — removes piercing HF
    std::array<float, 2> inputHpState { 0.0f, 0.0f };
    std::array<float, 2> washHpState { 0.0f, 0.0f };
    std::array<float, 2> washAirState { 0.0f, 0.0f };

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
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> pitchLpCoeffSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> sparkleMixSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> washAirCoeffSmoothed;
};
