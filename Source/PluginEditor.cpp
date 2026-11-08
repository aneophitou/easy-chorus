#include "PluginEditor.h"
#include "dsp/TempoHelpers.h"
#include <algorithm>

namespace
{
constexpr int kMinHit = 44;
constexpr int kPreferredWidth = 920;
constexpr int kPreferredHeight = 780;
constexpr int kSectionTitleH = 22;
constexpr int kPanelPad = 10;

const juce::Colour kBgTop      { 0xff1a1c1e };
const juce::Colour kBgBottom   { 0xff101214 };
const juce::Colour kPanel      { 0xff24282c };
const juce::Colour kSection    { 0xff181b1e };
const juce::Colour kAccent     { 0xffc4a574 };
const juce::Colour kDemoAccent { 0xff8fbc8f };
const juce::Colour kText       { 0xffe6e2da };
const juce::Colour kMuted      { 0xff9a9590 };
const juce::Colour kToggleOff  { 0xff1c1f22 };

constexpr float kStartAngle = juce::MathConstants<float>::pi * 225.0f / 180.0f;
constexpr float kEndAngle   = juce::MathConstants<float>::pi * 495.0f / 180.0f;

juce::String formatPercent (float v)
{
    return juce::String (juce::roundToInt (v * 100.0f)) + " %";
}

juce::String formatPitch (float v)
{
    const int semis = juce::roundToInt (v);
    return (semis > 0 ? "+" : "") + juce::String (semis) + " st";
}
} // namespace
