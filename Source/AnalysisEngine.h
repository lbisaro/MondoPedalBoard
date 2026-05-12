#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <array>

// Implementación simplificada de métricas LUFS (EBU R128)
class LufsMeter
{
public:
    LufsMeter() = default;

    void prepare (double sampleRate)
    {
        juce::dsp::ProcessSpec spec { sampleRate, 2048, 2 };
        
        // Filtro K-Weighting 1: High Shelf
        preFilter.state = juce::dsp::IIR::Coefficients<float>::makeHighShelf (sampleRate, 1681.97f, 0.7071f, juce::Decibels::decibelsToGain(4.0f));
        preFilter.prepare (spec);

        // Filtro K-Weighting 2: High Pass
        highPass.state = juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, 38.1354f, 0.5003f);
        highPass.prepare (spec);

        momentaryBufferSize = static_cast<int> (sampleRate * 0.4); // 400ms
        shortTermBufferSize = static_cast<int> (sampleRate * 3.0); // 3s
        buffer.setSize (1, shortTermBufferSize);
        buffer.clear();
        writeIndex = 0;
        
        momentaryLUFS = -70.0f;
        shortTermLUFS = -70.0f;
    }

    void process (const juce::AudioBuffer<float>& input)
    {
        if (momentaryBufferSize == 0 || shortTermBufferSize == 0 || input.getNumChannels() == 0) return;

        juce::AudioBuffer<float> copy;
        copy.makeCopyOf (input); 
        if (copy.getNumChannels() > 2) copy.setSize(2, copy.getNumSamples(), true, true, true);
        
        juce::dsp::AudioBlock<float> block (copy);
        juce::dsp::ProcessContextReplacing<float> context (block);
        
        preFilter.process (context);
        highPass.process (context);

        int numSamples = copy.getNumSamples();
        int numChannels = copy.getNumChannels();

        for (int i = 0; i < numSamples; ++i)
        {
            float sumSq = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float sample = copy.getSample (ch, i);
                sumSq += sample * sample;
            }
            
            buffer.setSample (0, writeIndex, sumSq); 
            writeIndex = (writeIndex + 1) % shortTermBufferSize;
        }

        // Calcular Momentary (400ms)
        float sumM = 0.0f;
        int mSamples = juce::jmin (momentaryBufferSize, shortTermBufferSize);
        for(int i = 0; i < mSamples; ++i)
        {
            int idx = (writeIndex - 1 - i + shortTermBufferSize) % shortTermBufferSize;
            sumM += buffer.getSample (0, idx);
        }
        momentaryLUFS = -0.691f + 10.0f * std::log10 (std::max (sumM / static_cast<float>(mSamples), 1e-10f));

        // Calcular Short Term (3s)
        float sumS = 0.0f;
        for(int i = 0; i < shortTermBufferSize; ++i)
            sumS += buffer.getSample (0, i);
            
        shortTermLUFS = -0.691f + 10.0f * std::log10 (std::max (sumS / static_cast<float>(shortTermBufferSize), 1e-10f));
    }

    float getMomentaryLUFS() const { return momentaryLUFS; }
    float getShortTermLUFS() const { return shortTermLUFS; }

private:
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> preFilter;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> highPass;
    
    juce::AudioBuffer<float> buffer;
    int writeIndex = 0;
    int momentaryBufferSize = 0;
    int shortTermBufferSize = 0;

    float momentaryLUFS = -70.0f;
    float shortTermLUFS = -70.0f;
};

// Motor de Análisis (Espectral y Dinámico)
class AnalysisEngine
{
public:
    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr int numBins = fftSize / 2;

    AnalysisEngine() : forwardFFT (fftOrder), window (fftSize, juce::dsp::WindowingFunction<float>::hann)
    {
    }

    void prepare (double sampleRate)
    {
        currentSampleRate = sampleRate;
        lufsMeter.prepare (sampleRate);
        fifoIndex = 0;
        nextFFTBlockReady = false;
        std::fill (fftData.begin(), fftData.end(), 0.0f);
        std::fill (fifo.begin(), fifo.end(), 0.0f);
        std::fill (latestFFTData.begin(), latestFFTData.end(), 0.0f);
    }

    void processBlock (const juce::AudioBuffer<float>& buffer)
    {
        // 1. Dinámica (True Peak, LUFS, PLR)
        float currentPeak = 0.0f;
        for (int ch = 0; ch < juce::jmin(2, buffer.getNumChannels()); ++ch)
        {
            currentPeak = juce::jmax (currentPeak, buffer.getMagnitude (ch, 0, buffer.getNumSamples()));
        }
        
        float peakDb = juce::Decibels::gainToDecibels (currentPeak, -120.0f);
        
        lufsMeter.process (buffer);
        
        float currentMomentary = lufsMeter.getMomentaryLUFS();
        float currentShortTerm = lufsMeter.getShortTermLUFS();
        float plr = peakDb - currentShortTerm; 
        
        momentaryLUFS.store (currentMomentary);
        shortTermLUFS.store (currentShortTerm);
        currentPLR.store (plr);

        // 2. Análisis Espectral (FFT)
        if (buffer.getNumChannels() > 0)
        {
            auto* channelData = buffer.getReadPointer (0); // Analizamos canal L (procesado)
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                pushNextSampleIntoFifo (channelData[i]);
        }

        // 3. Clasificación Básica
        float centroid = spectralCentroid.load();
        float flatness = spectralFlatness.load();
        
        int category = 0; // Warm
        if (centroid > 2000.0f && plr > 12.0f) {
            category = 2; // Lead
        } else if (flatness > 0.05f && plr < 10.0f) {
            category = 1; // Rhythm
        }
        currentCategory.store (category);
    }

    void pushNextSampleIntoFifo (float sample) noexcept
    {
        if (fifoIndex == fftSize)
        {
            if (! nextFFTBlockReady)
            {
                std::fill (fftData.begin(), fftData.end(), 0.0f);
                std::copy (fifo.begin(), fifo.end(), fftData.begin());
                nextFFTBlockReady = true;
                calculateSpectralMetrics();
                nextFFTBlockReady = false; // Liberamos para el siguiente bloque
            }
            fifoIndex = 0;
        }
        fifo[fifoIndex++] = sample;
    }

    void calculateSpectralMetrics()
    {
        window.multiplyWithWindowingTable (fftData.data(), fftSize);
        forwardFFT.performFrequencyOnlyForwardTransform (fftData.data());
        
        float nyquist = static_cast<float> (currentSampleRate) / 2.0f;
        
        float sumMagnitude = 0.0f;
        float sumWeighted = 0.0f;
        float logSum = 0.0f;
        
        for (int i = 0; i < numBins; ++i)
        {
            float magnitude = fftData[i];
            float freq = (static_cast<float>(i) / static_cast<float>(numBins)) * nyquist;
            
            sumMagnitude += magnitude;
            sumWeighted += magnitude * freq;
            logSum += std::log (std::max (magnitude, 1e-10f));
        }
        
        // Spectral Centroid
        float centroid = sumMagnitude > 0.0f ? sumWeighted / sumMagnitude : 0.0f;
        spectralCentroid.store (centroid);
        
        // Spectral Flatness
        float geometricMean = std::exp (logSum / static_cast<float>(numBins));
        float arithmeticMean = sumMagnitude / static_cast<float>(numBins);
        float flatness = arithmeticMean > 0.0f ? geometricMean / arithmeticMean : 0.0f;
        spectralFlatness.store (flatness);

        // Copiamos datos listos para el UI
        std::copy(fftData.begin(), fftData.begin() + numBins, latestFFTData.begin());
        fftDataReady.store(true);
    }

    // Datos exportables para el UI (Lock-Free)
    std::atomic<float> momentaryLUFS { -70.0f };
    std::atomic<float> shortTermLUFS { -70.0f };
    std::atomic<float> currentPLR { 0.0f };
    std::atomic<float> spectralCentroid { 0.0f };
    std::atomic<float> spectralFlatness { 0.0f };
    std::atomic<int> currentCategory { 0 }; // 0: Warm, 1: Rhythm, 2: Lead

    std::array<float, numBins> latestFFTData;
    std::atomic<bool> fftDataReady { false };

private:
    double currentSampleRate = 44100.0;
    juce::dsp::FFT forwardFFT;
    juce::dsp::WindowingFunction<float> window;
    
    std::array<float, fftSize> fifo;
    std::array<float, 2 * fftSize> fftData;
    int fifoIndex = 0;
    bool nextFFTBlockReady = false;

    LufsMeter lufsMeter;
};
