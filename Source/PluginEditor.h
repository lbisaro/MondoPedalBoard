#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "SettingsComponent.h"
#include "CustomLookAndFeel.h"
#include "HomeViewComponent.h"
#include "GuitarDIListViewComponent.h"
#include "GuitarDIRecorderViewComponent.h"
#include "PresetAnalyzerViewComponent.h"
#include "SamplesAnalyzerViewComponent.h"
#include "BlockAnalyzerViewComponent.h"

class MondoHelixAnalyzerAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    MondoHelixAnalyzerAudioProcessorEditor (MondoHelixAnalyzerAudioProcessor&);
    ~MondoHelixAnalyzerAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void parentHierarchyChanged() override;


private:
    MondoHelixAnalyzerAudioProcessor& audioProcessor;

    juce::ShapeButton settingsButton;
    std::unique_ptr<SettingsComponent> settingsPanel;

    // Vistas / Módulos de la Suite
    std::unique_ptr<HomeViewComponent> homeView;
    std::unique_ptr<GuitarDIListViewComponent> diListView;
    std::unique_ptr<GuitarDIRecorderViewComponent> diRecorderView;
    std::unique_ptr<PresetAnalyzerViewComponent> analyzerView;
    std::unique_ptr<SamplesAnalyzerViewComponent> samplesAnalyzerView;
    std::unique_ptr<BlockAnalyzerViewComponent> blockAnalyzerView;

    juce::Component* currentView = nullptr;
    juce::String currentModuleTitle;
    juce::ShapeButton homeButton;
    juce::Label moduleTitleLabel;

    void showView (juce::Component* newView, const juce::String& moduleTitle = "");

    CustomLookAndFeel customLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MondoHelixAnalyzerAudioProcessorEditor)
};
