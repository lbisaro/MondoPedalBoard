#pragma once
#include <JuceHeader.h>
#include <atomic>

class AppSettings
{
public:
    AppSettings()
    {
        // Valores por defecto (1-based index para que sea entendible por el humano)
        processedInputChannel = 3; // Escucha 3/4
        playbackOutputChannel = 1; // Reproduce por 1/2
        diInputChannel = 7;        // Escucha DI por 7

        dataFolderPath = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                            .getChildFile("MondoHelixAnalyzer").getFullPathName();

        loadSettings();
    }

    void loadSettings()
    {
        auto file = getSettingsFile();
        if (file.existsAsFile())
        {
            if (auto xml = juce::XmlDocument::parse(file))
            {
                processedInputChannel = xml->getIntAttribute("processedInputChannel", processedInputChannel);
                playbackOutputChannel = xml->getIntAttribute("playbackOutputChannel", playbackOutputChannel);
                diInputChannel = xml->getIntAttribute("diInputChannel", diInputChannel);
                dataFolderPath = xml->getStringAttribute("dataFolderPath", dataFolderPath);
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
    }

    // Atomics para leer seguramente desde el hilo de audio
    std::atomic<int> processedInputChannel { 3 }; 
    std::atomic<int> playbackOutputChannel { 1 };
    std::atomic<int> diInputChannel { 7 };
    
    // String (solo se usa en el hilo de la GUI)
    juce::String dataFolderPath;
};
