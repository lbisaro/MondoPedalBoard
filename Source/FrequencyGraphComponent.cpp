#include "FrequencyGraphComponent.h"

FrequencyGraphComponent::FrequencyGraphComponent()
{
    std::fill (fftData.begin(), fftData.end(), 0.0f);
}

FrequencyGraphComponent::~FrequencyGraphComponent() = default;

void FrequencyGraphComponent::setFFTData (const std::array<float, AnalysisEngine::numBins>& newData)
{
    std::copy (newData.begin(), newData.end(), fftData.begin());
    repaint();
}

void FrequencyGraphComponent::setSampleRate (double newSampleRate)
{
    if (newSampleRate > 0.0)
        sampleRate = newSampleRate;
}

void FrequencyGraphComponent::setCurveColourId (int customLookAndFeelColourId)
{
    curveColourId = customLookAndFeelColourId;
    repaint();
}

void FrequencyGraphComponent::paint (juce::Graphics& g)
{
    juce::Colour textColor = juce::Colours::white;
    juce::Colour graphBgColor = juce::Colours::darkgrey.darker();
    juce::Colour gridColor = juce::Colours::white.withAlpha (0.1f);
    juce::Colour curveColor = juce::Colours::cyan;

    if (auto* lf = dynamic_cast<juce::LookAndFeel_V4*>(&getLookAndFeel()))
    {
        textColor = lf->getCurrentColourScheme().getUIColour (juce::LookAndFeel_V4::ColourScheme::defaultText);
        graphBgColor = lf->getCurrentColourScheme().getUIColour (juce::LookAndFeel_V4::ColourScheme::windowBackground);
        gridColor = lf->getCurrentColourScheme().getUIColour (juce::LookAndFeel_V4::ColourScheme::widgetBackground);
        curveColor = lf->findColour (curveColourId);
    }

    auto plotArea = getLocalBounds();
    plotArea.removeFromRight (35);  // Espacio para etiquetas de dBs
    plotArea.removeFromBottom (20); // Espacio para etiquetas de Hz

    g.setColour (graphBgColor);
    g.fillRect (plotArea);

    float width = static_cast<float> (plotArea.getWidth());
    float height = static_cast<float> (plotArea.getHeight());
    float bottom = static_cast<float> (plotArea.getBottom());
    float left = static_cast<float> (plotArea.getX());

    float minFreq = 10.0f;
    float maxFreq = 20000.0f;
    float minLogFreq = std::log10 (minFreq);
    float maxLogFreq = std::log10 (maxFreq);
    float skewFactor = 1.5f; // Factor para expandir agudos
    float minDb = -30.0f;
    float maxDb = 30.0f;

    // 1. Dibujar Grid y Etiquetas de Frecuencia (X)
    g.setColour (gridColor);
    g.setFont (12.0f);
    std::array<float, 10> freqs = { 20.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f };
    for (float f : freqs)
    {
        float normX = (std::log10 (f) - minLogFreq) / (maxLogFreq - minLogFreq);
        normX = std::pow (normX, skewFactor);
        float x = left + width * normX;
        g.drawVerticalLine (juce::roundToInt(x), static_cast<float>(plotArea.getY()), static_cast<float>(plotArea.getBottom()));
        
        g.setColour (textColor);
        juce::String text = f >= 1000.0f ? juce::String (f / 1000.0f, 0) + "k" : juce::String (f, 0);
        g.drawText (text, static_cast<int>(x) - 20, static_cast<int>(bottom) + 5, 40, 15, juce::Justification::centredTop, false);
        g.setColour (gridColor);
    }

    // 2. Dibujar Grid y Etiquetas de dBs (Y)
    std::array<float, 7> dbs = { 30.0f, 20.0f, 10.0f, 0.0f, -10.0f, -20.0f, -30.0f };
    for (float db : dbs)
    {
        float level = juce::jmap (db, minDb, maxDb, 0.0f, 1.0f);
        float y = bottom - height * level;
        g.drawHorizontalLine (juce::roundToInt(y), static_cast<float>(plotArea.getX()), static_cast<float>(plotArea.getRight()));
        
        g.setColour (textColor);
        juce::String dbText = db > 0.0f ? "+" + juce::String (db, 0) : juce::String (db, 0);
        g.drawText (dbText, static_cast<int>(plotArea.getRight()) + 5, static_cast<int>(y) - 10, 30, 20, juce::Justification::centredLeft, false);
        g.setColour (gridColor);
    }

    // 3. Dibujar la curva FFT
    g.setColour (curveColor);
    juce::Path fftPath;
    
    int numBins = AnalysisEngine::numBins;
    bool pathStarted = false;

    for (int i = 0; i < numBins; ++i)
    {
        float magnitude = fftData[i];
        float dbVal = juce::Decibels::gainToDecibels(magnitude, minDb);
        float level = juce::jmap (dbVal, minDb, maxDb, 0.0f, 1.0f);
        level = juce::jlimit(0.0f, 1.0f, level);
        
        float binFreq = (static_cast<float>(i) * static_cast<float>(sampleRate)) / static_cast<float>(AnalysisEngine::fftSize);
        if (binFreq < minFreq) binFreq = minFreq;
        if (binFreq > maxFreq) binFreq = maxFreq;

        float normX = (std::log10 (binFreq) - minLogFreq) / (maxLogFreq - minLogFreq);
        normX = std::pow (normX, skewFactor);
        float x = left + width * normX;
        float y = bottom - height * level;

        if (! pathStarted && binFreq >= minFreq)
        {
            fftPath.startNewSubPath (x, y);
            pathStarted = true;
        }
        else if (pathStarted)
        {
            fftPath.lineTo (x, y);
        }
    }

    {
        juce::Graphics::ScopedSaveState state(g);
        g.reduceClipRegion (plotArea);
        g.strokePath (fftPath, juce::PathStrokeType (2.0f));
    }

    // 4. Dibujar el tooltip si el mouse está sobre el gráfico
    if (isMouseOverPlot)
    {
        float mouseX = static_cast<float>(mousePos.x);
        float mouseY = static_cast<float>(mousePos.y);

        mouseX = juce::jlimit(left, left + width, mouseX);
        mouseY = juce::jlimit(static_cast<float>(plotArea.getY()), bottom, mouseY);

        float skewedNormX = (mouseX - left) / width;
        float normX = std::pow (skewedNormX, 1.0f / skewFactor);
        float freq = std::pow (10.0f, minLogFreq + normX * (maxLogFreq - minLogFreq));
        
        float normY = (bottom - mouseY) / height;
        float db = juce::jmap (normY, 0.0f, 1.0f, minDb, maxDb);
        
        g.setColour (curveColor.withAlpha (0.15f));
        float dashes[] = { 3.0f, 4.0f };
        g.drawDashedLine (juce::Line<float>(mouseX, static_cast<float>(plotArea.getY()), mouseX, static_cast<float>(plotArea.getBottom())), dashes, 2, 1.0f);
        g.drawDashedLine (juce::Line<float>(static_cast<float>(plotArea.getX()), mouseY, static_cast<float>(plotArea.getRight()), mouseY), dashes, 2, 1.0f);
        
        juce::String textFreq = freq >= 1000.0f ? juce::String (freq / 1000.0f, 2) + " kHz" : juce::String (freq, 1) + " Hz";
        juce::String text = textFreq + " | " + juce::String (db, 1) + " dB";
        
        g.setFont (14.0f);
        int textWidth = g.getCurrentFont().getStringWidth (text) + 12;
        int textHeight = 22;
        
        int textX = plotArea.getRight() - textWidth;
        int textY = plotArea.getBottom() - textHeight;
            
        g.setColour (graphBgColor.withAlpha (0.95f));
        g.fillRect (textX, textY, textWidth, textHeight);
        
        g.setColour (textColor);
        g.drawText (text, textX, textY, textWidth, textHeight, juce::Justification::centred, false);
    }
}

void FrequencyGraphComponent::resized()
{
}

void FrequencyGraphComponent::mouseMove (const juce::MouseEvent& e)
{
    auto plotArea = getLocalBounds();
    plotArea.removeFromRight (35);
    plotArea.removeFromBottom (20);

    bool wasOver = isMouseOverPlot;
    juce::Point<int> oldPos = mousePos;

    if (plotArea.contains (e.getPosition()))
    {
        isMouseOverPlot = true;
        mousePos = e.getPosition();
        if (!wasOver || oldPos != mousePos)
            repaint (plotArea);
    }
    else if (isMouseOverPlot)
    {
        isMouseOverPlot = false;
        repaint (plotArea);
    }
}

void FrequencyGraphComponent::mouseExit (const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    if (isMouseOverPlot)
    {
        isMouseOverPlot = false;
        auto plotArea = getLocalBounds();
        plotArea.removeFromRight (35);
        plotArea.removeFromBottom (20);
        repaint (plotArea);
    }
}
