#include "PresetAnalyzerViewComponent.h"

PresetAnalyzerViewComponent::PresetAnalyzerViewComponent (MondoHelixAnalyzerAudioProcessor& p)
    : audioProcessor (p)
{
    addAndMakeVisible (lufsLabel);
    lufsLabel.setText ("LUFS: --", juce::dontSendNotification);
    
    addAndMakeVisible (plrLabel);
    plrLabel.setText ("PLR: --", juce::dontSendNotification);

    addAndMakeVisible (categoryLabel);
    categoryLabel.setText ("Type: Warm", juce::dontSendNotification);
    categoryLabel.setFont (juce::Font (20.0f, juce::Font::bold));
    categoryLabel.setJustificationType (juce::Justification::centredRight);

    addAndMakeVisible (frequencyGraph);

    startTimerHz (30);
}

PresetAnalyzerViewComponent::~PresetAnalyzerViewComponent() = default;

void PresetAnalyzerViewComponent::paint (juce::Graphics& g)
{
    // Aplicar colores del CustomLookAndFeel a los labels si está disponible
    if (auto* lf = dynamic_cast<juce::LookAndFeel_V4*>(&getLookAndFeel()))
    {
        juce::Colour dataColor = lf->findColour (CustomLookAndFeel::data1ColourId);
        lufsLabel.setColour (juce::Label::textColourId, dataColor);
        plrLabel.setColour (juce::Label::textColourId, dataColor);
        categoryLabel.setColour (juce::Label::textColourId, dataColor);
    }
    
    g.fillAll (juce::Colours::transparentBlack);
}

void PresetAnalyzerViewComponent::resized()
{
    auto bounds = getLocalBounds().reduced (10);

    auto infoBounds = bounds.removeFromTop (40);
    lufsLabel.setBounds (infoBounds.removeFromLeft (120));
    plrLabel.setBounds (infoBounds.removeFromLeft (120));
    categoryLabel.setBounds (infoBounds);

    frequencyGraph.setBounds (bounds);
}

void PresetAnalyzerViewComponent::timerCallback()
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
        frequencyGraph.setSampleRate (audioProcessor.getSampleRate());
        frequencyGraph.setFFTData (analyzer.latestFFTData);
    }
}
