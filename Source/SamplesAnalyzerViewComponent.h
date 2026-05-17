#pragma once
#include <JuceHeader.h>
#include "AppSettings.h"
#include "SamplesOfflineAnalyzer.h"

class SamplesAnalyzerViewComponent : public juce::Component, private juce::Thread
{
public:
    SamplesAnalyzerViewComponent(AppSettings&);
    ~SamplesAnalyzerViewComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void refreshFilesList();

private:
    void run() override; // Background thread entry point
    void importCustomSample();
    void runAnalysis();
    void loadExistingAnalysis();

    AppSettings& settings;

    juce::ComboBox sampleFilesComboBox;
    juce::ComboBox targetProfileComboBox;
    juce::TextButton browseButton;
    juce::TextButton refreshButton;
    juce::TextButton analyzeButton;

    juce::Label statusLabel;

    std::atomic<bool> isAnalyzing { false };
    juce::String statusText;

    SampleAnalysisResult currentResult;
    juce::File fileToAnalyze;
    juce::String categoryToAssign;

    juce::Rectangle<int> card1, card2, card3, card4, card5, card6, card7;

    juce::Array<juce::File> availableAudioFiles;

    std::shared_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplesAnalyzerViewComponent)
};
