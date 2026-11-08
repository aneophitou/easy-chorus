
void EasyChorusAudioProcessorEditor::refreshToggleLook (juce::TextButton& button, bool isOn)
{
    button.setColour (juce::TextButton::buttonColourId, isOn ? kAccent : kToggleOff);
    button.setColour (juce::TextButton::buttonOnColourId, kAccent);
    button.setColour (juce::TextButton::textColourOffId, isOn ? kBgBottom : kMuted);
    button.setColour (juce::TextButton::textColourOnId, kBgBottom);
}

void EasyChorusAudioProcessorEditor::refreshDemoLook()
{
    const bool isOn = demoToggle.getToggleState();
    demoToggle.setColour (juce::TextButton::buttonColourId, isOn ? kDemoAccent : kToggleOff);
    demoToggle.setColour (juce::TextButton::buttonOnColourId, kDemoAccent);
    demoToggle.setColour (juce::TextButton::textColourOffId, isOn ? kBgBottom : kMuted);
    demoToggle.setColour (juce::TextButton::textColourOnId, kBgBottom);
}

void EasyChorusAudioProcessorEditor::setupKnob (Knob& knob,
                                                const juce::String& name,
                                                const juce::String& paramId,
                                                std::function<juce::String (float)> formatter,
                                                std::vector<float> stepValues,
                                                double snapInterval)
{
    knob.dialHost.setInterceptsMouseClicks (false, true);
    addAndMakeVisible (knob.dialHost);

    knob.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    knob.slider.setRotaryParameters (kStartAngle, kEndAngle, true);
    knob.slider.setColour (juce::Slider::rotarySliderFillColourId, kAccent);
    knob.slider.setColour (juce::Slider::rotarySliderOutlineColourId, kMuted.withAlpha (0.55f));
    knob.slider.setColour (juce::Slider::thumbColourId, kText);
    knob.dialHost.addAndMakeVisible (knob.slider);

    knob.steps.setTargetSlider (&knob.slider);
    knob.steps.setRotarySpan (kStartAngle, kEndAngle);
    knob.steps.setColours (kMuted, kAccent);
    knob.steps.setSteps (std::move (stepValues));
    knob.dialHost.addAndMakeVisible (knob.steps);
    knob.steps.toFront (false);

    knob.title.setText (name, juce::dontSendNotification);
    knob.title.setJustificationType (juce::Justification::centred);
    knob.title.setColour (juce::Label::textColourId, kText);
    knob.title.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    addAndMakeVisible (knob.title);

    knob.value.setJustificationType (juce::Justification::centred);
    knob.value.setColour (juce::Label::textColourId, kMuted);
    knob.value.setFont (juce::FontOptions (12.0f));
    addAndMakeVisible (knob.value);

    knob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), paramId, knob.slider);

    if (snapInterval > 0.0)
        knob.slider.setRange (knob.slider.getMinimum(), knob.slider.getMaximum(), snapInterval);

    knob.slider.onValueChange = [&knob, formatter]()
    {
        knob.value.setText (formatter ((float) knob.slider.getValue()), juce::dontSendNotification);
        knob.steps.repaint();
    };

    knob.slider.onValueChange();
}

void EasyChorusAudioProcessorEditor::configureDelayFreeSteps()
{
    delayKnob.steps.setSteps ({ 50.0f, 100.0f, 150.0f, 200.0f, 250.0f, 300.0f,
                                380.0f, 500.0f, 600.0f, 750.0f, 1000.0f });
    delayKnob.slider.setRange (50.0, 1000.0, 1.0);
}

void EasyChorusAudioProcessorEditor::configureDelayMusicalSteps()
{
    const int n = TempoHelpers::delayDivisionLabels().size();
    std::vector<float> idxs;
    idxs.reserve ((size_t) n);
    for (int i = 0; i < n; ++i)
        idxs.push_back ((float) i);
    delayKnob.steps.setSteps (std::move (idxs));
    delayKnob.slider.setRange (0.0, (double) n - 1.0, 1.0);
}

void EasyChorusAudioProcessorEditor::rebindDelayKnob()
{
    const bool musical = audioProcessor.isDelayMusical();
    delayKnobMusical = musical;
    delayKnob.attachment.reset();

    if (musical)
    {
        delayKnob.title.setText ("Note", juce::dontSendNotification);
        configureDelayMusicalSteps();
        delayKnob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            audioProcessor.getAPVTS(), "delayDivision", delayKnob.slider);
    }
    else
    {
        delayKnob.title.setText ("Time", juce::dontSendNotification);
        configureDelayFreeSteps();
        delayKnob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            audioProcessor.getAPVTS(), "delay", delayKnob.slider);
    }

    delayKnob.slider.onValueChange = [this]()
    {
        updateDelayValueLabel();
        delayKnob.steps.repaint();
    };

    updateDelayValueLabel();
    delayKnob.steps.repaint();
}

void EasyChorusAudioProcessorEditor::updateDelayValueLabel()
{
    if (audioProcessor.isDelayMusical())
    {
        const int idx = juce::jlimit (0, TempoHelpers::delayDivisionLabels().size() - 1,
                                      juce::roundToInt (delayKnob.slider.getValue()));
        const float ms = audioProcessor.getCurrentDelayMs();
        delayKnob.value.setText (TempoHelpers::delayDivisionLabels()[idx]
                                     + "  (" + juce::String (juce::roundToInt (ms)) + " ms)",
                                 juce::dontSendNotification);
    }
    else
    {
        delayKnob.value.setText (juce::String (juce::roundToInt (delayKnob.slider.getValue())) + " ms",
                                 juce::dontSendNotification);
    }
}

void EasyChorusAudioProcessorEditor::updateBpmInteraction()
{
    const bool sync = audioProcessor.isTempoSyncEnabled();
    bpmKnob.slider.setEnabled (! sync);
    bpmKnob.slider.setAlpha (sync ? 0.45f : 1.0f);
    bpmKnob.steps.setEnabled (! sync);
    bpmKnob.steps.setAlpha (sync ? 0.35f : 1.0f);
    bpmKnob.title.setColour (juce::Label::textColourId, sync ? kMuted : kText);
}

void EasyChorusAudioProcessorEditor::refreshTempoLooks()
{
    refreshToggleLook (syncToggle, syncToggle.getToggleState());
    refreshToggleLook (musicalToggle, musicalToggle.getToggleState());
    updateBpmInteraction();

    if (audioProcessor.isTempoSyncEnabled())
    {
        const float bpm = audioProcessor.getEffectiveBpm();
        bpmKnob.value.setText (juce::String (bpm, 1) + " host", juce::dontSendNotification);
    }
}

void EasyChorusAudioProcessorEditor::refreshDelayModeLooks()
{
    refreshToggleLook (stereoToggle, stereoToggle.getToggleState());
    refreshToggleLook (pingPongToggle, pingPongToggle.getToggleState());
    refreshToggleLook (reverseToggle, reverseToggle.getToggleState());
    refreshToggleLook (etherealToggle, etherealToggle.getToggleState());
    refreshToggleLook (wavesToggle, wavesToggle.getToggleState());
}

void EasyChorusAudioProcessorEditor::refreshPresetLook()
{
    const int expectedId = audioProcessor.getPresetManager().getCurrentIndex() + 1;
    if (presetCombo.getSelectedId() != expectedId)
        syncPresetComboFromProcessor();
}

void EasyChorusAudioProcessorEditor::syncPresetComboFromProcessor()
{
    ignorePresetComboCallback = true;
    const int idx = audioProcessor.getPresetManager().getCurrentIndex();
    presetCombo.setSelectedId (idx + 1, juce::dontSendNotification);
    ignorePresetComboCallback = false;
}

void EasyChorusAudioProcessorEditor::loadSelectedPreset()
{
    const int id = presetCombo.getSelectedId();
    if (id <= 0)
        return;
    audioProcessor.loadPreset (id - 1);
    rebindDelayKnob();
    updateBpmInteraction();
    refreshDelayModeLooks();
    refreshTempoLooks();
}

void EasyChorusAudioProcessorEditor::selectPrevPreset()
{
    const int idx = audioProcessor.getPresetManager().prevIndex();
    audioProcessor.loadPreset (idx);
    syncPresetComboFromProcessor();
    rebindDelayKnob();
    updateBpmInteraction();
    refreshDelayModeLooks();
    refreshTempoLooks();
}

void EasyChorusAudioProcessorEditor::selectNextPreset()
{
    const int idx = audioProcessor.getPresetManager().nextIndex();
    audioProcessor.loadPreset (idx);
    syncPresetComboFromProcessor();
    rebindDelayKnob();
    updateBpmInteraction();
    refreshDelayModeLooks();
    refreshTempoLooks();
}

void EasyChorusAudioProcessorEditor::timerCallback()
{
    const float rate = audioProcessor.getAPVTS().getRawParameterValue ("rate")->load();
    lfoPhase += rate / 30.0f;
    if (lfoPhase >= 1.0f)
        lfoPhase -= 1.0f;

    refreshDemoLook();
    refreshToggleLook (chorusToggle,  chorusToggle.getToggleState());
    refreshToggleLook (delayToggle,   delayToggle.getToggleState());
    refreshToggleLook (shimmerToggle, shimmerToggle.getToggleState());
    refreshTempoLooks();
    refreshDelayModeLooks();
    refreshPresetLook();

    if (delayKnobMusical != audioProcessor.isDelayMusical())
        rebindDelayKnob();
    else if (audioProcessor.isDelayMusical())
        updateDelayValueLabel();

    bpmKnob.steps.repaint();
    rateKnob.steps.repaint();
    depthKnob.steps.repaint();
    delayKnob.steps.repaint();
    feedbackKnob.steps.repaint();
    shimmerKnob.steps.repaint();
    pitchKnob.steps.repaint();
    decayKnob.steps.repaint();
    chorusLevelKnob.steps.repaint();
    delayLevelKnob.steps.repaint();
    shimmerLevelKnob.steps.repaint();
    mixKnob.steps.repaint();
    masterKnob.steps.repaint();

    repaint (getLocalBounds().removeFromTop (56));
}
