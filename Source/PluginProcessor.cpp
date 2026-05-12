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

    // Limpiar canales de salida que no tienen entrada correspondiente
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    bool hasProcessedAudio = false; // Helix Channels 1/2 (índices 0/1)
    bool hasDryDI = false;          // Helix Channel 7 (índice 6)

    if (totalNumInputChannels >= 8)
    {
        auto* channelL = buffer.getReadPointer(0);
        auto* channelR = buffer.getReadPointer(1);
        auto* channelDI = buffer.getReadPointer(6);

        float maxProcessed = 0.0f;
        float maxDI = 0.0f;

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            maxProcessed = juce::jmax(maxProcessed, std::abs(channelL[sample]), std::abs(channelR[sample]));
            maxDI = juce::jmax(maxDI, std::abs(channelDI[sample]));
        }

        // Detección simple para ver si entra señal
        if (maxProcessed > 0.0001f) hasProcessedAudio = true;
        if (maxDI > 0.0001f) hasDryDI = true;

        // Pasamos el audio procesado al analyzer para las métricas
        analyzer.processBlock (buffer);
    }

    // Pass-through básico de los canales procesados (1 y 2) hacia la salida estéreo
    if (totalNumInputChannels >= 2 && totalNumOutputChannels >= 2)
    {
        // Limpiamos los canales a partir del 2 para que no suenen en el bus de salida
        for (int i = 2; i < totalNumInputChannels; ++i)
        {
            buffer.clear(i, 0, buffer.getNumSamples());
        }
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
