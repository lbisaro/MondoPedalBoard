#include "PluginProcessor.h"
#include "PluginEditor.h"

#if defined(JucePlugin_Build_Standalone) && JucePlugin_Build_Standalone
 #include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#endif

#if JUCE_WINDOWS
 #include <windows.h>
 #include <setupapi.h>
 #include <devguid.h>
 #pragma comment(lib, "setupapi.lib")
#endif


MondoHelixAnalyzerAudioProcessor::MondoHelixAnalyzerAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     .withInput  ("Input",  juce::AudioChannelSet::discreteChannels(8), true)
                     .withOutput ("Output", juce::AudioChannelSet::discreteChannels(8), true)
                     ),
       emulatorSequencer(guitarSynth)
#endif
{
    startTimer (200);
}

MondoHelixAnalyzerAudioProcessor::~MondoHelixAnalyzerAudioProcessor()
{
    stopTimer();
    
    // Indicar al host que deje de llamar al processBlock inmediatamente
    suspendProcessing (true);
    
    isShuttingDown.store (true);
    analyzer.setShuttingDown (true);
    
#if defined(JucePlugin_Build_Standalone) && JucePlugin_Build_Standalone
    if (auto* holder = juce::StandalonePluginHolder::getInstance())
    {
        // 1. Guardar el estado del dispositivo de audio para recordar el driver de la Helix
        holder->saveAudioDeviceState();
        
        // 2. Desvincular el procesador del reproductor de audio
        holder->stopPlaying();
        
        // 3. Cerrar activamente el dispositivo de audio para detener el hilo del driver ASIO de forma segura
        holder->deviceManager.closeAudioDevice();
        
        // 4. Pequeña espera para asegurar que el hilo de audio del driver ASIO haya terminado completamente
        juce::Thread::sleep (100);
    }
#endif

    stopDI();
    stopDIRecording();
    
    // Espera extendida para garantizar que cualquier hilo de audio o driver ASIO
    // termine su ciclo actual antes de que los miembros de la clase sean destruidos.
    // 50ms es suficiente para cubrir incluso buffers muy grandes (p.ej. 2048 samples).
    juce::Thread::sleep (50);
}

const juce::String MondoHelixAnalyzerAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MondoHelixAnalyzerAudioProcessor::acceptsMidi() const { return false; }
bool MondoHelixAnalyzerAudioProcessor::producesMidi() const { return false; }
bool MondoHelixAnalyzerAudioProcessor::isMidiEffect() const { return false; }
double MondoHelixAnalyzerAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int MondoHelixAnalyzerAudioProcessor::getNumPrograms() { return 1; }
int MondoHelixAnalyzerAudioProcessor::getCurrentProgram() { return 0; }
void MondoHelixAnalyzerAudioProcessor::setCurrentProgram (int index) { juce::ignoreUnused(index); }
const juce::String MondoHelixAnalyzerAudioProcessor::getProgramName (int index) { juce::ignoreUnused(index); return {}; }
void MondoHelixAnalyzerAudioProcessor::changeProgramName (int index, const juce::String& newName) { juce::ignoreUnused(index, newName); }

void MondoHelixAnalyzerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    analyzer.prepare(currentSampleRate);
    guitarSynth.prepare(currentSampleRate);
    blockAnalyzer.prepare(currentSampleRate);
    
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = currentSampleRate;
    spec.maximumBlockSize = uint32_t(samplesPerBlock);
    spec.numChannels = 1;
    emulatorSequencer.prepare(spec);
    
    // Asignar búfer para hasta 5 minutos de grabación DI mono en RAM
    diRecordBuffer.setSize (1, static_cast<int>(currentSampleRate * 60.0 * 5.0));
    diRecordBuffer.clear();
    diRecordSampleCount.store (0);
    
    // Asignar búfer temporal para la copia limpia de la señal procesada del BlockAnalyzer
    blockAnalyzerInputBackup.setSize (1, samplesPerBlock);
    blockAnalyzerInputBackup.clear();
}

void MondoHelixAnalyzerAudioProcessor::releaseResources()
{
    // Detener de forma determinista todos los procesos antes de liberar el hardware
    isShuttingDown.store (true);
    analyzer.setShuttingDown (true);
    stopDI();
    stopDIRecording();
    analyzer.resetMetrics();
    blockAnalyzer.reset();
}

bool MondoHelixAnalyzerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    juce::ignoreUnused (layouts);
    // Permitimos flexibilidad de canales para soportar plenamente la interfaz 8-in / 8-out de la Helix
    return true;
}

void MondoHelixAnalyzerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    
    if (isShuttingDown.load())
    {
        buffer.clear();
        return;
    }

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = buffer.getNumChannels();
    auto totalNumOutputChannels = buffer.getNumChannels();

    int procIdx = settings.processedInputChannel.load() - 1; // 1-based to 0-based index
    int diIdx = settings.diInputChannel.load() - 1;
    int outIdx = settings.playbackOutputChannel.load() - 1;

    // Respaldar la señal de retorno procesada (procIdx) de la Helix antes de que sea borrada por el flujo de salida
    if (procIdx >= 0 && procIdx < totalNumInputChannels)
    {
        // Redimensionar por seguridad si el tamaño de bloque difiere dinámicamente
        if (blockAnalyzerInputBackup.getNumSamples() != buffer.getNumSamples())
            blockAnalyzerInputBackup.setSize (1, buffer.getNumSamples(), false, false, true);

        blockAnalyzerInputBackup.copyFrom (0, 0, buffer, procIdx, 0, buffer.getNumSamples());
    }
    else
    {
        blockAnalyzerInputBackup.clear();
    }

    // 1. Procesamiento de la señal DI de entrada (Lectura pura sin alterar el buffer)
    float maxDI = 0.0f;
    if (diIdx >= 0 && diIdx < totalNumInputChannels)
    {
        auto* channelDI = buffer.getReadPointer(diIdx);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            maxDI = juce::jmax(maxDI, std::abs(channelDI[sample]));
        }

        // Copiar muestras de la DI a memoria si la grabación está activa
        if (recordingState.load() == RecordingState::Recording)
        {
            int currentCount = diRecordSampleCount.load();
            int maxSamples = diRecordBuffer.getNumSamples();
            auto* writePtr = diRecordBuffer.getWritePointer(0);

            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            {
                if (currentCount < maxSamples)
                {
                    writePtr[currentCount] = channelDI[sample];
                    currentCount++;
                }
            }
            diRecordSampleCount.store (currentCount);
        }
    }
    diInputLevel.store (maxDI);

    // 2. Procesamiento del audio estéreo principal para el Analyzer (Lectura pura de la señal entrante)
    if (procIdx >= 0 && (procIdx + 1) < totalNumInputChannels)
    {
        int diSourceIdx = (diIdx >= 0 && diIdx < totalNumInputChannels) ? diIdx : procIdx;
        analyzer.processBlock (buffer, procIdx, diSourceIdx, isPlayingDI.load());
    }

    // 3. Preparación y ruteo hacia las salidas de Audio
    if (isPlayingDI.load())
    {
        // Limpiamos los canales de salida para evitar que audio residual de otros procesos interfiera.
        for (auto i = 0; i < totalNumOutputChannels; ++i)
            buffer.clear (i, 0, buffer.getNumSamples());

        // A. Inyección de señal hacia la Helix
        if (useLiveGuitar.load())
        {
            // Guitarra en vivo: señal física, sin inyección artificial.
        }
        else if (useEmulator.load())
        {
            if (outIdx >= 0 && outIdx < totalNumOutputChannels)
            {
                auto* writeDIL = buffer.getWritePointer(outIdx);
                auto* writeDIR = ((outIdx + 1) < totalNumOutputChannels) ? buffer.getWritePointer(outIdx + 1) : nullptr;

                for (int i = 0; i < buffer.getNumSamples(); ++i)
                {
                    float sample = emulatorSequencer.getNextSample(currentSampleRate);
                    writeDIL[i] = sample;
                    if (writeDIR != nullptr) writeDIR[i] = sample;
                }
            }
        }
        else if (diPlaybackBuffer.getNumSamples() > 0)
        {
            int numSamples = buffer.getNumSamples();
            int pbPos = diPlaybackPosition.load();
            int pbMax = diPlaybackBuffer.getNumSamples();

            if (outIdx >= 0 && outIdx < totalNumOutputChannels)
            {
                auto* writeDIL = buffer.getWritePointer(outIdx);
                auto* writeDIR = ((outIdx + 1) < totalNumOutputChannels) ? buffer.getWritePointer(outIdx + 1) : nullptr;
                auto* readPB = diPlaybackBuffer.getReadPointer(0);

                for (int i = 0; i < numSamples; ++i)
                {
                    float sample = readPB[pbPos];
                    writeDIL[i] = sample;
                    if (writeDIR != nullptr) writeDIR[i] = sample;

                    pbPos++;
                    if (pbPos >= pbMax) pbPos = 0;
                }
            }
            diPlaybackPosition.store(pbPos);
        }

        // B. Monitoreo: Pasar el retorno de la Helix (procIdx) a las salidas de escucha (0 y 1)
        // Esto permite que el usuario ESCUCHE el resultado del análisis en tiempo real.
        if (procIdx >= 0 && procIdx < totalNumInputChannels)
        {
            // Solo copiamos si los canales de salida 0 y 1 existen y no son los mismos que usamos para enviar DI
            if (totalNumOutputChannels >= 2)
            {
                if (outIdx != 0) buffer.copyFrom (0, 0, buffer.getReadPointer(procIdx), buffer.getNumSamples());
                if (outIdx != 1) 
                {
                    int srcR = (procIdx + 1 < totalNumInputChannels) ? procIdx + 1 : procIdx;
                    buffer.copyFrom (1, 0, buffer.getReadPointer(srcR), buffer.getNumSamples());
                }
            }
        }
    }
    else
    {
        // Silencio absoluto si no hay transporte activo
        for (auto i = 0; i < totalNumOutputChannels; ++i)
            buffer.clear (i, 0, buffer.getNumSamples());
    }

    // 4. ESS Block Analyzer:
    //    - Inject sweep samples into the Helix output channel
    //    - Capture the Helix return (procIdx) sample-by-sample in the engine
    if (isPlayingDI.load() && outIdx >= 0 && outIdx < totalNumOutputChannels)
    {
        const int numSamples = buffer.getNumSamples();
        auto* outL   = buffer.getWritePointer (outIdx);
        auto* outR   = ((outIdx + 1) < totalNumOutputChannels) ? buffer.getWritePointer (outIdx + 1) : nullptr;
        const auto* procPtr = blockAnalyzerInputBackup.getReadPointer (0);

        for (int i = 0; i < numSamples; ++i)
        {
            // Get the next sweep sample and write it to the Helix output
            float sweepSample = blockAnalyzer.getNextSweepSample();
            outL[i] = sweepSample;
            if (outR != nullptr) outR[i] = sweepSample;

            // Feed the Helix return sample into the capture engine (aligned to sweep)
            blockAnalyzer.captureSample (procPtr[i]);
        }
    }
    else if (! isPlayingDI.load())
    {
        blockAnalyzer.decay();
    }
}

bool MondoHelixAnalyzerAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* MondoHelixAnalyzerAudioProcessor::createEditor()
{
    return new MondoHelixAnalyzerAudioProcessorEditor (*this);
}

void MondoHelixAnalyzerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ignoreUnused(destData);
}

void MondoHelixAnalyzerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::ignoreUnused(data, sizeInBytes);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MondoHelixAnalyzerAudioProcessor();
}

// =============================================================================
// Control de Grabación DI
// =============================================================================
void MondoHelixAnalyzerAudioProcessor::startDIRecording()
{
    diRecordBuffer.clear();
    diRecordSampleCount.store (0);
    recordingState.store (RecordingState::Recording);
}

void MondoHelixAnalyzerAudioProcessor::stopDIRecording()
{
    recordingState.store (RecordingState::Stopped);
}

bool MondoHelixAnalyzerAudioProcessor::saveRecordedDI (const juce::String& fileName)
{
    int totalSamples = diRecordSampleCount.load();
    if (totalSamples <= 0)
        return false;

    // Aplicar Fade In y Fade Out lineales de 30ms para evitar clics de bucle
    int fadeLen = static_cast<int>(currentSampleRate * 0.03);
    fadeLen = juce::jmin (fadeLen, totalSamples / 2);

    auto* samples = diRecordBuffer.getWritePointer(0);

    // Fade In lineal
    for (int i = 0; i < fadeLen; ++i)
    {
        float factor = static_cast<float>(i) / static_cast<float>(fadeLen);
        samples[i] *= factor;
    }

    // Fade Out lineal
    for (int i = 0; i < fadeLen; ++i)
    {
        int sampleIdx = totalSamples - 1 - i;
        float factor = static_cast<float>(i) / static_cast<float>(fadeLen);
        samples[sampleIdx] *= factor;
    }

    // Limpiar y asegurar extensión .wav
    juce::String cleanName = fileName.trim();
    if (cleanName.isEmpty())
        cleanName = "Guitar_DI_Track";
        
    if (! cleanName.endsWithIgnoreCase(".wav"))
        cleanName += ".wav";

    juce::File targetFile = settings.getDIFolder().getChildFile(cleanName);
    
    // Usamos la clase de formato WAV nativa para escribir el archivo
    juce::WavAudioFormat wavFormat;
    if (auto* writer = wavFormat.createWriterFor (new juce::FileOutputStream(targetFile),
                                                  currentSampleRate,
                                                  1,  // 1 canal (Mono)
                                                  24, // 24 bits
                                                  {}, 0))
    {
        std::unique_ptr<juce::AudioFormatWriter> scopedWriter(writer);
        bool success = scopedWriter->writeFromAudioSampleBuffer (diRecordBuffer, 0, totalSamples);
        return success;
    }

    return false;
}

void MondoHelixAnalyzerAudioProcessor::loadDIForPlayback(const juce::File& file)
{
    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::AudioFormatReader> reader(wavFormat.createReaderFor(new juce::FileInputStream(file), true));
    
    if (reader != nullptr)
    {
        diPlaybackBuffer.setSize(1, static_cast<int>(reader->lengthInSamples));
        reader->read(&diPlaybackBuffer, 0, static_cast<int>(reader->lengthInSamples), 0, true, false);
    }
}

void MondoHelixAnalyzerAudioProcessor::playDI()
{
    diPlaybackPosition.store (0);
    if (useEmulator.load())
        emulatorSequencer.start();
        
    isPlayingDI.store (true);
}

void MondoHelixAnalyzerAudioProcessor::stopDI()
{
    isPlayingDI.store(false);
    emulatorSequencer.stop();
}

void MondoHelixAnalyzerAudioProcessor::setEmulatorActive(bool active)
{
    useEmulator.store(active);
    if (!active) emulatorSequencer.stop();
}

bool MondoHelixAnalyzerAudioProcessor::isDIPlaying() const
{
    return isPlayingDI.load();
}

// =============================================================================
// Watchdog de la Helix (Evita cuelgues de Windows por desconexión de ASIO)
// =============================================================================

bool MondoHelixAnalyzerAudioProcessor::isHelixHardwareConnected()
{
#if JUCE_WINDOWS
    // 1. Direct hardware enumeration using Windows SetupAPI
    bool foundInSetupAPI = false;
    HDEVINFO hDevInfo = SetupDiGetClassDevs(NULL, NULL, NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT);
    if (hDevInfo != INVALID_HANDLE_VALUE)
    {
        SP_DEVINFO_DATA deviceInfoData;
        deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &deviceInfoData); i++)
        {
            char buffer[512];
            DWORD dataType;
            DWORD actualSize = 0;
            
            // Query Device Description
            if (SetupDiGetDeviceRegistryPropertyA(hDevInfo, &deviceInfoData, SPDRP_DEVICEDESC, 
                                                   &dataType, (BYTE*)buffer, sizeof(buffer), &actualSize))
            {
                juce::String desc(buffer);
                if (desc.containsIgnoreCase("Helix") || desc.containsIgnoreCase("Line 6") || desc.containsIgnoreCase("Line6"))
                {
                    foundInSetupAPI = true;
                    break;
                }
            }

            // Query Friendly Name
            if (SetupDiGetDeviceRegistryPropertyA(hDevInfo, &deviceInfoData, SPDRP_FRIENDLYNAME, 
                                                   &dataType, (BYTE*)buffer, sizeof(buffer), &actualSize))
            {
                juce::String friendly(buffer);
                if (friendly.containsIgnoreCase("Helix") || friendly.containsIgnoreCase("Line 6") || friendly.containsIgnoreCase("Line6"))
                {
                    foundInSetupAPI = true;
                    break;
                }
            }
        }
        SetupDiDestroyDeviceInfoList(hDevInfo);
    }

    if (foundInSetupAPI)
        return true;

    // 2. Fallback check: Midi Input Devices
    for (auto& info : juce::MidiInput::getAvailableDevices())
    {
        if (info.name.containsIgnoreCase("Helix") || info.name.containsIgnoreCase("Line 6") || info.name.containsIgnoreCase("Line6"))
            return true;
    }

    // 3. Fallback check: Midi Output Devices
    for (auto& info : juce::MidiOutput::getAvailableDevices())
    {
        if (info.name.containsIgnoreCase("Helix") || info.name.containsIgnoreCase("Line 6") || info.name.containsIgnoreCase("Line6"))
            return true;
    }

    return false;
#else
    return true; // Graceful on other systems
#endif
}

void MondoHelixAnalyzerAudioProcessor::handleHelixDisconnection()
{
    // Suspend audio processing to prevent the audio thread from calling empty/crashed callbacks
    suspendProcessing(true);
    
    // Stop DI playback and recorders
    stopDI();
    stopDIRecording();

#if defined(JucePlugin_Build_Standalone) && JucePlugin_Build_Standalone
    // Safe closing of the audio device immediately, before the driver thread freezes the system
    if (auto* holder = juce::StandalonePluginHolder::getInstance())
    {
        holder->stopPlaying();
        holder->deviceManager.closeAudioDevice();
    }
#endif

    // Log to console for debugging
    DBG("Helix Watchdog: Helix disconnected! Audio device closed safely.");

    // Warn the user with an asynchronous message box so it does not block the thread
    juce::MessageManager::callAsync([]()
    {
        juce::AlertWindow::showMessageBoxAsync (
            juce::AlertWindow::WarningIcon,
            "Helix Desconectada",
            "Se ha detectado la desconexion o apagado de la Line 6 Helix.\n\n"
            "El dispositivo de audio se ha cerrado de forma segura para evitar un cuelgue del sistema (tildado o BSOD de Windows).\n\n"
            "La aplicacion intentara reconectarse automaticamente en cuanto vuelvas a encender o conectar la Helix por USB.",
            "Entendido"
        );
    });
}

void MondoHelixAnalyzerAudioProcessor::handleHelixReconnection()
{
    DBG("Helix Watchdog: Helix reconnected! Attempting auto-reconnection...");

    // Wait a brief moment to let the Windows PnP service and drivers initialize fully
    juce::Thread::sleep(2000);

#if defined(JucePlugin_Build_Standalone) && JucePlugin_Build_Standalone
    if (auto* holder = juce::StandalonePluginHolder::getInstance())
    {
        juce::String helixDeviceName;
        juce::AudioIODeviceType* targetType = nullptr;

        // Scan all device types for the Helix
        for (auto* type : holder->deviceManager.getAvailableDeviceTypes())
        {
            type->scanForDevices();
            auto devices = type->getDeviceNames();
            for (auto& d : devices)
            {
                if (d.containsIgnoreCase("Helix") || d.containsIgnoreCase("Line 6") || d.containsIgnoreCase("Line6"))
                {
                    helixDeviceName = d;
                    targetType = type;
                    break;
                }
            }
            if (targetType != nullptr)
                break;
        }

        if (targetType != nullptr && helixDeviceName.isNotEmpty())
        {
            // Change the type first if necessary
            holder->deviceManager.setCurrentAudioDeviceType(targetType->getTypeName(), true);

            // Re-apply setup
            juce::AudioDeviceManager::AudioDeviceSetup setup;
            holder->deviceManager.getAudioDeviceSetup(setup);
            setup.inputDeviceName = helixDeviceName;
            setup.outputDeviceName = helixDeviceName;
            setup.useDefaultInputChannels = true;
            setup.useDefaultOutputChannels = true;
            
            juce::String error = holder->deviceManager.setAudioDeviceSetup(setup, true);
            if (error.isEmpty())
            {
                holder->startPlaying();
                suspendProcessing(false);

                DBG("Helix Watchdog: Reconnection successful!");

                juce::MessageManager::callAsync([]()
                {
                    juce::AlertWindow::showMessageBoxAsync (
                        juce::AlertWindow::InfoIcon,
                        "Helix Reconectada",
                        "¡Se ha detectado y reactivado la Line 6 Helix con exito!\n\n"
                        "El motor de audio ha sido restaurado y las entradas/salidas estan listas.",
                        "¡Genial!"
                    );
                });
                return;
            }
            else
            {
                DBG("Helix Watchdog: Reconnection failed with error: " + error);
            }
        }
    }
#endif

    // If we failed to reconnect, set state back to Disconnected so we will retry next tick
    lastHelixState = HelixState::Disconnected;
}

void MondoHelixAnalyzerAudioProcessor::timerCallback()
{
    if (isShuttingDown.load())
        return;

#if defined(JucePlugin_Build_Standalone) && JucePlugin_Build_Standalone
    if (auto* holder = juce::StandalonePluginHolder::getInstance())
    {
        auto* currentDevice = holder->deviceManager.getCurrentAudioDevice();
        juce::String deviceName = (currentDevice != nullptr) ? currentDevice->getName() : "";
        
        bool isCurrentlyHelix = deviceName.containsIgnoreCase("Helix") || 
                                deviceName.containsIgnoreCase("Line 6") || 
                                deviceName.containsIgnoreCase("Line6");
        
        if (isCurrentlyHelix)
        {
            wasUsingHelix = true;
            
            // Watch if it gets disconnected
            if (lastHelixState == HelixState::Connected && ! isHelixHardwareConnected())
            {
                lastHelixState = HelixState::Disconnected;
                handleHelixDisconnection();
            }
        }
        else
        {
            // If we were using the Helix and it is in disconnected state, check if we can reconnect
            if (wasUsingHelix && lastHelixState == HelixState::Disconnected)
            {
                if (isHelixHardwareConnected())
                {
                    lastHelixState = HelixState::Connected;
                    handleHelixReconnection();
                }
            }
        }
    }
#endif
}

