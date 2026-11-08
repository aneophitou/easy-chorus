#pragma once

#include <JuceHeader.h>

namespace TempoHelpers
{
inline juce::StringArray delayDivisionLabels()
{
    return {
        "1/1", "1/2", "1/2.", "1/4", "1/4.", "1/4T",
        "1/8", "1/8.", "1/8T", "1/16"
    };
}

/** Beats relative to a quarter note (= 1.0). */
inline float beatsForDivisionIndex (int index) noexcept
{
    switch (index)
    {
        case 0:  return 4.0f;       // 1/1
        case 1:  return 2.0f;       // 1/2
        case 2:  return 3.0f;       // 1/2 dotted
        case 3:  return 1.0f;       // 1/4
        case 4:  return 1.5f;       // 1/4 dotted
        case 5:  return 2.0f / 3.0f; // 1/4 triplet
        case 6:  return 0.5f;       // 1/8
        case 7:  return 0.75f;      // 1/8 dotted
        case 8:  return 1.0f / 3.0f; // 1/8 triplet
        case 9:  return 0.25f;      // 1/16
        default: return 1.0f;
    }
}

inline float delayMsFromMusical (float bpm, int divisionIndex) noexcept
{
    const float safeBpm = juce::jmax (1.0f, bpm);
    return beatsForDivisionIndex (divisionIndex) * (60000.0f / safeBpm);
}

inline constexpr int kDefaultDivisionIndex = 3; // 1/4
} // namespace TempoHelpers
