#include "GuitarDIListViewComponent.h"

// =============================================================================
// DIRowComponent
// =============================================================================
DIRowComponent::DIRowComponent (const juce::File& f, std::function<void(juce::File)> onDelete)
    : file (f)
{
    addAndMakeVisible (deleteButton);
    deleteButton.setButtonText ("Eliminar");
    
    // Aplicar estilo de botón Danger/Eliminar si es posible
    deleteButton.onClick = [this, onDelete] {
        if (onDelete) onDelete (file);
    };

    // Calcular duración del archivo de audio (WAV)
    durationStr = "00:00";
    juce::WavAudioFormat wavFormat;
    if (std::unique_ptr<juce::AudioFormatReader> reader { wavFormat.createReaderFor (new juce::FileInputStream (file), true) })
    {
        if (reader->sampleRate > 0)
        {
            double lengthInSeconds = reader->lengthInSamples / reader->sampleRate;
            int totalSeconds = static_cast<int>(std::round(lengthInSeconds));
            int mins = totalSeconds / 60;
            int secs = totalSeconds % 60;
            durationStr = juce::String::formatted("%02d:%02d", mins, secs);
        }
    }
}

void DIRowComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    juce::Colour bgBaseColor = juce::Colour::fromString("ff252526");
    juce::Colour textColor = juce::Colours::white;
    juce::Colour sizeColor = juce::Colours::grey;

    if (auto* lf = dynamic_cast<juce::LookAndFeel_V4*>(&getLookAndFeel()))
    {
        bgBaseColor = lf->getCurrentColourScheme().getUIColour (juce::LookAndFeel_V4::ColourScheme::widgetBackground);
        textColor = lf->getCurrentColourScheme().getUIColour (juce::LookAndFeel_V4::ColourScheme::defaultText);
    }

    if (isHovered)
        bgBaseColor = bgBaseColor.brighter (0.1f);

    // Fondo del renglón con leve espaciado
    g.setColour (bgBaseColor);
    g.fillRoundedRectangle (bounds.reduced(2.0f), 6.0f);

    // Texto del archivo
    g.setColour (textColor);
    g.setFont (juce::Font (16.0f, juce::Font::bold));
    g.drawText (file.getFileNameWithoutExtension(), bounds.reduced(15.0f, 0).removeFromLeft(300), juce::Justification::centredLeft, true);

    // Duración del archivo (Formato mm:ss)
    g.setColour (sizeColor);
    g.setFont (juce::Font (14.0f, juce::Font::plain));
    g.drawText (durationStr, bounds.reduced(15.0f, 0).removeFromLeft(400).removeFromRight(100), juce::Justification::centredRight, true);
}

void DIRowComponent::resized()
{
    auto bounds = getLocalBounds().reduced (10);
    deleteButton.setBounds (bounds.removeFromRight (80));
}

void DIRowComponent::mouseEnter (const juce::MouseEvent&)
{
    isHovered = true;
    repaint();
}

void DIRowComponent::mouseExit (const juce::MouseEvent&)
{
    isHovered = false;
    repaint();
}

// =============================================================================
// GuitarDIListViewComponent
// =============================================================================
GuitarDIListViewComponent::GuitarDIListViewComponent (AppSettings& s)
    : appSettings (s)
{
    // No back button per design
    addAndMakeVisible (recordNewButton);
    recordNewButton.onClick = [this] { if (onRecordNewRequested) onRecordNewRequested(); };

    // Configurar contenedor con scroll
    addAndMakeVisible (viewport);
    viewport.setViewedComponent (&listContainer, false);
    viewport.setScrollBarsShown (true, false);

    refreshList();
}

void GuitarDIListViewComponent::refreshList()
{
    rows.clear();

    juce::File folder = appSettings.getDIFolder();
    juce::Array<juce::File> files = folder.findChildFiles (juce::File::findFiles, false, "*.wav");

    int rowHeight = 50;
    int currentY = 0;

    for (const auto& f : files)
    {
        auto row = std::make_unique<DIRowComponent>(f, [this](juce::File fileToDelete) {
            // Confirmación simple de borrado
            if (fileToDelete.deleteFile())
                refreshList();
        });

        listContainer.addAndMakeVisible (row.get());
        row->setBounds (0, currentY, viewport.getWidth() - 20, rowHeight);
        currentY += rowHeight;
        
        rows.push_back (std::move (row));
    }

    listContainer.setSize (viewport.getWidth() - 20, currentY);
    resized();
}

void GuitarDIListViewComponent::paint (juce::Graphics& g)
{
    juce::Colour textColor = juce::Colours::white;
    if (auto* lf = dynamic_cast<juce::LookAndFeel_V4*>(&getLookAndFeel()))
        textColor = lf->getCurrentColourScheme().getUIColour (juce::LookAndFeel_V4::ColourScheme::defaultText);

    g.setColour (textColor);
    g.setFont (juce::Font (24.0f, juce::Font::bold));


    if (rows.empty())
    {
        g.setFont (juce::Font (16.0f, juce::Font::italic));
        g.setColour (juce::Colours::grey);
        g.drawText ("No hay pistas DI guardadas. Haz clic en 'Grabar Nuevo DI' para comenzar.", 
                    viewport.getBounds(), juce::Justification::centred, true);
    }
}

void GuitarDIListViewComponent::resized()
{
    auto bounds = getLocalBounds().reduced (20);

    auto header = bounds.removeFromTop (40);
    // Solo el botón de grabar ocupa todo el ancho del header
    recordNewButton.setBounds (header);


    bounds.removeFromTop (20); // Espaciador

    viewport.setBounds (bounds);

    // Reajustar anchos de las filas
    int rowHeight = 50;
    int currentY = 0;
    int contentWidth = viewport.getWidth() - (viewport.isVerticalScrollBarShown() ? 20 : 0);
    
    for (auto& row : rows)
    {
        row->setBounds (0, currentY, contentWidth, rowHeight);
        currentY += rowHeight;
    }
    listContainer.setSize (contentWidth, currentY);
}
