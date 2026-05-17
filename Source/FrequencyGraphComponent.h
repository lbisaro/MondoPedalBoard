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
    void setSmoothingDenominator (int denominator);
    
    void resetPeakCurve();

    void setReferenceCurve (const std::vector<float>& refCurve);
    void clearReferenceCurve();

    struct TargetRange { float minFreq, maxFreq; juce::String name; juce::Colour color; };
    void setTargetRanges (const std::vector<TargetRange>& ranges);

private:
    void calculateFractionalOctaveSmoothing();
    std::array<float, AnalysisEngine::numBins> fftData;
    std::array<float, AnalysisEngine::numBins> smoothedData;
    std::array<float, AnalysisEngine::numBins> representativeCurve;
    
    bool hasReferenceCurve = false;
    std::vector<float> referenceCurve;

    double sampleRate = 44100.0;
    int curveColourId = CustomLookAndFeel::data1ColourId;
    int smoothingDenominator = 6;

    juce::Point<int> mousePos;
    bool isMouseOverPlot = false;
    
    std::vector<TargetRange> targetRanges;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrequencyGraphComponent)
};
