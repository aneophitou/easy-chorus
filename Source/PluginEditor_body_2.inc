
void EasyChorusAudioProcessorEditor::paintSectionPanel (juce::Graphics& g,
                                                        juce::Rectangle<float> bounds,
                                                        const juce::String& title) const
{
    g.setColour (kSection);
    g.fillRoundedRectangle (bounds, 12.0f);
    g.setColour (kAccent.withAlpha (0.28f));
    g.drawRoundedRectangle (bounds, 12.0f, 1.2f);

    auto titleArea = bounds.removeFromTop ((float) kSectionTitleH + 8.0f);
    g.setColour (kAccent.withAlpha (0.12f));
    g.fillRoundedRectangle (titleArea.reduced (1.0f, 1.0f), 11.0f);

    g.setColour (kAccent);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText (title,
                juce::roundToInt (titleArea.getX() + 12.0f),
                juce::roundToInt (titleArea.getY() + 4.0f),
                juce::roundToInt (titleArea.getWidth() - 24.0f),
                kSectionTitleH,
                juce::Justification::centredLeft);
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
    g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    g.drawText ("Easy Chorus", 24, 16, 200, 28, juce::Justification::centredLeft);

    g.setColour (kMuted);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("chorus  ·  delay  ·  shimmer", 24, 40, 220, 16, juce::Justification::centredLeft);

    const float wobble = std::sin (lfoPhase * juce::MathConstants<float>::twoPi) * 6.0f;
    g.setColour (kAccent.withAlpha (0.85f));
    g.fillEllipse (220.0f + wobble, 26.0f, 6.0f, 6.0f);

    paintSectionPanel (g, chorusPanelBounds,  "Chorus");
    paintSectionPanel (g, delayPanelBounds,   "Delay");
    paintSectionPanel (g, shimmerPanelBounds, "Shimmer");
    paintSectionPanel (g, outputPanelBounds,  "Output");
}

void EasyChorusAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (22, 14);

    auto brandRow = area.removeFromTop (48);
    demoToggle.setBounds (brandRow.removeFromRight (110).withSizeKeepingCentre (110, juce::jmax (kMinHit, 44)));
    brandRow.removeFromRight (8);
    layoutPresetBar (brandRow.removeFromRight (juce::jmin (300, brandRow.getWidth())));

    area.removeFromTop (8);

    auto outputArea = area.removeFromBottom (150);
    area.removeFromBottom (8);

    const int gap = 10;
    const int colW = (area.getWidth() - gap * 2) / 3;

    auto chorusArea  = area.removeFromLeft (colW);
    area.removeFromLeft (gap);
    auto delayArea   = area.removeFromLeft (colW);
    area.removeFromLeft (gap);
    auto shimmerArea = area;

    chorusPanelBounds  = chorusArea.toFloat();
    delayPanelBounds   = delayArea.toFloat();
    shimmerPanelBounds = shimmerArea.toFloat();
    outputPanelBounds  = outputArea.toFloat();

    layoutChorusColumn (chorusArea.reduced (kPanelPad, kPanelPad));
    layoutDelayColumn (delayArea.reduced (kPanelPad, kPanelPad));
    layoutShimmerColumn (shimmerArea.reduced (kPanelPad, kPanelPad));
    layoutOutputRow (outputArea.reduced (kPanelPad, kPanelPad));
}

void EasyChorusAudioProcessorEditor::layoutPresetBar (juce::Rectangle<int> bounds)
{
    const int navW = 40;
    const int gap = 6;
    const int h = juce::jmax (40, juce::jmin (44, bounds.getHeight()));

    presetPrevButton.setBounds (bounds.removeFromLeft (navW).withSizeKeepingCentre (navW, h));
    bounds.removeFromLeft (gap);
    presetNextButton.setBounds (bounds.removeFromRight (navW).withSizeKeepingCentre (navW, h));
    bounds.removeFromRight (gap);
    presetCombo.setBounds (bounds.withSizeKeepingCentre (bounds.getWidth(), h));
}

void EasyChorusAudioProcessorEditor::layoutToggleRow (juce::Rectangle<int> bounds,
                                                      juce::TextButton* const* toggles,
                                                      int count,
                                                      int gap)
{
    if (count <= 0)
        return;

    const int cellW = (bounds.getWidth() - gap * (count - 1)) / count;
    const int height = juce::jmax (kMinHit - 4, juce::jmin (40, bounds.getHeight()));

    for (int i = 0; i < count; ++i)
    {
        auto cell = juce::Rectangle<int> (bounds.getX() + i * (cellW + gap),
                                          bounds.getCentreY() - height / 2,
                                          cellW,
                                          height);
        toggles[i]->setBounds (cell);
    }
}

void EasyChorusAudioProcessorEditor::layoutKnobStack (juce::Rectangle<int> bounds,
                                                      Knob* const* knobs,
                                                      int count)
{
    if (count <= 0)
        return;

    const int rowH = bounds.getHeight() / count;
    for (int i = 0; i < count; ++i)
    {
        auto cell = juce::Rectangle<int> (bounds.getX(),
                                          bounds.getY() + i * rowH,
                                          bounds.getWidth(),
                                          rowH).reduced (2, 2);
        layoutOneKnob (*knobs[i], cell);
    }
}

void EasyChorusAudioProcessorEditor::layoutChorusColumn (juce::Rectangle<int> bounds)
{
    bounds.removeFromTop (kSectionTitleH + 4);

    auto onRow = bounds.removeFromTop (40);
    chorusToggle.setBounds (onRow.withSizeKeepingCentre (juce::jmin (120, onRow.getWidth()), 36));
    bounds.removeFromTop (6);

    Knob* knobs[3] = { &rateKnob, &depthKnob, &chorusLevelKnob };
    layoutKnobStack (bounds, knobs, 3);
}

void EasyChorusAudioProcessorEditor::layoutDelayColumn (juce::Rectangle<int> bounds)
{
    bounds.removeFromTop (kSectionTitleH + 4);

    auto onRow = bounds.removeFromTop (34);
    delayToggle.setBounds (onRow.withSizeKeepingCentre (juce::jmin (100, onRow.getWidth()), 32));
    bounds.removeFromTop (4);

    juce::TextButton* modes[3] = { &stereoToggle, &pingPongToggle, &reverseToggle };
    layoutToggleRow (bounds.removeFromTop (32), modes, 3, 6);
    bounds.removeFromTop (6);

    // Time / Feedback / Level stacked; tempo controls share one compact row under them
    const int tempoRowH = 88;
    auto stack = bounds;
    auto tempoRow = stack.removeFromBottom (tempoRowH);
    stack.removeFromBottom (4);

    Knob* knobs[3] = { &delayKnob, &feedbackKnob, &delayLevelKnob };
    layoutKnobStack (stack, knobs, 3);

    auto toggleStrip = tempoRow.removeFromLeft (tempoRow.getWidth() / 2);
    juce::TextButton* tempo[2] = { &syncToggle, &musicalToggle };
    layoutToggleRow (toggleStrip.removeFromTop (36).reduced (0, 2), tempo, 2, 6);
    // keep lower half of toggle strip empty for breathing room
    layoutOneKnob (bpmKnob, tempoRow.reduced (2, 0));
}

void EasyChorusAudioProcessorEditor::layoutShimmerColumn (juce::Rectangle<int> bounds)
{
    bounds.removeFromTop (kSectionTitleH + 4);

    auto onRow = bounds.removeFromTop (36);
    shimmerToggle.setBounds (onRow.withSizeKeepingCentre (juce::jmin (100, onRow.getWidth()), 34));
    bounds.removeFromTop (4);

    juce::TextButton* modes[1] = { &etherealToggle };
    layoutToggleRow (bounds.removeFromTop (34), modes, 1, 0);
    bounds.removeFromTop (6);

    Knob* knobs[4] = { &shimmerKnob, &pitchKnob, &decayKnob, &shimmerLevelKnob };
    layoutKnobStack (bounds, knobs, 4);
}

void EasyChorusAudioProcessorEditor::layoutOutputRow (juce::Rectangle<int> bounds)
{
    bounds.removeFromTop (kSectionTitleH + 4);

    auto wavesRow = bounds.removeFromTop (40);
    wavesToggle.setBounds (wavesRow.withSizeKeepingCentre (juce::jmin (160, wavesRow.getWidth()), 36));
    bounds.removeFromTop (4);

    const int gap = 24;
    const int cellW = (bounds.getWidth() - gap) / 2;
    layoutOneKnob (mixKnob, juce::Rectangle<int> (bounds.getX(), bounds.getY(), cellW, bounds.getHeight()).reduced (40, 2));
    layoutOneKnob (masterKnob, juce::Rectangle<int> (bounds.getX() + cellW + gap, bounds.getY(), cellW, bounds.getHeight()).reduced (40, 2));
}

void EasyChorusAudioProcessorEditor::layoutOneKnob (Knob& knob, juce::Rectangle<int> cell)
{
    auto titleArea = cell.removeFromTop (18);
    auto valueArea = cell.removeFromBottom (16);
    auto dialArea  = cell;

    const int dialSize = juce::jmax (kMinHit + 8, juce::jmin (dialArea.getWidth(), dialArea.getHeight()));
    auto dial = juce::Rectangle<int> (dialSize, dialSize).withCentre (dialArea.getCentre());

    knob.title.setBounds (titleArea);
    knob.dialHost.setBounds (dial);
    knob.steps.setBounds (knob.dialHost.getLocalBounds());

    const int inset = juce::jmax (12, dialSize / 8);
    knob.slider.setBounds (knob.dialHost.getLocalBounds().reduced (inset));
    knob.value.setBounds (valueArea);
}
