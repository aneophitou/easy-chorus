
void EasyChorusAudioProcessorEditor::configureDelayMusicalSteps (Knob& knob)
{
    const int n = TempoHelpers::delayDivisionLabels().size();
    std::vector<float> idxs;
    idxs.reserve ((size_t) n);
    for (int i = 0; i < n; ++i)
        idxs.push_back ((float) i);
    knob.steps.setSteps (std::move (idxs));
    knob.slider.setRange (0.0, (double) n - 1.0, 1.0);
}

void EasyChorusAudioProcessorEditor::rebindDelayKnob (bool isSecond)
{
    Knob& knob = isSecond ? delay2Knob : delayKnob;
    const bool musical = isSecond ? audioProcessor.isDelay2Musical()
                                  : audioProcessor.isDelayMusical();
    if (isSecond)
        delay2KnobMusical = musical;
    else
        delayKnobMusical = musical;

    knob.attachment.reset();

    if (musical)
    {
        knob.title.setText ("Note", juce::dontSendNotification);
        configureDelayMusicalSteps (knob);
        knob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            audioProcessor.getAPVTS(), isSecond ? "delay2Division" : "delayDivision", knob.slider);
    }
    else
    {
        knob.title.setText ("Time", juce::dontSendNotification);
        configureDelayFreeSteps (knob);
        knob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            audioProcessor.getAPVTS(), isSecond ? "delay2" : "delay", knob.slider);
    }

    knob.slider.onValueChange = [this, isSecond]()
    {
        updateDelayValueLabel (isSecond);
        (isSecond ? delay2Knob : delayKnob).steps.repaint();
    };

    updateDelayValueLabel (isSecond);
    knob.steps.repaint();
}

void EasyChorusAudioProcessorEditor::updateDelayValueLabel (bool isSecond)
{
    Knob& knob = isSecond ? delay2Knob : delayKnob;
    const bool musical = isSecond ? audioProcessor.isDelay2Musical()
                                  : audioProcessor.isDelayMusical();
    if (musical)
    {
        const int idx = juce::jlimit (0, TempoHelpers::delayDivisionLabels().size() - 1,
                                      juce::roundToInt (knob.slider.getValue()));
        const float ms = isSecond ? audioProcessor.getCurrentDelay2Ms()
                                  : audioProcessor.getCurrentDelayMs();
        knob.value.setText (TempoHelpers::delayDivisionLabels()[idx]
                                + "  (" + juce::String (juce::roundToInt (ms)) + " ms)",
                            juce::dontSendNotification);
    }
    else
    {
        knob.value.setText (juce::String (juce::roundToInt (knob.slider.getValue())) + " ms",
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
    refreshToggleLook (musical2Toggle, musical2Toggle.getToggleState());
    updateBpmInteraction();

    if (audioProcessor.isTempoSyncEnabled())
    {
        const float bpm = audioProcessor.getEffectiveBpm();
        bpmKnob.value.setText (juce::String (bpm, 1) + " host", juce::dontSendNotification);
    }
}

void EasyChorusAudioProcessorEditor::refreshModeLooks()
{
    refreshToggleLook (stereoToggle, stereoToggle.getToggleState());
    refreshToggleLook (pingPongToggle, pingPongToggle.getToggleState());
    refreshToggleLook (reverseToggle, reverseToggle.getToggleState());
    refreshToggleLook (stereo2Toggle, stereo2Toggle.getToggleState());
    refreshToggleLook (pingPong2Toggle, pingPong2Toggle.getToggleState());
    refreshToggleLook (reverse2Toggle, reverse2Toggle.getToggleState());
    refreshToggleLook (etherealToggle, etherealToggle.getToggleState());
    refreshToggleLook (ethereal2Toggle, ethereal2Toggle.getToggleState());
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
    rebindDelayKnob (false);
    rebindDelayKnob (true);
    updateBpmInteraction();
    refreshModeLooks();
    refreshTempoLooks();
}

void EasyChorusAudioProcessorEditor::selectPrevPreset()
{
    const int idx = audioProcessor.getPresetManager().prevIndex();
    audioProcessor.loadPreset (idx);
    syncPresetComboFromProcessor();
    rebindDelayKnob (false);
    rebindDelayKnob (true);
    updateBpmInteraction();
    refreshModeLooks();
    refreshTempoLooks();
}

void EasyChorusAudioProcessorEditor::selectNextPreset()
{
    const int idx = audioProcessor.getPresetManager().nextIndex();
    audioProcessor.loadPreset (idx);
    syncPresetComboFromProcessor();
    rebindDelayKnob (false);
    rebindDelayKnob (true);
    updateBpmInteraction();
    refreshModeLooks();
    refreshTempoLooks();
}

void EasyChorusAudioProcessorEditor::timerCallback()
{
    const float rate = audioProcessor.getAPVTS().getRawParameterValue ("rate")->load();
    lfoPhase += rate / 30.0f;
    if (lfoPhase >= 1.0f)
        lfoPhase -= 1.0f;

    refreshDemoLook();
    refreshToggleLook (chorusToggle,   chorusToggle.getToggleState());
    refreshToggleLook (delayToggle,    delayToggle.getToggleState());
    refreshToggleLook (shimmerToggle,  shimmerToggle.getToggleState());
    refreshToggleLook (delay2Toggle,   delay2Toggle.getToggleState());
    refreshToggleLook (shimmer2Toggle, shimmer2Toggle.getToggleState());
    refreshTempoLooks();
    refreshModeLooks();
    refreshPresetLook();
    refreshPageTabLooks();

    if (delayKnobMusical != audioProcessor.isDelayMusical())
        rebindDelayKnob (false);
    else if (audioProcessor.isDelayMusical())
        updateDelayValueLabel (false);

    if (delay2KnobMusical != audioProcessor.isDelay2Musical())
        rebindDelayKnob (true);
    else if (audioProcessor.isDelay2Musical())
        updateDelayValueLabel (true);

    for (Knob* k : { &bpmKnob, &rateKnob, &depthKnob, &delayKnob, &feedbackKnob,
                     &shimmerKnob, &pitchKnob, &decayKnob, &chorusLevelKnob,
                     &delayLevelKnob, &shimmerLevelKnob, &delay2Knob, &feedback2Knob,
                     &delay2LevelKnob, &shimmer2Knob, &pitch2Knob, &decay2Knob,
                     &shimmer2LevelKnob, &mixKnob, &masterKnob })
        k->steps.repaint();

    repaint (getLocalBounds().removeFromTop (60));
}

void EasyChorusAudioProcessorEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setGradientFill (juce::ColourGradient (kBgTop, 0.0f, 0.0f,
                                             kBgBottom, 0.0f, bounds.getHeight(), false));
    g.fillAll();

    auto panel = bounds.reduced (14.0f, 12.0f);
    g.setColour (kPanel);
    g.fillRoundedRectangle (panel, 14.0f);
    g.setColour (kMuted.withAlpha (0.25f));
    g.drawRoundedRectangle (panel, 14.0f, 1.2f);

    g.setColour (kText);
    g.setFont (juce::FontOptions (24.0f, juce::Font::bold));
    g.drawText ("Easy Chorus", 24, 16, 220, 30, juce::Justification::centredLeft);

    g.setColour (kMuted);
    g.setFont (juce::FontOptions (13.0f));
    g.drawText ("one page at a time  ·  bigger controls", 24, 44, 320, 18, juce::Justification::centredLeft);

    const float wobble = std::sin (lfoPhase * juce::MathConstants<float>::twoPi) * 6.0f;
    g.setColour (kAccent.withAlpha (0.85f));
    g.fillEllipse (280.0f + wobble, 28.0f, 7.0f, 7.0f);

    auto paintPanel = [&] (juce::Rectangle<float> r, const juce::String& title)
    {
        g.setColour (kSection);
        g.fillRoundedRectangle (r, 12.0f);
        g.setColour (kAccent.withAlpha (0.28f));
        g.drawRoundedRectangle (r, 12.0f, 1.2f);
        g.setColour (kAccent);
        g.setFont (juce::FontOptions (15.0f, juce::Font::bold));
        g.drawText (title,
                    juce::roundToInt (r.getX() + 14.0f),
                    juce::roundToInt (r.getY() + 10.0f),
                    juce::roundToInt (r.getWidth() - 28.0f),
                    22,
                    juce::Justification::centredLeft);
    };

    const char* titles[] = { "Chorus", "Delay", "Shimmer", "Delay 2", "Shimmer 2" };
    const int idx = (int) activePage;
    paintPanel (contentPanelBounds, titles[juce::jlimit (0, 4, idx)]);
    paintPanel (outputPanelBounds, "Output");
}

void EasyChorusAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (20, 14);

    auto brandRow = area.removeFromTop (52);
    demoToggle.setBounds (brandRow.removeFromRight (110).withSizeKeepingCentre (110, 48));
    brandRow.removeFromRight (10);
    layoutPresetBar (brandRow.removeFromRight (juce::jmin (340, brandRow.getWidth())));

    area.removeFromTop (8);
    layoutPageTabs (area.removeFromTop (54));
    area.removeFromTop (10);

    auto outputArea = area.removeFromBottom (190);
    area.removeFromBottom (10);

    contentPanelBounds = area.toFloat();
    outputPanelBounds = outputArea.toFloat();

    layoutActivePage (area.reduced (14, 36));
    layoutOutputBar (outputArea.reduced (14, 36));
}

void EasyChorusAudioProcessorEditor::layoutPresetBar (juce::Rectangle<int> bounds)
{
    const int navW = 48;
    const int gap = 8;
    const int h = juce::jmax (44, juce::jmin (50, bounds.getHeight()));

    presetPrevButton.setBounds (bounds.removeFromLeft (navW).withSizeKeepingCentre (navW, h));
    bounds.removeFromLeft (gap);
    presetNextButton.setBounds (bounds.removeFromRight (navW).withSizeKeepingCentre (navW, h));
    bounds.removeFromRight (gap);
    presetCombo.setBounds (bounds.withSizeKeepingCentre (bounds.getWidth(), h));
}

void EasyChorusAudioProcessorEditor::layoutPageTabs (juce::Rectangle<int> bounds)
{
    juce::TextButton* tabs[5] = {
        &pageChorusTab, &pageDelayTab, &pageShimmerTab, &pageDelay2Tab, &pageShimmer2Tab
    };
    const int gap = 8;
    const int cellW = (bounds.getWidth() - gap * 4) / 5;
    const int h = juce::jmax (kMinHit, bounds.getHeight());

    for (int i = 0; i < 5; ++i)
    {
        auto cell = juce::Rectangle<int> (bounds.getX() + i * (cellW + gap),
                                          bounds.getCentreY() - h / 2,
                                          cellW,
                                          h);
        tabs[i]->setBounds (cell);
    }
}

void EasyChorusAudioProcessorEditor::layoutToggleRow (juce::Rectangle<int> bounds,
                                                      juce::TextButton* const* toggles,
                                                      int count,
                                                      int gap)
{
    if (count <= 0)
        return;

    const int cellW = (bounds.getWidth() - gap * (count - 1)) / count;
    const int height = juce::jmax (kMinHit, juce::jmin (56, bounds.getHeight()));

    for (int i = 0; i < count; ++i)
    {
        auto cell = juce::Rectangle<int> (bounds.getX() + i * (cellW + gap),
                                          bounds.getCentreY() - height / 2,
                                          cellW,
                                          height);
        toggles[i]->setBounds (cell);
    }
}

void EasyChorusAudioProcessorEditor::layoutKnobRow (juce::Rectangle<int> bounds,
                                                    Knob* const* knobs,
                                                    int count)
{
    if (count <= 0)
        return;

    const int cellW = bounds.getWidth() / count;
    for (int i = 0; i < count; ++i)
    {
        auto cell = juce::Rectangle<int> (bounds.getX() + i * cellW,
                                          bounds.getY(),
                                          cellW,
                                          bounds.getHeight()).reduced (8, 4);
        layoutOneKnob (*knobs[i], cell);
    }
}

void EasyChorusAudioProcessorEditor::layoutOneKnob (Knob& knob, juce::Rectangle<int> cell)
{
    auto titleArea = cell.removeFromTop (24);
    auto valueArea = cell.removeFromBottom (22);
    auto dialArea  = cell;

    const int dialSize = juce::jmax (kBigDialMin, juce::jmin (dialArea.getWidth(), dialArea.getHeight()));
    auto dial = juce::Rectangle<int> (dialSize, dialSize).withCentre (dialArea.getCentre());

    knob.title.setBounds (titleArea);
    knob.dialHost.setBounds (dial);
    knob.steps.setBounds (knob.dialHost.getLocalBounds());

    const int inset = juce::jmax (14, dialSize / 8);
    knob.slider.setBounds (knob.dialHost.getLocalBounds().reduced (inset));
    knob.value.setBounds (valueArea);
}

void EasyChorusAudioProcessorEditor::layoutActivePage (juce::Rectangle<int> bounds)
{
    switch (activePage)
    {
        case Page::chorus:   layoutChorusPage (bounds); break;
        case Page::delay:    layoutDelayPage (bounds, false); break;
        case Page::shimmer:  layoutShimmerPage (bounds, false); break;
        case Page::delay2:   layoutDelayPage (bounds, true); break;
        case Page::shimmer2: layoutShimmerPage (bounds, true); break;
    }
}

void EasyChorusAudioProcessorEditor::layoutChorusPage (juce::Rectangle<int> bounds)
{
    auto onRow = bounds.removeFromTop (56);
    chorusToggle.setBounds (onRow.withSizeKeepingCentre (160, 52));
    bounds.removeFromTop (12);
    Knob* knobs[3] = { &rateKnob, &depthKnob, &chorusLevelKnob };
    layoutKnobRow (bounds, knobs, 3);
}

void EasyChorusAudioProcessorEditor::layoutDelayPage (juce::Rectangle<int> bounds, bool isSecond)
{
    auto& onToggle = isSecond ? delay2Toggle : delayToggle;
    auto& musical = isSecond ? musical2Toggle : musicalToggle;
    auto& stereo = isSecond ? stereo2Toggle : stereoToggle;
    auto& ping = isSecond ? pingPong2Toggle : pingPongToggle;
    auto& reverse = isSecond ? reverse2Toggle : reverseToggle;
    auto& timeKnob = isSecond ? delay2Knob : delayKnob;
    auto& fbKnob = isSecond ? feedback2Knob : feedbackKnob;
    auto& levelKnob = isSecond ? delay2LevelKnob : delayLevelKnob;

    auto onRow = bounds.removeFromTop (56);
    onToggle.setBounds (onRow.withSizeKeepingCentre (160, 52));
    bounds.removeFromTop (8);

    juce::TextButton* modes[3] = { &stereo, &ping, &reverse };
    layoutToggleRow (bounds.removeFromTop (52), modes, 3, 10);
    bounds.removeFromTop (8);

    if (! isSecond)
    {
        juce::TextButton* tempo[2] = { &syncToggle, &musical };
        layoutToggleRow (bounds.removeFromTop (52), tempo, 2, 10);
        bounds.removeFromTop (10);
        Knob* knobs[4] = { &timeKnob, &fbKnob, &levelKnob, &bpmKnob };
        layoutKnobRow (bounds, knobs, 4);
    }
    else
    {
        juce::TextButton* tempo[1] = { &musical };
        layoutToggleRow (bounds.removeFromTop (52), tempo, 1, 0);
        bounds.removeFromTop (10);
        Knob* knobs[3] = { &timeKnob, &fbKnob, &levelKnob };
        layoutKnobRow (bounds, knobs, 3);
    }
}

void EasyChorusAudioProcessorEditor::layoutShimmerPage (juce::Rectangle<int> bounds, bool isSecond)
{
    auto& onToggle = isSecond ? shimmer2Toggle : shimmerToggle;
    auto& ethereal = isSecond ? ethereal2Toggle : etherealToggle;
    auto& amount = isSecond ? shimmer2Knob : shimmerKnob;
    auto& pitch = isSecond ? pitch2Knob : pitchKnob;
    auto& decay = isSecond ? decay2Knob : decayKnob;
    auto& level = isSecond ? shimmer2LevelKnob : shimmerLevelKnob;

    auto onRow = bounds.removeFromTop (56);
    onToggle.setBounds (onRow.withSizeKeepingCentre (160, 52));
    bounds.removeFromTop (8);

    juce::TextButton* modes[1] = { &ethereal };
    layoutToggleRow (bounds.removeFromTop (52), modes, 1, 0);
    bounds.removeFromTop (10);

    Knob* knobs[4] = { &amount, &pitch, &decay, &level };
    layoutKnobRow (bounds, knobs, 4);
}

void EasyChorusAudioProcessorEditor::layoutOutputBar (juce::Rectangle<int> bounds)
{
    auto wavesRow = bounds.removeFromTop (56);
    wavesToggle.setBounds (wavesRow.withSizeKeepingCentre (200, 52));
    bounds.removeFromTop (8);
    Knob* knobs[2] = { &mixKnob, &masterKnob };
    layoutKnobRow (bounds, knobs, 2);
}
