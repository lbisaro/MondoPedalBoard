#pragma once
#include <JuceHeader.h>
#include "AppSettings.h"
#include "CustomLookAndFeel.h"

#include "IconLibrary.h"

class DIRowComponent : public juce::Component
{
public:
    DIRowComponent (const juce::File& diFile, std::function<void(juce::File)> onDelete);
    ~DIRowComponent() override = default;

    void paint (juce::Graphics& g) override;
    void resized() override;
    
    void mouseEnter (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent& e) override;

private:
    juce::File file;
    juce::TextButton deleteButton;
    bool isHovered = false;
    juce::String durationStr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DIRowComponent)
};

class PlusIconButton : public juce::Button
{
public:
    PlusIconButton() : juce::Button ("RecordNewButton") {}

    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = getLocalBounds().toFloat();
        
        // Colores base premium basados en CustomLookAndFeel
        juce::Colour bgColor = juce::Colour(0xff00ffff); // color de resguardo
        juce::Colour iconColor = juce::Colours::white;
        
        if (auto* lf = dynamic_cast<CustomLookAndFeel*>(&getLookAndFeel()))
        {
            bgColor = lf->findColour (CustomLookAndFeel::defaultColourId);
            if (auto* v4 = dynamic_cast<juce::LookAndFeel_V4*>(lf))
                iconColor = v4->getCurrentColourScheme().getUIColour (juce::LookAndFeel_V4::ColourScheme::defaultText);
        }

        if (shouldDrawButtonAsDown)
            bgColor = bgColor.darker (0.2f);
        else if (shouldDrawButtonAsHighlighted)
            bgColor = bgColor.brighter (0.1f);

        // Dibujar fondo con esquinas suavemente redondeadas usando el color default
        g.setColour (bgColor);
        g.fillRoundedRectangle (bounds, 6.0f);

        // Dibujar el icono "+" usando nuestra IconLibrary centralizada
        g.setColour (iconColor);
        juce::Path iconPath = IconLibrary::getPlusPath();
        
        // Escalar y centrar con elegancia manteniendo las proporciones
        float targetSize = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        auto iconBounds = bounds.withSizeKeepingCentre (targetSize, targetSize);
        g.fillPath (iconPath, iconPath.getTransformToScaleToFit (iconBounds, true));
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlusIconButton)
};

class GuitarDIListViewComponent : public juce::Component
{
public:
    GuitarDIListViewComponent (AppSettings& settings);
    ~GuitarDIListViewComponent() override = default;

    void paint (juce::Graphics& g) override;
    void resized() override;

    void refreshList();

    std::function<void()> onRecordNewRequested;
    std::function<void()> onBackRequested;

private:
    AppSettings& appSettings;

    // juce::TextButton backButton; // removed per design
    PlusIconButton recordNewButton;
    
    juce::Viewport viewport;
    juce::Component listContainer;

    std::vector<std::unique_ptr<DIRowComponent>> rows;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GuitarDIListViewComponent)
};
