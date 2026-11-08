#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "dsp/TempoHelpers.h"
#include <cmath>

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
constexpr auto kShimmerEtherealId = "shimmerEthereal";
constexpr auto kWavesOnId        = "wavesOn";
constexpr auto kRateId           = "rate";
constexpr auto kDepthId          = "depth";
constexpr auto kDelayId          = "delay";
constexpr auto kFeedbackId       = "feedback";
constexpr auto kShimmerId        = "shimmer";
constexpr auto kShimmerPitchId   = "shimmerPitch";
constexpr auto kShimmerDecayId   = "shimmerDecay";
constexpr auto kChorusLevelId    = "chorusLevel";
constexpr auto kDelayLevelId     = "delayLevel";
constexpr auto kShimmerLevelId   = "shimmerLevel";
constexpr auto kDelay2OnId       = "delay2On";
constexpr auto kDelay2MusicalId  = "delay2Musical";
constexpr auto kDelay2DivisionId = "delay2Division";
constexpr auto kDelay2Id         = "delay2";
constexpr auto kFeedback2Id      = "feedback2";
constexpr auto kDelay2StereoId   = "delay2Stereo";
constexpr auto kDelay2PingPongId = "delay2PingPong";
constexpr auto kDelay2ReverseId  = "delay2Reverse";
constexpr auto kDelay2LevelId    = "delay2Level";
constexpr auto kShimmer2OnId     = "shimmer2On";
constexpr auto kShimmer2Id       = "shimmer2";
constexpr auto kShimmer2PitchId  = "shimmer2Pitch";
constexpr auto kShimmer2DecayId  = "shimmer2Decay";
constexpr auto kShimmer2EtherealId = "shimmer2Ethereal";
constexpr auto kShimmer2LevelId  = "shimmer2Level";
constexpr auto kMixId            = "mix";
constexpr auto kMasterId         = "master";

juce::AudioParameterFloatAttributes percentAttributes()
{
    return juce::AudioParameterFloatAttributes().withLabel ("%")
        .withStringFromValueFunction ([] (float v, int) {
            return juce::String (juce::roundToInt (v * 100.0f));
        })
        .withValueFromStringFunction ([] (const juce::String& s) {
            return s.getFloatValue() / 100.0f;
        });
}
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
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { kShimmerEtherealId, 1 }, "Shimmer Ethereal", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { kWavesOnId, 1 }, "Waves", false));

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
        percentAttributes()));

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
        percentAttributes()));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { kShimmerId, 1 },
        "Shimmer",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.25f,
        percentAttributes()));

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
        juce::ParameterID { kShimmerDecayId, 1 },
        "Shimmer Decay",
        juce::NormalisableRange<float> { 0.4f, 12.0f, 0.01f, 0.45f },
        3.5f,
        juce::AudioParameterFloatAttributes().withLabel ("s")
            .withStringFromValueFunction ([] (float v, int) {
                return juce::String (v, v < 10.0f ? 2 : 1) + " s";
            })
            .withValueFromStringFunction ([] (const juce::String& s) {
                return s.getFloatValue();
            })));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { kChorusLevelId, 1 },
        "Chorus Level",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        1.0f,
        percentAttributes()));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { kDelayLevelId, 1 },
        "Delay Level",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        1.0f,
        percentAttributes()));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { kShimmerLevelId, 1 },
        "Shimmer Level",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        1.0f,
        percentAttributes()));

    // --- Layer 2: second delay + shimmer (serial after layer 1) ---
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { kDelay2OnId, 1 }, "Delay 2 On", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { kDelay2MusicalId, 1 }, "Delay 2 Musical", false));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { kDelay2DivisionId, 1 },
        "Delay 2 Note",
        TempoHelpers::delayDivisionLabels(),
        TempoHelpers::kDefaultDivisionIndex));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { kDelay2Id, 1 },
        "Delay 2",
        juce::NormalisableRange<float> { 50.0f, 1000.0f, 0.1f, 0.4f },
        500.0f,
        juce::AudioParameterFloatAttributes().withLabel ("ms")));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { kFeedback2Id, 1 },
        "Feedback 2",
        juce::NormalisableRange<float> { 0.0f, 0.85f, 0.001f },
        0.28f,
        percentAttributes()));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { kDelay2StereoId, 1 }, "Delay 2 Stereo", true));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { kDelay2PingPongId, 1 }, "Delay 2 Ping Pong", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { kDelay2ReverseId, 1 }, "Delay 2 Reverse", false));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { kDelay2LevelId, 1 },
        "Delay 2 Level",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        1.0f,
        percentAttributes()));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { kShimmer2OnId, 1 }, "Shimmer 2 On", false));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { kShimmer2Id, 1 },
        "Shimmer 2",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.30f,
        percentAttributes()));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { kShimmer2PitchId, 1 },
        "Shimmer 2 Pitch",
        juce::NormalisableRange<float> { -12.0f, 24.0f, 1.0f },
        7.0f,
        juce::AudioParameterFloatAttributes().withLabel ("st")
            .withStringFromValueFunction ([] (float v, int) {
                const int semis = juce::roundToInt (v);
                return (semis > 0 ? "+" : "") + juce::String (semis) + " st";
            })
            .withValueFromStringFunction ([] (const juce::String& s) {
                return s.getFloatValue();
            })));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { kShimmer2DecayId, 1 },
        "Shimmer 2 Decay",
        juce::NormalisableRange<float> { 0.4f, 12.0f, 0.01f, 0.45f },
        5.0f,
        juce::AudioParameterFloatAttributes().withLabel ("s")
            .withStringFromValueFunction ([] (float v, int) {
                return juce::String (v, v < 10.0f ? 2 : 1) + " s";
            })
            .withValueFromStringFunction ([] (const juce::String& s) {
                return s.getFloatValue();
            })));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { kShimmer2EtherealId, 1 }, "Shimmer 2 Ethereal", false));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { kShimmer2LevelId, 1 },
        "Shimmer 2 Level",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        1.0f,
        percentAttributes()));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { kMixId, 1 },
        "Mix",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.45f,
        percentAttributes()));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { kMasterId, 1 },
        "Master",
        juce::NormalisableRange<float> { 0.0f, 1.5f, 0.001f },
        1.0f,
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

bool EasyChorusAudioProcessor::isDelay2Musical() const
{
    return apvts.getRawParameterValue (kDelay2MusicalId)->load() > 0.5f;
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

float EasyChorusAudioProcessor::resolveDelayMs (bool musical, const char* delayId, const char* divisionId) const
{
    if (musical)
    {
        const int division = juce::roundToInt (apvts.getRawParameterValue (divisionId)->load());
        return TempoHelpers::delayMsFromMusical (getEffectiveBpm(), division);
    }

    return apvts.getRawParameterValue (delayId)->load();
}

float EasyChorusAudioProcessor::getCurrentDelayMs() const
{
    return resolveDelayMs (isDelayMusical(), kDelayId, kDelayDivisionId);
}

float EasyChorusAudioProcessor::getCurrentDelay2Ms() const
{
    return resolveDelayMs (isDelay2Musical(), kDelay2Id, kDelay2DivisionId);
}

void EasyChorusAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    wavesPhase = 0.0f;
    wavesEnvelope.assign ((size_t) juce::jmax (1, samplesPerBlock), 1.0f);

    const int channels = getTotalNumOutputChannels();
    chorusEngine.prepare (sampleRate, samplesPerBlock, channels);
    delayEngine.prepare (sampleRate, samplesPerBlock, channels);
    delayEngine2.prepare (sampleRate, samplesPerBlock, channels);
    shimmerEngine.prepare (sampleRate, samplesPerBlock, channels);
    shimmerEngine2.prepare (sampleRate, samplesPerBlock, channels);

    dryBuffer.setSize (channels, samplesPerBlock, false, false, true);
    stageBuffer.setSize (channels, samplesPerBlock, false, false, true);
    demoTone.prepare (sampleRate, channels);

    const float smoothSec = 0.03f;
    mixSmoothed.reset (sampleRate, smoothSec);
    chorusLevelSmoothed.reset (sampleRate, smoothSec);
    delayLevelSmoothed.reset (sampleRate, smoothSec);
    shimmerLevelSmoothed.reset (sampleRate, smoothSec);
    delay2LevelSmoothed.reset (sampleRate, smoothSec);
    shimmer2LevelSmoothed.reset (sampleRate, smoothSec);
    masterSmoothed.reset (sampleRate, smoothSec);

    mixSmoothed.setCurrentAndTargetValue (0.45f);
    chorusLevelSmoothed.setCurrentAndTargetValue (1.0f);
    delayLevelSmoothed.setCurrentAndTargetValue (1.0f);
    shimmerLevelSmoothed.setCurrentAndTargetValue (1.0f);
    delay2LevelSmoothed.setCurrentAndTargetValue (1.0f);
    shimmer2LevelSmoothed.setCurrentAndTargetValue (1.0f);
    masterSmoothed.setCurrentAndTargetValue (1.0f);

    updateEnginesFromParameters();
}

void EasyChorusAudioProcessor::releaseResources()
{
    chorusEngine.reset();
    delayEngine.reset();
    delayEngine2.reset();
    shimmerEngine.reset();
    shimmerEngine2.reset();
    demoTone.reset();
    wavesPhase = 0.0f;
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
    shimmerEngine.setDecaySeconds (apvts.getRawParameterValue (kShimmerDecayId)->load());
    shimmerEngine.setEthereal (apvts.getRawParameterValue (kShimmerEtherealId)->load() > 0.5f);

    delayEngine2.setDelayMs (getCurrentDelay2Ms());
    delayEngine2.setFeedback (apvts.getRawParameterValue (kFeedback2Id)->load());
    delayEngine2.setStereo (apvts.getRawParameterValue (kDelay2StereoId)->load() > 0.5f);
    delayEngine2.setPingPong (apvts.getRawParameterValue (kDelay2PingPongId)->load() > 0.5f);
    delayEngine2.setReverse (apvts.getRawParameterValue (kDelay2ReverseId)->load() > 0.5f);

    shimmerEngine2.setAmount (apvts.getRawParameterValue (kShimmer2Id)->load());
    shimmerEngine2.setPitchSemitones (apvts.getRawParameterValue (kShimmer2PitchId)->load());
    shimmerEngine2.setDecaySeconds (apvts.getRawParameterValue (kShimmer2DecayId)->load());
    shimmerEngine2.setEthereal (apvts.getRawParameterValue (kShimmer2EtherealId)->load() > 0.5f);

    mixSmoothed.setTargetValue (apvts.getRawParameterValue (kMixId)->load());
    chorusLevelSmoothed.setTargetValue (apvts.getRawParameterValue (kChorusLevelId)->load());
    delayLevelSmoothed.setTargetValue (apvts.getRawParameterValue (kDelayLevelId)->load());
    shimmerLevelSmoothed.setTargetValue (apvts.getRawParameterValue (kShimmerLevelId)->load());
    delay2LevelSmoothed.setTargetValue (apvts.getRawParameterValue (kDelay2LevelId)->load());
    shimmer2LevelSmoothed.setTargetValue (apvts.getRawParameterValue (kShimmer2LevelId)->load());
    masterSmoothed.setTargetValue (apvts.getRawParameterValue (kMasterId)->load());
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
    stageBuffer.setSize (numChannels, numSamples, false, false, true);
    for (int ch = 0; ch < numChannels; ++ch)
        dryBuffer.copyFrom (ch, 0, buffer, ch, 0, numSamples);

    const bool chorusOn   = apvts.getRawParameterValue (kChorusOnId)->load() > 0.5f;
    const bool delayOn    = apvts.getRawParameterValue (kDelayOnId)->load() > 0.5f;
    const bool shimmerOn  = apvts.getRawParameterValue (kShimmerOnId)->load() > 0.5f;
    const bool delay2On   = apvts.getRawParameterValue (kDelay2OnId)->load() > 0.5f;
    const bool shimmer2On = apvts.getRawParameterValue (kShimmer2OnId)->load() > 0.5f;
    const bool wavesOn    = apvts.getRawParameterValue (kWavesOnId)->load() > 0.5f;

    // Tempo-synced swell for delay + shimmer (2 bars @ effective BPM)
    if (wavesOn)
    {
        if ((int) wavesEnvelope.size() < numSamples)
            wavesEnvelope.resize ((size_t) numSamples, 1.0f);

        const float bpm = juce::jmax (40.0f, getEffectiveBpm());
        constexpr float kBeatsPerCycle = 8.0f; // two bars in 4/4
        const float phaseInc = (bpm / 60.0f) / kBeatsPerCycle / (float) currentSampleRate;

        for (int i = 0; i < numSamples; ++i)
        {
            const float sine = std::sin (wavesPhase * juce::MathConstants<float>::twoPi);
            const float rise = 0.5f * (1.0f + sine);
            const float shaped = rise * rise; // linger near quiet, swell into the wash
            wavesEnvelope[(size_t) i] = 0.05f + 0.95f * shaped;
            wavesPhase += phaseInc;
            if (wavesPhase >= 1.0f)
                wavesPhase -= 1.0f;
        }
    }

    auto applyStageLevel = [&] (juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>& levelSmoothed,
                                bool pulseWithWaves)
    {
        // Scale only the stage's wet contribution: out = pre + (post - pre) * level
        for (int i = 0; i < numSamples; ++i)
        {
            float level = levelSmoothed.getNextValue();
            if (pulseWithWaves && wavesOn)
                level *= wavesEnvelope[(size_t) i];

            for (int ch = 0; ch < numChannels; ++ch)
            {
                const float pre  = stageBuffer.getSample (ch, i);
                const float post = buffer.getSample (ch, i);
                buffer.setSample (ch, i, pre + (post - pre) * level);
            }
        }
    };

    if (chorusOn)
    {
        for (int ch = 0; ch < numChannels; ++ch)
            stageBuffer.copyFrom (ch, 0, buffer, ch, 0, numSamples);
        chorusEngine.process (buffer);
        applyStageLevel (chorusLevelSmoothed, false);
    }
    else
    {
        chorusLevelSmoothed.skip (numSamples);
    }

    if (delayOn)
    {
        for (int ch = 0; ch < numChannels; ++ch)
            stageBuffer.copyFrom (ch, 0, buffer, ch, 0, numSamples);
        delayEngine.process (buffer);
        applyStageLevel (delayLevelSmoothed, true);
    }
    else
    {
        delayLevelSmoothed.skip (numSamples);
    }

    if (shimmerOn)
    {
        for (int ch = 0; ch < numChannels; ++ch)
            stageBuffer.copyFrom (ch, 0, buffer, ch, 0, numSamples);
        shimmerEngine.process (buffer);
        applyStageLevel (shimmerLevelSmoothed, true);
    }
    else
    {
        shimmerLevelSmoothed.skip (numSamples);
    }

    if (delay2On)
    {
        for (int ch = 0; ch < numChannels; ++ch)
            stageBuffer.copyFrom (ch, 0, buffer, ch, 0, numSamples);
        delayEngine2.process (buffer);
        applyStageLevel (delay2LevelSmoothed, true);
    }
    else
    {
        delay2LevelSmoothed.skip (numSamples);
    }

    if (shimmer2On)
    {
        for (int ch = 0; ch < numChannels; ++ch)
            stageBuffer.copyFrom (ch, 0, buffer, ch, 0, numSamples);
        shimmerEngine2.process (buffer);
        applyStageLevel (shimmer2LevelSmoothed, true);
    }
    else
    {
        shimmer2LevelSmoothed.skip (numSamples);
    }

    for (int i = 0; i < numSamples; ++i)
    {
        const float mix = mixSmoothed.getNextValue();
        const float master = masterSmoothed.getNextValue();
        const float dryGain = (1.0f - mix) * master;
        const float wetGain = mix * master;

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

int EasyChorusAudioProcessor::getNumPrograms()
{
    return juce::jmax (1, presetManager.getNumPresets());
}

int EasyChorusAudioProcessor::getCurrentProgram()
{
    return presetManager.getCurrentIndex();
}

void EasyChorusAudioProcessor::setCurrentProgram (int index)
{
    loadPreset (index);
}

const juce::String EasyChorusAudioProcessor::getProgramName (int index)
{
    return presetManager.getPresetName (index);
}

void EasyChorusAudioProcessor::loadPreset (int index)
{
    presetManager.applyPreset (index, apvts);
}

void EasyChorusAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("presetIndex", presetManager.getCurrentIndex(), nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void EasyChorusAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            const int presetIndex = (int) tree.getProperty ("presetIndex", 0);
            apvts.replaceState (tree);
            presetManager.setCurrentIndex (presetIndex);
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EasyChorusAudioProcessor();
}
