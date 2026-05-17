#include "PresetAnalyzerViewComponent.h"
#include "IconLibrary.h"
#include "SamplesOfflineAnalyzer.h"
#include "TargetProfiles.h"

PresetAnalyzerViewComponent::PresetAnalyzerViewComponent(
    MondoHelixAnalyzerAudioProcessor &p)
    : audioProcessor(p),
      playButton("Play", juce::Colours::green.withAlpha(0.8f),
                 juce::Colours::green, juce::Colours::green.darker()),
      stopButton("Stop", juce::Colours::red.withAlpha(0.8f), juce::Colours::red,
                 juce::Colours::red.darker()),
      resetButton("Reset", juce::Colours::orange.withAlpha(0.8f),
                  juce::Colours::orange, juce::Colours::orange.darker()) {
  setWantsKeyboardFocus(true);
  addAndMakeVisible(diComboBox);
  diComboBox.setTextWhenNoChoicesAvailable("No DI Tracks Found");
  diComboBox.setTextWhenNothingSelected("Selecciona un DI...");
  diComboBox.onChange = [this] {
    int selId = diComboBox.getSelectedId();
    bool isLive = (selId == liveGuitarId);
    bool isPink = (selId == pinkNoiseId);
    bool isEmulator = (selId == emulatorId);

    audioProcessor.setLiveGuitarActive(isLive);
    audioProcessor.useInternalNoise.store(isPink);
    audioProcessor.setEmulatorActive(isEmulator);
    playButton.setEnabled(selId > 0);
  };

  addAndMakeVisible(targetComboBox);
  refreshTargetList();
  targetComboBox.setSelectedId(2);
  targetComboBox.onChange = [this]() {
    updateFrequencyGraphBands();
    updateStaticTargetReferences();
  };
  updateStaticTargetReferences();

  addAndMakeVisible(playButton);
  playButton.setShape(IconLibrary::getIcon("play"), true, true, false);
  playButton.setEnabled(false);
  playButton.onClick = [this] {
    // Solo resetear si no estaba sonando (evita resets en loops o clicks dobles)
    if (!audioProcessor.isDIPlaying()) {
      audioProcessor.analyzer.resetMetrics();
      frequencyGraph.resetPeakCurve();
    }

    int selId = diComboBox.getSelectedId();
    if (selId == liveGuitarId) {
      audioProcessor.setLiveGuitarActive(true);
      audioProcessor.useInternalNoise.store(false);
      audioProcessor.setEmulatorActive(false);
      audioProcessor.playDI();
    } else if (selId == pinkNoiseId) {
      audioProcessor.setLiveGuitarActive(false);
      audioProcessor.useInternalNoise.store(true);
      audioProcessor.setEmulatorActive(false);
      audioProcessor.playDI();
    } else if (selId == emulatorId) {
      audioProcessor.setLiveGuitarActive(false);
      audioProcessor.useInternalNoise.store(false);
      audioProcessor.setEmulatorActive(true);
      audioProcessor.playDI();
    } else if (selId > 0) {
      int fileIdx = selId - 2; // Offset by 1 (Guitar Live) and 1-based indexing
      if (fileIdx >= 0 && fileIdx < diFiles.size()) {
        audioProcessor.setLiveGuitarActive(false);
        audioProcessor.useInternalNoise.store(false);
        audioProcessor.setEmulatorActive(false);
        audioProcessor.loadDIForPlayback(diFiles[fileIdx]);
        audioProcessor.playDI();
      }
    }
  };

  addAndMakeVisible(stopButton);
  stopButton.setShape(IconLibrary::getIcon("stop"), true, true, false);
  stopButton.setEnabled(false);
  stopButton.onClick = [this] { audioProcessor.stopDI(); };

  addAndMakeVisible(lufsGauge);
  lufsGauge.setSuffix(" LUFS");
  lufsGauge.setEndLabels("", "LOUD");

  addAndMakeVisible(plrGauge);
  plrGauge.setSuffix(" PLR");
  plrGauge.setEndLabels("PLAIN", "DYNAMIC");

  addAndMakeVisible(brightnessGauge);
  brightnessGauge.setSuffix(" Hz");

  addAndMakeVisible(brilloRatioGauge);
  brilloRatioGauge.setSuffix("%");

  addAndMakeVisible(bodyGauge);
  bodyGauge.setSuffix("%");
  bodyGauge.setEndLabels("THIN", "MUDDY");

  addAndMakeVisible(cutGauge);
  cutGauge.setSuffix("%");

  addAndMakeVisible(routingStatusLabel);
  routingStatusLabel.setFont(juce::Font(13.0f, juce::Font::bold));
  routingStatusLabel.setJustificationType(juce::Justification::centred);
  routingStatusLabel.setColour(juce::Label::textColourId,
                               juce::Colours::orange);

  addAndMakeVisible(resetButton);
  resetButton.setShape(IconLibrary::getIcon("reset"), true, true, false);
  resetButton.onClick = [this] {
    audioProcessor.analyzer.resetMetrics();
    frequencyGraph.resetPeakCurve();
  };

  addAndMakeVisible(frequencyGraph);

  refreshDIList();
  updateFrequencyGraphBands();
  startTimerHz(30);
}

PresetAnalyzerViewComponent::~PresetAnalyzerViewComponent() {
  audioProcessor.stopDI();
}

void PresetAnalyzerViewComponent::refreshDIList() {
  diComboBox.clear(juce::dontSendNotification);
  diFiles.clear();

  int id = 1;
  liveGuitarId = id++;
  diComboBox.addItem("Guitar (Live)", liveGuitarId);

  juce::File folder = audioProcessor.settings.getDIFolder();
  juce::Array<juce::File> files =
      folder.findChildFiles(juce::File::findFiles, false, "*.wav");

  for (const auto &f : files) {
    diFiles.add(f);
    diComboBox.addItem(f.getFileNameWithoutExtension(), id++);
  }

  pinkNoiseId = id++;
  diComboBox.addItem("Pink Noise (Internal)", pinkNoiseId);

  emulatorId = id++;
  diComboBox.addItem("Run Full Calibration (Internal)", emulatorId);

  // Dejamos por default "Guitar (Live)"
  diComboBox.setSelectedId(liveGuitarId, juce::dontSendNotification);
  audioProcessor.setLiveGuitarActive(true);
  audioProcessor.useInternalNoise.store(false);
  audioProcessor.setEmulatorActive(false);

  playButton.setEnabled(true);
  refreshTargetList();
}

void PresetAnalyzerViewComponent::refreshTargetList() {
  // Guardamos la selección actual para intentar restaurarla tras refrescar
  int currentSel = targetComboBox.getSelectedId();

  targetComboBox.clear(juce::dontSendNotification);
  targetComboBox.addItem("AMBIENT", 1);
  targetComboBox.addItem("RHYTHM", 2);
  targetComboBox.addItem("LEAD", 3);

  juce::File folder = audioProcessor.settings.getGTRSamplesFolder();
  juce::Array<juce::File> files =
      folder.findChildFiles(juce::File::findFiles, false, "*.gtr_analysis");

  if (!files.isEmpty()) {
    targetComboBox.addSeparator();

    int id = 100;
    customTargetFiles.clear();
    for (const auto &f : files) {
      customTargetFiles.add(f);
      SampleAnalysisResult res = SamplesOfflineAnalyzer::loadAnalysisResult(f);
      juce::String displayName = f.getFileNameWithoutExtension();
      if (res.success) {
        displayName =
            "[" + res.targetCategory.toUpperCase() + "] " + displayName;
      }
      targetComboBox.addItem(displayName, id++);
    }
  }

  // Intentamos restaurar la selección previa, si no volvemos a Rhythm (2)
  if (targetComboBox.getItemText(targetComboBox.indexOfItemId(currentSel))
          .isNotEmpty()) {
    targetComboBox.setSelectedId(currentSel, juce::dontSendNotification);
  } else {
    targetComboBox.setSelectedId(2, juce::dontSendNotification);
  }
}

void PresetAnalyzerViewComponent::paint(juce::Graphics &g) {
  g.fillAll(juce::Colours::transparentBlack);

  // Helper para dibujar cards
  auto drawCard = [&](juce::Rectangle<int> area, bool highlighted) {
    g.setColour(juce::Colour(0xff2a2a35).withAlpha(0.6f));
    if (highlighted)
      g.setColour(juce::Colour(0xff3a3a4a).withAlpha(0.8f));

    g.fillRoundedRectangle(area.toFloat(), 8.0f);

    g.setColour(juce::Colours::white.withAlpha(0.1f));
    if (highlighted)
      g.setColour(juce::Colours::cyan.withAlpha(0.3f));

    g.drawRoundedRectangle(area.toFloat(), 8.0f, 1.5f);
  };

  drawCard(card1, true);  // Target
  drawCard(card2, false); // LUFS
  drawCard(card3, false); // PLR
  drawCard(card4, false); // Brillo (Hz)
  drawCard(card5, false); // Brillo (%)
  drawCard(card6, false); // Cuerpo
  drawCard(card7, false); // Corte

  // Titulos internos de cards
  g.setColour(juce::Colours::white.withAlpha(0.5f));
  g.setFont(juce::Font(10.0f, juce::Font::bold));
  auto drawTitle = [&](const juce::String &text, juce::Rectangle<int> card) {
    g.drawText(text, card.withHeight(18).reduced(5, 0),
               juce::Justification::left);
  };

  drawTitle("TARGET PROFILE", card1);
  drawTitle("LOUDNESS (INTEGRATED)", card2);
  drawTitle("DYNAMICS (PLR)", card3);
  drawTitle("SPECTRAL BRIGHTNESS", card4);
  drawTitle("BRIGHTNESS RATIO", card5);
  drawTitle("BODY RATIO", card6);
  drawTitle("CUT RATIO", card7);

  // Dibujar barra horizontal de duración y avance si está reproduciendo un
  // archivo DI físico
  bool playing = audioProcessor.isDIPlaying();
  bool isInternal = audioProcessor.useInternalNoise.load();

  if (playing && !isInternal) {
    float progress = audioProcessor.getDIPlaybackProgress();
    double totalSecs = audioProcessor.getDITotalDurationSeconds();
    double currentSecs = totalSecs * static_cast<double>(progress);

    auto barArea = progressBarBounds.toFloat();

    // Fondo de la barra
    g.setColour(juce::Colours::darkgrey.darker());
    g.fillRoundedRectangle(barArea, 4.0f);

    // Relleno de avance
    if (progress > 0.0f) {
      auto fillArea = barArea.withWidth(barArea.getWidth() * progress);
      g.setColour(juce::Colours::cyan);
      g.fillRoundedRectangle(fillArea, 4.0f);
    }

    // Borde fino contenedor
    g.setColour(juce::Colours::white.withAlpha(0.15f));
    g.drawRoundedRectangle(barArea, 4.0f, 1.0f);

    // Leyenda superpuesta de tiempo formateado (ej. 0:03 / 0:15)
    auto formatSecs = [](double s) {
      int totalInt = static_cast<int>(s);
      int mins = totalInt / 60;
      int secs = totalInt % 60;
      return juce::String(mins) + ":" + juce::String(secs).paddedLeft('0', 2);
    };

    juce::String timeStr =
        formatSecs(currentSecs) + " / " + formatSecs(totalSecs);

    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(11.0f, juce::Font::bold));
    g.drawText(timeStr, progressBarBounds, juce::Justification::centred, false);
  }
}

bool PresetAnalyzerViewComponent::keyPressed(const juce::KeyPress &key) {
  if (key == juce::KeyPress::spaceKey) {
    if (audioProcessor.isDIPlaying()) {
      audioProcessor.stopDI();
    } else {
      // Disparar el click del botón play para reutilizar su lógica de selección
      playButton.triggerClick();
    }
    return true;
  }
  return false;
}

void PresetAnalyzerViewComponent::mouseDown(const juce::MouseEvent &e) {
  grabKeyboardFocus();
}

void PresetAnalyzerViewComponent::resized() {
  auto bounds = getLocalBounds().reduced(15);

  // Barra superior de selección y transporte
  auto topBar = bounds.removeFromTop(40);
  diComboBox.setBounds(topBar.removeFromLeft(300).reduced(0, 4));
  topBar.removeFromLeft(15);
  playButton.setBounds(topBar.removeFromLeft(32).reduced(2));
  topBar.removeFromLeft(10);
  stopButton.setBounds(topBar.removeFromLeft(32).reduced(2));
  topBar.removeFromLeft(10);
  resetButton.setBounds(topBar.removeFromLeft(32).reduced(2));
  topBar.removeFromLeft(20);
  progressBarBounds = topBar.reduced(0, 10);

  // 4-Column Responsive Layout (Target Profile spans 2 columns, others span 1
  // column)
  int gridSpacing = 10;
  int cardW = (bounds.getWidth() - (gridSpacing * 3)) / 4;
  int cardH = 80;

  auto row1 = bounds.removeFromTop(cardH);
  card1 = row1.removeFromLeft(cardW * 2 + gridSpacing);
  row1.removeFromLeft(gridSpacing);
  card2 = row1.removeFromLeft(cardW);
  row1.removeFromLeft(gridSpacing);
  card3 = row1;

  bounds.removeFromTop(gridSpacing);

  auto row2 = bounds.removeFromTop(cardH);
  card4 = row2.removeFromLeft(cardW);
  row2.removeFromLeft(gridSpacing);
  card5 = row2.removeFromLeft(cardW);
  row2.removeFromLeft(gridSpacing);
  card6 = row2.removeFromLeft(cardW);
  row2.removeFromLeft(gridSpacing);
  card7 = row2;

  // Posicionar componentes dentro de las cards
  targetComboBox.setBounds(card1.reduced(15, 20).withTrimmedTop(10));
  lufsGauge.setBounds(card2.reduced(10, 10).withTrimmedTop(10));
  plrGauge.setBounds(card3.reduced(10, 10).withTrimmedTop(10));
  brightnessGauge.setBounds(card4.reduced(10, 10).withTrimmedTop(10));
  brilloRatioGauge.setBounds(card5.reduced(10, 10).withTrimmedTop(10));
  bodyGauge.setBounds(card6.reduced(10, 10).withTrimmedTop(10));
  cutGauge.setBounds(card7.reduced(10, 10).withTrimmedTop(10));

  bounds.removeFromTop(15);

  // Barra inferior de estado
  auto bottomBar = bounds.removeFromBottom(25);
  routingStatusLabel.setBounds(bottomBar);

  frequencyGraph.setBounds(bounds);
}

void PresetAnalyzerViewComponent::timerCallback() {
  // Actualizar estado activo y opacidad visual de los botones de transporte
  bool playing = audioProcessor.isDIPlaying();
  stopButton.setEnabled(playing);
  stopButton.setAlpha(playing ? 1.0f : 0.3f);

  bool hasDI = diComboBox.getSelectedId() > 0;
  playButton.setEnabled(hasDI);
  playButton.setAlpha(hasDI ? 1.0f : 0.3f);

  auto &analyzer = audioProcessor.analyzer;
  float lufsI = analyzer.integratedLUFS.load();
  float plr = analyzer.currentPLR.load();
  float body = analyzer.averageBodyRatio.load();
  float cut = analyzer.averageCutRatio.load();
  float brightness = analyzer.averageCentroidHz.load();
  float brilloRatio = analyzer.averageBrilloRatio.load();

  // Configurar rangos y valores en los Gauges según activeTargetProfile
  // (cargado de forma optimizada)
  using namespace TargetProfiles;

  auto updateGauges = [&](float lMin, float lMax, float pMin, float pMax,
                          float bMin, float bMax, float brMin, float brMax,
                          float bdMin, float bdMax, float cMin, float cMax) {
    lufsGauge.setRange(LUFS_000, LUFS_100, lMin, lMax);
    plrGauge.setRange(PLR_000, PLR_100, pMin, pMax);
    brightnessGauge.setRange(BRILLO_000, BRILLO_100, bMin, bMax);
    brilloRatioGauge.setRange(BRILLO_RATIO_000, BRILLO_RATIO_100, brMin, brMax);
    bodyGauge.setRange(CUERPO_000, CUERPO_100, bdMin, bdMax);
    cutGauge.setRange(CORTE_000, CORTE_100, cMin, cMax);
  };

  // Sincronizar dinámicamente las frecuencias límite con el motor de DSP para
  // que calcule la energía
  float dynamicBMin = 1200.0f, dynamicBMax = 2000.0f;

  if (activeTargetProfile == TargetType::Ambient) {
    dynamicBMin = AMBIENT_BRILLO_MIN;
    dynamicBMax = AMBIENT_BRILLO_MAX;
    updateGauges(AMBIENT_LUFS_MIN, AMBIENT_LUFS_MAX, AMBIENT_PLR_MIN,
                 AMBIENT_PLR_MAX, dynamicBMin, dynamicBMax,
                 AMBIENT_BRILLO_RATIO_MIN, AMBIENT_BRILLO_RATIO_MAX,
                 AMBIENT_CUERPO_MIN, AMBIENT_CUERPO_MAX, AMBIENT_CORTE_MIN,
                 AMBIENT_CORTE_MAX);
  } else if (activeTargetProfile == TargetType::Rhythm) {
    dynamicBMin = RHYTHM_BRILLO_MIN;
    dynamicBMax = RHYTHM_BRILLO_MAX;
    updateGauges(RHYTHM_LUFS_MIN, RHYTHM_LUFS_MAX, RHYTHM_PLR_MIN,
                 RHYTHM_PLR_MAX, dynamicBMin, dynamicBMax,
                 RHYTHM_BRILLO_RATIO_MIN, RHYTHM_BRILLO_RATIO_MAX,
                 RHYTHM_CUERPO_MIN, RHYTHM_CUERPO_MAX, RHYTHM_CORTE_MIN,
                 RHYTHM_CORTE_MAX);
  } else if (activeTargetProfile == TargetType::Lead) {
    dynamicBMin = LEAD_BRILLO_MIN;
    dynamicBMax = LEAD_BRILLO_MAX;
    updateGauges(LEAD_LUFS_MIN, LEAD_LUFS_MAX, LEAD_PLR_MIN, LEAD_PLR_MAX,
                 dynamicBMin, dynamicBMax, LEAD_BRILLO_RATIO_MIN,
                 LEAD_BRILLO_RATIO_MAX, LEAD_CUERPO_MIN, LEAD_CUERPO_MAX,
                 LEAD_CORTE_MIN, LEAD_CORTE_MAX);
  } else {
    updateGauges(-20.0f, -16.0f, 8.0f, 14.0f, 1000.0f, 3000.0f, 15.0f, 45.0f,
                 20.0f, 50.0f, 20.0f, 50.0f);
  }

  analyzer.targetBrilloMin.store(dynamicBMin);
  analyzer.targetBrilloMax.store(dynamicBMax);

  lufsGauge.setCurrentValue(lufsI);
  plrGauge.setCurrentValue(plr);
  brightnessGauge.setCurrentValue(brightness);
  brilloRatioGauge.setCurrentValue(brilloRatio * 100.0f);
  bodyGauge.setCurrentValue(body * 100.0f);
  cutGauge.setCurrentValue(cut * 100.0f);

  static bool wasPlaying = false;
  if (playing) {
    int outCh = audioProcessor.settings.playbackOutputChannel.load();
    int inCh = audioProcessor.settings.processedInputChannel.load();
    routingStatusLabel.setText(
        juce::String::formatted("Ruta activa: Enviando DI a USB %d/%d  |  "
                                "Leyendo Helix por USB %d/%d",
                                outCh, outCh + 1, inCh, inCh + 1),
        juce::dontSendNotification);

    if (!audioProcessor.useInternalNoise.load())
      repaint(progressBarBounds);
  } else {
    routingStatusLabel.setText("Ruta en reposo (Transporte detenido)",
                               juce::dontSendNotification);
  }

  if (playing != wasPlaying) {
    wasPlaying = playing;
    repaint(progressBarBounds);
  }

  if (analyzer.fftDataReady.exchange(false)) {
    frequencyGraph.setSampleRate(audioProcessor.getSampleRate());
    frequencyGraph.setSmoothingDenominator(
        audioProcessor.settings.fftSmoothingDenominator.load());
    frequencyGraph.setFFTData(analyzer.latestFFTData);
  }
}

void PresetAnalyzerViewComponent::updateFrequencyGraphBands() {
  using namespace TargetProfiles;
  std::vector<FrequencyGraphComponent::TargetRange> ranges;

  // 1. Banda de Cuerpo (Púrpura Translúcido)
  ranges.push_back({100.0f, 500.0f, "BODY", juce::Colours::mediumpurple});

  // 2. Banda de Corte (Verde Esmeralda Translúcido)
  ranges.push_back({2000.0f, 5000.0f, "CUT", juce::Colours::mediumspringgreen});

  // 3. Banda de Brillo Objetivo (Naranja/Dorado Translúcido)
  int selectedId = targetComboBox.getSelectedId();
  bool isCustom = (selectedId >= 100);

  float bMin = 0.0f, bMax = 0.0f;
  TargetType targetForBands = TargetType::Rhythm;

  if (isCustom) {
    int idx = selectedId - 100;
    if (idx >= 0 && idx < customTargetFiles.size()) {
      SampleAnalysisResult res =
          SamplesOfflineAnalyzer::loadAnalysisResult(customTargetFiles[idx]);
      if (res.success) {
        juce::String cat = res.targetCategory.toLowerCase();
        if (cat == "ambient")
          targetForBands = TargetType::Ambient;
        else if (cat == "rhythm")
          targetForBands = TargetType::Rhythm;
        else if (cat == "lead")
          targetForBands = TargetType::Lead;
      }
    }
  } else {
    targetForBands = static_cast<TargetType>(selectedId - 1);
  }

  if (targetForBands == TargetType::Ambient) {
    bMin = AMBIENT_BRILLO_MIN;
    bMax = AMBIENT_BRILLO_MAX;
  } else if (targetForBands == TargetType::Rhythm) {
    bMin = RHYTHM_BRILLO_MIN;
    bMax = RHYTHM_BRILLO_MAX;
  } else if (targetForBands == TargetType::Lead) {
    bMin = LEAD_BRILLO_MIN;
    bMax = LEAD_BRILLO_MAX;
  }

  if (bMax > 0.0f)
    ranges.push_back({bMin, bMax, "BRIGHTNESS", juce::Colours::orange});

  frequencyGraph.setTargetRanges(ranges);
}

void PresetAnalyzerViewComponent::updateStaticTargetReferences() {
  int selectedId = targetComboBox.getSelectedId();
  bool isCustom = (selectedId >= 100);

  if (isCustom) {
    int idx = selectedId - 100;
    if (idx >= 0 && idx < customTargetFiles.size()) {
      SampleAnalysisResult res =
          SamplesOfflineAnalyzer::loadAnalysisResult(customTargetFiles[idx]);
      if (res.success) {
        // Resolver activeTargetProfile para los medidores
        juce::String cat = res.targetCategory.toLowerCase();
        if (cat == "ambient")
          activeTargetProfile = TargetType::Ambient;
        else if (cat == "rhythm")
          activeTargetProfile = TargetType::Rhythm;
        else if (cat == "lead")
          activeTargetProfile = TargetType::Lead;
        else
          activeTargetProfile = TargetType::Rhythm;

        // Inyectar los valores de referencia a los gauges (se dibujarán en
        // color data2: #8dc63f)
        lufsGauge.setTargetReferenceValue(res.lufs);
        plrGauge.setTargetReferenceValue(res.plr);
        brightnessGauge.setTargetReferenceValue(res.brightness);
        bodyGauge.setTargetReferenceValue(res.bodyRatio * 100.0f);
        cutGauge.setTargetReferenceValue(res.cutRatio * 100.0f);
        brilloRatioGauge.setTargetReferenceValue(res.brilloRatio * 100.0f);

        // Inyectar la curva de frecuencia a la gráfica (se dibujará en color
        // data2: #8dc63f)
        frequencyGraph.setReferenceCurve(res.fftCurve);
        return;
      }
    }
  }

  // Si no es custom, o falló la carga
  if (selectedId >= 1 && selectedId <= 3) {
    activeTargetProfile = static_cast<TargetType>(selectedId - 1);
  } else {
    activeTargetProfile = TargetType::Rhythm;
  }

  lufsGauge.clearTargetReferenceValue();
  plrGauge.clearTargetReferenceValue();
  brightnessGauge.clearTargetReferenceValue();
  brilloRatioGauge.clearTargetReferenceValue();
  bodyGauge.clearTargetReferenceValue();
  cutGauge.clearTargetReferenceValue();

  frequencyGraph.clearReferenceCurve();
}
