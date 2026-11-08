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

        // Defaults for E-Bow / Reverb / chain order (overridden when listed in `values`)
        const ParamValue stageDefaults[] = {
            { "ebowOn", 0 }, { "ebow", 0.45f }, { "ebowHarmonics", 0.30f },
            { "ebowSustain", 0.75f }, { "ebowTone", 0.40f }, { "ebowLevel", 1.0f },
            { "reverbOn", 0 }, { "reverbSize", 0.55f }, { "reverbDecay", 3.2f },
            { "reverbDamp", 0.35f }, { "reverbMix", 0.35f }, { "reverbCharacter", 1 },
            { "reverbLevel", 1.0f },
            // Default chain: E-Bow → Chorus → Delay → Shimmer → Delay2 → Shimmer2 → Reverb
            { "chainSlot0", 5 }, { "chainSlot1", 0 }, { "chainSlot2", 1 },
            { "chainSlot3", 2 }, { "chainSlot4", 3 }, { "chainSlot5", 4 }, { "chainSlot6", 6 },
        };
        p.values.assign (std::begin (stageDefaults), std::end (stageDefaults));

        for (const auto& pv : values)
        {
            bool replaced = false;
            for (auto& existing : p.values)
            {
                if (juce::String (existing.id) == pv.id)
                {
                    existing.value = pv.value;
                    replaced = true;
                    break;
                }
            }
            if (! replaced)
                p.values.push_back (pv);
        }

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
            { "shimmer", 0.08f }, { "shimmerPitch", 12.0f }, { "shimmerDecay", 4.0f }, { "shimmerEthereal", 0 }, { "wavesOn", 1 }, { "delay2On", 1 }, { "delay2Musical", 0 }, { "delay2Division", 3 }, { "delay2", 720.0f }, { "feedback2", 0.28f }, { "delay2Stereo", 1 }, { "delay2PingPong", 0 }, { "delay2Reverse", 1 }, { "delay2Level", 0.70f }, { "shimmer2On", 1 }, { "shimmer2", 0.18f }, { "shimmer2Pitch", 7.0f }, { "shimmer2Decay", 5.0f }, { "shimmer2Ethereal", 0 }, { "shimmer2Level", 0.65f },
            { "chorusLevel", 0.60f }, { "delayLevel", 0.75f }, { "shimmerLevel", 0.45f },
            { "mix", 0.48f }, { "master", 1.0f },
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

        // Forward delay + airy wash; Delay 2 reverse bloom behind the dry line
        addPreset ("Ethereal Wash", {
            { "chorusOn", 1 }, { "delayOn", 1 }, { "shimmerOn", 1 },
            { "tempoSync", 0 }, { "bpm", 85 }, { "delayMusical", 0 }, { "delayDivision", 3 },
            { "delayStereo", 1 }, { "delayPingPong", 1 }, { "delayReverse", 0 },
            { "rate", 0.36f }, { "depth", 0.28f }, { "delay", 400.0f }, { "feedback", 0.22f },
            { "shimmer", 0.28f }, { "shimmerPitch", 12.0f }, { "shimmerDecay", 4.5f }, { "shimmerEthereal", 1 },
            { "wavesOn", 1 },
            { "delay2On", 1 }, { "delay2Musical", 0 }, { "delay2Division", 3 }, { "delay2", 720.0f }, { "feedback2", 0.24f },
            { "delay2Stereo", 1 }, { "delay2PingPong", 0 }, { "delay2Reverse", 1 }, { "delay2Level", 0.42f },
            { "shimmer2On", 1 }, { "shimmer2", 0.22f }, { "shimmer2Pitch", 7.0f }, { "shimmer2Decay", 5.5f },
            { "shimmer2Ethereal", 1 }, { "shimmer2Level", 0.55f },
            { "chorusLevel", 0.55f }, { "delayLevel", 0.32f }, { "shimmerLevel", 0.60f },
            { "mix", 0.35f }, { "master", 1.0f },
        });

        // Soft glitter + reverse swell behind the dry line
        addPreset ("Starfield", {
            { "chorusOn", 1 }, { "delayOn", 1 }, { "shimmerOn", 1 },
            { "tempoSync", 0 }, { "bpm", 90 }, { "delayMusical", 0 }, { "delayDivision", 3 },
            { "delayStereo", 1 }, { "delayPingPong", 0 }, { "delayReverse", 0 },
            { "rate", 0.42f }, { "depth", 0.24f }, { "delay", 360.0f }, { "feedback", 0.18f },
            { "shimmer", 0.30f }, { "shimmerPitch", 19.0f }, { "shimmerDecay", 4.0f }, { "shimmerEthereal", 1 },
            { "wavesOn", 1 },
            { "delay2On", 1 }, { "delay2Musical", 0 }, { "delay2Division", 3 }, { "delay2", 780.0f }, { "feedback2", 0.26f },
            { "delay2Stereo", 1 }, { "delay2PingPong", 0 }, { "delay2Reverse", 1 }, { "delay2Level", 0.45f },
            { "shimmer2On", 1 }, { "shimmer2", 0.24f }, { "shimmer2Pitch", 12.0f }, { "shimmer2Decay", 5.0f },
            { "shimmer2Ethereal", 1 }, { "shimmer2Level", 0.55f },
            { "chorusLevel", 0.48f }, { "delayLevel", 0.28f }, { "shimmerLevel", 0.62f },
            { "mix", 0.35f }, { "master", 1.0f },
        });

        addPreset ("High Halo", {
            { "chorusOn", 0 }, { "delayOn", 0 }, { "shimmerOn", 1 },
            { "tempoSync", 1 }, { "bpm", 120 }, { "delayMusical", 0 }, { "delayDivision", 3 },
            { "delayStereo", 0 }, { "delayPingPong", 0 }, { "delayReverse", 0 },
            { "rate", 0.70f }, { "depth", 0.20f }, { "delay", 380.0f }, { "feedback", 0.20f },
            { "shimmer", 0.28f }, { "shimmerPitch", 12.0f }, { "shimmerDecay", 4.5f }, { "shimmerEthereal", 1 },
            { "wavesOn", 1 },
            { "delay2On", 0 }, { "delay2Musical", 0 }, { "delay2Division", 3 }, { "delay2", 500.0f }, { "feedback2", 0.28f },
            { "delay2Stereo", 1 }, { "delay2PingPong", 0 }, { "delay2Reverse", 0 }, { "delay2Level", 1.0f },
            { "shimmer2On", 1 }, { "shimmer2", 0.18f }, { "shimmer2Pitch", 7.0f }, { "shimmer2Decay", 5.0f },
            { "shimmer2Ethereal", 1 }, { "shimmer2Level", 0.48f },
            { "chorusLevel", 0.0f }, { "delayLevel", 0.0f }, { "shimmerLevel", 0.62f },
            { "mix", 0.35f }, { "master", 1.0f },
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

        // Synth-like infinite sustain up front, gentle hall behind
        addPreset ("Synth Bow", {
            { "chorusOn", 1 }, { "delayOn", 1 }, { "shimmerOn", 0 },
            { "tempoSync", 0 }, { "bpm", 88 }, { "delayMusical", 0 }, { "delayDivision", 3 },
            { "delayStereo", 1 }, { "delayPingPong", 0 }, { "delayReverse", 0 },
            { "rate", 0.45f }, { "depth", 0.22f }, { "delay", 420.0f }, { "feedback", 0.22f },
            { "shimmer", 0.0f }, { "shimmerPitch", 12.0f }, { "shimmerDecay", 3.5f }, { "shimmerEthereal", 0 },
            { "wavesOn", 1 },
            { "delay2On", 0 }, { "shimmer2On", 0 },
            { "ebowOn", 1 }, { "ebow", 0.50f }, { "ebowHarmonics", 0.28f },
            { "ebowSustain", 0.88f }, { "ebowTone", 0.38f }, { "ebowLevel", 0.75f },
            { "reverbOn", 1 }, { "reverbSize", 0.50f }, { "reverbDecay", 3.5f },
            { "reverbDamp", 0.40f }, { "reverbMix", 0.28f }, { "reverbCharacter", 1 },
            { "reverbLevel", 0.70f },
            { "chorusLevel", 0.55f }, { "delayLevel", 0.35f }, { "shimmerLevel", 0.0f },
            { "mix", 0.42f }, { "master", 1.0f },
        });

        // Plate verb + soft e-bow, reverb late in chain
        addPreset ("Plate Bloom", {
            { "chorusOn", 1 }, { "delayOn", 0 }, { "shimmerOn", 0 },
            { "tempoSync", 1 }, { "bpm", 110 }, { "delayMusical", 0 }, { "delayDivision", 3 },
            { "rate", 0.55f }, { "depth", 0.30f },
            { "wavesOn", 1 },
            { "delay2On", 0 }, { "shimmer2On", 0 },
            { "ebowOn", 1 }, { "ebow", 0.32f }, { "ebowHarmonics", 0.22f },
            { "ebowSustain", 0.80f }, { "ebowTone", 0.42f }, { "ebowLevel", 0.65f },
            { "reverbOn", 1 }, { "reverbSize", 0.62f }, { "reverbDecay", 4.5f },
            { "reverbDamp", 0.25f }, { "reverbMix", 0.40f }, { "reverbCharacter", 2 },
            { "reverbLevel", 0.85f },
            { "chorusLevel", 0.70f }, { "delayLevel", 0.0f }, { "shimmerLevel", 0.0f },
            { "mix", 0.40f }, { "master", 1.0f },
            // Chorus → E-Bow → Reverb
            { "chainSlot0", 0 }, { "chainSlot1", 5 }, { "chainSlot2", 6 },
            { "chainSlot3", 1 }, { "chainSlot4", 2 }, { "chainSlot5", 3 }, { "chainSlot6", 4 },
        });

        // Spring character + reverse delay bloom
        addPreset ("Spring Well", {
            { "chorusOn", 0 }, { "delayOn", 1 }, { "shimmerOn", 0 },
            { "tempoSync", 0 }, { "bpm", 96 }, { "delayMusical", 0 }, { "delayDivision", 3 },
            { "delayStereo", 1 }, { "delayPingPong", 0 }, { "delayReverse", 1 },
            { "delay", 520.0f }, { "feedback", 0.30f },
            { "wavesOn", 1 },
            { "delay2On", 0 }, { "shimmer2On", 0 },
            { "ebowOn", 0 },
            { "reverbOn", 1 }, { "reverbSize", 0.45f }, { "reverbDecay", 2.8f },
            { "reverbDamp", 0.30f }, { "reverbMix", 0.38f }, { "reverbCharacter", 3 },
            { "reverbLevel", 0.80f },
            { "chorusLevel", 0.0f }, { "delayLevel", 0.55f }, { "shimmerLevel", 0.0f },
            { "mix", 0.40f }, { "master", 1.0f },
            // Delay → Reverb early
            { "chainSlot0", 1 }, { "chainSlot1", 6 }, { "chainSlot2", 0 },
            { "chainSlot3", 5 }, { "chainSlot4", 2 }, { "chainSlot5", 3 }, { "chainSlot6", 4 },
        });

        // Room verb under bowed synth pad
        addPreset ("Room Sustain", {
            { "chorusOn", 1 }, { "delayOn", 1 }, { "shimmerOn", 1 },
            { "tempoSync", 1 }, { "bpm", 100 }, { "delayMusical", 0 }, { "delayDivision", 3 },
            { "delayStereo", 0 }, { "delayPingPong", 0 }, { "delayReverse", 0 },
            { "rate", 0.50f }, { "depth", 0.25f }, { "delay", 300.0f }, { "feedback", 0.20f },
            { "shimmer", 0.18f }, { "shimmerPitch", 12.0f }, { "shimmerDecay", 3.0f }, { "shimmerEthereal", 0 },
            { "wavesOn", 0 },
            { "delay2On", 0 }, { "shimmer2On", 0 },
            { "ebowOn", 1 }, { "ebow", 0.45f }, { "ebowHarmonics", 0.25f },
            { "ebowSustain", 0.82f }, { "ebowTone", 0.36f }, { "ebowLevel", 0.70f },
            { "reverbOn", 1 }, { "reverbSize", 0.35f }, { "reverbDecay", 1.6f },
            { "reverbDamp", 0.45f }, { "reverbMix", 0.30f }, { "reverbCharacter", 0 },
            { "reverbLevel", 0.75f },
            { "chorusLevel", 0.50f }, { "delayLevel", 0.30f }, { "shimmerLevel", 0.40f },
            { "mix", 0.38f }, { "master", 1.0f },
        });
    }

    std::vector<Preset> presets;
    int currentIndex = 0;
};
