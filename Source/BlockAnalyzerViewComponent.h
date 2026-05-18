#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "CustomLookAndFeel.h"

// =============================================================================
// EQ Transfer Function Graph — estilo Preset Analyzer
// Muestra la curva de magnitud en dB obtenida por ESS, con escala -16 a +16 dB
// =============================================================================
class EQTransferFunctionGraph : public juce::Component
{
public:
    explicit EQTransferFunctionGraph (BlockAnalyzer& analyzer);
    ~EQTransferFunctionGraph() override = default;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseExit  (const juce::MouseEvent& e) override;

    void updateCurve();    // Llamado desde el timer de la vista

private:
    BlockAnalyzer& blockAnalyzer;

    std::array<float, BlockAnalyzer::numBins> magnitudeData {};
    std::array<float, BlockAnalyzer::numBins> smoothedData  {};

    // Escala fija: -16 dB a +16 dB (pedida por el usuario)
    static constexpr float kMinDb = -16.0f;
    static constexpr float kMaxDb =  16.0f;

    juce::Point<int> mousePos;
    bool isMouseOver = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQTransferFunctionGraph)
};

// =============================================================================
// Vista Principal del Módulo Block Analyzer
// =============================================================================
class BlockAnalyzerViewComponent : public juce::Component, private juce::Timer
{
public:
    explicit BlockAnalyzerViewComponent (MondoHelixAnalyzerAudioProcessor& processor);
    ~BlockAnalyzerViewComponent() override;

    void paint   (juce::Graphics& g) override;
    void resized () override;
    void parentHierarchyChanged() override;

private:
    void timerCallback() override;

    // Arranca/para el sweep via el procesador
    void startSweep();
    void stopSweep();

    MondoHelixAnalyzerAudioProcessor& audioProcessor;

    // Unique component -- single EQ magnitude graph
    EQTransferFunctionGraph eqGraph;

    // Header row 1: title (left) + cycle counter (right)
    juce::Label titleLabel;
    juce::Label statusLabel;    // cycle count, right-aligned

    // Header row 2: routing info (full width)
    juce::Label routingLabel;

    int measurementCycle = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BlockAnalyzerViewComponent)
};
