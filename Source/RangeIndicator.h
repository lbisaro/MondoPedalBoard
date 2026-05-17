#pragma once
#include <JuceHeader.h>
#include "TargetProfiles.h"

/**
 * RangeIndicator
 * Un medidor visual que muestra un valor actual dentro de un rango absoluto,
 * resaltando una "zona verde" (target) y zonas de advertencia.
 */
class RangeIndicator : public juce::Component
{
public:
    RangeIndicator() {}

    void setRange(float min, float max, float targetMin, float targetMax)
    {
        absMin = min;
        absMax = max;
        tMin = (targetMin == TargetProfiles::NO_LIMIT) ? min : targetMin;
        tMax = (targetMax == TargetProfiles::NO_LIMIT) ? max : targetMax;
        repaint();
    }

    void setCurrentValue(float val)
    {
        rawDisplayValue = val;
        currentValue = juce::jlimit(absMin, absMax, val);
        repaint();
    }

    void setSuffix(const juce::String& s) { suffix = s; }
    
    void setEndLabels(const juce::String& left, const juce::String& right)
    {
        leftLabel = left;
        rightLabel = right;
        repaint();
    }

    void setTargetReferenceValue(float val)
    {
        hasTargetRef = true;
        targetRefValue = val;
        repaint();
    }

    void clearTargetReferenceValue()
    {
        hasTargetRef = false;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        auto w = bounds.getWidth();
        auto h = bounds.getHeight();

        // Configuración de dimensiones
        float barHeight = 12.0f;
        float barY = h * 0.6f;
        juce::Rectangle<float> barArea(0, barY, w, barHeight);

        // 1. Dibujar fondo de la barra (Gris oscuro)
        g.setColour(juce::Colour(0xff1a1a1a));
        g.fillRoundedRectangle(barArea, barHeight * 0.5f);

        // 2. Dibujar zonas de color
        auto valToX = [&](float v) {
            return juce::jmap(v, absMin, absMax, 0.0f, w);
        };

        float xTMin = valToX(tMin);
        float xTMax = valToX(tMax);

        // Zona Verde (Target)
        g.setColour(juce::Colours::limegreen.withAlpha(0.8f));
        g.fillRoundedRectangle(xTMin, barY, xTMax - xTMin, barHeight, barHeight * 0.5f);

        // 3. Dibujar el puntero (Línea vertical blanca sutil)
        float xCurrent = valToX(currentValue);
        
        g.setColour(juce::Colours::white.withAlpha(0.9f));
        g.fillRect(xCurrent - 1.0f, barY - 4.0f, 2.0f, barHeight + 8.0f);
        
        // Brillo sutil alrededor de la línea (Cyan)
        g.setColour(juce::Colours::cyan.withAlpha(0.4f));
        g.fillRect(xCurrent - 2.0f, barY - 4.0f, 4.0f, barHeight + 8.0f);

        // 3b. Dibujar el puntero de referencia si está activo (Línea vertical verde lima con brillo)
        if (hasTargetRef)
        {
            float xRef = valToX(juce::jlimit(absMin, absMax, targetRefValue));
            juce::Colour data2Color = juce::Colour(0xff8dc63f); // Verde lima predeterminado (#8dc63f)
            if (getLookAndFeel().isColourSpecified(0x2000101)) {
                data2Color = getLookAndFeel().findColour(0x2000101);
            }
            
            g.setColour(data2Color.withAlpha(0.95f));
            g.fillRect(xRef - 1.5f, barY - 3.0f, 3.0f, barHeight + 6.0f);
            
            g.setColour(data2Color.withAlpha(0.4f));
            g.fillRect(xRef - 3.0f, barY - 3.0f, 6.0f, barHeight + 6.0f);
        }

        // 4. Dibujar Texto del valor actual (Usando el valor real, no el limitado)
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(18.0f, juce::Font::bold));
        juce::String valStr = juce::String(rawDisplayValue, 1) + suffix;
        if (absMax > 1000.0f) valStr = juce::String(juce::roundToInt(rawDisplayValue)) + suffix;
        
        if (hasTargetRef)
        {
            // Con referencia, subimos el valor actual e imprimimos la referencia abajo en color data2 (#8dc63f)
            g.drawText(valStr, 0, 0, (int)w, (int)(barY * 0.45f), juce::Justification::centred);
            
            juce::Colour data2Color = juce::Colour(0xff8dc63f);
            if (getLookAndFeel().isColourSpecified(0x2000101)) {
                data2Color = getLookAndFeel().findColour(0x2000101);
            }
            g.setColour(data2Color);
            g.setFont(juce::Font(11.0f, juce::Font::bold));
            
            juce::String refStr = "REF: " + juce::String(targetRefValue, 1) + suffix;
            if (absMax > 1000.0f) refStr = "REF: " + juce::String(juce::roundToInt(targetRefValue)) + suffix;
            
            g.drawText(refStr, 0, (int)(barY * 0.45f), (int)w, (int)(barY * 0.5f), juce::Justification::centred);
        }
        else
        {
            g.drawText(valStr, 0, 0, (int)w, (int)barY - 5, juce::Justification::centred);
        }

        // 5. Dibujar etiquetas de límites (Posicionadas arriba de la barra)
        g.setFont(juce::Font(9.0f, juce::Font::bold));
        g.setColour(juce::Colours::white.withAlpha(0.6f));
        float labelY = barY - 11.0f;
        
        if (leftLabel.isNotEmpty())
            g.drawText(leftLabel, 0, (int)labelY, 60, 10, juce::Justification::left);
        if (rightLabel.isNotEmpty())
            g.drawText(rightLabel, (int)w - 60, (int)labelY, 60, 10, juce::Justification::right);
    }

private:
    float absMin = 0.0f, absMax = 100.0f;
    float tMin = 25.0f, tMax = 75.0f;
    float currentValue = 0.0f;
    float rawDisplayValue = 0.0f;
    juce::String suffix = "";
    juce::String leftLabel = "", rightLabel = "";
    bool hasTargetRef = false;
    float targetRefValue = 0.0f;
};
