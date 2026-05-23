#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "FrequencyGraphComponent.h"
#include "RangeIndicator.h"
#include "CustomLookAndFeel.h"

class PresetAnalyzerViewComponent : public juce::Component, private juce::Timer
{
public:
    PresetAnalyzerViewComponent (MondoHelixAnalyzerAudioProcessor&);
    ~PresetAnalyzerViewComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress& key) override;

    void refreshDIList();
    void refreshTargetList();

private:
    void timerCallback() override;
    void updateFrequencyGraphBands();
    void updateStaticTargetReferences();

    TargetType activeTargetProfile = TargetType::Rhythm;

    MondoHelixAnalyzerAudioProcessor& audioProcessor;

    juce::ComboBox diComboBox;
    juce::ComboBox targetComboBox;
    juce::ShapeButton playButton;
    juce::ShapeButton stopButton;
    juce::ShapeButton resetButton;
    juce::Array<juce::File> diFiles;
    juce::Array<juce::File> customTargetFiles;
    int liveGuitarId = 1;

    RangeIndicator lufsGauge;
    RangeIndicator plrGauge;
    RangeIndicator brightnessGauge;
    RangeIndicator brilloRatioGauge;
    RangeIndicator bodyGauge;
    RangeIndicator cutGauge;

    juce::Label routingStatusLabel;
    
    FrequencyGraphComponent frequencyGraph;
    juce::Rectangle<int> progressBarBounds;
    
    juce::Rectangle<int> card1, card2, card3, card4, card5, card6, card7;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetAnalyzerViewComponent)
};
