#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "dsp/TempoHelpers.h"

namespace
{
constexpr auto kDemoOnId         = "demoOn";
constexpr auto kChorusOnId       = "chorusOn";
constexpr auto kDelayOnId        = "delayOn";
constexpr auto kShimmerOnId      = "shimmerOn";
constexpr auto kTempoSyncId      = "tempoSync";
constexpr auto kBpmId            = "bpm";
constexpr auto kDelayMusicalId   = "delayMusical";
constexpr auto kDelayDivisionId  = "delayDivision";
constexpr auto kDelayStereoId    = "delayStereo";
constexpr auto kDelayPingPongId  = "delayPingPong";
constexpr auto kDelayReverseId   = "delayReverse";
constexpr auto kRateId           = "rate";
constexpr auto kDepthId          = "depth";
constexpr auto kDelayId          = "delay";
constexpr auto kFeedbackId       = "feedback";
constexpr auto kShimmerId        = "shimmer";
constexpr auto kShimmerPitchId   = "shimmerPitch";
constexpr auto kMixId            = "mix";
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

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { kDemoOnId, 1 }, "Demo", false));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { kChorusOnId, 1 }, "Chorus On", true));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { kDelayOnId, 1 }, "Delay On", true));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { kShimmerOnId, 1 }, "Shimmer On", false));

    // Tempo: Sync to DAW, or set BPM manually; musical vs free (ms) delay
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { kTempoSyncId, 1 }, "Tempo Sync", true));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { kBpmId, 1 },
        "BPM",
        juce::NormalisableRange<float> { 40.0f, 240.0f, 0.1f },
        120.0f,
        juce::AudioParameterFloatAttributes().withLabel ("BPM")));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { kDelayMusicalId, 1 }, "Musical Delay", false));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { kDelayDivisionId, 1 },
        "Delay Note",
        TempoHelpers::delayDivisionLabels(),
        TempoHelpers::kDefaultDivisionIndex));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { kDelayStereoId, 1 }, "Delay Stereo", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { kDelayPingPongId, 1 }, "Delay Ping Pong", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { kDelayReverseId, 1 }, "Delay Reverse", false));

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
        juce::ParameterID { kShimmerPitchId, 1 },
        "Shimmer Pitch",
        juce::NormalisableRange<float> { -12.0f, 24.0f, 1.0f },
        12.0f,
        juce::AudioParameterFloatAttributes().withLabel ("st")
            .withStringFromValueFunction ([] (float v, int) {
                const int semis = juce::roundToInt (v);
                return (semis > 0 ? "+" : "") + juce::String (semis) + " st";
            })
            .withValueFromStringFunction ([] (const juce::String& s) {
                return s.getFloatValue();
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

float EasyChorusAudioProcessor::readHostBpm() const
{
    if (auto* playHead = getPlayHead())
    {
        if (auto position = playHead->getPosition())
        {
            if (auto bpm = position->getBpm())
            {
                if (*bpm > 1.0)
                    return (float) *bpm;
            }
        }
    }

    return 0.0f;
}

bool EasyChorusAudioProcessor::isTempoSyncEnabled() const
{
    return apvts.getRawParameterValue (kTempoSyncId)->load() > 0.5f;
}

bool EasyChorusAudioProcessor::isDelayMusical() const
{
    return apvts.getRawParameterValue (kDelayMusicalId)->load() > 0.5f;
}

float EasyChorusAudioProcessor::getEffectiveBpm() const
{
    const float manual = apvts.getRawParameterValue (kBpmId)->load();

    if (isTempoSyncEnabled())
    {
        const float host = readHostBpm();
        if (host > 1.0f)
            return host;

        // Standalone / no playhead tempo: fall back to manual BPM
        return manual;
    }

    return manual;
}

float EasyChorusAudioProcessor::getCurrentDelayMs() const
{
    if (isDelayMusical())
    {
        const int division = juce::roundToInt (apvts.getRawParameterValue (kDelayDivisionId)->load());
        return TempoHelpers::delayMsFromMusical (getEffectiveBpm(), division);
    }

    return apvts.getRawParameterValue (kDelayId)->load();
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

    const float host = readHostBpm();
    if (host > 1.0f)
        lastHostBpm = host;

    delayEngine.setDelayMs (getCurrentDelayMs());
    delayEngine.setFeedback (apvts.getRawParameterValue (kFeedbackId)->load());
    delayEngine.setStereo (apvts.getRawParameterValue (kDelayStereoId)->load() > 0.5f);
    delayEngine.setPingPong (apvts.getRawParameterValue (kDelayPingPongId)->load() > 0.5f);
    delayEngine.setReverse (apvts.getRawParameterValue (kDelayReverseId)->load() > 0.5f);
    shimmerEngine.setAmount (apvts.getRawParameterValue (kShimmerId)->load());
    shimmerEngine.setPitchSemitones (apvts.getRawParameterValue (kShimmerPitchId)->load());
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

    const bool demoOn = apvts.getRawParameterValue (kDemoOnId)->load() > 0.5f;
    if (demoOn)
        demoTone.render (buffer);

    dryBuffer.setSize (numChannels, numSamples, false, false, true);
    for (int ch = 0; ch < numChannels; ++ch)
        dryBuffer.copyFrom (ch, 0, buffer, ch, 0, numSamples);

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
