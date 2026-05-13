#pragma once
#include <JuceHeader.h>
#include "CustomLookAndFeel.h"

class ModuleCardComponent : public juce::Component
{
public:
    ModuleCardComponent (const juce::String& title, const juce::String& description, int colorId);
    ~ModuleCardComponent() override = default;

    void paint (juce::Graphics& g) override;
    
    void mouseEnter (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;

    std::function<void()> onClick;

private:
    juce::String moduleTitle;
    juce::String moduleDesc;
    int themeColorId;
    bool isHovered = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModuleCardComponent)
};

class HomeViewComponent : public juce::Component
{
public:
    HomeViewComponent();
    ~HomeViewComponent() override = default;

    void paint (juce::Graphics& g) override;
    void resized() override;

    std::function<void(int)> onModuleSelected;

private:
    std::unique_ptr<ModuleCardComponent> guitarDICard;
    std::unique_ptr<ModuleCardComponent> presetAnalyzerCard;
    std::unique_ptr<ModuleCardComponent> presetComparerCard;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HomeViewComponent)
};
