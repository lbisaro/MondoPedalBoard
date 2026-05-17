#pragma once
#include <JuceHeader.h>
#include "AnalysisEngine.h"
#include "GuitarEmulator.h"
#include "AppSettings.h"
#include "PinkNoiseGenerator.h"

class MondoHelixAnalyzerAudioProcessor : public juce::AudioProcessor
{
public:
    MondoHelixAnalyzerAudioProcessor();
    ~MondoHelixAnalyzerAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    AnalysisEngine analyzer;
    AppSettings settings;
    PinkNoiseGenerator pinkNoiseGen;
    GuitarStringSynth guitarSynth;
    CalibrationSequencer emulatorSequencer;
    
    std::atomic<bool> useInternalNoise { false };
    std::atomic<bool> useEmulator { false };
    std::atomic<bool> useLiveGuitar { false };
    std::atomic<bool> isShuttingDown { false };

    // =========================================================================
    // Sistema de Grabación de DI en RAM
    // =========================================================================
    enum class RecordingState { Stopped, Countdown, Recording };
    std::atomic<RecordingState> recordingState { RecordingState::Stopped };
    std::atomic<float> diInputLevel { 0.0f };

    void startDIRecording();
    void stopDIRecording();
    bool saveRecordedDI (const juce::String& fileName);

    // =========================================================================
    // Sistema de Reproducción de DI en Loop
    // =========================================================================
    void loadDIForPlayback(const juce::File& file);
    void playDI();
    void stopDI();
    void setEmulatorActive(bool active);
    bool isEmulatorActive() const { return useEmulator.load(); }
    void setLiveGuitarActive(bool active) { useLiveGuitar.store(active); }
    bool isLiveGuitarActive() const { return useLiveGuitar.load(); }
    bool isDIPlaying() const;
    
    float getDIPlaybackProgress() const
    {
        if (! isPlayingDI.load() || diPlaybackBuffer.getNumSamples() <= 0)
            return 0.0f;
        return static_cast<float>(diPlaybackPosition.load()) / static_cast<float>(diPlaybackBuffer.getNumSamples());
    }

    double getDITotalDurationSeconds() const
    {
        if (diPlaybackBuffer.getNumSamples() <= 0 || currentSampleRate <= 0.0)
            return 0.0;
        return static_cast<double>(diPlaybackBuffer.getNumSamples()) / currentSampleRate;
    }

private:
    juce::AudioBuffer<float> diPlaybackBuffer;
    std::atomic<int> diPlaybackPosition { 0 };
    std::atomic<bool> isPlayingDI { false };

    juce::AudioBuffer<float> diRecordBuffer;
    std::atomic<int> diRecordSampleCount { 0 };
    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MondoHelixAnalyzerAudioProcessor)
};
