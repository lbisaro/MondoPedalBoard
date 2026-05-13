#include "GuitarDIRecorderViewComponent.h"

GuitarDIRecorderViewComponent::GuitarDIRecorderViewComponent(
    MondoHelixAnalyzerAudioProcessor &p)
    : audioProcessor(p) {
  setWantsKeyboardFocus(true);

  addAndMakeVisible(namePromptLabel);
  namePromptLabel.setText("Nombre del Archivo:", juce::dontSendNotification);

  addAndMakeVisible(nameInput);
  // Empieza siempre en blanco para no olvidar ingresar uno
  nameInput.setText("", juce::dontSendNotification);
  nameInput.setSelectAllWhenFocused(true);

  addAndMakeVisible(actionButton);
  actionButton.onClick = [this] {
    if (currentState == UIState::Idle) {
      currentState = UIState::Countdown;
      countdownTicks = 5;
      updateStateUI();
      grabKeyboardFocus(); // Para que la barra espaciadora siga interceptándose
    } else if (currentState == UIState::Countdown ||
               currentState == UIState::Recording) {
      audioProcessor.stopDIRecording();
      currentState = UIState::Review;
      // Vaciamos el cuadro de texto para forzar la escritura de uno nuevo
      nameInput.setText("", juce::dontSendNotification);
      updateStateUI();
      grabKeyboardFocus();
    }
  };

  addAndMakeVisible(saveButton);
  saveButton.setButtonText("Guardar Pista");
  saveButton.onClick = [this] {
    juce::String fileName = nameInput.getText().trim();
    
    // Validación obligatoria de nombre de archivo
    if (fileName.isEmpty()) {
      juce::AlertWindow::showMessageBoxAsync(
          juce::AlertWindow::WarningIcon, "Nombre Requerido",
          "Por favor, ingresa un nombre valido para el archivo antes de guardarlo.");
      return;
    }

    if (audioProcessor.saveRecordedDI(fileName)) {
      if (onFinished)
        onFinished();
    } else {
      juce::AlertWindow::showMessageBoxAsync(
          juce::AlertWindow::WarningIcon, "Error al Guardar",
          "No se pudo guardar la pista DI. Verifica que el búfer contenga "
          "audio y la carpeta tenga permisos.");
    }
  };

  addAndMakeVisible(retryButton);
  retryButton.setButtonText("Volver a Grabar");
  retryButton.onClick = [this] {
    audioProcessor.stopDIRecording();
    currentState = UIState::Idle;
    updateStateUI();
    grabKeyboardFocus();
  };

  updateStateUI();
  startTimerHz(30);
}

GuitarDIRecorderViewComponent::~GuitarDIRecorderViewComponent() {
  audioProcessor.stopDIRecording();
}

void GuitarDIRecorderViewComponent::resetState() {
  audioProcessor.stopDIRecording();
  currentState = UIState::Idle;
  countdownTicks = 5;
  nameInput.setText("", juce::dontSendNotification);
  updateStateUI();
}

void GuitarDIRecorderViewComponent::updateStateUI() {
  saveButton.setVisible(currentState == UIState::Review);
  retryButton.setVisible(currentState == UIState::Review);
  namePromptLabel.setVisible(currentState == UIState::Review);
  nameInput.setVisible(currentState == UIState::Review);

  actionButton.setVisible(currentState != UIState::Review);
  nameInput.setEnabled(currentState == UIState::Review);

  if (auto *lf = dynamic_cast<CustomLookAndFeel *>(&getLookAndFeel())) {
    actionButton.setColour(juce::TextButton::buttonColourId,
                           lf->findColour(CustomLookAndFeel::defaultColourId));
  }
  actionButton.setColour(juce::TextButton::textColourOffId,
                         juce::Colours::white);
  actionButton.setColour(juce::TextButton::textColourOnId,
                         juce::Colours::white);

  if (currentState == UIState::Idle) {
    actionButton.setButtonText("Iniciar Grabacion (Espacio)");
  } else if (currentState == UIState::Countdown) {
    actionButton.setButtonText("Comenzando en " + juce::String(countdownTicks) +
                               "... (Detener)");
  } else if (currentState == UIState::Recording) {
    actionButton.setButtonText("GRABANDO... [Detener (Espacio)]");
  }

  resized();
  repaint();
}

void GuitarDIRecorderViewComponent::timerCallback() {
  // 1. Leer nivel de entrada de la DI
  currentMeterLevel = audioProcessor.diInputLevel.load();

  // 2. Gestionar la cuenta regresiva frame a frame (30 Hz)
  static int frameCounter = 0;
  if (currentState == UIState::Countdown) {
    frameCounter++;
    if (frameCounter >= 30) {
      frameCounter = 0;
      countdownTicks--;

      if (countdownTicks <= 0) {
        audioProcessor.startDIRecording();
        currentState = UIState::Recording;
      }
      updateStateUI();
    }
  } else {
    frameCounter = 0;
  }

  // Repintar la zona del medidor de señal vertical en el extremo derecho
  repaint(getLocalBounds().removeFromRight(80));
}

bool GuitarDIRecorderViewComponent::keyPressed(const juce::KeyPress &key) {
  // Acceso directo con barra espaciadora para iniciar/detener
  if (key.getKeyCode() == juce::KeyPress::spaceKey) {
    if (actionButton.isVisible() && actionButton.isEnabled()) {
      actionButton.triggerClick();
      return true;
    }
  }
  return juce::Component::keyPressed(key);
}

void GuitarDIRecorderViewComponent::paint(juce::Graphics &g) {
  juce::Colour textColor = juce::Colours::white;
  juce::Colour meterBg = juce::Colours::darkgrey.darker();
  juce::Colour meterFill = juce::Colours::cyan;

  if (auto *lf = dynamic_cast<CustomLookAndFeel *>(&getLookAndFeel())) {
    if (auto *v4 = dynamic_cast<juce::LookAndFeel_V4 *>(lf)) {
      textColor = v4->getCurrentColourScheme().getUIColour(
          juce::LookAndFeel_V4::ColourScheme::defaultText);
      meterBg = v4->getCurrentColourScheme().getUIColour(
          juce::LookAndFeel_V4::ColourScheme::widgetBackground);
    }

    if (currentState == UIState::Recording)
      meterFill = lf->findColour(CustomLookAndFeel::dangerColourId);
    else
      meterFill = lf->findColour(CustomLookAndFeel::data1ColourId);
  }

  g.fillAll(juce::Colours::transparentBlack);

  // =========================================================================
  // Vúmetro Vertical y LED de Grabación en el Lateral Derecho
  // =========================================================================
  auto rightStrip = getLocalBounds().removeFromRight(80).toFloat();

  // 1. LED Rótulo Superior
  g.setColour(textColor.withAlpha(0.6f));
  g.setFont(juce::Font(10.0f, juce::Font::bold));
  g.drawText("REC", rightStrip.removeFromTop(20), juce::Justification::centredBottom, true);

  // 2. Dibujar LED Indicador
  float ledSize = 14.0f;
  float ledX = rightStrip.getCentreX() - ledSize * 0.5f;
  float ledY = rightStrip.getY() + 4.0f;

  if (currentState == UIState::Recording) {
    // Brillo exterior (Glow)
    g.setColour(juce::Colours::red.withAlpha(0.3f));
    g.fillEllipse(ledX - 4.0f, ledY - 4.0f, ledSize + 8.0f, ledSize + 8.0f);
    // LED encendido
    g.setColour(juce::Colours::red);
    g.fillEllipse(ledX, ledY, ledSize, ledSize);
  } else {
    // LED apagado en reposo
    g.setColour(juce::Colours::darkred.withAlpha(0.3f));
    g.fillEllipse(ledX, ledY, ledSize, ledSize);
  }
  rightStrip.removeFromTop(25); // Espaciador bajo el LED

  // 3. Etiqueta de la señal de entrada
  g.setColour(textColor);
  g.setFont(juce::Font(12.0f, juce::Font::bold));
  g.drawText("IN", rightStrip.removeFromTop(20), juce::Justification::centred, true);

  // 4. Barra del Vúmetro Vertical
  // Ocupa una altura segura sin desbordar hacia arriba ni abajo
  auto meterBounds = rightStrip.removeFromTop(200).withSizeKeepingCentre(16.0f, 200.0f);

  g.setColour(meterBg);
  g.fillRoundedRectangle(meterBounds, 6.0f);

  // Convertir nivel lineal a dBs para calcular la altura de relleno
  float db = juce::Decibels::gainToDecibels(currentMeterLevel, -60.0f);
  float fillRatio = juce::jmap(db, -60.0f, 0.0f, 0.0f, 1.0f);
  fillRatio = juce::jlimit(0.0f, 1.0f, fillRatio);

  if (fillRatio > 0.0f) {
    g.setColour(meterFill);
    float filledHeight = meterBounds.getHeight() * fillRatio;
    auto fillBounds = meterBounds.withTrimmedTop(meterBounds.getHeight() - filledHeight);
    g.fillRoundedRectangle(fillBounds, 6.0f);
  }
}

void GuitarDIRecorderViewComponent::resized() {
  // Dejamos el lado derecho libre en exclusiva para el Vúmetro vertical
  auto leftArea = getLocalBounds().reduced(30).withTrimmedRight(80);

  if (currentState == UIState::Review) {
    leftArea.removeFromTop(40); // Margen superior
    
    // Fila para el nombre del archivo
    auto nameInputBounds = leftArea.removeFromTop(40);
    namePromptLabel.setBounds(nameInputBounds.removeFromLeft(160));
    nameInput.setBounds(nameInputBounds.removeFromLeft(300));

    leftArea.removeFromTop(40); // Espaciador central

    // Fila de botones Guardar y Reintentar
    auto buttonArea = leftArea.removeFromTop(50);
    retryButton.setBounds(buttonArea.removeFromLeft(200));
    buttonArea.removeFromLeft(20); // Espaciador
    saveButton.setBounds(buttonArea.removeFromLeft(200));
  } else {
    // Durante los estados de Grabación/Espera, centramos el botón principal
    auto buttonArea = leftArea.withSizeKeepingCentre(350, 60);
    actionButton.setBounds(buttonArea);
  }
}
