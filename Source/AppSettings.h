#pragma once
#include <JuceHeader.h>
#include <atomic>

class AppSettings
{
public:
    AppSettings()
    {
        // Valores por defecto orientados al flujo estándar de Re-amping de la Line 6 Helix
        processedInputChannel = 1; // Escucha el procesado principal por USB 1/2 (Salida Multi por defecto en presets)
        playbackOutputChannel = 7; // Envía la pista DI hacia USB 7/8 (Entrada estándar de inyección para re-amping)
        diInputChannel = 7;        // Escucha DI seco por USB 7

        dataFolderPath = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                            .getChildFile("MondoHelixAnalyzer").getFullPathName();

        windowWidth = 938;  // Base 750 * 1.25
        windowHeight = 625; // Base 500 * 1.25
        windowX = -1;       // -1 means centered
        windowY = -1;
        fftSmoothingDenominator = 6; // Default to 1/6 octave

        loadSettings();
    }

    void loadSettings()
    {
        auto file = getSettingsFile();
        if (file.existsAsFile())
        {
            if (auto xml = juce::XmlDocument::parse(file))
            {
                // Load raw values
                int procCh = xml->getIntAttribute("processedInputChannel", processedInputChannel);
                int playCh = xml->getIntAttribute("playbackOutputChannel", playbackOutputChannel);
                int diCh   = xml->getIntAttribute("diInputChannel", diInputChannel);
                // Clamp to valid ranges (1-8 for all channel settings)
                procCh = juce::jlimit(1, 8, procCh);
                playCh = juce::jlimit(1, 8, playCh);
                diCh   = juce::jlimit(1, 8, diCh);
                processedInputChannel = procCh;
                playbackOutputChannel = playCh;
                diInputChannel = diCh;
                juce::String savedPath = xml->getStringAttribute("dataFolderPath", dataFolderPath);
                if (savedPath.isNotEmpty())
                    dataFolderPath = savedPath;
                
                windowWidth = xml->getIntAttribute("windowWidth", windowWidth);
                windowHeight = xml->getIntAttribute("windowHeight", windowHeight);
                windowX = xml->getIntAttribute("windowX", windowX);
                windowY = xml->getIntAttribute("windowY", windowY);
                fftSmoothingDenominator = xml->getIntAttribute("fftSmoothingDenominator", 6);
            }
        }
        ensureDataFolderExists();
    }

    void saveSettings()
    {
        juce::XmlElement xml("MondoHelixAnalyzerSettings");
        xml.setAttribute("processedInputChannel", processedInputChannel.load());
        xml.setAttribute("playbackOutputChannel", playbackOutputChannel.load());
        xml.setAttribute("diInputChannel", diInputChannel.load());
        xml.setAttribute("dataFolderPath", dataFolderPath);
        xml.setAttribute("windowWidth", windowWidth);
        xml.setAttribute("windowHeight", windowHeight);
        xml.setAttribute("windowX", windowX);
        xml.setAttribute("windowY", windowY);
        xml.setAttribute("fftSmoothingDenominator", fftSmoothingDenominator.load());

        auto file = getSettingsFile();
        file.getParentDirectory().createDirectory();
        xml.writeTo(file);

        ensureDataFolderExists();
    }

    juce::File getSettingsFile() const
    {
        return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
               .getChildFile("MondoHelixAnalyzer")
               .getChildFile("settings.xml");
    }

    void ensureDataFolderExists()
    {
        juce::File folder (dataFolderPath);
        if (! folder.exists())
            folder.createDirectory();
            
        folder.getChildFile("DI_Tracks").createDirectory();
        folder.getChildFile("Analysis_Data").createDirectory();
        folder.getChildFile("GTR_Samples").createDirectory();
    }

    juce::File getDIFolder() const
    {
        return juce::File(dataFolderPath).getChildFile("DI_Tracks");
    }

    juce::File getGTRSamplesFolder() const
    {
        return juce::File(dataFolderPath).getChildFile("GTR_Samples");
    }

    // Atomics para leer seguramente desde el hilo de audio
    std::atomic<int> processedInputChannel { 1 }; 
    std::atomic<int> playbackOutputChannel { 7 };
    std::atomic<int> diInputChannel { 7 };
    std::atomic<int> fftSmoothingDenominator { 6 };
    
    // Window state
    int windowWidth, windowHeight, windowX, windowY;

    // String (solo se usa en el hilo de la GUI)
    juce::String dataFolderPath;
};
