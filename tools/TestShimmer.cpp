#include <JuceHeader.h>
#include "../Source/dsp/ShimmerEngine.h"
#include <cmath>
#include <iostream>

static void runCase (const char* name, bool ethereal, float amount)
{
    constexpr double sr = 48000.0;
    constexpr int n = 48000;
    constexpr int block = 512;

    ShimmerEngine eng;
    eng.prepare (sr, block, 2);
    eng.setAmount (amount);
    eng.setPitchSemitones (12.0f);
    eng.setEthereal (ethereal);

    juce::AudioBuffer<float> buf (2, block);
    double wetEnergy = 0.0; // energy above dry (impulse residual)
    double energy = 0.0;
    int nanCount = 0;
    float peak = 0.0f;
    int total = 0;
    int samplesLeft = n;

    while (samplesLeft > 0)
    {
        const int todo = juce::jmin (block, samplesLeft);
        buf.setSize (2, todo, false, false, true);
        buf.clear();
        if ((total % 12000) == 0)
        {
            buf.setSample (0, 0, 0.8f);
            buf.setSample (1, 0, 0.8f);
        }

        eng.process (buf);

        for (int i = 0; i < todo; ++i)
        {
            const float L = buf.getSample (0, i);
            const float R = buf.getSample (1, i);
            if (! std::isfinite (L) || ! std::isfinite (R))
                ++nanCount;
            peak = juce::jmax (peak, juce::jmax (std::abs (L), std::abs (R)));
            energy += (double) L * L + (double) R * R;
            // after first sample of impulse blocks, remaining should be wash
            if (! ((total % 12000) == 0 && i == 0))
                wetEnergy += (double) L * L + (double) R * R;
        }

        samplesLeft -= todo;
        total += todo;
    }

    std::cout << name
              << " amount=" << amount
              << " energy=" << energy
              << " wetEnergy=" << wetEnergy
              << " peak=" << peak
              << " nans=" << nanCount
              << "\n";
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    runCase ("normal", false, 0.05f);
    runCase ("normal-loud", false, 0.5f);
    runCase ("ethereal", true, 0.5f);
    runCase ("ethereal-soft", true, 0.05f);
    return 0;
}
