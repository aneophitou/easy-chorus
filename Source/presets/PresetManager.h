#pragma once

#include <JuceHeader.h>
#include <vector>

/**
 * Factory presets for Easy Chorus, applied through the APVTS.
 * Navigation is handled in the editor (prev / next / combo).
 */
class PresetManager
{
public:
    struct ParamValue
    {
        const char* id;
        float value; // raw parameter units (bool 0/1, choice index, float value)
    };

    struct Preset
    {
        juce::String name;
        std::vector<ParamValue> values;
    };

    PresetManager()
    {
        buildFactoryPresets();
    }

    int getNumPresets() const noexcept { return (int) presets.size(); }

    juce::String getPresetName (int index) const
    {
        if (! isPositiveAndBelow (index, getNumPresets()))
            return {};
        return presets[(size_t) index].name;
    }

    juce::StringArray getPresetNames() const
    {
        juce::StringArray names;
        for (auto& p : presets)
            names.add (p.name);
        return names;
    }

    int getCurrentIndex() const noexcept { return currentIndex; }

    void setCurrentIndex (int index) noexcept
    {
        if (isPositiveAndBelow (index, getNumPresets()))
            currentIndex = index;
    }

    int nextIndex() const noexcept
    {
        if (getNumPresets() <= 0)
            return 0;
        return (currentIndex + 1) % getNumPresets();
    }

    int prevIndex() const noexcept
    {
        if (getNumPresets() <= 0)
            return 0;
        return (currentIndex + getNumPresets() - 1) % getNumPresets();
    }

    /** Apply factory preset. Leaves demoOn unchanged so audition keeps playing. */
    void applyPreset (int index, juce::AudioProcessorValueTreeState& apvts)
    {
        if (! isPositiveAndBelow (index, getNumPresets()))
            return;

        currentIndex = index;
        const auto& preset = presets[(size_t) index];

        for (const auto& pv : preset.values)
        {
            if (juce::String (pv.id) == "demoOn")
                continue;

            if (auto* param = apvts.getParameter (pv.id))
            {
                if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param))
                {
                    const float norm = ranged->convertTo0to1 (pv.value);
                    param->beginChangeGesture();
                    param->setValueNotifyingHost (norm);
                    param->endChangeGesture();
                }
            }
        }
    }

private:
    static bool isPositiveAndBelow (int i, int n) noexcept
    {
        return i >= 0 && i < n;
    }

    void addPreset (juce::String name, std::initializer_list<ParamValue> values)
    {
        Preset p;
        p.name = std::move (name);
        p.values.assign (values);
        presets.push_back (std::move (p));
    }

    void buildFactoryPresets()
    {
        // 0 — plugin defaults / BFMV clean starting point
        addPreset ("BFMV Clean", {
            { "chorusOn", 1 }, { "delayOn", 1 }, { "shimmerOn", 0 },
            { "tempoSync", 1 }, { "bpm", 120 }, { "delayMusical", 0 }, { "delayDivision", 3 },
            { "delayStereo", 0 }, { "delayPingPong", 0 }, { "delayReverse", 0 },
            { "rate", 0.70f }, { "depth", 0.35f }, { "delay", 380.0f }, { "feedback", 0.32f },
            { "shimmer", 0.25f }, { "shimmerPitch", 12.0f }, { "shimmerDecay", 3.5f }, { "shimmerEthereal", 0 }, { "wavesOn", 0 }, { "delay2On", 0 }, { "delay2Musical", 0 }, { "delay2Division", 3 }, { "delay2", 500.0f }, { "feedback2", 0.28f }, { "delay2Stereo", 1 }, { "delay2PingPong", 0 }, { "delay2Reverse", 0 }, { "delay2Level", 1.0f }, { "shimmer2On", 0 }, { "shimmer2", 0.30f }, { "shimmer2Pitch", 7.0f }, { "shimmer2Decay", 5.0f }, { "shimmer2Ethereal", 0 }, { "shimmer2Level", 1.0f },
            { "chorusLevel", 1.0f }, { "delayLevel", 1.0f }, { "shimmerLevel", 1.0f },
            { "mix", 0.45f }, { "master", 1.0f },
        });

        addPreset ("Chorus Glow", {
            { "chorusOn", 1 }, { "delayOn", 0 }, { "shimmerOn", 0 },
            { "tempoSync", 1 }, { "bpm", 120 }, { "delayMusical", 0 }, { "delayDivision", 3 },
            { "delayStereo", 0 }, { "delayPingPong", 0 }, { "delayReverse", 0 },
            { "rate", 0.55f }, { "depth", 0.48f }, { "delay", 380.0f }, { "feedback", 0.20f },
            { "shimmer", 0.0f }, { "shimmerPitch", 12.0f }, { "shimmerDecay", 3.5f }, { "shimmerEthereal", 0 }, { "wavesOn", 0 }, { "delay2On", 0 }, { "delay2Musical", 0 }, { "delay2Division", 3 }, { "delay2", 500.0f }, { "feedback2", 0.28f }, { "delay2Stereo", 1 }, { "delay2PingPong", 0 }, { "delay2Reverse", 0 }, { "delay2Level", 1.0f }, { "shimmer2On", 0 }, { "shimmer2", 0.30f }, { "shimmer2Pitch", 7.0f }, { "shimmer2Decay", 5.0f }, { "shimmer2Ethereal", 0 }, { "shimmer2Level", 1.0f },
            { "chorusLevel", 1.0f }, { "delayLevel", 0.0f }, { "shimmerLevel", 0.0f },
            { "mix", 0.55f }, { "master", 1.0f },
        });

        addPreset ("Delay Trail", {
            { "chorusOn", 0 }, { "delayOn", 1 }, { "shimmerOn", 0 },
            { "tempoSync", 1 }, { "bpm", 120 }, { "delayMusical", 0 }, { "delayDivision", 3 },
            { "delayStereo", 0 }, { "delayPingPong", 0 }, { "delayReverse", 0 },
            { "rate", 0.70f }, { "depth", 0.20f }, { "delay", 420.0f }, { "feedback", 0.42f },
            { "shimmer", 0.0f }, { "shimmerPitch", 12.0f }, { "shimmerDecay", 3.5f }, { "shimmerEthereal", 0 }, { "wavesOn", 0 }, { "delay2On", 0 }, { "delay2Musical", 0 }, { "delay2Division", 3 }, { "delay2", 500.0f }, { "feedback2", 0.28f }, { "delay2Stereo", 1 }, { "delay2PingPong", 0 }, { "delay2Reverse", 0 }, { "delay2Level", 1.0f }, { "shimmer2On", 0 }, { "shimmer2", 0.30f }, { "shimmer2Pitch", 7.0f }, { "shimmer2Decay", 5.0f }, { "shimmer2Ethereal", 0 }, { "shimmer2Level", 1.0f },
            { "chorusLevel", 0.0f }, { "delayLevel", 1.0f }, { "shimmerLevel", 0.0f },
            { "mix", 0.40f }, { "master", 1.0f },
        });

        addPreset ("Ping Pong", {
            { "chorusOn", 1 }, { "delayOn", 1 }, { "shimmerOn", 0 },
            { "tempoSync", 1 }, { "bpm", 120 }, { "delayMusical", 1 }, { "delayDivision", 6 },
            { "delayStereo", 1 }, { "delayPingPong", 1 }, { "delayReverse", 0 },
            { "rate", 0.65f }, { "depth", 0.30f }, { "delay", 380.0f }, { "feedback", 0.48f },
            { "shimmer", 0.0f }, { "shimmerPitch", 12.0f }, { "shimmerDecay", 3.5f }, { "shimmerEthereal", 0 }, { "wavesOn", 0 }, { "delay2On", 0 }, { "delay2Musical", 0 }, { "delay2Division", 3 }, { "delay2", 500.0f }, { "feedback2", 0.28f }, { "delay2Stereo", 1 }, { "delay2PingPong", 0 }, { "delay2Reverse", 0 }, { "delay2Level", 1.0f }, { "shimmer2On", 0 }, { "shimmer2", 0.30f }, { "shimmer2Pitch", 7.0f }, { "shimmer2Decay", 5.0f }, { "shimmer2Ethereal", 0 }, { "shimmer2Level", 1.0f },
            { "chorusLevel", 0.70f }, { "delayLevel", 1.0f }, { "shimmerLevel", 0.0f },
            { "mix", 0.50f }, { "master", 1.0f },
        });

        addPreset ("Reverse Dream", {
            { "chorusOn", 1 }, { "delayOn", 1 }, { "shimmerOn", 0 },
            { "tempoSync", 0 }, { "bpm", 95 }, { "delayMusical", 0 }, { "delayDivision", 3 },
            { "delayStereo", 1 }, { "delayPingPong", 0 }, { "delayReverse", 1 },
            { "rate", 0.40f }, { "depth", 0.40f }, { "delay", 600.0f }, { "feedback", 0.38f },
            { "shimmer", 0.08f }, { "shimmerPitch", 12.0f }, { "shimmerDecay", 4.0f }, { "shimmerEthereal", 0 }, { "wavesOn", 1 }, { "delay2On", 1 }, { "delay2Musical", 0 }, { "delay2Division", 3 }, { "delay2", 720.0f }, { "feedback2", 0.28f }, { "delay2Stereo", 1 }, { "delay2PingPong", 0 }, { "delay2Reverse", 1 }, { "delay2Level", 1.0f }, { "shimmer2On", 1 }, { "shimmer2", 0.22f }, { "shimmer2Pitch", 7.0f }, { "shimmer2Decay", 5.0f }, { "shimmer2Ethereal", 0 }, { "shimmer2Level", 1.0f },
            { "chorusLevel", 0.60f }, { "delayLevel", 1.0f }, { "shimmerLevel", 0.50f },
            { "mix", 0.55f }, { "master", 1.0f },
        });

        addPreset ("Shimmer Cloud", {
            { "chorusOn", 1 }, { "delayOn", 1 }, { "shimmerOn", 1 },
            { "tempoSync", 1 }, { "bpm", 110 }, { "delayMusical", 0 }, { "delayDivision", 3 },
            { "delayStereo", 1 }, { "delayPingPong", 0 }, { "delayReverse", 0 },
            { "rate", 0.50f }, { "depth", 0.28f }, { "delay", 320.0f }, { "feedback", 0.28f },
            { "shimmer", 0.35f }, { "shimmerPitch", 12.0f }, { "shimmerDecay", 4.5f }, { "shimmerEthereal", 0 }, { "wavesOn", 0 }, { "delay2On", 0 }, { "delay2Musical", 0 }, { "delay2Division", 3 }, { "delay2", 500.0f }, { "feedback2", 0.28f }, { "delay2Stereo", 1 }, { "delay2PingPong", 0 }, { "delay2Reverse", 0 }, { "delay2Level", 1.0f }, { "shimmer2On", 0 }, { "shimmer2", 0.30f }, { "shimmer2Pitch", 7.0f }, { "shimmer2Decay", 5.0f }, { "shimmer2Ethereal", 0 }, { "shimmer2Level", 1.0f },
            { "chorusLevel", 0.70f }, { "delayLevel", 0.65f }, { "shimmerLevel", 1.0f },
            { "mix", 0.50f }, { "master", 1.0f },
        });

        // Soft halo behind the dry line — low Mix, gentle amounts, one quiet air layer
        addPreset ("Ethereal Wash", {
            { "chorusOn", 1 }, { "delayOn", 1 }, { "shimmerOn", 1 },
            { "tempoSync", 0 }, { "bpm", 85 }, { "delayMusical", 0 }, { "delayDivision", 3 },
            { "delayStereo", 1 }, { "delayPingPong", 0 }, { "delayReverse", 0 },
            { "rate", 0.40f }, { "depth", 0.28f }, { "delay", 420.0f }, { "feedback", 0.22f },
            { "shimmer", 0.18f }, { "shimmerPitch", 12.0f }, { "shimmerDecay", 4.5f }, { "shimmerEthereal", 1 },
            { "wavesOn", 1 },
            { "delay2On", 0 }, { "delay2Musical", 0 }, { "delay2Division", 3 }, { "delay2", 640.0f }, { "feedback2", 0.20f },
            { "delay2Stereo", 1 }, { "delay2PingPong", 0 }, { "delay2Reverse", 0 }, { "delay2Level", 0.35f },
            { "shimmer2On", 1 }, { "shimmer2", 0.10f }, { "shimmer2Pitch", 19.0f }, { "shimmer2Decay", 5.5f },
            { "shimmer2Ethereal", 1 }, { "shimmer2Level", 0.35f },
            { "chorusLevel", 0.70f }, { "delayLevel", 0.40f }, { "shimmerLevel", 0.45f },
            { "mix", 0.30f }, { "master", 1.0f },
        });

        addPreset ("High Halo", {
            { "chorusOn", 0 }, { "delayOn", 0 }, { "shimmerOn", 1 },
            { "tempoSync", 1 }, { "bpm", 120 }, { "delayMusical", 0 }, { "delayDivision", 3 },
            { "delayStereo", 0 }, { "delayPingPong", 0 }, { "delayReverse", 0 },
            { "rate", 0.70f }, { "depth", 0.20f }, { "delay", 380.0f }, { "feedback", 0.20f },
            { "shimmer", 0.45f }, { "shimmerPitch", 24.0f }, { "shimmerDecay", 6.0f }, { "shimmerEthereal", 1 }, { "wavesOn", 0 }, { "delay2On", 0 }, { "delay2Musical", 0 }, { "delay2Division", 3 }, { "delay2", 500.0f }, { "feedback2", 0.28f }, { "delay2Stereo", 1 }, { "delay2PingPong", 0 }, { "delay2Reverse", 0 }, { "delay2Level", 1.0f }, { "shimmer2On", 1 }, { "shimmer2", 0.35f }, { "shimmer2Pitch", -5.0f }, { "shimmer2Decay", 4.0f }, { "shimmer2Ethereal", 0 }, { "shimmer2Level", 1.0f },
            { "chorusLevel", 0.0f }, { "delayLevel", 0.0f }, { "shimmerLevel", 1.0f },
            { "mix", 0.50f }, { "master", 1.0f },
        });

        addPreset ("Musical 1/8", {
            { "chorusOn", 1 }, { "delayOn", 1 }, { "shimmerOn", 0 },
            { "tempoSync", 1 }, { "bpm", 140 }, { "delayMusical", 1 }, { "delayDivision", 6 },
            { "delayStereo", 0 }, { "delayPingPong", 0 }, { "delayReverse", 0 },
            { "rate", 0.80f }, { "depth", 0.30f }, { "delay", 380.0f }, { "feedback", 0.30f },
            { "shimmer", 0.0f }, { "shimmerPitch", 12.0f }, { "shimmerDecay", 3.5f }, { "shimmerEthereal", 0 }, { "wavesOn", 0 }, { "delay2On", 0 }, { "delay2Musical", 0 }, { "delay2Division", 3 }, { "delay2", 500.0f }, { "feedback2", 0.28f }, { "delay2Stereo", 1 }, { "delay2PingPong", 0 }, { "delay2Reverse", 0 }, { "delay2Level", 1.0f }, { "shimmer2On", 0 }, { "shimmer2", 0.30f }, { "shimmer2Pitch", 7.0f }, { "shimmer2Decay", 5.0f }, { "shimmer2Ethereal", 0 }, { "shimmer2Level", 1.0f },
            { "chorusLevel", 0.85f }, { "delayLevel", 0.90f }, { "shimmerLevel", 0.0f },
            { "mix", 0.45f }, { "master", 1.0f },
        });

        addPreset ("Wide Stereo", {
            { "chorusOn", 1 }, { "delayOn", 1 }, { "shimmerOn", 1 },
            { "tempoSync", 1 }, { "bpm", 100 }, { "delayMusical", 0 }, { "delayDivision", 3 },
            { "delayStereo", 1 }, { "delayPingPong", 0 }, { "delayReverse", 0 },
            { "rate", 0.60f }, { "depth", 0.42f }, { "delay", 280.0f }, { "feedback", 0.25f },
            { "shimmer", 0.18f }, { "shimmerPitch", 7.0f }, { "shimmerDecay", 3.0f }, { "shimmerEthereal", 0 }, { "wavesOn", 0 }, { "delay2On", 0 }, { "delay2Musical", 0 }, { "delay2Division", 3 }, { "delay2", 500.0f }, { "feedback2", 0.28f }, { "delay2Stereo", 1 }, { "delay2PingPong", 0 }, { "delay2Reverse", 0 }, { "delay2Level", 1.0f }, { "shimmer2On", 0 }, { "shimmer2", 0.30f }, { "shimmer2Pitch", 7.0f }, { "shimmer2Decay", 5.0f }, { "shimmer2Ethereal", 0 }, { "shimmer2Level", 1.0f },
            { "chorusLevel", 1.0f }, { "delayLevel", 0.75f }, { "shimmerLevel", 0.70f },
            { "mix", 0.48f }, { "master", 1.0f },
        });
    }

    std::vector<Preset> presets;
    int currentIndex = 0;
};
