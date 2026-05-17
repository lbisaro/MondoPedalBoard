#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <array>
#include <vector>
#include <algorithm>

// Implementación simplificada de métricas LUFS (EBU R128) con Gating y Métricas de Rango
class LufsMeter
{
public:
    LufsMeter() = default;

    void prepare (double sampleRate)
    {
        currentSampleRate = sampleRate;
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
        
        hopSizeBlock = static_cast<int> (sampleRate * 0.1); // 100ms superposición
        hopSizeShortTerm = static_cast<int> (sampleRate * 1.0); // 1s superposición
        
        reset();
    }

    void process (const juce::AudioBuffer<float>& input)
    {
        if (momentaryBufferSize == 0 || shortTermBufferSize == 0 || input.getNumChannels() == 0) return;

        juce::ScopedLock sl (lock);
        
        sidechainBuffer.makeCopyOf (input, true); 
        if (sidechainBuffer.getNumChannels() > 2) sidechainBuffer.setSize(2, sidechainBuffer.getNumSamples(), true, true, true);
        
        juce::dsp::AudioBlock<float> block (sidechainBuffer);
        juce::dsp::ProcessContextReplacing<float> context (block);
        
        preFilter.process (context);
        highPass.process (context);

        int numSamples = sidechainBuffer.getNumSamples();
        int numChannels = sidechainBuffer.getNumChannels();

        for (int i = 0; i < numSamples; ++i)
        {
            float sumSq = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float sample = sidechainBuffer.getSample (ch, i);
                sumSq += sample * sample;
            }
            
            buffer.setSample (0, writeIndex, sumSq); 
            writeIndex = (writeIndex + 1) % shortTermBufferSize;

            // Disparo de bloques Momentary (400ms) cada 100ms para acumulación EBU R128 Integrada
            sampleCounterForBlock++;
            if (sampleCounterForBlock >= hopSizeBlock)
            {
                sampleCounterForBlock = 0;
                
                double blockEnergy = 0.0;
                int mSamples = juce::jmin (momentaryBufferSize, shortTermBufferSize);
                for (int k = 0; k < mSamples; ++k)
                {
                    int idx = (writeIndex - 1 - k + shortTermBufferSize) % shortTermBufferSize;
                    blockEnergy += buffer.getSample (0, idx);
                }
                blockEnergy /= static_cast<double>(mSamples);
                
                // Gating Absoluto: Solo contabilizar bloques por encima de -70 LUFS
                double blockLufs = -0.691 + 10.0 * std::log10 (std::max (blockEnergy, 1e-10));
                if (blockLufs >= -70.0)
                {
                    blockEnergies.push_back (blockEnergy);
                }
            }

            // Disparo de bloques Short-Term (3s) cada segundo para historia de Loudness Range (LRA)
            sampleCounterForShortTerm++;
            if (sampleCounterForShortTerm >= hopSizeShortTerm)
            {
                sampleCounterForShortTerm = 0;
                
                double stEnergy = 0.0;
                for (int k = 0; k < shortTermBufferSize; ++k)
                    stEnergy += buffer.getSample (0, k);
                stEnergy /= static_cast<double>(shortTermBufferSize);
                
                double stLufs = -0.691 + 10.0 * std::log10 (std::max (stEnergy, 1e-10));
                if (stLufs >= -70.0)
                {
                    shortTermHistory.push_back (static_cast<float>(stLufs));
                }
            }
        }

        // Calcular Momentary Instantáneo (400ms)
        float sumM = 0.0f;
        int mSamples = juce::jmin (momentaryBufferSize, shortTermBufferSize);
        for (int i = 0; i < mSamples; ++i)
        {
            int idx = (writeIndex - 1 - i + shortTermBufferSize) % shortTermBufferSize;
            sumM += buffer.getSample (0, idx);
        }
        momentaryLUFS = -0.691f + 10.0f * std::log10 (std::max (sumM / static_cast<float>(mSamples), 1e-10f));

        // Calcular Short Term Instantáneo (3s)
        float sumS = 0.0f;
        for (int i = 0; i < shortTermBufferSize; ++i)
            sumS += buffer.getSample (0, i);
            
        shortTermLUFS = -0.691f + 10.0f * std::log10 (std::max (sumS / static_cast<float>(shortTermBufferSize), 1e-10f));

        // Calcular Integrated Loudness (EBU R128 con Gating Absoluto y Relativo)
        if (! blockEnergies.empty())
        {
            // 1. Calcular promedio de energía temporal post-Gating absoluto
            double absSum = 0.0;
            for (double e : blockEnergies)
                absSum += e;
            double absAvg = absSum / blockEnergies.size();
            double absLufs = -0.691 + 10.0 * std::log10 (std::max (absAvg, 1e-10));
            
            // 2. Gating Relativo: Conservar solo los bloques por encima de (absLufs - 10.0 LU)
            double relThresholdLufs = absLufs - 10.0;
            double relThresholdEnergy = std::pow (10.0, (relThresholdLufs + 0.691) / 10.0);
            
            double relSum = 0.0;
            int relCount = 0;
            for (double e : blockEnergies)
            {
                if (e >= relThresholdEnergy)
                {
                    relSum += e;
                    relCount++;
                }
            }
            
            if (relCount > 0)
            {
                double relAvg = relSum / relCount;
                integratedLUFS = static_cast<float> (-0.691 + 10.0 * std::log10 (std::max (relAvg, 1e-10)));
            }
            else
            {
                integratedLUFS = static_cast<float> (absLufs);
            }
            if (integratedLUFS < -70.0f)
                integratedLUFS = -70.0f;
        }
    }

    float getLoudnessRange() const
    {
        juce::ScopedLock sl (lock);
        if (shortTermHistory.size() < 2) return 0.0f;
        std::vector<float> sorted = shortTermHistory;
        std::sort (sorted.begin(), sorted.end());
        
        size_t idxLow = static_cast<size_t> (sorted.size() * 0.10f);
        size_t idxHigh = static_cast<size_t> (sorted.size() * 0.95f);
        if (idxHigh >= sorted.size()) idxHigh = sorted.size() - 1;
        
        float lra = sorted[idxHigh] - sorted[idxLow];
        return std::max (lra, 0.0f);
    }

    void reset()
    {
        juce::ScopedLock sl (lock);
        buffer.clear();
        writeIndex = 0;
        sampleCounterForBlock = 0;
        sampleCounterForShortTerm = 0;
        blockEnergies.clear();
        shortTermHistory.clear();
        momentaryLUFS = -70.0f;
        shortTermLUFS = -70.0f;
        integratedLUFS = -70.0f;
    }

    float getMomentaryLUFS() const { return momentaryLUFS; }
    float getShortTermLUFS() const { return shortTermLUFS; }
    float getIntegratedLUFS() const { return integratedLUFS; }

private:
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> preFilter;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> highPass;
    
    juce::AudioBuffer<float> buffer;
    juce::AudioBuffer<float> sidechainBuffer;
    juce::CriticalSection lock;

    int writeIndex = 0;
    int momentaryBufferSize = 0;
    int shortTermBufferSize = 0;

    double currentSampleRate = 44100.0;
    int hopSizeBlock = 0;
    int hopSizeShortTerm = 0;
    int sampleCounterForBlock = 0;
    int sampleCounterForShortTerm = 0;
    
    std::vector<double> blockEnergies;
    std::vector<float> shortTermHistory;

    float momentaryLUFS = -70.0f;
    float shortTermLUFS = -70.0f;
    float integratedLUFS = -70.0f;
};

enum class TargetType { Ambient = 0, Rhythm, Lead };

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

    void setShuttingDown (bool shuttingDown) { isShuttingDown.store (shuttingDown); }

    void resetMetrics()
    {
        lufsMeter.reset();
        momentaryLUFS.store (-70.0f);
        shortTermLUFS.store (-70.0f);
        integratedLUFS.store (-70.0f);
        currentPLR.store (0.0f);
        globalMaxPeakDb.store (-120.0f);
        
        std::fill (shortTermPeakHistory.begin(), shortTermPeakHistory.end(), -120.0f);
        shortTermPeakIndex = 0;
        currentHopMaxPeakDb = -120.0f;
        samplesInHop = 0;
        
        smoothedCentroidHz = 0.0f;
        accumulatedCentroidHz = 0.0;
        accumulatedBodyRatio = 0.0;
        accumulatedCutRatio = 0.0;
        accumulatedBrilloRatio = 0.0;
        centroidCount = 0;
        
        averageCentroidHz.store (0.0f);
        averageBodyRatio.store (0.0f);
        averageCutRatio.store (0.0f);
        brilloEnergyRatio.store (0.0f);
        averageBrilloRatio.store (0.0f);
    }

    void prepare (double sampleRate)
    {
        currentSampleRate = sampleRate;
        lufsMeter.prepare (sampleRate);
        
        // Sobremuestreo 4x (factor = 2) para cumplir con la norma ITU-R BS.1770-4
        oversampling = std::make_unique<juce::dsp::Oversampling<float>> (2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true);
        oversampling->initProcessing (2048);
        
        // Tiempo de suavizado para el seguidor de envolvente en caída (~20ms)
        decayAlpha = static_cast<float> (std::exp (-1.0 / (0.02 * sampleRate)));
        
        fifoIndex = 0;
        nextFFTBlockReady = false;
        std::fill (fftData.begin(), fftData.end(), 0.0f);
        std::fill (fifo.begin(), fifo.end(), 0.0f);
        std::fill (latestFFTData.begin(), latestFFTData.end(), 0.0f);
        resetMetrics();
    }

    void processBlock (const juce::AudioBuffer<float>& buffer, int procIdx, int diIdx, bool isPlaying)
    {
        if (! isPlaying || isShuttingDown.load()) return;

        juce::ignoreUnused (diIdx);

        int numSamples = buffer.getNumSamples();
        if (numSamples <= 0) return;
        
        if (procBuffer.getNumSamples() != numSamples)
            procBuffer.setSize (2, numSamples, false, false, true);

        procBuffer.clear();
        
        if (buffer.getNumChannels() > procIdx)
            procBuffer.copyFrom (0, 0, buffer, procIdx, 0, numSamples);
        if (buffer.getNumChannels() > procIdx + 1)
            procBuffer.copyFrom (1, 0, buffer, procIdx + 1, 0, numSamples);
        else if (buffer.getNumChannels() > procIdx)
            procBuffer.copyFrom (1, 0, buffer, procIdx, 0, numSamples); // Estéreo si es mono

        // 1. Dinámica: Detección de True Peak con sobremuestreo 4x (norma ITU-R BS.1770-4)
        float currentPeak = 0.0f;
        
        if (auto* os = oversampling.get())
        {
            juce::dsp::AudioBlock<float> inputBlock (procBuffer);
            auto oversampledBlock = os->processSamplesUp (inputBlock);
            
            for (size_t ch = 0; ch < oversampledBlock.getNumChannels(); ++ch)
            {
                auto* data = oversampledBlock.getChannelPointer (ch);
                for (size_t i = 0; i < oversampledBlock.getNumSamples(); ++i)
                {
                    float mag = std::abs (data[i]);
                    if (mag > currentPeak)
                        currentPeak = mag;
                }
            }
            
            os->processSamplesDown (inputBlock);
        }
        else
        {
            currentPeak = juce::jmax (procBuffer.getMagnitude (0, 0, numSamples),
                                      procBuffer.getMagnitude (1, 0, numSamples));
        }
            
        float peakDb = juce::Decibels::gainToDecibels (currentPeak, -120.0f);
        
        // Procesar sonoridad LUFS sobre los canales seleccionados
        lufsMeter.process (procBuffer);
        
        float currentMomentary = lufsMeter.getMomentaryLUFS();
        float currentShortTerm = lufsMeter.getShortTermLUFS();
        float currentIntegrated = lufsMeter.getIntegratedLUFS();
        
        // Garantizar el gate de silencio de -70 LUFS acumulado para el loop DI
        if (currentIntegrated < -70.0f)
            currentIntegrated = -70.0f;
        
        // Almacenar valores de LUFS
        momentaryLUFS.store (currentMomentary);
        shortTermLUFS.store (currentShortTerm);
        integratedLUFS.store (std::max(-70.0f, currentIntegrated));

        // Actualizar Pico Máximo Global para PLR Integrado
        if (peakDb > globalMaxPeakDb.load())
            globalMaxPeakDb.store (peakDb);

        // Calcular PLR Integrado (Pico Máximo Global - Integrated LUFS)
        if (currentIntegrated > -65.0f)
        {
            float integratedPlr = std::max (0.0f, globalMaxPeakDb.load() - currentIntegrated);
            // Mantenemos un suavizado leve para la transición inicial
            float smoothedPlr = 0.9f * currentPLR.load() + 0.1f * integratedPlr;
            currentPLR.store (smoothedPlr);
        }
        else
        {
            currentPLR.store (0.0f);
        }

        // 2. Análisis Espectral (FFT)
        if (buffer.getNumChannels() > procIdx)
        {
            auto* channelData = buffer.getReadPointer (procIdx);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                pushNextSampleIntoFifo (channelData[i]);
        }
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
        if (isShuttingDown.load()) return;
        
        window.multiplyWithWindowingTable (fftData.data(), fftSize);
        forwardFFT.performFrequencyOnlyForwardTransform (fftData.data());
        
        // Normalización requerida por el tamaño de la ventana (N)
        float normFactor = 1.0f / static_cast<float>(fftSize);
        for (int i = 0; i < numBins; ++i)
            fftData[i] *= normFactor;
        
        float nyquist = static_cast<float> (currentSampleRate) / 2.0f;
        
        float sumMagnitude = 0.0f;
        float sumWeighted = 0.0f;
        float logSum = 0.0f;
        
        for (int i = 0; i < numBins; ++i)
        {
            float magnitude = fftData[i];
            float freq = (static_cast<float>(i) / static_cast<float>(numBins)) * nyquist;
            
            // Rango de Análisis: 20 Hz - 20 kHz
            if (freq >= 20.0f && freq <= 20000.0f)
            {
                sumMagnitude += magnitude;
                sumWeighted += magnitude * freq;
            }
            logSum += std::log (std::max (magnitude, 1e-10f));
        }
        
        float sumGuitarRange = 0.0f;
        float bodyEnergy = 0.0f;
        float cutEnergy = 0.0f;
        float brilloEnergy = 0.0f;
        float bMin = targetBrilloMin.load();
        float bMax = targetBrilloMax.load();

        for (int i = 0; i < numBins; ++i)
        {
            float magnitude = fftData[i];
            float freq = (static_cast<float>(i) / static_cast<float>(numBins)) * nyquist;
            
            // Rango Útil de Guitarra para Normalización (80 Hz - 8 kHz)
            if (freq >= 80.0f && freq <= 8000.0f)
                sumGuitarRange += magnitude;
            
            // Bandas Específicas
            if (freq >= 100.0f && freq <= 500.0f) bodyEnergy += magnitude;
            else if (freq >= 2000.0f && freq <= 5000.0f) cutEnergy += magnitude;
            
            if (freq >= bMin && freq <= bMax) brilloEnergy += magnitude;

            // Rango completo para centroide y otros cálculos
            if (freq >= 20.0f && freq <= 20000.0f)
            {
                sumMagnitude += magnitude;
                sumWeighted += magnitude * freq;
            }
        }

        // Solo calculamos métricas si hay señal medible
        if (sumGuitarRange > 0.0001f)
        {
            // Usamos el ratio real de energía (0.0 a 1.0)
            float bRatio = (bodyEnergy / sumGuitarRange); 
            float cRatio = (cutEnergy / sumGuitarRange);
            float brRatio = (brilloEnergy / sumGuitarRange);

            // Guardamos el ratio (0-1), el UI lo multiplicará por 100
            bodyEnergyRatio.store (bRatio);
            cutEnergyRatio.store (cRatio);
            brilloEnergyRatio.store (brRatio);
            
            // Spectral Centroid instantáneo
            float instCentroid = sumWeighted / std::max(0.0001f, sumMagnitude);
            
            // Suavizado EMA para la GUI
            smoothedCentroidHz = 0.85f * smoothedCentroidHz + 0.15f * instCentroid;
            spectralCentroid.store (smoothedCentroidHz);
            
            // Acumulación para el promedio
            accumulatedCentroidHz += instCentroid;
            accumulatedBodyRatio += bRatio;
            accumulatedCutRatio += cRatio;
            accumulatedBrilloRatio += brRatio;
            centroidCount++;
            
            averageCentroidHz.store (static_cast<float> (accumulatedCentroidHz / centroidCount));
            averageBodyRatio.store (static_cast<float> (accumulatedBodyRatio / centroidCount));
            averageCutRatio.store (static_cast<float> (accumulatedCutRatio / centroidCount));
            averageBrilloRatio.store (static_cast<float> (accumulatedBrilloRatio / centroidCount));
        }
        else
        {
            bodyEnergyRatio.store (0.0f);
            cutEnergyRatio.store (0.0f);
            brilloEnergyRatio.store (0.0f);
            spectralFlatness.store (0.0f);
        }

        // Suavizado temporal de la FFT (Attack rápido, Decay lento)
        const float attack = 0.3f; // 0.0 es instantáneo, 1.0 es congelado
        const float decay = 0.85f; 
        
        for (int i = 0; i < numBins; ++i)
        {
            float current = latestFFTData[i];
            float next = fftData[i];
            
            if (next > current)
                latestFFTData[i] = attack * current + (1.0f - attack) * next; // Sube rápido
            else
                latestFFTData[i] = decay * current + (1.0f - decay) * next;   // Baja más lento
        }

        fftDataReady.store(true);
    }

    // Datos exportables para el UI (Lock-Free)
    std::atomic<float> momentaryLUFS { -70.0f };
    std::atomic<float> shortTermLUFS { -70.0f };
    std::atomic<float> integratedLUFS { -70.0f };
    std::atomic<float> currentPLR { 0.0f };
    std::atomic<float> globalMaxPeakDb { -120.0f };
    std::atomic<float> spectralCentroid { 0.0f };
    std::atomic<float> averageCentroidHz { 0.0f };
    std::atomic<float> targetBrilloMin { 1200.0f };
    std::atomic<float> targetBrilloMax { 2000.0f };
    std::atomic<float> brilloEnergyRatio { 0.0f };
    std::atomic<float> averageBrilloRatio { 0.0f };
    std::atomic<float> spectralFlatness { 0.0f };
    std::atomic<float> bodyEnergyRatio { 0.0f };
    std::atomic<float> cutEnergyRatio { 0.0f };
    std::atomic<float> averageBodyRatio { 0.0f };
    std::atomic<float> averageCutRatio { 0.0f };
    std::atomic<int> currentCategory { 0 }; // 0: Warm, 1: Balanced, 2: Bright

    std::array<float, numBins> latestFFTData;
    std::atomic<bool> fftDataReady { false };

private:
    std::atomic<bool> isShuttingDown { false };
    float decayAlpha = 0.99f;
    std::array<float, 30> shortTermPeakHistory;
    int shortTermPeakIndex = 0;
    float currentHopMaxPeakDb = -120.0f;
    int samplesInHop = 0;

    float smoothedCentroidHz = 0.0f;
    double accumulatedCentroidHz = 0.0;
    double accumulatedBodyRatio = 0.0;
    double accumulatedCutRatio = 0.0;
    double accumulatedBrilloRatio = 0.0;
    int centroidCount = 0;

    double currentSampleRate = 44100.0;
    juce::dsp::FFT forwardFFT;
    juce::dsp::WindowingFunction<float> window;
    
    std::array<float, fftSize> fifo;
    std::array<float, 2 * fftSize> fftData;
    int fifoIndex = 0;
    bool nextFFTBlockReady = false;

    LufsMeter lufsMeter;
    juce::AudioBuffer<float> procBuffer;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
};
