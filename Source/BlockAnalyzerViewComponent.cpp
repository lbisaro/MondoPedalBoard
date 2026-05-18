#include "BlockAnalyzerViewComponent.h"
#include "IconLibrary.h"
#include "CustomLookAndFeel.h"

// =============================================================================
// EQTransferFunctionGraph
// =============================================================================
EQTransferFunctionGraph::EQTransferFunctionGraph (BlockAnalyzer& analyzer)
    : blockAnalyzer (analyzer)
{
    magnitudeData.fill (0.0f);
    smoothedData.fill  (0.0f);
    setMouseCursor (juce::MouseCursor::CrosshairCursor);
}

void EQTransferFunctionGraph::updateCurve()
{
    if (! blockAnalyzer.hasResult()) return;
    blockAnalyzer.getMagnitudeDb (magnitudeData);

    // Extra UI-side EMA for visual smoothness
    const float uiEma = 0.7f;
    for (int i = 0; i < BlockAnalyzer::numBins; ++i)
        smoothedData[i] = uiEma * smoothedData[i] + (1.0f - uiEma) * magnitudeData[i];

    repaint();
}

void EQTransferFunctionGraph::paint (juce::Graphics& g)
{
    // --- Resolve data1 color from the LookAndFeel ---
    juce::Colour curveColor = juce::Colour (0xfff38d48);   // data1 default
    juce::Colour bgColor    = juce::Colour (0xff14141a);
    juce::Colour textColor  = juce::Colours::white.withAlpha (0.6f);

    if (auto* lf = dynamic_cast<juce::LookAndFeel_V4*> (&getLookAndFeel()))
    {
        curveColor = lf->findColour (CustomLookAndFeel::data1ColourId);
        bgColor    = lf->getCurrentColourScheme().getUIColour (
                         juce::LookAndFeel_V4::ColourScheme::windowBackground);
    }

    const juce::Colour gridColor  = juce::Colours::white.withAlpha (0.06f);
    const juce::Colour gridEmph   = juce::Colours::white.withAlpha (0.14f);
    const juce::Colour zeroColor  = juce::Colours::white.withAlpha (0.28f);

    // --- Plot area ---
    auto plotArea = getLocalBounds();
    plotArea.removeFromLeft   (38);
    plotArea.removeFromBottom (20);
    plotArea.removeFromRight  (6);

    g.setColour (bgColor);
    g.fillRect (plotArea);
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.drawRect (plotArea, 1);

    const float left   = static_cast<float> (plotArea.getX());
    const float top    = static_cast<float> (plotArea.getY());
    const float width  = static_cast<float> (plotArea.getWidth());
    const float height = static_cast<float> (plotArea.getHeight());
    const float bottom = top + height;

    const float minLogF = std::log10 (20.0f);
    const float maxLogF = std::log10 (20000.0f);
    const float skew    = 1.5f;

    auto freqToX = [&] (float freq) -> float
    {
        float n = (std::log10 (freq) - minLogF) / (maxLogF - minLogF);
        return left + width * std::pow (n, skew);
    };
    auto dbToY = [&] (float db) -> float
    {
        return bottom - height * juce::jmap (db, kMinDb, kMaxDb, 0.0f, 1.0f);
    };

    // --- Frequency grid (vertical lines) ---
    const float freqMarks[] = { 20.f, 50.f, 100.f, 200.f, 500.f,
                                 1000.f, 2000.f, 5000.f, 10000.f, 20000.f };
    const char* freqLabels[] = { "20", "50", "100", "200", "500",
                                  "1k", "2k", "5k", "10k", "20k" };
    g.setFont (juce::Font (10.0f));
    for (int fi = 0; fi < (int) std::size (freqMarks); ++fi)
    {
        float x = freqToX (freqMarks[fi]);
        bool isMain = (freqMarks[fi] == 100.f || freqMarks[fi] == 1000.f || freqMarks[fi] == 10000.f);
        g.setColour (isMain ? gridEmph : gridColor);
        g.drawVerticalLine (juce::roundToInt (x), top, bottom);

        g.setColour (textColor);
        g.drawText (freqLabels[fi],
                    juce::roundToInt (x) - 18, juce::roundToInt (bottom) + 3, 36, 15,
                    juce::Justification::centredTop, false);
    }

    // --- dB grid (horizontal lines) + left labels ---
    const float dbMarks[] = { -16.f, -12.f, -8.f, -4.f, 0.f, 4.f, 8.f, 12.f, 16.f };
    for (float db : dbMarks)
    {
        float y = dbToY (db);
        bool isZero = (std::abs (db) < 0.01f);

        g.setColour (isZero ? zeroColor : gridColor);
        g.drawHorizontalLine (juce::roundToInt (y), left, left + width);

        g.setColour (isZero ? textColor.brighter (0.6f) : textColor);
        juce::String lbl = (db > 0.f ? "+" : "") + juce::String (static_cast<int> (db));
        g.drawText (lbl, 0, juce::roundToInt (y) - 8, 36, 16,
                    juce::Justification::centredRight, false);
    }

    // --- Magnitude curve ---
    if (blockAnalyzer.hasResult())
    {
        juce::Path curve;
        bool started = false;

        for (int k = 0; k < BlockAnalyzer::numBins; ++k)
        {
            double normK = static_cast<double> (k) / static_cast<double> (BlockAnalyzer::numBins - 1);
            double freq  = BlockAnalyzer::kSweepStartHz
                           * std::pow (BlockAnalyzer::kSweepEndHz / BlockAnalyzer::kSweepStartHz, normK);

            float x = freqToX (static_cast<float> (freq));
            float y = dbToY   (juce::jlimit (kMinDb, kMaxDb, smoothedData[k]));

            if (x < left - 1.f || x > left + width + 1.f) continue;

            if (! started) { curve.startNewSubPath (x, y); started = true; }
            else             curve.lineTo (x, y);
        }

        if (started)
        {
            juce::Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (plotArea);

            // Translucent fill
            float y0 = dbToY (0.0f);
            juce::Path fill = curve;
            fill.lineTo (left + width, y0);
            fill.lineTo (left, y0);
            fill.closeSubPath();
            g.setColour (curveColor.withAlpha (0.15f));
            g.fillPath (fill);

            // Main stroke
            g.setColour (curveColor);
            g.strokePath (curve, juce::PathStrokeType (2.0f,
                          juce::PathStrokeType::curved,
                          juce::PathStrokeType::rounded));

            // Highlight glow
            g.setColour (curveColor.brighter (0.5f).withAlpha (0.35f));
            g.strokePath (curve, juce::PathStrokeType (1.0f));
        }
    }
    else
    {
        g.setFont (juce::Font (13.0f));
        g.setColour (textColor.withAlpha (0.4f));
        g.drawText ("Measuring EQ response...",
                    plotArea.reduced (20), juce::Justification::centred, false);
    }

    // --- Mouse crosshair + label (identical style to FrequencyGraphComponent) ---
    if (isMouseOver && plotArea.contains (mousePos))
    {
        float mx = static_cast<float> (mousePos.x);
        float my = static_cast<float> (mousePos.y);
        mx = juce::jlimit (left, left + width, mx);
        my = juce::jlimit (top,  bottom,       my);

        // Frequency under cursor
        float skewedNorm = (mx - left) / width;
        float normX      = std::pow (skewedNorm, 1.0f / skew);
        float freq = std::pow (10.0f, minLogF + normX * (maxLogF - minLogF));

        // dB under cursor
        float normY = (bottom - my) / height;
        float db    = kMinDb + normY * (kMaxDb - kMinDb);

        // Dashed crosshair
        float dashes[] = { 3.0f, 4.0f };
        g.setColour (curveColor.withAlpha (0.3f));
        g.drawDashedLine (juce::Line<float> (mx, top,  mx, bottom),      dashes, 2, 1.0f);
        g.drawDashedLine (juce::Line<float> (left, my, left + width, my), dashes, 2, 1.0f);

        // Label — top-right corner of plot area (same as FrequencyGraphComponent)
        juce::String freqStr = (freq >= 1000.0f)
            ? juce::String (freq / 1000.0f, 2) + " kHz"
            : juce::String (static_cast<int> (freq)) + " Hz";
        juce::String text = freqStr + " | " + juce::String (db, 1) + " dB";

        g.setFont (juce::Font (13.0f));
        int tw = g.getCurrentFont().getStringWidth (text) + 12;
        int th = 22;
        int tx = plotArea.getRight() - tw;
        int ty = plotArea.getY() + 4;

        g.setColour (bgColor.withAlpha (0.95f));
        g.fillRect (tx, ty, tw, th);
        g.setColour (textColor.brighter (0.4f));
        g.drawText (text, tx, ty, tw, th, juce::Justification::centred, false);
    }
}

void EQTransferFunctionGraph::resized() {}

void EQTransferFunctionGraph::mouseMove (const juce::MouseEvent& e)
{
    auto plotArea = getLocalBounds();
    plotArea.removeFromLeft   (38);
    plotArea.removeFromBottom (20);
    plotArea.removeFromRight  (6);

    bool wasOver  = isMouseOver;
    auto oldPos   = mousePos;

    if (plotArea.contains (e.getPosition()))
    {
        isMouseOver = true;
        mousePos    = e.getPosition();
        if (! wasOver || oldPos != mousePos)
            repaint (plotArea);
    }
    else if (isMouseOver)
    {
        isMouseOver = false;
        repaint (plotArea);
    }
}

void EQTransferFunctionGraph::mouseExit (const juce::MouseEvent&)
{
    if (isMouseOver)
    {
        isMouseOver = false;
        auto plotArea = getLocalBounds();
        plotArea.removeFromLeft   (38);
        plotArea.removeFromBottom (20);
        plotArea.removeFromRight  (6);
        repaint (plotArea);
    }
}

// =============================================================================
// BlockAnalyzerViewComponent
// =============================================================================
BlockAnalyzerViewComponent::BlockAnalyzerViewComponent (MondoHelixAnalyzerAudioProcessor& p)
    : audioProcessor (p),
      eqGraph (p.blockAnalyzer)
{
    addAndMakeVisible (eqGraph);

    // --- Title label (left of row 1) ---
    juce::Colour data1 = juce::Colour (0xfff38d48);
    titleLabel.setText ("EQ TRANSFER FUNCTION  |  Exponential Sine Sweep (Farina Method)",
                        juce::dontSendNotification);
    titleLabel.setFont (juce::Font (11.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, data1);
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (titleLabel);

    // --- Cycle counter (right of row 1) ---
    statusLabel.setFont (juce::Font (11.0f));
    statusLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.5f));
    statusLabel.setJustificationType (juce::Justification::centredRight);
    statusLabel.setText ("Initializing...", juce::dontSendNotification);
    addAndMakeVisible (statusLabel);

    // --- Routing info (row 2, full width) ---
    routingLabel.setFont (juce::Font (10.0f));
    routingLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.3f));
    routingLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (routingLabel);

    startTimerHz (30);
}

BlockAnalyzerViewComponent::~BlockAnalyzerViewComponent()
{
    stopTimer();
    stopSweep();
}

void BlockAnalyzerViewComponent::parentHierarchyChanged()
{
    if (isShowing())
        startSweep();
    else
        stopSweep();
}

void BlockAnalyzerViewComponent::startSweep()
{
    audioProcessor.blockAnalyzer.reset();
    audioProcessor.setLiveGuitarActive   (false);
    audioProcessor.setEmulatorActive     (false);
    audioProcessor.useInternalNoise.store  (false);
    audioProcessor.useModulatedNoise.store (false);
    audioProcessor.playDI();

    measurementCycle = 0;
    statusLabel.setText ("Measuring...", juce::dontSendNotification);
}

void BlockAnalyzerViewComponent::stopSweep()
{
    audioProcessor.stopDI();
    statusLabel.setText ("Stopped", juce::dontSendNotification);
}

void BlockAnalyzerViewComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::transparentBlack);
}

void BlockAnalyzerViewComponent::resized()
{
    auto bounds = getLocalBounds().reduced (15);

    // Row 1: title (left 70%) + cycle counter (right 30%)
    auto row1 = bounds.removeFromTop (22);
    titleLabel.setBounds  (row1.removeFromLeft (row1.getWidth() * 7 / 10));
    statusLabel.setBounds (row1);

    bounds.removeFromTop (2);

    // Row 2: routing info
    auto row2 = bounds.removeFromTop (18);
    routingLabel.setBounds (row2);

    bounds.removeFromTop (6);

    eqGraph.setBounds (bounds);
}

void BlockAnalyzerViewComponent::timerCallback()
{
    eqGraph.updateCurve();

    // Update routing info label
    int outCh = audioProcessor.settings.playbackOutputChannel.load();
    int inCh  = audioProcessor.settings.processedInputChannel.load();
    routingLabel.setText (
        "Output -> Helix: USB " + juce::String (outCh) + "/" + juce::String (outCh + 1)
        + "   |   Input <- Helix: USB " + juce::String (inCh) + "/" + juce::String (inCh + 1),
        juce::dontSendNotification);

    // Update cycle counter
    if (audioProcessor.blockAnalyzer.hasResult())
    {
        ++measurementCycle;
        statusLabel.setText ("Cycle " + juce::String (measurementCycle),
                             juce::dontSendNotification);
    }
}
