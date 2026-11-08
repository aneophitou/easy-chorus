#pragma once

#include <JuceHeader.h>
#include <vector>

/**
 * Clickable tick marks around a rotary slider.
 * Tap a mark to jump the dial to that step value.
 */
class DialStepOverlay : public juce::Component
{
public:
    DialStepOverlay()
    {
        setInterceptsMouseClicks (true, false);
    }

    void setTargetSlider (juce::Slider* sliderToDrive) noexcept { slider = sliderToDrive; }

    void setSteps (std::vector<float> newSteps)
    {
        steps = std::move (newSteps);
        repaint();
    }

    void setRotarySpan (float startRadians, float endRadians) noexcept
    {
        startAngle = startRadians;
        endAngle = endRadians;
        repaint();
    }

    void setColours (juce::Colour tick, juce::Colour activeTick) noexcept
    {
        tickColour = tick;
        activeColour = activeTick;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        if (slider == nullptr || steps.empty())
            return;

        const auto centre = getLocalBounds().getCentre().toFloat();
        const float radius = (float) juce::jmin (getWidth(), getHeight()) * 0.42f;

        const float minV = (float) slider->getMinimum();
        const float maxV = (float) slider->getMaximum();
        const float span = juce::jmax (1.0e-6f, maxV - minV);
        const float current = (float) slider->getValue();

        for (float step : steps)
        {
            const float t = juce::jlimit (0.0f, 1.0f, (step - minV) / span);
            const float angle = startAngle + t * (endAngle - startAngle);
            // JUCE rotary angles: clockwise from 12 o'clock
            const float x = centre.x + radius * std::sin (angle);
            const float y = centre.y - radius * std::cos (angle);

            const bool active = std::abs (current - step) <= span * 0.02f + 1.0e-3f;
            g.setColour (active ? activeColour : tickColour);
            const float size = active ? 9.0f : 7.0f;
            g.fillEllipse (x - size * 0.5f, y - size * 0.5f, size, size);

            // Outer ring for touch affordance
            g.setColour (tickColour.withAlpha (0.35f));
            g.drawEllipse (x - 11.0f, y - 11.0f, 22.0f, 22.0f, 1.0f);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (slider == nullptr || ! slider->isEnabled() || steps.empty())
            return;

        const int hit = hitTestStep (e.getPosition());
        if (hit >= 0)
        {
            slider->setValue ((double) steps[(size_t) hit], juce::sendNotificationSync);
            repaint();
        }
    }

    void mouseMove (const juce::MouseEvent& e) override
    {
        setMouseCursor (hitTestStep (e.getPosition()) >= 0 ? juce::MouseCursor::PointingHandCursor
                                                           : juce::MouseCursor::NormalCursor);
    }

    /** Only capture clicks on step dots so the dial underneath stays draggable. */
    bool hitTest (int x, int y) override
    {
        return hitTestStep ({ x, y }) >= 0;
    }

private:
    int hitTestStep (juce::Point<int> pos) const
    {
        if (slider == nullptr)
            return -1;

        const auto centre = getLocalBounds().getCentre().toFloat();
        const float radius = (float) juce::jmin (getWidth(), getHeight()) * 0.42f;
        const float minV = (float) slider->getMinimum();
        const float maxV = (float) slider->getMaximum();
        const float span = juce::jmax (1.0e-6f, maxV - minV);

        int best = -1;
        float bestDist = 18.0f; // touch-friendly hit radius (px)

        for (int i = 0; i < (int) steps.size(); ++i)
        {
            const float t = juce::jlimit (0.0f, 1.0f, (steps[(size_t) i] - minV) / span);
            const float angle = startAngle + t * (endAngle - startAngle);
            const float x = centre.x + radius * std::sin (angle);
            const float y = centre.y - radius * std::cos (angle);
            const float d = pos.toFloat().getDistanceFrom ({ x, y });
            if (d < bestDist)
            {
                bestDist = d;
                best = i;
            }
        }

        return best;
    }

    juce::Slider* slider = nullptr;
    std::vector<float> steps;
    float startAngle = juce::degreesToRadians (225.0f);
    float endAngle   = juce::degreesToRadians (495.0f);
    juce::Colour tickColour { 0xff9a9590 };
    juce::Colour activeColour { 0xffc4a574 };
};
