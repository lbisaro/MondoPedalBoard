#include "PluginProcessor.h"
#include "PluginEditor.h"

MondoHelixAnalyzerAudioProcessorEditor::MondoHelixAnalyzerAudioProcessorEditor (MondoHelixAnalyzerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&customLookAndFeel);

    setSize (600, 400);
    setResizable (true, true);
    setResizeLimits (600, 400, 3840, 2160); // Permite escalar libremente hasta 4K

    addAndMakeVisible (lufsLabel);
    lufsLabel.setText ("LUFS: --", juce::dontSendNotification);
    lufsLabel.setColour (juce::Label::textColourId, customLookAndFeel.findColour(CustomLookAndFeel::data1ColourId));
    
    addAndMakeVisible (plrLabel);
    plrLabel.setText ("PLR: --", juce::dontSendNotification);
    plrLabel.setColour (juce::Label::textColourId, customLookAndFeel.findColour(CustomLookAndFeel::data1ColourId));

    addAndMakeVisible (categoryLabel);
    categoryLabel.setText ("Type: Warm", juce::dontSendNotification);
    categoryLabel.setFont (juce::Font (20.0f, juce::Font::bold));
    categoryLabel.setJustificationType (juce::Justification::centredRight);
    categoryLabel.setColour (juce::Label::textColourId, customLookAndFeel.findColour(CustomLookAndFeel::data1ColourId));

    addAndMakeVisible (settingsButton);
    settingsButton.setButtonText ("Preferences");
    
    // Conectado al LookAndFeel, por lo que usará los colores del Theme
    // settingsButton.setColour (...);
    
    settingsButton.onClick = [this] {
        settingsPanel = std::make_unique<SettingsComponent>(audioProcessor.settings);
        addAndMakeVisible(settingsPanel.get());
        settingsPanel->setBounds(getLocalBounds());
        settingsPanel->onClose = [this] {
            settingsPanel.reset();
        };
    };

    addAndMakeVisible (frequencyGraph);

    startTimerHz (30);
}

MondoHelixAnalyzerAudioProcessorEditor::~MondoHelixAnalyzerAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void MondoHelixAnalyzerAudioProcessorEditor::paint (juce::Graphics& g)
{
    juce::Colour windowBgColor = getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId);
    juce::Colour textColor = juce::Colours::white;

    if (auto* lf = dynamic_cast<juce::LookAndFeel_V4*>(&getLookAndFeel()))
    {
        windowBgColor = lf->getCurrentColourScheme().getUIColour (juce::LookAndFeel_V4::ColourScheme::windowBackground);
        textColor = lf->getCurrentColourScheme().getUIColour (juce::LookAndFeel_V4::ColourScheme::defaultText);
    }

    g.fillAll (windowBgColor);

    g.setColour (textColor);
    g.setFont (24.0f);
    g.drawFittedText ("Mondo Helix Analyzer", getLocalBounds().removeFromTop(40), juce::Justification::centred, 1);
}

void MondoHelixAnalyzerAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (20);
    
    auto header = bounds.removeFromTop(40);
    
    // Título al centro, Preferences a la izquierda
    settingsButton.setBounds (header.removeFromLeft(120).reduced(0, 5));

    auto infoBounds = bounds.removeFromTop (40);
    lufsLabel.setBounds (infoBounds.removeFromLeft (120));
    plrLabel.setBounds (infoBounds.removeFromLeft (120));
    categoryLabel.setBounds (infoBounds);

    frequencyGraph.setBounds (bounds);

    if (settingsPanel != nullptr)
        settingsPanel->setBounds(getLocalBounds());
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
        frequencyGraph.setSampleRate (audioProcessor.getSampleRate());
        frequencyGraph.setFFTData (analyzer.latestFFTData);
    }
}


