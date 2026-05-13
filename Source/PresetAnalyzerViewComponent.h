#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "FrequencyGraphComponent.h"
#include "CustomLookAndFeel.h"

class PresetAnalyzerViewComponent : public juce::Component, private juce::Timer
{
public:
    PresetAnalyzerViewComponent (MondoHelixAnalyzerAudioProcessor&);
    ~PresetAnalyzerViewComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    MondoHelixAnalyzerAudioProcessor& audioProcessor;

    juce::Label lufsLabel;
    juce::Label plrLabel;
    juce::Label categoryLabel;
    
    FrequencyGraphComponent frequencyGraph;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetAnalyzerViewComponent)
};
