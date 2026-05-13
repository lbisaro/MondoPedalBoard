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

    // Color principal de fondo de ventanas y paneles generales.
    juce::String strWindowBackground = "1a1a24"; // Gris-azul ultra profundo

    // Color de fondo para controles (cajas de texto, combo boxes, listas).
    juce::String strWidgetBackground = "22222f";

    // Fondo para menús desplegables (Pop-up Menus).
    juce::String strMenuBackground = "2a2a3a"; // Tono oscuro azulado elegante

    // Bordes y contornos limpios.
    juce::String strOutline = "3a3a4f";

    // Color principal de la tipografía.
    juce::String strDefaultText = "e0e0e0";

    // Rellenos inactivos genéricos.
    juce::String strDefaultFill = "404055";

    // Texto resaltado o al pasar el mouse.
    juce::String strHighlightedText = "ffffff";

    // Relleno de selección/resalte (Azul corporativo premium).
    juce::String strHighlightedFill = "1e73be";

    // Texto dentro de los menús desplegables.
    juce::String strMenuText = "ffffff";

    // =========================================================================
    // 2. APLICACIÓN DE LOS COLORES AL ESQUEMA
    // =========================================================================
    auto scheme = juce::LookAndFeel_V4::getMidnightColourScheme();

    scheme.setUIColour(juce::LookAndFeel_V4::ColourScheme::windowBackground, parseWebColor(strWindowBackground));
    scheme.setUIColour(juce::LookAndFeel_V4::ColourScheme::widgetBackground, parseWebColor(strWidgetBackground));
    scheme.setUIColour(juce::LookAndFeel_V4::ColourScheme::menuBackground, parseWebColor(strMenuBackground));
    scheme.setUIColour(juce::LookAndFeel_V4::ColourScheme::outline, parseWebColor(strOutline));
    scheme.setUIColour(juce::LookAndFeel_V4::ColourScheme::defaultText, parseWebColor(strDefaultText));
    scheme.setUIColour(juce::LookAndFeel_V4::ColourScheme::defaultFill, parseWebColor(strDefaultFill));
    scheme.setUIColour(juce::LookAndFeel_V4::ColourScheme::highlightedText, parseWebColor(strHighlightedText));
    scheme.setUIColour(juce::LookAndFeel_V4::ColourScheme::highlightedFill, parseWebColor(strHighlightedFill));
    scheme.setUIColour(juce::LookAndFeel_V4::ColourScheme::menuText, parseWebColor(strMenuText));

    setColourScheme(scheme);

    // Sobrescribimos explícitamente los IDs de Popups y ComboBoxes para garantizar el estilo Dark UI
    setColour (juce::PopupMenu::backgroundColourId, parseWebColor("2a2a3a"));
    setColour (juce::PopupMenu::textColourId, parseWebColor("ffffff"));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, parseWebColor("1e73be"));
    setColour (juce::PopupMenu::highlightedTextColourId, parseWebColor("ffffff"));

    setColour (juce::ComboBox::backgroundColourId, parseWebColor("22222f"));
    setColour (juce::ComboBox::textColourId, parseWebColor("ffffff"));
    setColour (juce::ComboBox::outlineColourId, parseWebColor("3a3a4f"));
    setColour (juce::ComboBox::arrowColourId, parseWebColor("5ba5ef"));

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
