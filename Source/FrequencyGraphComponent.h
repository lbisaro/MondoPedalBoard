#pragma once
#include <JuceHeader.h>
#include "AnalysisEngine.h"
#include "CustomLookAndFeel.h"

class FrequencyGraphComponent : public juce::Component
{
public:
    FrequencyGraphComponent();
    ~FrequencyGraphComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent& e) override;

    // Métodos para inyectar datos desde el exterior
    void setFFTData (const std::array<float, AnalysisEngine::numBins>& newData);
    void setSampleRate (double newSampleRate);
    void setCurveColourId (int customLookAndFeelColourId);

private:
    std::array<float, AnalysisEngine::numBins> fftData;
    double sampleRate = 44100.0;
    int curveColourId = CustomLookAndFeel::data1ColourId;

    juce::Point<int> mousePos;
    bool isMouseOverPlot = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrequencyGraphComponent)
};
