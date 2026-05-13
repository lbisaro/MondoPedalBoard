#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "CustomLookAndFeel.h"

class GuitarDIRecorderViewComponent : public juce::Component, private juce::Timer
{
public:
    GuitarDIRecorderViewComponent (MondoHelixAnalyzerAudioProcessor&);
    ~GuitarDIRecorderViewComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    
    bool keyPressed (const juce::KeyPress& key) override;

    std::function<void()> onFinished;
    void resetState();

private:
    void timerCallback() override;
    void updateStateUI();

    MondoHelixAnalyzerAudioProcessor& audioProcessor;

    enum class UIState {
        Idle,
        Countdown,
        Recording,
        Review
    };

    UIState currentState = UIState::Idle;
    int countdownTicks = 5;
    float currentMeterLevel = 0.0f;

    juce::Label namePromptLabel;
    juce::TextEditor nameInput;

    juce::TextButton actionButton;
    juce::TextButton saveButton;
    juce::TextButton retryButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GuitarDIRecorderViewComponent)
};
