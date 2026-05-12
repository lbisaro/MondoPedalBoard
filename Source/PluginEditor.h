#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class MondoHelixAnalyzerAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    MondoHelixAnalyzerAudioProcessorEditor (MondoHelixAnalyzerAudioProcessor&);
    ~MondoHelixAnalyzerAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    MondoHelixAnalyzerAudioProcessor& audioProcessor;

    juce::Label lufsLabel;
    juce::Label plrLabel;
    juce::Label categoryLabel;

    std::array<float, AnalysisEngine::numBins> fftDataUI;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MondoHelixAnalyzerAudioProcessorEditor)
};
