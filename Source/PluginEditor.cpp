#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "IconLibrary.h"

MondoHelixAnalyzerAudioProcessorEditor::MondoHelixAnalyzerAudioProcessorEditor (MondoHelixAnalyzerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
      settingsButton ("Settings", juce::Colours::white.withAlpha(0.7f), juce::Colours::white, juce::Colours::cyan),
      homeButton ("Home", juce::Colours::white.withAlpha(0.7f), juce::Colours::white, juce::Colours::cyan)
{
    setLookAndFeel (&customLookAndFeel);

    // Tamaño base optimizado para alojar las tarjetas con elegancia
    setSize (750, 500);
    setResizable (true, true);
    setResizeLimits (600, 400, 3840, 2160);

    // Inicializar submódulos
    homeView = std::make_unique<HomeViewComponent>();
    diListView = std::make_unique<GuitarDIListViewComponent>(audioProcessor.settings);
    diRecorderView = std::make_unique<GuitarDIRecorderViewComponent>(audioProcessor);
    analyzerView = std::make_unique<PresetAnalyzerViewComponent>(audioProcessor);
    settingsPanel = std::make_unique<SettingsComponent>(audioProcessor.settings);

    // Configurar enrutamiento (Routing)
    homeView->onModuleSelected = [this](int moduleIndex) {
        if (moduleIndex == 0)
        {
            diListView->refreshList();
            showView (diListView.get(), "Guitar DI");
        }
        else if (moduleIndex == 1)
        {
            showView (analyzerView.get(), "Preset Analyzer");
        }
        else if (moduleIndex == 2)
        {
            juce::AlertWindow::showMessageBoxAsync (
                juce::AlertWindow::InfoIcon,
                "Modulo en Desarrollo",
                "El modulo 'Preset Comparer' estara disponible en la proxima actualizacion de la suite."
            );
        }
    };

    diListView->onBackRequested = [this] {
        showView (homeView.get()); // Home has no title
    };

    diListView->onRecordNewRequested = [this] {
        diRecorderView->resetState(); // Asegura empezar siempre desde Iniciar Grabacion
        showView (diRecorderView.get(), "Grabar Nueva Pista DI"); // Recorder view title
    };

    diRecorderView->onFinished = [this] {
        diListView->refreshList();
        showView (diListView.get(), "Guitar DI"); // Return to list with title
    };

    // Botón de preferencias global con icono vectorial engranaje
    settingsButton.setShape (IconLibrary::getSettingsPath(), true, true, false);
    addAndMakeVisible (settingsButton);
    settingsButton.onClick = [this] {
        showView (settingsPanel.get(), "Preferences");
    };

    // Home icon button (icono) para navegación usando el catálogo centralizado
    homeButton.setShape (IconLibrary::getHomePath(), true, true, false);
    addAndMakeVisible (homeButton);
    homeButton.onClick = [this] { showView (homeView.get(), ""); };
    homeButton.setVisible (false);

    // Etiqueta para mostrar el nombre del módulo en la barra superior
    addAndMakeVisible (moduleTitleLabel);
    moduleTitleLabel.setFont (juce::Font (18.0f, juce::Font::bold));
    moduleTitleLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    moduleTitleLabel.setJustificationType (juce::Justification::centredLeft);
    moduleTitleLabel.setVisible (false);

    // Iniciar mostrando la pantalla de Home
    showView (homeView.get(), "");
}

MondoHelixAnalyzerAudioProcessorEditor::~MondoHelixAnalyzerAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void MondoHelixAnalyzerAudioProcessorEditor::showView (juce::Component* newView, const juce::String& moduleTitle)
{
    if (currentView != nullptr)
        removeChildComponent (currentView);

    currentView = newView;
    currentModuleTitle = moduleTitle;

    if (currentView != nullptr)
    {
        addAndMakeVisible (currentView);
        settingsButton.setVisible (currentView == homeView.get());
        // Mostrar/ocultar botón Home y etiqueta de título según sea necesario
        homeButton.setVisible (currentView != homeView.get());
        moduleTitleLabel.setText (moduleTitle, juce::dontSendNotification);
        moduleTitleLabel.setVisible (! moduleTitle.isEmpty());
        resized();
    }
}

void MondoHelixAnalyzerAudioProcessorEditor::paint (juce::Graphics& g)
{
    juce::Colour windowBgColor = getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId);
    juce::Colour textColor = juce::Colours::white;
    juce::Colour outlineColor = juce::Colour(0xff2a2a35);

    if (auto* lf = dynamic_cast<juce::LookAndFeel_V4*>(&getLookAndFeel()))
    {
        windowBgColor = lf->getCurrentColourScheme().getUIColour (juce::LookAndFeel_V4::ColourScheme::windowBackground);
        textColor = lf->getCurrentColourScheme().getUIColour (juce::LookAndFeel_V4::ColourScheme::defaultText);
        outlineColor = lf->getCurrentColourScheme().getUIColour (juce::LookAndFeel_V4::ColourScheme::outline);
    }

    g.fillAll (windowBgColor);

    // Barra de Navegación Superior (referencia dorada, pero usamos color temático)
    auto topBar = getLocalBounds().removeFromTop (50);
    g.setColour (windowBgColor.darker (0.2f));
    g.fillRect (topBar);
    g.setColour (outlineColor);
    g.drawLine (0.0f, 50.0f, static_cast<float>(getWidth()), 50.0f, 1.0f);

    // Dibujar botón Home y título del módulo (si corresponde)
    // Los componentes ya gestionan su propio dibujo, aquí solo reservamos espacio
    // (no se dibuja texto directamente para que el Label pueda gestionar fuentes y accesibilidad)

    // Barra de Estado Inferior
    auto bottomBar = getLocalBounds().removeFromBottom (30);
    g.setColour (windowBgColor.darker (0.3f));
    g.fillRect (bottomBar);
    g.setColour (outlineColor);
    g.drawLine (0.0f, static_cast<float>(getHeight() - 30), static_cast<float>(getWidth()), static_cast<float>(getHeight() - 30), 1.0f);

    // Informar estados en la barra inferior
    g.setColour (textColor.withAlpha (0.6f));
    g.setFont (juce::Font (12.0f));
    g.drawText ("Estado: Listo | ASIO Driver Conectado", bottomBar.reduced (15, 0), juce::Justification::centredLeft, true);
}

void MondoHelixAnalyzerAudioProcessorEditor::resized()
{
    auto topBar = getLocalBounds().removeFromTop (50);
    
    // Posicionamos el nuevo botón de engranaje (Preferencias) a la extrema derecha
    settingsButton.setBounds (topBar.removeFromRight (50).reduced (12));

    // Posicionar Home button y etiqueta de título del módulo a la izquierda
    homeButton.setBounds (topBar.removeFromLeft (50).reduced (12));
    moduleTitleLabel.setBounds (topBar.reduced (10, 8));

    // Espacio exclusivo para los módulos entre las dos barras
    auto centerArea = getLocalBounds().withTrimmedTop (50).withTrimmedBottom (30);

    if (currentView != nullptr)
        currentView->setBounds (centerArea);
}
