#include "HomeViewComponent.h"

// =============================================================================
// ModuleCardComponent
// =============================================================================
ModuleCardComponent::ModuleCardComponent (const juce::String& t, const juce::String& d, int cId)
    : moduleTitle (t), moduleDesc (d), themeColorId (cId)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void ModuleCardComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    juce::Colour accentColor = juce::Colours::cyan;
    juce::Colour bgBaseColor = juce::Colour::fromString("ff252526");
    juce::Colour textColor = juce::Colours::white;
    juce::Colour descColor = juce::Colours::grey;

    if (auto* lf = dynamic_cast<juce::LookAndFeel_V4*>(&getLookAndFeel()))
    {
        accentColor = lf->findColour (themeColorId);
        bgBaseColor = lf->getCurrentColourScheme().getUIColour (juce::LookAndFeel_V4::ColourScheme::widgetBackground);
        textColor = lf->getCurrentColourScheme().getUIColour (juce::LookAndFeel_V4::ColourScheme::defaultText);
    }

    // Efecto hover premium: aclaramos ligeramente el fondo
    if (isHovered)
    {
        bgBaseColor = bgBaseColor.brighter(0.1f);
    }

    // Guardamos el estado gráfico para aplicar un recorte (clipping) maestro
    g.saveState();

    // Creamos la silueta exacta con bordes redondeados de la tarjeta
    juce::Path cardClipPath;
    cardClipPath.addRoundedRectangle (bounds, 12.0f);
    g.reduceClipRegion (cardClipPath);

    // 1. Rellenamos todo el fondo general de la tarjeta
    g.setColour (bgBaseColor);
    g.fillAll();

    // 2. Dibujamos la franja de acento a la izquierda
    // Al estar activa la región de recorte, las esquinas superior e inferior
    // izquierda seguirán exactamente la curvatura nativa exterior
    g.setColour (accentColor);
    g.fillRect (bounds.withWidth (6.0f));

    // Restauramos el estado gráfico para quitar el recorte
    g.restoreState();

    // 3. Dibujamos el contorno sutil superpuesto con perfecta alineación
    g.setColour (isHovered ? accentColor : bgBaseColor.brighter(0.2f));
    g.drawRoundedRectangle (bounds.reduced(0.5f), 12.0f, isHovered ? 2.0f : 1.0f);

    // Textos
    bounds.removeFromLeft (6.0f);
    bounds.reduce (15.0f, 15.0f);
    
    g.setColour (textColor);
    g.setFont (juce::Font (22.0f, juce::Font::bold));
    g.drawText (moduleTitle, bounds.removeFromTop(30), juce::Justification::centredLeft, true);

    bounds.removeFromTop (10.0f); // Espaciador

    g.setColour (descColor);
    g.setFont (juce::Font (14.0f, juce::Font::plain));
    g.drawMultiLineText (moduleDesc, static_cast<int>(bounds.getX()), static_cast<int>(bounds.getY()), static_cast<int>(bounds.getWidth()));
}

void ModuleCardComponent::mouseEnter (const juce::MouseEvent&)
{
    isHovered = true;
    repaint();
}

void ModuleCardComponent::mouseExit (const juce::MouseEvent&)
{
    isHovered = false;
    repaint();
}

void ModuleCardComponent::mouseUp (const juce::MouseEvent& e)
{
    if (onClick && contains(e.getPosition()))
        onClick();
}

// =============================================================================
// HomeViewComponent
// =============================================================================
HomeViewComponent::HomeViewComponent()
{
    guitarDICard = std::make_unique<ModuleCardComponent>(
        "Guitar DI", 
        "Administra pistas DI mono WAV. Captura nuevas tomas desde tu Helix con aplicacion automatica de desvanecimientos (Fades) para loops limpios.", 
        CustomLookAndFeel::defaultColourId
    );
    addAndMakeVisible (guitarDICard.get());
    guitarDICard->onClick = [this] { if (onModuleSelected) onModuleSelected(0); };

    presetAnalyzerCard = std::make_unique<ModuleCardComponent>(
        "Preset Analyzer", 
        "Visualizador espectral en tiempo real con suavizado fraccional, analisis de LUFS instantaneo y clasificacion tonal automatica (PLR/Type).", 
        CustomLookAndFeel::defaultColourId
    );
    addAndMakeVisible (presetAnalyzerCard.get());
    presetAnalyzerCard->onClick = [this] { if (onModuleSelected) onModuleSelected(1); };

    blockAnalyzerCard = std::make_unique<ModuleCardComponent>(
        "BLOCK ANALIZER", 
        "Analiza la respuesta en frecuencia (funcion de transferencia) y dinamica (compresion, knee y gain reduction) del pedalboard en tiempo real con inyeccion de ruido rosa.", 
        CustomLookAndFeel::defaultColourId
    );
    addAndMakeVisible (blockAnalyzerCard.get());
    blockAnalyzerCard->onClick = [this] { if (onModuleSelected) onModuleSelected(2); };

    samplesAnalyzerCard = std::make_unique<ModuleCardComponent>(
        "Samples Analyzer", 
        "Analiza archivos WAV/MP3 offline con el algoritmo de la suite. Guarda referencias de tonos legendarios (LUFS/PLR/Centroid) y usalas como objetivos.", 
        CustomLookAndFeel::defaultColourId
    );
    addAndMakeVisible (samplesAnalyzerCard.get());
    samplesAnalyzerCard->onClick = [this] { if (onModuleSelected) onModuleSelected(3); };
}

void HomeViewComponent::paint (juce::Graphics&)
{
    // Sin leyendas adicionales según lo solicitado
}

void HomeViewComponent::resized()
{
    auto bounds = getLocalBounds().reduced (20);

    // Grilla dinámica de 2 columnas
    int cardGap = 20;
    int numCols = 2;
    int cardWidth = (bounds.getWidth() - cardGap * (numCols - 1)) / numCols;
    int cardHeight = 130;

    // Fila 1
    guitarDICard->setBounds (bounds.getX(), bounds.getY(), cardWidth, cardHeight);
    presetAnalyzerCard->setBounds (bounds.getX() + cardWidth + cardGap, bounds.getY(), cardWidth, cardHeight);

    // Fila 2
    blockAnalyzerCard->setBounds (bounds.getX(), bounds.getY() + cardHeight + cardGap, cardWidth, cardHeight);
    samplesAnalyzerCard->setBounds (bounds.getX() + cardWidth + cardGap, bounds.getY() + cardHeight + cardGap, cardWidth, cardHeight);
}
