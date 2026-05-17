#include "SettingsComponent.h"

SettingsComponent::SettingsComponent(AppSettings& s) : settings(s)
{

    setupComboBox(procInCombo, procInLabel, "Processed Input (Helix -> App):", settings.processedInputChannel.load(), true);
    setupComboBox(playOutCombo, playOutLabel, "Playback Output (App -> Helix):", settings.playbackOutputChannel.load(), true);
    setupComboBox(diInCombo, diInLabel, "DI Input (Helix -> App):", settings.diInputChannel.load(), false);

    procInCombo.onChange = [this] { settings.processedInputChannel.store(procInCombo.getSelectedId()); settings.saveSettings(); };
    playOutCombo.onChange = [this] { settings.playbackOutputChannel.store(playOutCombo.getSelectedId()); settings.saveSettings(); };
    diInCombo.onChange = [this] { settings.diInputChannel.store(diInCombo.getSelectedId()); settings.saveSettings(); };

    addAndMakeVisible(smoothingLabel);
    smoothingLabel.setText("FFT Smoothing (Octave):", juce::dontSendNotification);
    addAndMakeVisible(smoothingCombo);
    smoothingCombo.addItem("1/3 Octave", 3);
    smoothingCombo.addItem("1/6 Octave", 6);
    smoothingCombo.addItem("1/12 Octave", 12);
    smoothingCombo.setSelectedId(settings.fftSmoothingDenominator.load(), juce::dontSendNotification);
    smoothingCombo.onChange = [this] { 
        settings.fftSmoothingDenominator.store(smoothingCombo.getSelectedId()); 
        settings.saveSettings(); 
    };

    addAndMakeVisible(folderLabel);
    folderLabel.setText("Data Folder:", juce::dontSendNotification);
    
    addAndMakeVisible(folderPathLabel);
    folderPathLabel.setText(settings.dataFolderPath, juce::dontSendNotification);
    folderPathLabel.setColour(juce::Label::backgroundColourId, juce::Colours::black.withAlpha(0.3f));

    addAndMakeVisible(browseButton);
    browseButton.setButtonText("Browse...");
    browseButton.onClick = [this] {
        fileChooser = std::make_unique<juce::FileChooser>("Select Data Folder", juce::File(settings.dataFolderPath));
        auto flags = juce::FileBrowserComponent::canSelectDirectories | juce::FileBrowserComponent::openMode;
        fileChooser->launchAsync(flags, [this](const juce::FileChooser& fc) {
            auto result = fc.getResult();
            if (result.exists()) {
                settings.dataFolderPath = result.getFullPathName();
                folderPathLabel.setText(settings.dataFolderPath, juce::dontSendNotification);
                settings.saveSettings();
            }
        });
    };
}

SettingsComponent::~SettingsComponent() {}

void SettingsComponent::setupComboBox(juce::ComboBox& box, juce::Label& label, const juce::String& text, int selectedVal, bool isStereo)
{
    addAndMakeVisible(label);
    label.setText(text, juce::dontSendNotification);
    
    addAndMakeVisible(box);
    for (int i = 1; i <= 8; i += (isStereo ? 2 : 1))
    {
        juce::String itemText = isStereo ? "Channels " + juce::String(i) + "/" + juce::String(i+1) : "Channel " + juce::String(i);
        box.addItem(itemText, i);
    }
    box.setSelectedId(selectedVal, juce::dontSendNotification);
}

void SettingsComponent::paint(juce::Graphics&)
{
    // El fondo principal ya lo provee la ventana del módulo global
}

void SettingsComponent::resized()
{
    // Centramos elegantemente el bloque de preferencias en el área disponible
    auto bounds = getLocalBounds().reduced(20).withSizeKeepingCentre(550, 260);
    
    auto row1 = bounds.removeFromTop(35);
    procInLabel.setBounds(row1.removeFromLeft(240));
    procInCombo.setBounds(row1.reduced(0, 3));

    bounds.removeFromTop(15);

    auto row2 = bounds.removeFromTop(35);
    playOutLabel.setBounds(row2.removeFromLeft(240));
    playOutCombo.setBounds(row2.reduced(0, 3));

    bounds.removeFromTop(15);

    auto row3 = bounds.removeFromTop(35);
    diInLabel.setBounds(row3.removeFromLeft(240));
    diInCombo.setBounds(row3.reduced(0, 3));

    bounds.removeFromTop(15);

    auto rowSmooth = bounds.removeFromTop(35);
    smoothingLabel.setBounds(rowSmooth.removeFromLeft(240));
    smoothingCombo.setBounds(rowSmooth.reduced(0, 3));

    bounds.removeFromTop(20);

    auto row4 = bounds.removeFromTop(35);
    folderLabel.setBounds(row4.removeFromLeft(120));
    browseButton.setBounds(row4.removeFromRight(100).reduced(0, 3));
    folderPathLabel.setBounds(row4.reduced(10, 3));
}
