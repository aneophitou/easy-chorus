#include "DelayEngine.h"
#include <cmath>

namespace
{
float hannWindow (float phase01) noexcept
{
    return 0.5f * (1.0f - std::cos (juce::MathConstants<float>::twoPi * phase01));
}
} // namespace

void DelayEngine::prepare (double newSampleRate, int /*maximumBlockSize*/, int newNumChannels)
{
    sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
    // Always keep dual delay lines so Stereo / Ping can engage without reallocating
    numChannels = juce::jlimit (1, 2, newNumChannels);
    const int lines = 2;

    bufferSize = juce::jmax (64, (int) std::ceil ((kMaxDelayMs / 1000.0f) * (float) sampleRate) + 8);
    delayLine.assign ((size_t) lines, std::vector<float> ((size_t) bufferSize, 0.0f));
    writeIndex = 0;

    const float smoothSec = 0.05f;
    delaySamplesSmoothed.reset (sampleRate, smoothSec);
    feedbackSmoothed.reset (sampleRate, smoothSec);

    // BFMV clean default ~380 ms (covers Tears Don’t Fall ~370–400 ms)
    const float defaultMs = 380.0f;
    delaySamplesSmoothed.setCurrentAndTargetValue (defaultMs * 0.001f * (float) sampleRate);
    feedbackSmoothed.setCurrentAndTargetValue (0.32f);

    // Mild HPF — clean digital repeats, not dark tape
    fbHpfCoeff = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * 90.0f / (float) sampleRate);
    fbLpState = { 0.0f, 0.0f };
    reversePhase = { 0.0f, 0.5f };

    reset();
}

void DelayEngine::reset()
{
    for (auto& ch : delayLine)
        std::fill (ch.begin(), ch.end(), 0.0f);

    writeIndex = 0;
    fbLpState = { 0.0f, 0.0f };
    reversePhase = { 0.0f, 0.5f };
}

void DelayEngine::setDelayMs (float delayMs)
{
    const float ms = juce::jlimit (kMinDelayMs, kMaxDelayMs, delayMs);
    delaySamplesSmoothed.setTargetValue (ms * 0.001f * (float) sampleRate);
}

void DelayEngine::setFeedback (float feedback01)
{
    feedbackSmoothed.setTargetValue (juce::jlimit (0.0f, kMaxFeedback, feedback01));
}

float DelayEngine::readAt (int channel, float delaySamples) const
{
    float readPos = (float) writeIndex - delaySamples;
    while (readPos < 0.0f)
        readPos += (float) bufferSize;

    const int i0 = (int) readPos;
    const float frac = readPos - (float) i0;
    const int i1 = (i0 + 1) % bufferSize;

    const auto& buf = delayLine[(size_t) channel];
    return buf[(size_t) i0] + frac * (buf[(size_t) i1] - buf[(size_t) i0]);
}

float DelayEngine::readReverseAt (int channel, float delaySamples, float phase) const noexcept
{
    // phase 0→1 reads newest→oldest (tape reverse of the recent window)
    const float safeDelay = juce::jmax (2.0f, delaySamples);
    const float window = hannWindow (phase);
    const float age = phase * safeDelay;
    return readAt (channel, age) * window;
}

void DelayEngine::advanceReversePhases (float delaySamples) noexcept
{
    const float safeDelay = juce::jmax (2.0f, delaySamples);
    for (auto& phase : reversePhase)
    {
        phase += 1.0f / safeDelay;
        if (phase >= 1.0f)
            phase -= 1.0f;
    }
}

float DelayEngine::filterFeedback (int channel, float delayed) noexcept
{
    fbLpState[(size_t) channel] += fbHpfCoeff * (delayed - fbLpState[(size_t) channel]);
    return delayed - fbLpState[(size_t) channel];
}

void DelayEngine::process (juce::AudioBuffer<float>& buffer)
{
    if (bufferSize <= 0 || delayLine.size() < 2)
        return;

    const int numSamples = buffer.getNumSamples();
    const int bufChannels = buffer.getNumChannels();
    if (bufChannels <= 0)
        return;

    const bool stereoBuf = bufChannels > 1;
    const bool usePing = pingPongEnabled && stereoBuf;
    const bool useStereoOffset = stereoEnabled && stereoBuf;

    const float stereoOffsetSamples = kStereoOffsetMs * 0.001f * (float) sampleRate;

    for (int i = 0; i < numSamples; ++i)
    {
        const float delaySamples = delaySamplesSmoothed.getNextValue();
        const float feedback = feedbackSmoothed.getNextValue();

        const float inL = buffer.getSample (0, i);
        const float inR = stereoBuf ? buffer.getSample (1, i) : inL;

        auto tap = [this] (int channel, float samples) noexcept
        {
            if (! reverseEnabled)
                return readAt (channel, samples);

            float mixed = 0.0f;
            float windowSum = 0.0f;
            for (float phase : reversePhase)
            {
                const float window = hannWindow (phase);
                mixed += readReverseAt (channel, samples, phase);
                windowSum += window;
            }
            return windowSum > 1.0e-4f ? mixed / windowSum : mixed;
        };

        if (! stereoEnabled && ! usePing)
        {
            // Linked mono delay: one clear echo image in both speakers
            const float monoIn = 0.5f * (inL + inR);
            const float delayed = tap (0, delaySamples);
            const float fbFiltered = filterFeedback (0, delayed);

            delayLine[0][(size_t) writeIndex] = monoIn + fbFiltered * feedback;
            delayLine[1][(size_t) writeIndex] = delayLine[0][(size_t) writeIndex];

            const float wet = monoIn + delayed * kWetLevel;
            buffer.setSample (0, i, wet);
            if (stereoBuf)
                buffer.setSample (1, i, wet);
        }
        else if (usePing)
        {
            // Classic ping-pong: each side feeds the other
            const float delayL = delaySamples;
            const float delayR = delaySamples + (useStereoOffset ? stereoOffsetSamples : 0.0f);

            const float delayedL = tap (0, delayL);
            const float delayedR = tap (1, delayR);

            const float fbL = filterFeedback (0, delayedL);
            const float fbR = filterFeedback (1, delayedR);

            delayLine[0][(size_t) writeIndex] = inL + fbR * feedback;
            delayLine[1][(size_t) writeIndex] = inR + fbL * feedback;

            buffer.setSample (0, i, inL + delayedL * kWetLevel);
            buffer.setSample (1, i, inR + delayedR * kWetLevel);
        }
        else
        {
            // Stereo (parallel) delay with optional right-channel offset for width
            const float delayL = delaySamples;
            const float delayR = delaySamples + (useStereoOffset ? stereoOffsetSamples : 0.0f);

            const float delayedL = tap (0, delayL);
            const float delayedR = tap (1, delayR);

            const float fbL = filterFeedback (0, delayedL);
            const float fbR = filterFeedback (1, delayedR);

            delayLine[0][(size_t) writeIndex] = inL + fbL * feedback;
            delayLine[1][(size_t) writeIndex] = inR + fbR * feedback;

            buffer.setSample (0, i, inL + delayedL * kWetLevel);
            if (stereoBuf)
                buffer.setSample (1, i, inR + delayedR * kWetLevel);
        }

        if (reverseEnabled)
            advanceReversePhases (delaySamples);

        writeIndex = (writeIndex + 1) % bufferSize;
    }

    juce::ignoreUnused (numChannels);
}
