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

    int procIdx = settings.processedInputChannel.load() - 1; // 1-based to 0-based index
    int diIdx = settings.diInputChannel.load() - 1;
    int outIdx = settings.playbackOutputChannel.load() - 1;

    // Limpiar canales de salida que no tienen entrada correspondiente o que no sean el principal
    for (auto i = 0; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    bool hasProcessedAudio = false; 
    bool hasDryDI = false;          

    if (totalNumInputChannels > juce::jmax(procIdx + 1, diIdx))
    {
        auto* channelL = buffer.getReadPointer(procIdx);
        auto* channelR = buffer.getReadPointer(procIdx + 1);
        auto* channelDI = buffer.getReadPointer(diIdx);

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
        analyzer.processBlock (buffer, procIdx, diIdx);
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
