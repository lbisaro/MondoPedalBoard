#pragma once
#include <JuceHeader.h>
#include <vector>
#include <random>

/**
 * GuitarStringSynth: Implementación de Karplus-Strong con Delay Fraccional.
 */
class GuitarStringSynth
{
public:
    GuitarStringSynth() 
    {
        delayLine.resize(1024, 0.0f);
        randomGen.seed(std::random_device()());
    }

    void prepare(double sampleRate)
    {
        currentSampleRate = sampleRate;
        std::fill(delayLine.begin(), delayLine.end(), 0.0f);
        writePos = 0;
    }

    void trigger(float frequency, float velocity)
    {
        float periodSamples = static_cast<float>(currentSampleRate / frequency);
        currentDelayLength = juce::jlimit(2.0f, static_cast<float>(delayLine.size() - 2), periodSamples);
        
        // Excitación: Llenamos el buffer completo con silencio y el inicio con ruido
        std::fill(delayLine.begin(), delayLine.end(), 0.0f);
        
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        
        // Calibrado según Lead.wav: Pico de 0.32
        // Usamos una base de 0.32 corregida por la velocidad
        float amp = (velocity * velocity) * 0.32f;
        
        for (int i = 0; i < static_cast<int>(currentDelayLength); ++i)
        {
            delayLine[i] = dist(randomGen) * amp;
        }
        
        writePos = static_cast<int>(currentDelayLength) % delayLine.size();
        lastOutput = 0.0f;
        isActive = true;
    }

    float getNextSample()
    {
        if (!isActive) return 0.0f;

        // Lectura con interpolación lineal (para Karplus-Strong básico, 
        // pero suficiente para calibración si el SR es alto)
        float readPos = static_cast<float>(writePos) - currentDelayLength;
        while (readPos < 0) readPos += static_cast<float>(delayLine.size());
        
        int i0 = static_cast<int>(readPos);
        int i1 = (i0 + 1) % delayLine.size();
        float frac = readPos - static_cast<float>(i0);
        
        float out = delayLine[i0] * (1.0f - frac) + delayLine[i1] * frac;

        // Feedback con Filtro de un polo estándar (0.996)
        float sustainFactor = 0.996f;
        float newSample = out * sustainFactor; 
        
        // Amortiguación simple (promedio)
        float feedback = (newSample + lastOutput) * 0.5f;
        lastOutput = feedback;

        delayLine[writePos] = feedback;
        writePos = (writePos + 1) % delayLine.size();

        return out;
    }

    void stop() { isActive = false; }
    bool getIsActive() const { return isActive; }

private:
    std::vector<float> delayLine;
    int writePos = 0;
    float currentDelayLength = 0.0f;
    double currentSampleRate = 44100.0;
    float lastOutput = 0.0f;
    bool isActive = false;
    std::mt19937 randomGen;
};

/**
 * PickupEmulator: Filtro IIR para emular la resonancia de una pastilla.
 */
class PickupEmulator
{
public:
    PickupEmulator()
    {
        filter.coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter (44100.0, 3500.0, 2.0, 6.0f);
    }

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        filter.prepare (spec);
        filter.coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter (spec.sampleRate, 3500.0, 2.0, 6.0f);
    }

    float process (float sample)
    {
        return filter.processSample (sample);
    }

private:
    juce::dsp::IIR::Filter<float> filter;
};

/**
 * CalibrationSequencer: Lógica de automatización para barridos.
 */
class CalibrationSequencer
{
public:
    enum class Phase { Idle, DynamicTest, SpectralBase };

    CalibrationSequencer(GuitarStringSynth& s) : synth(s) {}

    void start()
    {
        currentPhase = Phase::DynamicTest;
        currentNote = 40; // E2 (Mi grave)
        samplesUntilNextNote = 0;
        phaseCounter = 0;
    }

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        pickup.prepare (spec);
    }

    void stop()
    {
        currentPhase = Phase::Idle;
        synth.stop();
    }

    float getNextSample(double sampleRate)
    {
        if (currentPhase == Phase::Idle) return 0.0f;

        if (samplesUntilNextNote <= 0)
        {
            if (currentNote > 88) // Llegamos a E6 (Afinación alta)
            {
                if (currentPhase == Phase::DynamicTest)
                {
                    currentPhase = Phase::SpectralBase;
                    currentNote = 40;
                }
                else
                {
                    currentPhase = Phase::Idle;
                    return 0.0f;
                }
            }

            float freq = 440.0f * std::pow(2.0f, (static_cast<float>(currentNote) - 69.0f) / 12.0f);
            
            float vel = 1.0f;
            if (currentPhase == Phase::DynamicTest)
            {
                // Rango dinámico aleatorio (de 0.6 a 1.0 de velocidad)
                std::uniform_real_distribution<float> dist(0.6f, 1.0f);
                vel = dist(randomGen);
            }

            synth.trigger(freq, vel);
            currentNote++;
            
            // Tiempo entre notas (0.25 segundos)
            samplesUntilNextNote = static_cast<int>(sampleRate * 0.25);
        }

        samplesUntilNextNote--;
        float raw = synth.getNextSample();
        return pickup.process(raw);
    }

    bool isRunning() const { return currentPhase != Phase::Idle; }

private:
    GuitarStringSynth& synth;
    PickupEmulator pickup;
    Phase currentPhase = Phase::Idle;
    int currentNote = 40;
    int samplesUntilNextNote = 0;
    int phaseCounter = 0;
    std::mt19937 randomGen;
};
