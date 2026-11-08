#include "DelayEngine.h"
#include <cmath>

void DelayEngine::prepare (double newSampleRate, int /*maximumBlockSize*/, int newNumChannels)
{
    sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
    numChannels = juce::jlimit (1, 2, newNumChannels);

    bufferSize = juce::jmax (64, (int) std::ceil ((kMaxDelayMs / 1000.0f) * (float) sampleRate) + 8);
    delayLine.assign ((size_t) numChannels, std::vector<float> ((size_t) bufferSize, 0.0f));
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

    reset();
}

void DelayEngine::reset()
{
    for (auto& ch : delayLine)
        std::fill (ch.begin(), ch.end(), 0.0f);

    writeIndex = 0;
    fbLpState = { 0.0f, 0.0f };
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

void DelayEngine::process (juce::AudioBuffer<float>& buffer)
{
    if (bufferSize <= 0 || delayLine.empty())
        return;

    const int numSamples = buffer.getNumSamples();
    const int channels = juce::jmin (buffer.getNumChannels(), numChannels);

    for (int i = 0; i < numSamples; ++i)
    {
        const float delaySamples = delaySamplesSmoothed.getNextValue();
        const float feedback = feedbackSmoothed.getNextValue();

        for (int ch = 0; ch < channels; ++ch)
        {
            const float input = buffer.getSample (ch, i);
            float delayed = readAt (ch, delaySamples);

            fbLpState[(size_t) ch] += fbHpfCoeff * (delayed - fbLpState[(size_t) ch]);
            const float fbFiltered = delayed - fbLpState[(size_t) ch];

            delayLine[(size_t) ch][(size_t) writeIndex] = input + fbFiltered * feedback;

            // Clean digital: keep direct, add one clear echo trail
            buffer.setSample (ch, i, input + delayed * 0.85f);
        }

        writeIndex = (writeIndex + 1) % bufferSize;
    }
}
