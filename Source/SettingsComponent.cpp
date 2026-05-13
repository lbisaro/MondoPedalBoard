#include "SettingsComponent.h"

SettingsComponent::SettingsComponent(AppSettings& s) : settings(s)
{
    addAndMakeVisible(titleLabel);
    titleLabel.setText("PREFERENCES", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(22.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(closeButton);
    closeButton.setButtonText("✕");
    // closeButton.setColour(...);
    closeButton.onClick = [this] { if (onClose) onClose(); };

    setupComboBox(procInCombo, procInLabel, "Processed Input (Helix -> App):", settings.processedInputChannel.load(), true);
    setupComboBox(playOutCombo, playOutLabel, "Playback Output (App -> Helix):", settings.playbackOutputChannel.load(), true);
    setupComboBox(diInCombo, diInLabel, "DI Input (Helix -> App):", settings.diInputChannel.load(), false);

    procInCombo.onChange = [this] { settings.processedInputChannel.store(procInCombo.getSelectedId()); settings.saveSettings(); };
    playOutCombo.onChange = [this] { settings.playbackOutputChannel.store(playOutCombo.getSelectedId()); settings.saveSettings(); };
    diInCombo.onChange = [this] { settings.diInputChannel.store(diInCombo.getSelectedId()); settings.saveSettings(); };

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

void SettingsComponent::paint(juce::Graphics& g)
{
    // Fondo oscurecido para dar foco al modal
    g.fillAll(juce::Colours::black.withAlpha(0.7f));

    auto panelBounds = getLocalBounds().reduced(40);
    
    juce::Colour bgColor = juce::Colour(0xff1e1e24);
    juce::Colour outlineColor = juce::Colour(0xff3a3a45);

    if (auto* lf = dynamic_cast<juce::LookAndFeel_V4*>(&getLookAndFeel()))
    {
        bgColor = lf->getCurrentColourScheme().getUIColour (juce::LookAndFeel_V4::ColourScheme::windowBackground);
        outlineColor = lf->getCurrentColourScheme().getUIColour (juce::LookAndFeel_V4::ColourScheme::outline);
    }

    // Modal Box
    g.setColour(bgColor);
    g.fillRoundedRectangle(panelBounds.toFloat(), 12.0f);

    // Borde sutil
    g.setColour(outlineColor);
    g.drawRoundedRectangle(panelBounds.toFloat(), 12.0f, 1.5f);
}

void SettingsComponent::resized()
{
    auto panelBounds = getLocalBounds().reduced(40);
    auto bounds = panelBounds.reduced(25); // Padding interno del modal
    
    auto header = bounds.removeFromTop(40);
    closeButton.setBounds(header.removeFromRight(30).reduced(5));
    titleLabel.setBounds(header);

    bounds.removeFromTop(20); // Espaciador

    auto row1 = bounds.removeFromTop(35);
    procInLabel.setBounds(row1.removeFromLeft(220));
    procInCombo.setBounds(row1.reduced(0, 3));

    bounds.removeFromTop(10);

    auto row2 = bounds.removeFromTop(35);
    playOutLabel.setBounds(row2.removeFromLeft(220));
    playOutCombo.setBounds(row2.reduced(0, 3));

    bounds.removeFromTop(10);

    auto row3 = bounds.removeFromTop(35);
    diInLabel.setBounds(row3.removeFromLeft(220));
    diInCombo.setBounds(row3.reduced(0, 3));

    bounds.removeFromTop(30);

    auto row4 = bounds.removeFromTop(35);
    folderLabel.setBounds(row4.removeFromLeft(120));
    browseButton.setBounds(row4.removeFromRight(90).reduced(0, 3));
    folderPathLabel.setBounds(row4.reduced(5, 5));
}
