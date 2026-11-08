#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>

/**
 * Clean digital stereo delay for BFMV-style arpeggio/lead trails.
 * Time range includes ~370–400 ms (Tears Don’t Fall quarter-ish).
 * Feedback capped for stability; light HPF keeps repeats pristine.
 *
 * Modes:
 * - Mono: linked L/R (same time, shared image)
 * - Stereo: independent L/R with a slight right-channel offset for width
 * - Ping-pong: cross-channel feedback (L↔R) for bouncing repeats
 * - Reverse: overlapping reverse grains (tape-reverse echoes)
 */
class DelayEngine
{
public:
    DelayEngine() = default;

    void prepare (double sampleRate, int maximumBlockSize, int numChannels);
    void reset();

    void setDelayMs (float delayMs);
    void setFeedback (float feedback01);
    void setStereo (bool enabled) noexcept { stereoEnabled = enabled; }
    void setPingPong (bool enabled) noexcept { pingPongEnabled = enabled; }
    void setReverse (bool enabled) noexcept { reverseEnabled = enabled; }

    void process (juce::AudioBuffer<float>& buffer);

private:
    float readAt (int channel, float delaySamples) const;
    float readReverseAt (int channel, float delaySamples, float phase) const noexcept;
    void  advanceReversePhases (float delaySamples) noexcept;
    float filterFeedback (int channel, float delayed) noexcept;

    double sampleRate = 44100.0;
    int numChannels = 2;
    int writeIndex = 0;
    int bufferSize = 0;

    bool stereoEnabled = false;
    bool pingPongEnabled = false;
    bool reverseEnabled = false;

    // Headroom for musical whole notes at slow tempos (~40 BPM → 6 s)
    static constexpr float kMaxDelayMs = 6000.0f;
    static constexpr float kMinDelayMs = 20.0f;
    static constexpr float kMaxFeedback = 0.85f;
    // Haas-ish right offset when Stereo is on
    static constexpr float kStereoOffsetMs = 12.0f;
    static constexpr float kWetLevel = 0.85f;
    static constexpr int kReverseGrains = 2;

    std::vector<std::vector<float>> delayLine;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> delaySamplesSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> feedbackSmoothed;

    // Two overlapping reverse phasors (0..1) for click-free reverse playback
    std::array<float, kReverseGrains> reversePhase { 0.0f, 0.5f };

    // Feedback-path HPF (~90 Hz) to keep repeats from muddying
    std::array<float, 2> fbLpState { 0.0f, 0.0f };
    float fbHpfCoeff = 0.0f;
};
