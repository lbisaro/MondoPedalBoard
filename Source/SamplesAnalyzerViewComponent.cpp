#include "SamplesAnalyzerViewComponent.h"

SamplesAnalyzerViewComponent::SamplesAnalyzerViewComponent(AppSettings& s)
    : Thread("SamplesOfflineAnalyzerThread"), settings(s)
{
    addAndMakeVisible(sampleFilesComboBox);
    sampleFilesComboBox.setTextWhenNoChoicesAvailable("No se encontraron muestras");
    sampleFilesComboBox.setTextWhenNothingSelected("Selecciona una muestra...");
    sampleFilesComboBox.onChange = [this] { loadExistingAnalysis(); };

    addAndMakeVisible(targetProfileComboBox);
    targetProfileComboBox.addItem("AMBIENT", 1);
    targetProfileComboBox.addItem("RHYTHM", 2);
    targetProfileComboBox.addItem("LEAD", 3);
    targetProfileComboBox.setSelectedId(2); // Default to Rhythm

    addAndMakeVisible(refreshButton);
    refreshButton.setButtonText("Actualizar");
    refreshButton.onClick = [this] { refreshFilesList(); };

    addAndMakeVisible(browseButton);
    browseButton.setButtonText("Importar...");
    browseButton.onClick = [this] { importCustomSample(); };

    addAndMakeVisible(analyzeButton);
    analyzeButton.setButtonText("ANALIZAR MUESTRA");
    analyzeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff5c2d91)); // Purple Neon
    analyzeButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    analyzeButton.onClick = [this] { runAnalysis(); };

    addAndMakeVisible(statusLabel);
    statusLabel.setFont(juce::Font(13.0f, juce::Font::bold));
    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::orange);

    refreshFilesList();
}

SamplesAnalyzerViewComponent::~SamplesAnalyzerViewComponent()
{
    if (isThreadRunning())
    {
        stopThread(1000);
    }
}

void SamplesAnalyzerViewComponent::refreshFilesList()
{
    sampleFilesComboBox.clear(juce::dontSendNotification);
    availableAudioFiles.clear();

    juce::File folder = settings.getGTRSamplesFolder();
    
    // Scan WAV files
    juce::Array<juce::File> wavFiles = folder.findChildFiles(juce::File::findFiles, false, "*.wav");
    for (const auto& f : wavFiles)
        availableAudioFiles.add(f);

    // Scan MP3 files
    juce::Array<juce::File> mp3Files = folder.findChildFiles(juce::File::findFiles, false, "*.mp3");
    for (const auto& f : mp3Files)
        availableAudioFiles.add(f);

    for (int i = 0; i < availableAudioFiles.size(); ++i)
    {
        sampleFilesComboBox.addItem(availableAudioFiles[i].getFileName(), i + 1);
    }

    if (!availableAudioFiles.isEmpty())
    {
        sampleFilesComboBox.setSelectedId(1);
    }
    else
    {
        currentResult = SampleAnalysisResult();
        statusLabel.setText("Carpeta GTR_Samples vacía. Importa un archivo WAV o MP3.", juce::dontSendNotification);
        repaint();
    }
}

void SamplesAnalyzerViewComponent::loadExistingAnalysis()
{
    int selId = sampleFilesComboBox.getSelectedId();
    if (selId <= 0)
    {
        currentResult = SampleAnalysisResult();
        repaint();
        return;
    }

    juce::File selectedFile = availableAudioFiles[selId - 1];
    juce::File analysisFile = settings.getGTRSamplesFolder().getChildFile(selectedFile.getFileNameWithoutExtension() + ".gtr_analysis");

    if (analysisFile.existsAsFile())
    {
        currentResult = SamplesOfflineAnalyzer::loadAnalysisResult(analysisFile);
        if (currentResult.success)
        {
            // Alinear la categoría de target cargada
            if (currentResult.targetCategory.equalsIgnoreCase("Ambient"))
                targetProfileComboBox.setSelectedId(1, juce::dontSendNotification);
            else if (currentResult.targetCategory.equalsIgnoreCase("Rhythm"))
                targetProfileComboBox.setSelectedId(2, juce::dontSendNotification);
            else if (currentResult.targetCategory.equalsIgnoreCase("Lead"))
                targetProfileComboBox.setSelectedId(3, juce::dontSendNotification);

            statusLabel.setText("Cargado análisis existente para: " + currentResult.sourceFile, juce::dontSendNotification);
        }
        else
        {
            statusLabel.setText("Listo para analizar: " + selectedFile.getFileName(), juce::dontSendNotification);
        }
    }
    else
    {
        currentResult = SampleAnalysisResult();
        statusLabel.setText("Muestra sin analizar. Haz clic en 'ANALIZAR MUESTRA'.", juce::dontSendNotification);
    }

    repaint();
}

void SamplesAnalyzerViewComponent::importCustomSample()
{
    fileChooser = std::make_shared<juce::FileChooser>(
        "Selecciona un archivo de audio WAV o MP3...",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.wav;*.mp3"
    );

    fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file.existsAsFile())
            {
                auto destFile = settings.getGTRSamplesFolder().getChildFile(file.getFileName());
                
                // Si el archivo de destino existe, lo sobreescribimos
                if (destFile.existsAsFile())
                    destFile.deleteFile();

                if (file.copyFileTo(destFile))
                {
                    refreshFilesList();
                    
                    // Seleccionar la nueva muestra importada
                    for (int i = 0; i < availableAudioFiles.size(); ++i)
                    {
                        if (availableAudioFiles[i] == destFile)
                        {
                            sampleFilesComboBox.setSelectedId(i + 1);
                            break;
                        }
                    }
                }
            }
        });
}

void SamplesAnalyzerViewComponent::runAnalysis()
{
    int selId = sampleFilesComboBox.getSelectedId();
    if (selId <= 0)
    {
        statusLabel.setText("Por favor, selecciona una muestra válida.", juce::dontSendNotification);
        return;
    }

    fileToAnalyze = availableAudioFiles[selId - 1];
    categoryToAssign = targetProfileComboBox.getText();

    isAnalyzing.store(true);
    statusLabel.setText("Analizando archivo: " + fileToAnalyze.getFileName() + "...", juce::dontSendNotification);

    analyzeButton.setEnabled(false);
    browseButton.setEnabled(false);
    refreshButton.setEnabled(false);
    sampleFilesComboBox.setEnabled(false);
    targetProfileComboBox.setEnabled(false);

    startThread();
}

void SamplesAnalyzerViewComponent::run()
{
    auto result = SamplesOfflineAnalyzer::analyzeFile(fileToAnalyze, categoryToAssign);

    if (result.success)
    {
        juce::File analysisFile = settings.getGTRSamplesFolder().getChildFile(fileToAnalyze.getFileNameWithoutExtension() + ".gtr_analysis");
        SamplesOfflineAnalyzer::saveAnalysisResult(result, analysisFile);
        currentResult = result;
        statusText = "¡Análisis offline completado y guardado con éxito!";
    }
    else
    {
        statusText = "Error: No se pudo analizar el archivo. Comprueba el formato.";
    }

    isAnalyzing.store(false);

    juce::MessageManager::callAsync([this]
    {
        analyzeButton.setEnabled(true);
        browseButton.setEnabled(true);
        refreshButton.setEnabled(true);
        sampleFilesComboBox.setEnabled(true);
        targetProfileComboBox.setEnabled(true);

        statusLabel.setText(statusText, juce::dontSendNotification);
        repaint();
    });
}

void SamplesAnalyzerViewComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::transparentBlack);

    auto drawCard = [&](juce::Rectangle<int> area, bool highlighted, const juce::String& title, const juce::String& valText)
    {
        // Fondo card
        g.setColour(juce::Colour(0xff2a2a35).withAlpha(0.6f));
        if (highlighted)
            g.setColour(juce::Colour(0xff3a3a4a).withAlpha(0.8f));

        g.fillRoundedRectangle(area.toFloat(), 8.0f);

        // Borde
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        if (highlighted)
            g.setColour(juce::Colours::cyan.withAlpha(0.3f));

        g.drawRoundedRectangle(area.toFloat(), 8.0f, 1.5f);

        // Titulo interno de card
        g.setColour(juce::Colours::white.withAlpha(0.5f));
        g.setFont(juce::Font(10.0f, juce::Font::bold));
        g.drawText(title, area.withHeight(18).reduced(8, 0), juce::Justification::left);

        // Valor
        if (title.contains("MUESTRA"))
        {
            g.setColour(juce::Colours::white);
            g.setFont(juce::Font(12.0f, juce::Font::bold));
            g.drawMultiLineText(valText, area.getX() + 10, area.getY() + 32, area.getWidth() - 20);
        }
        else
        {
            g.setColour(highlighted ? juce::Colours::cyan : juce::Colours::white);
            g.setFont(juce::Font(22.0f, juce::Font::bold));
            g.drawText(valText, area.withTrimmedTop(15), juce::Justification::centred);
        }
    };

    // Card 1: Info Muestra
    juce::String infoText;
    if (currentResult.success)
    {
        infoText = "Archivo:\n" + currentResult.sourceFile + "\n\nTarget asignado: " + currentResult.targetCategory;
    }
    else if (isAnalyzing.load())
    {
        infoText = "Ejecutando análisis offline de True Peak y respuesta espectral en hilo secundario...";
    }
    else
    {
        infoText = "Ningún análisis cargado.\nSelecciona o importa una pista y haz clic en ANALIZAR.";
    }

    drawCard(card1, currentResult.success, "INFO MUESTRA DE REFERENCIA", infoText);

    // Card 2: Loudness
    juce::String lufsVal = currentResult.success ? juce::String(currentResult.lufs, 1) + " LUFS" : "-- LUFS";
    drawCard(card2, false, "LOUDNESS (INTEGRATED)", lufsVal);

    // Card 3: PLR
    juce::String plrVal = currentResult.success ? juce::String(currentResult.plr, 1) + " PLR" : "-- PLR";
    drawCard(card3, false, "DYNAMICS (PLR)", plrVal);

    // Card 4: Brightness (Hz)
    juce::String brightVal = currentResult.success ? juce::String(juce::roundToInt(currentResult.brightness)) + " Hz" : "-- Hz";
    drawCard(card4, false, "SPECTRAL BRIGHTNESS", brightVal);

    // Card 7: Brightness Band Energy (%)
    juce::String brilloRatioVal = currentResult.success ? juce::String(currentResult.brilloRatio * 100.0f, 1) + "%" : "-- %";
    drawCard(card7, false, "BRIGHTNESS BAND ENERGY", brilloRatioVal);

    // Card 5: Body
    juce::String bodyVal = currentResult.success ? juce::String(currentResult.bodyRatio * 100.0f, 1) + "%" : "-- %";
    drawCard(card5, false, "TONAL BODY", bodyVal);

    // Card 6: Cut
    juce::String cutVal = currentResult.success ? juce::String(currentResult.cutRatio * 100.0f, 1) + "%" : "-- %";
    drawCard(card6, false, "TONAL CUT", cutVal);
}

void SamplesAnalyzerViewComponent::resized()
{
    auto bounds = getLocalBounds().reduced(20);

    // Top Bar (altura 40)
    auto topBar = bounds.removeFromTop(40);
    sampleFilesComboBox.setBounds(topBar.removeFromLeft(220).reduced(0, 4));
    topBar.removeFromLeft(8);
    refreshButton.setBounds(topBar.removeFromLeft(80).reduced(0, 4));
    topBar.removeFromLeft(8);
    browseButton.setBounds(topBar.removeFromLeft(90).reduced(0, 4));
    topBar.removeFromLeft(15);
    
    // Target Category ComboBox and analyze button
    targetProfileComboBox.setBounds(topBar.removeFromLeft(120).reduced(0, 4));
    topBar.removeFromLeft(8);
    analyzeButton.setBounds(topBar.removeFromLeft(150).reduced(0, 4));

    bounds.removeFromTop(15);

    // 4-Column Responsive Layout (Info card spans 2 columns, others span 1 column)
    int gridSpacing = 12;
    int cardW = (bounds.getWidth() - (gridSpacing * 3)) / 4;
    int cardH = (bounds.getHeight() - gridSpacing - 30) / 2; // dejar 30 para el statusLabel

    auto row1 = bounds.removeFromTop(cardH);
    card1 = row1.removeFromLeft(cardW * 2 + gridSpacing);
    row1.removeFromLeft(gridSpacing);
    card2 = row1.removeFromLeft(cardW);
    row1.removeFromLeft(gridSpacing);
    card3 = row1;

    bounds.removeFromTop(gridSpacing);

    auto row2 = bounds.removeFromTop(cardH);
    card4 = row2.removeFromLeft(cardW);
    row2.removeFromLeft(gridSpacing);
    card7 = row2.removeFromLeft(cardW);
    row2.removeFromLeft(gridSpacing);
    card5 = row2.removeFromLeft(cardW);
    row2.removeFromLeft(gridSpacing);
    card6 = row2;

    bounds.removeFromTop(10);
    statusLabel.setBounds(bounds.removeFromBottom(25));
}
