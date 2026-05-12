#include "PluginProcessor.h"
#include "PluginEditor.h"

MondoHelixAnalyzerAudioProcessorEditor::MondoHelixAnalyzerAudioProcessorEditor (MondoHelixAnalyzerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (600, 400);

    addAndMakeVisible (lufsLabel);
    lufsLabel.setText ("LUFS: --", juce::dontSendNotification);
    
    addAndMakeVisible (plrLabel);
    plrLabel.setText ("PLR: --", juce::dontSendNotification);

    addAndMakeVisible (categoryLabel);
    categoryLabel.setText ("Type: Warm", juce::dontSendNotification);
    categoryLabel.setFont (juce::Font (20.0f, juce::Font::bold));
    categoryLabel.setJustificationType (juce::Justification::centredRight);

    std::fill (fftDataUI.begin(), fftDataUI.end(), 0.0f);

    startTimerHz (30);
}

MondoHelixAnalyzerAudioProcessorEditor::~MondoHelixAnalyzerAudioProcessorEditor()
{
}

void MondoHelixAnalyzerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (24.0f);
    g.drawFittedText ("Mondo Helix Analyzer", getLocalBounds().removeFromTop(40), juce::Justification::centred, 1);

    // Dibujar la FFT
    auto bounds = getLocalBounds().reduced (20);
    bounds.removeFromTop (60);
    
    g.setColour (juce::Colours::darkgrey.darker());
    g.fillRect (bounds);
    
    g.setColour (juce::Colours::cyan);
    
    juce::Path fftPath;
    float width = static_cast<float> (bounds.getWidth());
    float height = static_cast<float> (bounds.getHeight());
    float bottom = static_cast<float> (bounds.getBottom());
    float left = static_cast<float> (bounds.getX());

    int numBins = AnalysisEngine::numBins;
    
    for (int i = 0; i < numBins; ++i)
    {
        float magnitude = fftDataUI[i];
        float level = juce::jmap (juce::Decibels::gainToDecibels(magnitude, -100.0f), -100.0f, 0.0f, 0.0f, 1.0f);
        level = juce::jlimit(0.0f, 1.0f, level);
        
        // Mapeo logarítmico en frecuencia
        float x = left + width * (std::log10(1.0f + static_cast<float>(i)) / std::log10(1.0f + static_cast<float>(numBins)));
        float y = bottom - height * level;

        if (i == 0)
            fftPath.startNewSubPath (x, y);
        else
            fftPath.lineTo (x, y);
    }

    g.strokePath (fftPath, juce::PathStrokeType (2.0f));
}

void MondoHelixAnalyzerAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (20);
    bounds.removeFromTop (20); 

    auto infoBounds = bounds.removeFromTop (40);
    lufsLabel.setBounds (infoBounds.removeFromLeft (120));
    plrLabel.setBounds (infoBounds.removeFromLeft (120));
    categoryLabel.setBounds (infoBounds);
}

void MondoHelixAnalyzerAudioProcessorEditor::timerCallback()
{
    auto& analyzer = audioProcessor.analyzer;

    float momentary = analyzer.momentaryLUFS.load();
    float plr = analyzer.currentPLR.load();
    int cat = analyzer.currentCategory.load();

    lufsLabel.setText (juce::String::formatted ("LUFS M: %.1f", momentary), juce::dontSendNotification);
    plrLabel.setText (juce::String::formatted ("PLR: %.1f", plr), juce::dontSendNotification);

    juce::String catName = "Warm";
    if (cat == 1) catName = "Rhythm";
    else if (cat == 2) catName = "Lead";
    
    categoryLabel.setText ("Type: " + catName, juce::dontSendNotification);

    if (analyzer.fftDataReady.exchange (false))
    {
        std::copy (analyzer.latestFFTData.begin(), analyzer.latestFFTData.end(), fftDataUI.begin());
        repaint();
    }
}
