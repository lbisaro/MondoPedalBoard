#include "FrequencyGraphComponent.h"

FrequencyGraphComponent::FrequencyGraphComponent() {
  std::fill(fftData.begin(), fftData.end(), 0.0f);
  std::fill(representativeCurve.begin(), representativeCurve.end(), 0.0f);
}

FrequencyGraphComponent::~FrequencyGraphComponent() = default;

void FrequencyGraphComponent::resetPeakCurve() {
  std::fill(representativeCurve.begin(), representativeCurve.end(), 0.0f);
  repaint();
}

void FrequencyGraphComponent::setFFTData(
    const std::array<float, AnalysisEngine::numBins> &newData) {
  std::copy(newData.begin(), newData.end(), fftData.begin());

  // Calcular la versión suavizada por octava para la vista
  calculateFractionalOctaveSmoothing();

  for (size_t i = 0; i < representativeCurve.size(); ++i) {
    if (smoothedData[i] > representativeCurve[i])
      representativeCurve[i] = smoothedData[i];
  }
  repaint();
}

void FrequencyGraphComponent::setSmoothingDenominator (int denominator)
{
    smoothingDenominator = juce::jlimit(1, 24, denominator);
}

void FrequencyGraphComponent::calculateFractionalOctaveSmoothing()
{
    const float fraction = 1.0f / static_cast<float>(smoothingDenominator);
  const float octaveFactor = std::pow(2.0f, fraction / 2.0f);
  const float lowFactor = 1.0f / octaveFactor;
  const float highFactor = octaveFactor;

  const int numBins = static_cast<int>(fftData.size());
  const float binFreqWidth = static_cast<float>(sampleRate) /
                             static_cast<float>(AnalysisEngine::fftSize);

  for (int i = 0; i < numBins; ++i) {
    float centerFreq = static_cast<float>(i) * binFreqWidth;

    // En frecuencias muy bajas, no suavizamos para no perder definición rítmica
    if (centerFreq < 40.0f) {
      smoothedData[i] = fftData[i];
      continue;
    }

    float lowFreq = centerFreq * lowFactor;
    float highFreq = centerFreq * highFactor;

    int startBin = static_cast<int>(std::floor(lowFreq / binFreqWidth));
    int endBin = static_cast<int>(std::ceil(highFreq / binFreqWidth));

    startBin = juce::jlimit(0, numBins - 1, startBin);
    endBin = juce::jlimit(0, numBins - 1, endBin);

    float sum = 0.0f;
    int count = 0;
    for (int j = startBin; j <= endBin; ++j) {
      sum += fftData[j];
      count++;
    }

    smoothedData[i] =
        (count > 0) ? (sum / static_cast<float>(count)) : fftData[i];
  }
}

void FrequencyGraphComponent::setSampleRate(double newSampleRate) {
  if (newSampleRate > 0.0)
    sampleRate = newSampleRate;
}

void FrequencyGraphComponent::setCurveColourId(int customLookAndFeelColourId) {
  curveColourId = customLookAndFeelColourId;
  repaint();
}

void FrequencyGraphComponent::setTargetRanges(
    const std::vector<TargetRange> &ranges) {
  targetRanges = ranges;
  repaint();
}

void FrequencyGraphComponent::paint(juce::Graphics &g) {
  juce::Colour textColor = juce::Colours::white;
  juce::Colour graphBgColor = juce::Colours::darkgrey.darker();
  juce::Colour gridColor = juce::Colours::white.withAlpha(0.1f);
  juce::Colour curveColor = juce::Colours::cyan;

  if (auto *lf = dynamic_cast<juce::LookAndFeel_V4 *>(&getLookAndFeel())) {
    textColor = lf->getCurrentColourScheme().getUIColour(
        juce::LookAndFeel_V4::ColourScheme::defaultText);
    graphBgColor = lf->getCurrentColourScheme().getUIColour(
        juce::LookAndFeel_V4::ColourScheme::windowBackground);
    gridColor = lf->getCurrentColourScheme().getUIColour(
        juce::LookAndFeel_V4::ColourScheme::widgetBackground);
    curveColor = lf->findColour(curveColourId);
  }

  auto plotArea = getLocalBounds();
  plotArea.removeFromRight(35);  // Espacio para etiquetas de dBs
  plotArea.removeFromBottom(20); // Espacio para etiquetas de Hz

  g.setColour(graphBgColor);
  g.fillRect(plotArea);

  float width = static_cast<float>(plotArea.getWidth());
  float height = static_cast<float>(plotArea.getHeight());
  float bottom = static_cast<float>(plotArea.getBottom());
  float left = static_cast<float>(plotArea.getX());

  float minFreq = 10.0f;
  float maxFreq = 20000.0f;
  float minLogFreq = std::log10(minFreq);
  float maxLogFreq = std::log10(maxFreq);
  float skewFactor = 1.5f; // Factor para expandir agudos
  float minDb = -80.0f;
  float maxDb = 0.0f;

  // 0. Dibujar Bandas de Target (Fondo)
  for (auto &range : targetRanges) {
    float x1 = left + width * std::pow((std::log10(juce::jlimit(
                                            minFreq, maxFreq, range.minFreq)) -
                                        minLogFreq) /
                                           (maxLogFreq - minLogFreq),
                                       skewFactor);
    float x2 = left + width * std::pow((std::log10(juce::jlimit(
                                            minFreq, maxFreq, range.maxFreq)) -
                                        minLogFreq) /
                                           (maxLogFreq - minLogFreq),
                                       skewFactor);

    juce::Rectangle<float> bandRect(x1, plotArea.getY(), x2 - x1, height);

    g.setColour(range.color.withAlpha(0.08f));
    g.fillRect(bandRect);

    // Borde lateral sutil
    g.setColour(range.color.withAlpha(0.2f));
    g.drawVerticalLine(juce::roundToInt(x1), bandRect.getY(),
                       bandRect.getBottom());
    g.drawVerticalLine(juce::roundToInt(x2), bandRect.getY(),
                       bandRect.getBottom());

    // Etiqueta
    g.setColour(range.color.withAlpha(0.5f));
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.drawText(range.name, (int)x1, (int)plotArea.getY() + 2, (int)(x2 - x1),
               15, juce::Justification::centredTop);
  }

  // 1. Dibujar Grid y Etiquetas de Frecuencia (X)
  g.setColour(gridColor);
  g.setFont(12.0f);
  std::array<float, 10> freqs = {20.0f,   50.0f,   100.0f,  200.0f,   500.0f,
                                 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f};
  for (float f : freqs) {
    float normX = (std::log10(f) - minLogFreq) / (maxLogFreq - minLogFreq);
    normX = std::pow(normX, skewFactor);
    float x = left + width * normX;
    g.drawVerticalLine(juce::roundToInt(x), static_cast<float>(plotArea.getY()),
                       static_cast<float>(plotArea.getBottom()));

    g.setColour(textColor);
    juce::String text =
        f >= 1000.0f ? juce::String(f / 1000.0f, 0) + "k" : juce::String(f, 0);
    g.drawText(text, static_cast<int>(x) - 20, static_cast<int>(bottom) + 5, 40,
               15, juce::Justification::centredTop, false);
    g.setColour(gridColor);
  }

  // 2. Dibujar Grid y Etiquetas de dBs (Y)
  std::array<float, 9> dbs = {0.0f,   -10.0f, -20.0f, -30.0f, -40.0f,
                              -50.0f, -60.0f, -70.0f, -80.0f};
  for (float db : dbs) {
    float level = juce::jmap(db, minDb, maxDb, 0.0f, 1.0f);
    float y = bottom - height * level;
    g.drawHorizontalLine(juce::roundToInt(y),
                         static_cast<float>(plotArea.getX()),
                         static_cast<float>(plotArea.getRight()));

    g.setColour(textColor);
    juce::String dbText =
        db > 0.0f ? "+" + juce::String(db, 0) : juce::String(db, 0);
    g.drawText(dbText, static_cast<int>(plotArea.getRight()) + 5,
               static_cast<int>(y) - 10, 30, 20,
               juce::Justification::centredLeft, false);
    g.setColour(gridColor);
    // 3. Dibujar la curva representativa acumulada (Pro-Q 3 Style), la curva de referencia
  // estática y la curva FFT en vivo
  juce::Path repPath;
  juce::Path fftPath;
  juce::Path refPath;
 
  int numBins = AnalysisEngine::numBins;
  bool repStarted = false;
  bool pathStarted = false;
  bool refStarted = false;
  float firstX = 0.0f;
  float lastX = 0.0f;
  float firstRefX = 0.0f;
  float lastRefX = 0.0f;
 
  for (int i = 0; i < numBins; ++i) {
    // Cálculo para curva representativa
    float repMag = representativeCurve[i];
    float repDb = juce::Decibels::gainToDecibels(repMag, minDb);
    float repLevel = juce::jmap(repDb, minDb, maxDb, 0.0f, 1.0f);
    repLevel = juce::jlimit(0.0f, 1.0f, repLevel);
 
    // Cálculo para curva en vivo (USANDO DATOS SUAVIZADOS POR 1/3 OCTAVA)
    float magnitude = smoothedData[i];
    float dbVal = juce::Decibels::gainToDecibels(magnitude, minDb);
    float level = juce::jmap(dbVal, minDb, maxDb, 0.0f, 1.0f);
    level = juce::jlimit(0.0f, 1.0f, level);

    // Cálculo para curva de referencia estática
    float refMag = (hasReferenceCurve && i < (int)referenceCurve.size()) ? referenceCurve[i] : 0.0f;
    float refDb = juce::Decibels::gainToDecibels(refMag, minDb);
    float refLevel = juce::jmap(refDb, minDb, maxDb, 0.0f, 1.0f);
    refLevel = juce::jlimit(0.0f, 1.0f, refLevel);
 
    float binFreq = (static_cast<float>(i) * static_cast<float>(sampleRate)) /
                    static_cast<float>(AnalysisEngine::fftSize);
    if (binFreq < minFreq)
      binFreq = minFreq;
    if (binFreq > maxFreq)
      binFreq = maxFreq;
 
    float normX =
        (std::log10(binFreq) - minLogFreq) / (maxLogFreq - minLogFreq);
    normX = std::pow(normX, skewFactor);
    float x = left + width * normX;
 
    float repY = bottom - height * repLevel;
    float y = bottom - height * level;
    float refY = bottom - height * refLevel;
 
    if (!repStarted && binFreq >= minFreq) {
      repPath.startNewSubPath(x, repY);
      firstX = x;
      repStarted = true;
    } else if (repStarted) {
      repPath.lineTo(x, repY);
      lastX = x;
    }
 
    if (!pathStarted && binFreq >= minFreq) {
      fftPath.startNewSubPath(x, y);
      pathStarted = true;
    } else if (pathStarted) {
      fftPath.lineTo(x, y);
    }

    if (hasReferenceCurve) {
      if (!refStarted && binFreq >= minFreq) {
        refPath.startNewSubPath(x, refY);
        firstRefX = x;
        refStarted = true;
      } else if (refStarted) {
        refPath.lineTo(x, refY);
        lastRefX = x;
      }
    }
  }
 
  {
    juce::Graphics::ScopedSaveState state(g);
    g.reduceClipRegion(plotArea);

    // Dibujar Curva Estática de Referencia (Color data2 - Verde Lima)
    if (hasReferenceCurve && refStarted) {
      juce::Colour refCurveColor = juce::Colours::limegreen;
      if (auto* lf = dynamic_cast<juce::LookAndFeel_V4*>(&getLookAndFeel())) {
          refCurveColor = lf->findColour(CustomLookAndFeel::data2ColourId);
      }
      
      // Relleno sutil
      juce::Path refFillPath = refPath;
      refFillPath.lineTo(lastRefX, bottom);
      refFillPath.lineTo(firstRefX, bottom);
      refFillPath.closeSubPath();
      g.setColour(refCurveColor.withAlpha(0.08f));
      g.fillPath(refFillPath);

      // Trazo nítido de la señal de referencia
      g.setColour(refCurveColor.withAlpha(0.85f));
      g.strokePath(refPath, juce::PathStrokeType(2.0f));
    }
 
    if (repStarted) {
      // Relleno translúcido de la envolvente global representativa
      repPath.lineTo(lastX, bottom);
      repPath.lineTo(firstX, bottom);
      repPath.closeSubPath();
 
      g.setColour(curveColor.withAlpha(0.22f));
      g.fillPath(repPath);
    }
 
    // Trazado nítido de la señal instantánea en vivo
    g.setColour(curveColor);
    g.strokePath(fftPath, juce::PathStrokeType(2.0f));
  }
  }

  // 4. Dibujar el tooltip si el mouse está sobre el gráfico
  if (isMouseOverPlot) {
    float mouseX = static_cast<float>(mousePos.x);
    float mouseY = static_cast<float>(mousePos.y);

    mouseX = juce::jlimit(left, left + width, mouseX);
    mouseY = juce::jlimit(static_cast<float>(plotArea.getY()), bottom, mouseY);

    float skewedNormX = (mouseX - left) / width;
    float normX = std::pow(skewedNormX, 1.0f / skewFactor);
    float freq =
        std::pow(10.0f, minLogFreq + normX * (maxLogFreq - minLogFreq));

    float normY = (bottom - mouseY) / height;
    float db = juce::jmap(normY, 0.0f, 1.0f, minDb, maxDb);

    g.setColour(curveColor.withAlpha(0.15f));
    float dashes[] = {3.0f, 4.0f};
    g.drawDashedLine(
        juce::Line<float>(mouseX, static_cast<float>(plotArea.getY()), mouseX,
                          static_cast<float>(plotArea.getBottom())),
        dashes, 2, 1.0f);
    g.drawDashedLine(
        juce::Line<float>(static_cast<float>(plotArea.getX()), mouseY,
                          static_cast<float>(plotArea.getRight()), mouseY),
        dashes, 2, 1.0f);

    juce::String textFreq = freq >= 1000.0f
                                ? juce::String(freq / 1000.0f, 2) + " kHz"
                                : juce::String(freq, 1) + " Hz";
    juce::String text = textFreq + " | " + juce::String(db, 1) + " dB";

    g.setFont(14.0f);
    int textWidth = g.getCurrentFont().getStringWidth(text) + 12;
    int textHeight = 22;

    int textX = plotArea.getRight() - textWidth;
    int textY = plotArea.getY() + 4;

    g.setColour(graphBgColor.withAlpha(0.95f));
    g.fillRect(textX, textY, textWidth, textHeight);

    g.setColour(textColor);
    g.drawText(text, textX, textY, textWidth, textHeight,
               juce::Justification::centred, false);
  }
}

void FrequencyGraphComponent::resized() {}

void FrequencyGraphComponent::mouseMove(const juce::MouseEvent &e) {
  auto plotArea = getLocalBounds();
  plotArea.removeFromRight(35);
  plotArea.removeFromBottom(20);

  bool wasOver = isMouseOverPlot;
  juce::Point<int> oldPos = mousePos;

  if (plotArea.contains(e.getPosition())) {
    isMouseOverPlot = true;
    mousePos = e.getPosition();
    if (!wasOver || oldPos != mousePos)
      repaint(plotArea);
  } else if (isMouseOverPlot) {
    isMouseOverPlot = false;
    repaint(plotArea);
  }
}

void FrequencyGraphComponent::mouseExit(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  if (isMouseOverPlot) {
    isMouseOverPlot = false;
    auto plotArea = getLocalBounds();
    plotArea.removeFromRight(35);
    plotArea.removeFromBottom(20);
    repaint(plotArea);
  }
}

void FrequencyGraphComponent::setReferenceCurve(const std::vector<float>& refCurve)
{
    if (refCurve.size() != AnalysisEngine::numBins)
    {
        hasReferenceCurve = false;
        repaint();
        return;
    }
    
    // Suavizar la curva usando el smoothingDenominator actual (1/3 o 1/6 octava)
    const float fraction = 1.0f / static_cast<float>(smoothingDenominator);
    const float octaveFactor = std::pow(2.0f, fraction / 2.0f);
    const float lowFactor = 1.0f / octaveFactor;
    const float highFactor = octaveFactor;
    
    const int numBins = static_cast<int>(refCurve.size());
    const float binFreqWidth = static_cast<float>(sampleRate) / static_cast<float>(AnalysisEngine::fftSize);
    
    referenceCurve.resize(numBins);
    
    for (int i = 0; i < numBins; ++i)
    {
        float centerFreq = static_cast<float>(i) * binFreqWidth;
        
        if (centerFreq < 40.0f)
        {
            referenceCurve[i] = refCurve[i];
            continue;
        }
        
        float lowFreq = centerFreq * lowFactor;
        float highFreq = centerFreq * highFactor;
        
        int startBin = static_cast<int>(std::floor(lowFreq / binFreqWidth));
        int endBin = static_cast<int>(std::ceil(highFreq / binFreqWidth));
        
        startBin = juce::jlimit(0, numBins - 1, startBin);
        endBin = juce::jlimit(0, numBins - 1, endBin);
        
        float sum = 0.0f;
        int count = 0;
        for (int j = startBin; j <= endBin; ++j)
        {
            sum += refCurve[j];
            count++;
        }
        
        referenceCurve[i] = (count > 0) ? (sum / static_cast<float>(count)) : refCurve[i];
    }
    
    hasReferenceCurve = true;
    repaint();
}

void FrequencyGraphComponent::clearReferenceCurve()
{
    hasReferenceCurve = false;
    referenceCurve.clear();
    repaint();
}
