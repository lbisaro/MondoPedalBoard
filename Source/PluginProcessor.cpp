#include "PluginProcessor.h"
#include "PluginEditor.h"

MondoHelixAnalyzerAudioProcessor::MondoHelixAnalyzerAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     .withInput  ("Input",  juce::AudioChannelSet::discreteChannels(8), true)
                     .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     )
#endif
{
}

MondoHelixAnalyzerAudioProcessor::~MondoHelixAnalyzerAudioProcessor()
{
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
    analyzer.prepare(sampleRate);

    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    
    // Asignar búfer para hasta 5 minutos de grabación DI mono en RAM
    diRecordBuffer.setSize (1, static_cast<int>(currentSampleRate * 60.0 * 5.0));
    diRecordBuffer.clear();
    diRecordSampleCount.store (0);
}

void MondoHelixAnalyzerAudioProcessor::releaseResources()
{
}

bool MondoHelixAnalyzerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Solo soportamos salida estéreo
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // Y exigimos 8 canales de entrada
    if (layouts.getMainInputChannels() != 8)
        return false;

    return true;
}

void MondoHelixAnalyzerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    int procIdx = settings.processedInputChannel.load() - 1; // 1-based to 0-based index
    int diIdx = settings.diInputChannel.load() - 1;
    int outIdx = settings.playbackOutputChannel.load() - 1;

    // Limpiar canales de salida que no tienen entrada correspondiente o que no sean el principal
    for (auto i = 0; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // 1. Procesamiento independiente de la señal DI
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

    // 2. Procesamiento del audio estéreo principal para el Analyzer
    if (procIdx >= 0 && procIdx + 1 < totalNumInputChannels)
    {
        // Pasamos el audio procesado al analyzer para las métricas
        analyzer.processBlock (buffer, procIdx, diIdx < totalNumInputChannels ? diIdx : procIdx);
    }

    // Pass-through básico de los canales procesados hacia la salida estéreo configurada
    if (totalNumInputChannels > procIdx + 1 && totalNumOutputChannels > outIdx + 1)
    {
        buffer.copyFrom(outIdx, 0, buffer, procIdx, 0, buffer.getNumSamples());
        buffer.copyFrom(outIdx + 1, 0, buffer, procIdx + 1, 0, buffer.getNumSamples());
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
