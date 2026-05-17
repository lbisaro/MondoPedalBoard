#pragma once
#include <JuceHeader.h>

// Generador eficiente de Ruido Rosa basado en el algoritmo de Voss-McCartney
class PinkNoiseGenerator
{
public:
    PinkNoiseGenerator()
    {
        // Inicializar los generadores para 2 canales independientes (estéreo descorrelacionado)
        for (int ch = 0; ch < 2; ++ch)
        {
            runningSum[ch] = 0.0f;
            for (int i = 0; i < numRows; ++i)
            {
                rows[ch][i] = random.nextFloat() * 2.0f - 1.0f;
                runningSum[ch] += rows[ch][i];
            }
        }
    }

    void setLevelDecibels (float dbVal)
    {
        gainMultiplier = juce::Decibels::decibelsToGain (dbVal);
    }

    void processBlock (juce::AudioBuffer<float>& buffer)
    {
        int numSamples = buffer.getNumSamples();
        int numChannels = juce::jmin (buffer.getNumChannels(), 2);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* channelData = buffer.getWritePointer (ch);
            for (int i = 0; i < numSamples; ++i)
            {
                // Voss-McCartney: determinamos el bit menos significativo en cero del contador
                counter[ch]++;
                int zeros = 0;
                unsigned int c = counter[ch];
                while ((c & 1) == 0 && zeros < numRows - 1)
                {
                    zeros++;
                    c >>= 1;
                }

                // Actualizamos únicamente la fila correspondiente para mantener la pendiente -3dB/octava
                runningSum[ch] -= rows[ch][zeros];
                rows[ch][zeros] = random.nextFloat() * 2.0f - 1.0f;
                runningSum[ch] += rows[ch][zeros];

                // Añadimos una fuente blanca para completar el espectro en altas frecuencias extremas
                float white = random.nextFloat() * 2.0f - 1.0f;

                // Dividimos por el número total de fuentes activas para garantizar que nunca exceda [-1.0, 1.0]
                float sample = (runningSum[ch] + white) / static_cast<float>(numRows + 1);
                
                channelData[i] = sample * gainMultiplier;
            }
        }
    }

private:
    static constexpr int numRows = 12; // Cubre la respuesta plana hasta frecuencias extremadamente bajas (~10 Hz)
    float rows[2][numRows];
    float runningSum[2];
    unsigned int counter[2] = { 0, 0 };

    float gainMultiplier = juce::Decibels::decibelsToGain (-15.0f); // Nivel óptimo calibrado por defecto
    juce::Random random;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PinkNoiseGenerator)
};
