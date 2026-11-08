#include "ShimmerEngine.h"
#include <cmath>

namespace
{
float hann (float phase01) noexcept
{
    return 0.5f * (1.0f - std::cos (juce::MathConstants<float>::twoPi * phase01));
}

int msToSamples (double sampleRate, float ms) noexcept
{
    return juce::jmax (2, (int) std::round (sampleRate * (double) ms * 0.001));
}

float onePoleCoeff (double sampleRate, float hz) noexcept
{
    return 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * hz / (float) sampleRate);
}

constexpr float kDiffMs[4] = { 7.1f, 11.3f, 17.9f, 23.7f };
constexpr float kFdnMs[8]  = { 29.3f, 37.1f, 47.9f, 58.3f, 71.9f, 83.7f, 97.1f, 113.3f };
} // namespace

float ShimmerEngine::softClip (float x) noexcept
{
    // Gentle tanh-ish clip keeps ethereal feedback from running away
    const float ax = std::abs (x);
    if (ax <= 1.0f)
        return x;
    return std::copysign (1.0f + std::tanh (ax - 1.0f), x);
}

void ShimmerEngine::prepare (double newSampleRate, int /*maximumBlockSize*/, int newNumChannels)
{
    sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
    numChannels = juce::jlimit (1, 2, newNumChannels);

    for (int i = 0; i < kNumDiffusers; ++i)
    {
        auto setup = [&] (Diffuser& d, float ms)
        {
            d.size = msToSamples (sampleRate, ms);
            d.buffer.assign ((size_t) d.size, 0.0f);
            d.index = 0;
            d.z = 0.0f;
        };
        setup (diffL[(size_t) i], kDiffMs[i]);
        setup (diffR[(size_t) i], kDiffMs[i] * 1.13f);
    }

    for (int i = 0; i < kFdnSize; ++i)
    {
        fdn[(size_t) i].size = msToSamples (sampleRate, kFdnMs[i]);
        fdn[(size_t) i].buffer.assign ((size_t) fdn[(size_t) i].size, 0.0f);
        fdn[(size_t) i].index = 0;
        fdn[(size_t) i].filterState = 0.0f;
    }

    grainLengthSamples = juce::jmax (64.0f, kGrainMs * 0.001f * (float) sampleRate);
    pitchDelaySize = juce::jmax (256, (int) std::ceil (grainLengthSamples * kMaxPitchRatio) + 64);
    pitchDelay.assign ((size_t) pitchDelaySize, 0.0f);
    pitchWrite = 0;

    for (int g = 0; g < kNumGrains; ++g)
        grains[(size_t) g].phase = (float) g / (float) kNumGrains;

    preDelaySize = msToSamples (sampleRate, kMaxPreDelayMs);
    preDelayL.assign ((size_t) preDelaySize, 0.0f);
    preDelayR.assign ((size_t) preDelaySize, 0.0f);
    preDelayWrite = 0;
    spaceLfoPhase = 0.0f;

    const float smoothSec = 0.12f;
    amountSmoothed.reset (sampleRate, 0.05f);
    amountSmoothed.setCurrentAndTargetValue (0.25f);
    pitchRatioSmoothed.reset (sampleRate, 0.08f);
    pitchRatioSmoothed.setCurrentAndTargetValue (2.0f);
    etherealSmoothed.reset (sampleRate, smoothSec);
    etherealSmoothed.setCurrentAndTargetValue (0.0f);
    decaySmoothed.reset (sampleRate, smoothSec);
    pitchFbSmoothed.reset (sampleRate, smoothSec);
    dampCoeffSmoothed.reset (sampleRate, smoothSec);
    inputHpCoeffSmoothed.reset (sampleRate, smoothSec);
    washHpCoeffSmoothed.reset (sampleRate, smoothSec);
    preDelaySamplesSmoothed.reset (sampleRate, smoothSec);
    pitchLpCoeffSmoothed.reset (sampleRate, smoothSec);
    sparkleMixSmoothed.reset (sampleRate, smoothSec);
    washAirCoeffSmoothed.reset (sampleRate, smoothSec);

    etherealEnabled = false;
    decaySeconds = kDefaultDecaySec;
    updateModeTargets();
    refreshDecayTarget();
    decaySmoothed.setCurrentAndTargetValue (decaySmoothed.getTargetValue());
    pitchFbSmoothed.setCurrentAndTargetValue (pitchFbSmoothed.getTargetValue());
    dampCoeffSmoothed.setCurrentAndTargetValue (dampCoeffSmoothed.getTargetValue());
    inputHpCoeffSmoothed.setCurrentAndTargetValue (inputHpCoeffSmoothed.getTargetValue());
    washHpCoeffSmoothed.setCurrentAndTargetValue (washHpCoeffSmoothed.getTargetValue());
    preDelaySamplesSmoothed.setCurrentAndTargetValue (preDelaySamplesSmoothed.getTargetValue());
    pitchLpCoeffSmoothed.setCurrentAndTargetValue (pitchLpCoeffSmoothed.getTargetValue());
    sparkleMixSmoothed.setCurrentAndTargetValue (sparkleMixSmoothed.getTargetValue());
    washAirCoeffSmoothed.setCurrentAndTargetValue (washAirCoeffSmoothed.getTargetValue());

    reset();
}

void ShimmerEngine::reset()
{
    for (auto& d : diffL)
    {
        std::fill (d.buffer.begin(), d.buffer.end(), 0.0f);
        d.index = 0;
        d.z = 0.0f;
    }
    for (auto& d : diffR)
    {
        std::fill (d.buffer.begin(), d.buffer.end(), 0.0f);
        d.index = 0;
        d.z = 0.0f;
    }
    for (auto& line : fdn)
    {
        std::fill (line.buffer.begin(), line.buffer.end(), 0.0f);
        line.index = 0;
        line.filterState = 0.0f;
    }

    std::fill (pitchDelay.begin(), pitchDelay.end(), 0.0f);
    pitchWrite = 0;
    for (int g = 0; g < kNumGrains; ++g)
        grains[(size_t) g].phase = (float) g / (float) kNumGrains;

    std::fill (preDelayL.begin(), preDelayL.end(), 0.0f);
    std::fill (preDelayR.begin(), preDelayR.end(), 0.0f);
    preDelayWrite = 0;

    inputHpState = { 0.0f, 0.0f };
    washHpState = { 0.0f, 0.0f };
    washAirState = { 0.0f, 0.0f };
    pitchLpState = 0.0f;
    sparkleHpState = 0.0f;
    sparkleAirState = 0.0f;
    spaceLfoPhase = 0.0f;
}

void ShimmerEngine::setAmount (float amount01)
{
    amountSmoothed.setTargetValue (juce::jlimit (0.0f, 1.0f, amount01));
}

void ShimmerEngine::setPitchSemitones (float semitones)
{
    const float semis = juce::jlimit (kMinSemitones, kMaxSemitones, semitones);
    const float ratio = std::pow (2.0f, semis / 12.0f);
    pitchRatioSmoothed.setTargetValue (juce::jlimit (0.5f, kMaxPitchRatio, ratio));
}

void ShimmerEngine::setDecaySeconds (float seconds)
{
    decaySeconds = juce::jlimit (kMinDecaySec, kMaxDecaySec, seconds);
    refreshDecayTarget();
}

float ShimmerEngine::feedbackFromDecaySeconds (float seconds) const noexcept
{
    const float rt = juce::jmax (0.2f, seconds);
    const float avgDelaySec = kAvgFdnDelayMs * 0.001f;
    // RT60: feedback^(rt/delay) ≈ 0.001
    const float fb = std::pow (0.001f, avgDelaySec / rt);
    return juce::jlimit (0.05f, kMaxFeedback, fb);
}

void ShimmerEngine::refreshDecayTarget() noexcept
{
    const float scaled = decaySeconds * (etherealEnabled ? kEtherealDecayScale : 1.0f);
    decaySmoothed.setTargetValue (feedbackFromDecaySeconds (scaled));
}

void ShimmerEngine::setEthereal (bool enabled) noexcept
{
    if (etherealEnabled == enabled)
        return;
    etherealEnabled = enabled;
    updateModeTargets();
    refreshDecayTarget();
}

void ShimmerEngine::updateModeTargets() noexcept
{
    etherealSmoothed.setTargetValue (etherealEnabled ? 1.0f : 0.0f);

    const float e = etherealEnabled ? 1.0f : 0.0f;
    auto lerp = [e] (float a, float b) { return a + (b - a) * e; };

    pitchFbSmoothed.setTargetValue (lerp (kPitchFbNormal, kPitchFbEthereal));
    dampCoeffSmoothed.setTargetValue (onePoleCoeff (sampleRate, lerp (kDampingHzNormal, kDampingHzEthereal)));
    inputHpCoeffSmoothed.setTargetValue (onePoleCoeff (sampleRate, lerp (kInputHpfHzNormal, kInputHpfHzEthereal)));
    washHpCoeffSmoothed.setTargetValue (onePoleCoeff (sampleRate, lerp (kWashHpfHzNormal, kWashHpfHzEthereal)));
    preDelaySamplesSmoothed.setTargetValue (
        lerp (kPreDelayMsNormal, kPreDelayMsEthereal) * 0.001f * (float) sampleRate);
    pitchLpCoeffSmoothed.setTargetValue (lerp (kPitchLpCoeffNormal, kPitchLpCoeffEthereal));
    sparkleMixSmoothed.setTargetValue (lerp (kSparkleMixNormal, kSparkleMixEthereal));
    washAirCoeffSmoothed.setTargetValue (onePoleCoeff (sampleRate, lerp (kWashAirHzNormal, kWashAirHzEthereal)));
}

float ShimmerEngine::processDiffuser (Diffuser& d, float input) noexcept
{
    constexpr float g = 0.6f;
    const float bufOut = d.buffer[(size_t) d.index];
    const float y = -g * input + bufOut;
    d.buffer[(size_t) d.index] = input + g * bufOut;
    d.index = (d.index + 1) % d.size;

    d.z += 0.35f * (y - d.z);
    return d.z;
}

float ShimmerEngine::grainRead (float readPos) const noexcept
{
    while (readPos < 0.0f)
        readPos += (float) pitchDelaySize;
    while (readPos >= (float) pitchDelaySize)
        readPos -= (float) pitchDelaySize;

    const int i0 = (int) readPos;
    const float frac = readPos - (float) i0;
    const int i1 = (i0 + 1) % pitchDelaySize;

    const int im1 = (i0 - 1 + pitchDelaySize) % pitchDelaySize;
    const int i2  = (i0 + 2) % pitchDelaySize;
    const float y0 = pitchDelay[(size_t) im1];
    const float y1 = pitchDelay[(size_t) i0];
    const float y2 = pitchDelay[(size_t) i1];
    const float y3 = pitchDelay[(size_t) i2];
    const float c0 = y1;
    const float c1 = 0.5f * (y2 - y0);
    const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
    return ((c3 * frac + c2) * frac + c1) * frac + c0;
}

float ShimmerEngine::pitchShift (float input) noexcept
{
    pitchDelay[(size_t) pitchWrite] = input;

    const float pitchRatio = pitchRatioSmoothed.getNextValue();

    float shifted = 0.0f;
    float windowSum = 0.0f;

    for (auto& grain : grains)
    {
        const float window = hann (grain.phase);
        const float age = grain.phase * grainLengthSamples;
        const float readPos = (float) pitchWrite - age * pitchRatio;
        shifted += grainRead (readPos) * window;
        windowSum += window;

        grain.phase += 1.0f / grainLengthSamples;
        if (grain.phase >= 1.0f)
            grain.phase -= 1.0f;
    }

    pitchWrite = (pitchWrite + 1) % pitchDelaySize;

    if (windowSum > 1.0e-4f)
        shifted /= windowSum;

    return softClip (shifted);
}

void ShimmerEngine::processFdn (float inL, float inR, float& outL, float& outR,
                                float& sparkleL, float& sparkleR,
                                float decay, float pitchFb, float pitchLpCoeff) noexcept
{
    std::array<float, kFdnSize> taps {};
    for (int i = 0; i < kFdnSize; ++i)
    {
        auto& line = fdn[(size_t) i];
        taps[(size_t) i] = line.buffer[(size_t) line.index];
    }

    float sum = 0.0f;
    for (float t : taps)
        sum += t;
    const float scale = 2.0f / (float) kFdnSize;

    std::array<float, kFdnSize> mixed {};
    for (int i = 0; i < kFdnSize; ++i)
        mixed[(size_t) i] = taps[(size_t) i] - scale * sum;

    const float monoTap = 0.5f * (taps[0] + taps[4]);
    const float pitchedRaw = pitchShift (monoTap);

    // +24 (ratio≈4) stacks harmonics fast — pull feedback/brightness back as pitch climbs
    const float pitchRatio = pitchRatioSmoothed.getCurrentValue();
    const float pitchHarsh = juce::jlimit (0.0f, 1.0f, (pitchRatio - 1.0f) / 3.0f);

    // Parallel sparkle: band-ish (HP then gentle LP) so glitter isn’t piercing
    sparkleHpState += 0.06f * (pitchedRaw - sparkleHpState);
    float sparkle = pitchedRaw - sparkleHpState;
    const float sparkleAirFollow = 0.12f + 0.08f * (1.0f - pitchHarsh); // darker at high pitch
    sparkleAirState += sparkleAirFollow * (sparkle - sparkleAirState);
    sparkle = sparkleAirState * (1.0f - 0.55f * pitchHarsh);
    sparkleL = sparkle;
    sparkleR = sparkle * 0.92f;

    // FDN pitch inject: darker / quieter as pitch rises (coeff near 1 = bright)
    const float lpFollow = juce::jlimit (0.12f, 0.70f, pitchLpCoeff * (1.0f - 0.55f * pitchHarsh));
    pitchLpState += lpFollow * (pitchedRaw - pitchLpState);
    const float pitched = pitchLpState;

    const float dampCoeff = dampCoeffSmoothed.getNextValue();
    // Keep pitched feedback below unity; strong attenuation at high ratios
    const float fbScale = 1.0f - 0.65f * pitchHarsh;
    const float safePitchFb = juce::jmin (pitchFb * fbScale, kMaxSafePitchFb);
    const float injectL = softClip (inL + pitched * safePitchFb);
    const float injectR = softClip (inR + pitched * safePitchFb);

    for (int i = 0; i < kFdnSize; ++i)
    {
        auto& line = fdn[(size_t) i];
        const float input = (i & 1) ? injectR : injectL;
        float sample = mixed[(size_t) i] * decay + input * (0.18f + 0.015f * (float) i);

        line.filterState += dampCoeff * (sample - line.filterState);
        sample = softClip (line.filterState);

        line.buffer[(size_t) line.index] = sample;
        line.index = (line.index + 1) % line.size;
    }

    outL = 0.0f;
    outR = 0.0f;
    for (int i = 0; i < kFdnSize; i += 2)
    {
        outL += taps[(size_t) i];
        outR += taps[(size_t) i + 1];
    }
    outL *= 0.55f;
    outR *= 0.55f;
}

void ShimmerEngine::process (juce::AudioBuffer<float>& buffer)
{
    if (preDelaySize <= 0)
        return;

    const int numSamples = buffer.getNumSamples();
    const int channels = juce::jmin (buffer.getNumChannels(), numChannels);

    for (int i = 0; i < numSamples; ++i)
    {
        const float amount = amountSmoothed.getNextValue();
        const float ethereal = etherealSmoothed.getNextValue();
        const float decay = decaySmoothed.getNextValue();
        const float pitchFb = pitchFbSmoothed.getNextValue();
        const float inputHpCoeff = inputHpCoeffSmoothed.getNextValue();
        const float washHpCoeff = washHpCoeffSmoothed.getNextValue();
        const float pitchLpCoeff = pitchLpCoeffSmoothed.getNextValue();
        const float sparkleMix = sparkleMixSmoothed.getNextValue();
        const float washAirCoeff = washAirCoeffSmoothed.getNextValue();
        const float preDelaySamples = juce::jlimit (
            1.0f, (float) preDelaySize - 1.0f, preDelaySamplesSmoothed.getNextValue());

        float inL = buffer.getSample (0, i);
        float inR = channels > 1 ? buffer.getSample (1, i) : inL;

        inputHpState[0] += inputHpCoeff * (inL - inputHpState[0]);
        inputHpState[1] += inputHpCoeff * (inR - inputHpState[1]);
        const float hpL = inL - inputHpState[0];
        const float hpR = inR - inputHpState[1];

        preDelayL[(size_t) preDelayWrite] = hpL;
        preDelayR[(size_t) preDelayWrite] = hpR;

        float readPos = (float) preDelayWrite - preDelaySamples;
        while (readPos < 0.0f)
            readPos += (float) preDelaySize;
        const int i0 = (int) readPos;
        const float frac = readPos - (float) i0;
        const int i1 = (i0 + 1) % preDelaySize;
        const float pdL = preDelayL[(size_t) i0] + frac * (preDelayL[(size_t) i1] - preDelayL[(size_t) i0]);
        const float pdR = preDelayR[(size_t) i0] + frac * (preDelayR[(size_t) i1] - preDelayR[(size_t) i0]);
        preDelayWrite = (preDelayWrite + 1) % preDelaySize;

        float dL = pdL;
        float dR = pdR;
        for (int d = 0; d < kNumDiffusers; ++d)
        {
            dL = processDiffuser (diffL[(size_t) d], dL);
            dR = processDiffuser (diffR[(size_t) d], dR);
        }

        float revL = 0.0f, revR = 0.0f;
        float sparkleL = 0.0f, sparkleR = 0.0f;
        processFdn (dL, dR, revL, revR, sparkleL, sparkleR, decay, pitchFb, pitchLpCoeff);

        washHpState[0] += washHpCoeff * (revL - washHpState[0]);
        washHpState[1] += washHpCoeff * (revR - washHpState[1]);
        revL -= washHpState[0];
        revR -= washHpState[1];

        // Ethereal: slow stereo drift + mild cross-feed (no extra feedback)
        spaceLfoPhase += 0.07f / (float) sampleRate;
        if (spaceLfoPhase >= 1.0f)
            spaceLfoPhase -= 1.0f;
        const float space = std::sin (spaceLfoPhase * juce::MathConstants<float>::twoPi);
        const float width = 0.22f * ethereal;
        const float cross = 0.18f * ethereal;
        float outWashL = revL * (1.0f + space * width) + revR * cross;
        float outWashR = revR * (1.0f - space * width) + revL * cross;

        // Soft glitter on top of the wash (gain falls as pitch climbs into harsh territory)
        const float pitchRatio = pitchRatioSmoothed.getCurrentValue();
        const float pitchHarsh = juce::jlimit (0.0f, 1.0f, (pitchRatio - 1.0f) / 3.0f);
        const float sparkleGain = sparkleMix * (1.0f - 0.65f * pitchHarsh);
        outWashL = outWashL * kWashMakeup + sparkleL * sparkleGain;
        outWashR = outWashR * kWashMakeup + sparkleR * sparkleGain;

        // Final air shelf — cut piercing HF before soft-clip / mix
        washAirState[0] += washAirCoeff * (outWashL - washAirState[0]);
        washAirState[1] += washAirCoeff * (outWashR - washAirState[1]);
        outWashL = softClip (washAirState[0]);
        outWashR = softClip (washAirState[1]);

        // No amount boost at high pitch — ethereal only adds a touch when pitch is mild
        const float amountScale = 1.0f + 0.08f * ethereal * (1.0f - pitchHarsh);

        buffer.setSample (0, i, inL + amount * amountScale * outWashL);
        if (channels > 1)
            buffer.setSample (1, i, inR + amount * amountScale * outWashR);
    }
}
