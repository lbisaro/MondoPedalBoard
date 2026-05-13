#pragma once
#include <JuceHeader.h>
#include "AppSettings.h"

class SettingsComponent : public juce::Component
{
public:
    SettingsComponent(AppSettings& s);
    ~SettingsComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    std::function<void()> onClose;

private:
    AppSettings& settings;

    juce::Label titleLabel;
    juce::TextButton closeButton;

    juce::Label procInLabel;
    juce::ComboBox procInCombo;

    juce::Label playOutLabel;
    juce::ComboBox playOutCombo;

    juce::Label diInLabel;
    juce::ComboBox diInCombo;

    juce::Label folderLabel;
    juce::Label folderPathLabel;
    juce::TextButton browseButton;

    std::unique_ptr<juce::FileChooser> fileChooser;

    void setupComboBox(juce::ComboBox& box, juce::Label& label, const juce::String& text, int selectedVal, bool isStereo);
    void saveAndNotify();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SettingsComponent)
};
