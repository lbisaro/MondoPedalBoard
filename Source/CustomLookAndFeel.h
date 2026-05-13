#pragma once
#include <JuceHeader.h>

class CustomLookAndFeel : public juce::LookAndFeel_V4 {
public:
  enum CustomColourIds {
    data1ColourId = 0x2000000,
    data2ColourId,
    data3ColourId,
    data4ColourId,
    defaultColourId,
    successColourId,
    infoColourId,
    warningColourId,
    dangerColourId
  };

  // =========================================================================
  // Helper para interpretar colores exactamente como te los da VS Code
  // (CSS/Web) Soporta formato web: RRGGBB y RRGGBBAA (con o sin el # al
  // principio)
  // =========================================================================
  static juce::Colour parseWebColor(juce::String hex) {
    hex = hex.removeCharacters("#");

    // Si pasaste un color web normal de 6 letras (ej: 1e1e1e)
    if (hex.length() == 6) {
      return juce::Colour::fromString("ff" + hex); // Lo hace 100% opaco
    }
    // Si pasaste un color web con opacidad de 8 letras (ej: 007acc7c) ->
    // RRGGBBAA
    else if (hex.length() == 8) {
      // JUCE necesita AARRGGBB, así que movemos las 2 últimas letras (Alpha) al
      // principio
      juce::String alpha = hex.substring(6, 8);
      juce::String rgb = hex.substring(0, 6);
      return juce::Colour::fromString(alpha + rgb);
    }

    return juce::Colours::black; // Fallback por si hay un error
  }

  CustomLookAndFeel() {
    // =========================================================================
    // 1. DEFINICIÓN DE COLORES (FORMATO WEB: RRGGBB o RRGGBBAA)
    // ¡AHORA SÍ! Puedes pegar exactamente el código que te da VS Code.
    // =========================================================================

    // Color principal que ves en el fondo de las ventanas y paneles generales.
    juce::String strWindowBackground = "1e1e1e"; // #1e1e1e

    // Color de fondo para controles como las cajas de texto o el área interna
    // de listas.
    juce::String strWidgetBackground = "252526"; // #252526

    // Fondo para menús desplegables (Pop-up Menus).
    juce::String strMenuBackground = "ffd0d0d0"; // #ffd0d0d0

    // Para dibujar los bordes y contornos de botones, ventanas y sliders.
    juce::String strOutline = "007acc"; // #007acc

    // Color principal de la tipografía (etiquetas, texto de botones, etc).
    juce::String strDefaultText = "c8ffffff"; // #c8ffffff

    // Para los rellenos por defecto (como el cuerpo de un slider cuando no está
    // activo).
    juce::String strDefaultFill = "ffd8d8d8"; // #ffd8d8d8

    // Para el texto cuando pasas el mouse por encima o seleccionas un elemento.
    juce::String strHighlightedText = "ffffffff"; // #ffffffff

    // Para el "brillo" o resalte cuando un botón está presionado o un área está
    // seleccionada.
    juce::String strHighlightedFill = "7cdcfe"; // #7cdcfe

    // Para el texto dentro de los menús desplegables (que tienen fondo claro).
    juce::String strMenuText = "ff000000"; // #ff000000

    // =========================================================================
    // 2. APLICACIÓN DE LOS COLORES AL ESQUEMA
    // =========================================================================

    // Creamos una paleta basada en Midnight como punto de partida
    auto scheme = juce::LookAndFeel_V4::getMidnightColourScheme();

    // Transformamos los strings a hexadecimal usando nuestro nuevo helper
    scheme.setUIColour(juce::LookAndFeel_V4::ColourScheme::windowBackground,
                       parseWebColor(strWindowBackground));
    scheme.setUIColour(juce::LookAndFeel_V4::ColourScheme::widgetBackground,
                       parseWebColor(strWidgetBackground));
    scheme.setUIColour(juce::LookAndFeel_V4::ColourScheme::menuBackground,
                       parseWebColor(strMenuBackground));
    scheme.setUIColour(juce::LookAndFeel_V4::ColourScheme::outline,
                       parseWebColor(strOutline));
    scheme.setUIColour(juce::LookAndFeel_V4::ColourScheme::defaultText,
                       parseWebColor(strDefaultText));
    scheme.setUIColour(juce::LookAndFeel_V4::ColourScheme::defaultFill,
                       parseWebColor(strDefaultFill));
    scheme.setUIColour(juce::LookAndFeel_V4::ColourScheme::highlightedText,
                       parseWebColor(strHighlightedText));
    scheme.setUIColour(juce::LookAndFeel_V4::ColourScheme::highlightedFill,
                       parseWebColor(strHighlightedFill));
    scheme.setUIColour(juce::LookAndFeel_V4::ColourScheme::menuText,
                       parseWebColor(strMenuText));

    // Aplicamos nuestro esquema personalizado al LookAndFeel
    setColourScheme(scheme);

    // =========================================================================
    // 3. COLORES CUSTOM (DATA Y ESTADOS)
    // =========================================================================
    setColour(data1ColourId, parseWebColor("f38d48")); // #f38d48
    setColour(data2ColourId, parseWebColor("8dc63f")); // #8dc63f
    setColour(data3ColourId, parseWebColor("018699")); // #018699
    setColour(data4ColourId, parseWebColor("a95a96")); // #a95a96

    setColour(defaultColourId, parseWebColor("1e73be")); // #1e73be
    setColour(successColourId, parseWebColor("73af1f")); // #73af1f
    setColour(infoColourId, parseWebColor("5ba5ef"));    // #5ba5ef
    setColour(warningColourId, parseWebColor("ffd468")); // #ffd468
    setColour(dangerColourId, parseWebColor("f42929"));  // #f42929
  }
};
