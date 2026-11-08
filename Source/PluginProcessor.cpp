#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr auto kDemoOnId    = "demoOn";
constexpr auto kChorusOnId  = "chorusOn";
constexpr auto kDelayOnId   = "delayOn";
constexpr auto kShimmerOnId = "shimmerOn";
constexpr auto kRateId      = "rate";
constexpr auto kDepthId     = "depth";
constexpr auto kDelayId     = "delay";
constexpr auto kFeedbackId  = "feedback";
constexpr auto kShimmerId   = "shimmer";
constexpr auto kMixId       = "mix";
} // namespace

EasyChorusAudioProcessor::EasyChorusAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout EasyChorusAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Demo tone — injects plucked arpeggio when device input is silent/muted
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { kDemoOnId, 1 }, "Demo", false));

    // Section bypass — BFMV clean: Chorus + Delay on, Shimmer off
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { kChorusOnId, 1 }, "Chorus On", true));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { kDelayOnId, 1 }, "Delay On", true));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { kShimmerOnId, 1 }, "Shimmer On", false));

    // Defaults = BFMV-style clean (Tears Don’t Fall / Hearts Burst Into Fire vibe)
    // CE chorus + ~380 ms digital delay; shimmer amount low (and section off)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { kRateId, 1 },
        "Rate",
        juce::NormalisableRange<float> { 0.1f, 4.0f, 0.001f, 0.5f },
        0.70f,
        juce::AudioParameterFloatAttributes().withLabel ("Hz")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { kDepthId, 1 },
        "Depth",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.35f,
        juce::AudioParameterFloatAttributes().withLabel ("%")
            .withStringFromValueFunction ([] (float v, int) {
                return juce::String (juce::roundToInt (v * 100.0f));
            })
            .withValueFromStringFunction ([] (const juce::String& s) {
                return s.getFloatValue() / 100.0f;
            })));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { kDelayId, 1 },
        "Delay",
        juce::NormalisableRange<float> { 50.0f, 1000.0f, 0.1f, 0.4f },
        380.0f,
        juce::AudioParameterFloatAttributes().withLabel ("ms")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { kFeedbackId, 1 },
        "Feedback",
        juce::NormalisableRange<float> { 0.0f, 0.85f, 0.001f },
        0.32f,
        juce::AudioParameterFloatAttributes().withLabel ("%")
            .withStringFromValueFunction ([] (float v, int) {
                return juce::String (juce::roundToInt (v * 100.0f));
            })
            .withValueFromStringFunction ([] (const juce::String& s) {
                return s.getFloatValue() / 100.0f;
            })));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { kShimmerId, 1 },
        "Shimmer",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.05f,
        juce::AudioParameterFloatAttributes().withLabel ("%")
            .withStringFromValueFunction ([] (float v, int) {
                return juce::String (juce::roundToInt (v * 100.0f));
            })
            .withValueFromStringFunction ([] (const juce::String& s) {
                return s.getFloatValue() / 100.0f;
            })));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { kMixId, 1 },
        "Mix",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.45f,
        juce::AudioParameterFloatAttributes().withLabel ("%")
            .withStringFromValueFunction ([] (float v, int) {
                return juce::String (juce::roundToInt (v * 100.0f));
            })
            .withValueFromStringFunction ([] (const juce::String& s) {
                return s.getFloatValue() / 100.0f;
            })));

    return { params.begin(), params.end() };
}

void EasyChorusAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const int channels = getTotalNumOutputChannels();
    chorusEngine.prepare (sampleRate, samplesPerBlock, channels);
    delayEngine.prepare (sampleRate, samplesPerBlock, channels);
    shimmerEngine.prepare (sampleRate, samplesPerBlock, channels);

    dryBuffer.setSize (channels, samplesPerBlock, false, false, true);
    demoTone.prepare (sampleRate, channels);
    mixSmoothed.reset (sampleRate, 0.03);
    mixSmoothed.setCurrentAndTargetValue (0.45f);

    updateEnginesFromParameters();
}

void EasyChorusAudioProcessor::releaseResources()
{
    chorusEngine.reset();
    delayEngine.reset();
    shimmerEngine.reset();
    demoTone.reset();
}

bool EasyChorusAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainOut = layouts.getMainOutputChannelSet();
    const auto& mainIn  = layouts.getMainInputChannelSet();

    if (mainOut != mainIn)
        return false;

    return mainOut == juce::AudioChannelSet::mono()
        || mainOut == juce::AudioChannelSet::stereo();
}

void EasyChorusAudioProcessor::updateEnginesFromParameters()
{
    chorusEngine.setRateHz (apvts.getRawParameterValue (kRateId)->load());
    chorusEngine.setDepth  (apvts.getRawParameterValue (kDepthId)->load());
    delayEngine.setDelayMs (apvts.getRawParameterValue (kDelayId)->load());
    delayEngine.setFeedback (apvts.getRawParameterValue (kFeedbackId)->load());
    shimmerEngine.setAmount (apvts.getRawParameterValue (kShimmerId)->load());
    mixSmoothed.setTargetValue (apvts.getRawParameterValue (kMixId)->load());
}

void EasyChorusAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    midi.clear();

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    updateEnginesFromParameters();

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // Demo replaces the input buffer so Standalone works with muted/silent devices
    const bool demoOn = apvts.getRawParameterValue (kDemoOnId)->load() > 0.5f;
    if (demoOn)
        demoTone.render (buffer);

    dryBuffer.setSize (numChannels, numSamples, false, false, true);
    for (int ch = 0; ch < numChannels; ++ch)
        dryBuffer.copyFrom (ch, 0, buffer, ch, 0, numSamples);

    // Wet chain with true per-stage bypass (skip block when off)
    const bool chorusOn  = apvts.getRawParameterValue (kChorusOnId)->load() > 0.5f;
    const bool delayOn   = apvts.getRawParameterValue (kDelayOnId)->load() > 0.5f;
    const bool shimmerOn = apvts.getRawParameterValue (kShimmerOnId)->load() > 0.5f;

    if (chorusOn)
        chorusEngine.process (buffer);
    if (delayOn)
        delayEngine.process (buffer);
    if (shimmerOn)
        shimmerEngine.process (buffer);

    for (int i = 0; i < numSamples; ++i)
    {
        const float mix = mixSmoothed.getNextValue();
        const float dryGain = 1.0f - mix;
        const float wetGain = mix;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float dry = dryBuffer.getSample (ch, i);
            const float wet = buffer.getSample (ch, i);
            buffer.setSample (ch, i, dry * dryGain + wet * wetGain);
        }
    }
}

juce::AudioProcessorEditor* EasyChorusAudioProcessor::createEditor()
{
    return new EasyChorusAudioProcessorEditor (*this);
}

void EasyChorusAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void EasyChorusAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EasyChorusAudioProcessor();
}
