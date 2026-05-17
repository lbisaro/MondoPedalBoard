#pragma once
#include <JuceHeader.h>

class IconLibrary
{
public:
    // =========================================================================
    // Catálogo Centralizado de Trazos Vectoriales (Iconos de 24x24 píxeles)
    // =========================================================================

    static juce::Path getHomePath()
    {
        juce::Path p;
        // Contorno exterior principal curvo premium basado en tu código SVG
        p.startNewSubPath (22.0f, 12.2039f);
        p.lineTo (22.0f, 13.725f);
        p.cubicTo (22.0f, 17.6258f, 22.0f, 19.5763f, 20.8284f, 20.7881f);
        p.cubicTo (19.6569f, 22.0f, 17.7712f, 22.0f, 14.0f, 22.0f);
        p.lineTo (10.0f, 22.0f);
        p.cubicTo (6.22876f, 22.0f, 4.34315f, 22.0f, 3.17157f, 20.7881f);
        p.cubicTo (2.0f, 19.5763f, 2.0f, 17.6258f, 2.0f, 13.725f);
        p.lineTo (2.0f, 12.2039f);
        p.cubicTo (2.0f, 9.91549f, 2.0f, 8.77128f, 2.5192f, 7.82274f);
        p.cubicTo (3.0384f, 6.87421f, 3.98695f, 6.28551f, 5.88403f, 5.10813f);
        p.lineTo (7.88403f, 3.86687f);
        p.cubicTo (9.88939f, 2.62229f, 10.8921f, 2.0f, 12.0f, 2.0f);
        p.cubicTo (13.1079f, 2.0f, 14.1106f, 2.62229f, 16.116f, 3.86687f);
        p.lineTo (18.116f, 5.10812f);
        p.cubicTo (20.0131f, 6.28551f, 20.9616f, 6.87421f, 21.4808f, 7.82274f);
        p.closeSubPath();

        // Trazo horizontal interior
        p.startNewSubPath (15.0f, 18.0f);
        p.lineTo (9.0f, 18.0f);

        // Convertimos el trazo lineal en un contorno relleno para que el ShapeButton
        // dibuje la silueta hueca (exactamente como se ve en el diseño original)
        juce::PathStrokeType stroke (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
        juce::Path strokedPath;
        stroke.createStrokedPath (strokedPath, p);

        return strokedPath;
    }

    static juce::Path getPlusPath()
    {
        juce::Path p;
        // Trazo horizontal cruzando el centro
        p.addRoundedRectangle (4.0f, 10.5f, 16.0f, 3.0f, 1.0f);
        // Trazo vertical cruzando el centro
        p.addRoundedRectangle (10.5f, 4.0f, 3.0f, 16.0f, 1.0f);
        return p;
    }

    static juce::Path getBackArrowPath()
    {
        juce::Path p;
        // Punta de flecha hacia la izquierda
        p.addTriangle (3.0f, 12.0f, 11.0f, 4.0f, 11.0f, 20.0f);
        // Cuerpo (tallo) de la flecha
        p.addRoundedRectangle (10.0f, 10.0f, 11.0f, 4.0f, 1.0f);
        return p;
    }

    static juce::Path getTrashPath()
    {
        juce::Path p;
        // Tapa superior de la papelera
        p.addRoundedRectangle (4.0f, 4.0f, 16.0f, 2.0f, 0.5f);
        // Agarre central de la tapa
        p.addRectangle (9.0f, 2.0f, 6.0f, 2.0f);
        // Contenedor principal
        p.addRoundedRectangle (5.0f, 7.0f, 14.0f, 15.0f, 1.5f);
        return p;
    }

    static juce::Path getSettingsPath()
    {
        juce::Path p;
        
        // 1. Círculo central concéntrico: cx="12" cy="12" r="3"
        p.addEllipse (9.0f, 9.0f, 6.0f, 6.0f);

        // 2. Trazado exterior curvilíneo suave del engranaje
        p.startNewSubPath (13.7654f, 2.15224f);
        p.cubicTo (13.3978f, 2.0f, 12.9319f, 2.0f, 12.0f, 2.0f);
        p.cubicTo (11.0681f, 2.0f, 10.6022f, 2.0f, 10.2346f, 2.15224f);
        p.cubicTo (9.74457f, 2.35523f, 9.35522f, 2.74458f, 9.15223f, 3.23463f);
        p.cubicTo (9.05957f, 3.45834f, 9.0233f, 3.7185f, 9.00911f, 4.09799f);
        p.cubicTo (8.98826f, 4.65568f, 8.70226f, 5.17189f, 8.21894f, 5.45093f);
        p.cubicTo (7.73564f, 5.72996f, 7.14559f, 5.71954f, 6.65219f, 5.45876f);
        p.cubicTo (6.31645f, 5.2813f, 6.07301f, 5.18262f, 5.83294f, 5.15102f);
        p.cubicTo (5.30704f, 5.08178f, 4.77518f, 5.22429f, 4.35436f, 5.5472f);
        p.cubicTo (4.03874f, 5.78938f, 3.80577f, 6.1929f, 3.33983f, 6.99993f);
        p.cubicTo (2.87389f, 7.80697f, 2.64092f, 8.21048f, 2.58899f, 8.60491f);
        p.cubicTo (2.51976f, 9.1308f, 2.66227f, 9.66266f, 2.98518f, 10.0835f);
        p.cubicTo (3.13256f, 10.2756f, 3.3397f, 10.437f, 3.66119f, 10.639f);
        p.cubicTo (4.1338f, 10.936f, 4.43789f, 11.4419f, 4.43786f, 12.0f);
        p.cubicTo (4.43783f, 12.5581f, 4.13375f, 13.0639f, 3.66118f, 13.3608f);
        p.cubicTo (3.33965f, 13.5629f, 3.13248f, 13.7244f, 2.98508f, 13.9165f);
        p.cubicTo (2.66217f, 14.3373f, 2.51966f, 14.8691f, 2.5889f, 15.395f);
        p.cubicTo (2.64082f, 15.7894f, 2.87379f, 16.193f, 3.33973f, 17.0f);
        p.cubicTo (3.80568f, 17.807f, 4.03865f, 18.2106f, 4.35426f, 18.4527f);
        p.cubicTo (4.77508f, 18.7756f, 5.30694f, 18.9181f, 5.83284f, 18.8489f);
        p.cubicTo (6.07289f, 18.8173f, 6.31632f, 18.7186f, 6.65204f, 18.5412f);
        p.cubicTo (7.14547f, 18.2804f, 7.73556f, 18.27f, 8.2189f, 18.549f);
        p.cubicTo (8.70224f, 18.8281f, 8.98826f, 19.3443f, 9.00911f, 19.9021f);
        p.cubicTo (9.02331f, 20.2815f, 9.05957f, 20.5417f, 9.15223f, 20.7654f);
        p.cubicTo (9.35522f, 21.2554f, 9.74457f, 21.6448f, 10.2346f, 21.8478f);
        p.cubicTo (10.6022f, 22.0f, 11.0681f, 22.0f, 12.0f, 22.0f);
        p.cubicTo (12.9319f, 22.0f, 13.3978f, 22.0f, 13.7654f, 21.8478f);
        p.cubicTo (14.2554f, 21.6448f, 14.6448f, 21.2554f, 14.8477f, 20.7654f);
        p.cubicTo (14.9404f, 20.5417f, 14.9767f, 20.2815f, 14.9909f, 19.902f);
        p.cubicTo (15.0117f, 19.3443f, 15.2977f, 18.8281f, 15.781f, 18.549f);
        p.cubicTo (16.2643f, 18.2699f, 16.8544f, 18.2804f, 17.3479f, 18.5412f);
        p.cubicTo (17.6836f, 18.7186f, 17.927f, 18.8172f, 18.167f, 18.8488f);
        p.cubicTo (18.6929f, 18.9181f, 19.2248f, 18.7756f, 19.6456f, 18.4527f);
        p.cubicTo (19.9612f, 18.2105f, 20.1942f, 17.807f, 20.6601f, 16.9999f);
        p.cubicTo (21.1261f, 16.1929f, 21.3591f, 15.7894f, 21.411f, 15.395f);
        p.cubicTo (21.4802f, 14.8691f, 21.3377f, 14.3372f, 21.0148f, 13.9164f);
        p.cubicTo (20.8674f, 13.7243f, 20.6602f, 13.5628f, 20.3387f, 13.3608f);
        p.cubicTo (19.8662f, 13.0639f, 19.5621f, 12.558f, 19.5621f, 11.9999f);
        p.cubicTo (19.5621f, 11.4418f, 19.8662f, 10.9361f, 20.3387f, 10.6392f);
        p.cubicTo (20.6603f, 10.4371f, 20.8675f, 10.2757f, 21.0149f, 10.0835f);
        p.cubicTo (21.3378f, 9.66273f, 21.4803f, 9.13087f, 21.4111f, 8.60497f);
        p.cubicTo (21.3592f, 8.21055f, 21.1262f, 7.80703f, 20.6602f, 7.0f);
        p.cubicTo (20.1943f, 6.19297f, 19.9613f, 5.78945f, 19.6457f, 5.54727f);
        p.cubicTo (19.2249f, 5.22436f, 18.693f, 5.08185f, 18.1671f, 5.15109f);
        p.cubicTo (17.9271f, 5.18269f, 17.6837f, 5.28136f, 17.3479f, 5.4588f);
        p.cubicTo (16.8545f, 5.71959f, 16.2644f, 5.73002f, 15.7811f, 5.45096f);
        p.cubicTo (15.2977f, 5.17191f, 15.0117f, 4.65566f, 14.9909f, 4.09794f);
        p.cubicTo (14.9767f, 3.71848f, 14.9404f, 3.45833f, 14.8477f, 3.23463f);
        p.cubicTo (14.6448f, 2.74458f, 14.2554f, 2.35523f, 13.7654f, 2.15224f);
        p.closeSubPath();

        // Convertimos el trazado lineal base en un contorno de silueta rellenada de 1.5px
        juce::PathStrokeType stroke (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
        juce::Path strokedPath;
        stroke.createStrokedPath (strokedPath, p);
        return strokedPath;
    }

    static juce::Path getPlayPath()
    {
        juce::Path p;
        p.startNewSubPath (16.6582f, 9.28638f);
        p.cubicTo (18.098f, 10.1862f, 18.8178f, 10.6361f, 19.0647f, 11.2122f);
        p.cubicTo (19.2803f, 11.7152f, 19.2803f, 12.2847f, 19.0647f, 12.7878f);
        p.cubicTo (18.8178f, 13.3638f, 18.098f, 13.8137f, 16.6582f, 14.7136f);
        p.lineTo (9.896f, 18.94f);
        p.cubicTo (8.29805f, 19.9387f, 7.49907f, 20.4381f, 6.83973f, 20.385f);
        p.cubicTo (6.26501f, 20.3388f, 5.73818f, 20.0469f, 5.3944f, 19.584f);
        p.cubicTo (5.0f, 19.053f, 5.0f, 18.1108f, 5.0f, 16.2264f);
        p.lineTo (5.0f, 7.77357f);
        p.cubicTo (5.0f, 5.88919f, 5.0f, 4.94701f, 5.3944f, 4.41598f);
        p.cubicTo (5.73818f, 3.9531f, 6.26501f, 3.66111f, 6.83973f, 3.6149f);
        p.cubicTo (7.49907f, 3.5619f, 8.29805f, 4.06126f, 9.896f, 5.05998f);
        p.lineTo (16.6582f, 9.28638f);
        p.closeSubPath();

        juce::PathStrokeType stroke (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
        juce::Path strokedPath;
        stroke.createStrokedPath (strokedPath, p);
        return strokedPath;
    }

    static juce::Path getStopPath()
    {
        juce::Path p;
        p.startNewSubPath (2.0f, 12.0f);
        p.cubicTo (2.0f, 7.28595f, 2.0f, 4.92893f, 3.46447f, 3.46447f);
        p.cubicTo (4.92893f, 2.0f, 7.28595f, 2.0f, 12.0f, 2.0f);
        p.cubicTo (16.714f, 2.0f, 19.0711f, 2.0f, 20.5355f, 3.46447f);
        p.cubicTo (22.0f, 4.92893f, 22.0f, 7.28595f, 22.0f, 12.0f);
        p.cubicTo (22.0f, 16.714f, 22.0f, 19.0711f, 20.5355f, 20.5355f);
        p.cubicTo (19.0711f, 22.0f, 16.714f, 22.0f, 12.0f, 22.0f);
        p.cubicTo (7.28595f, 22.0f, 4.92893f, 22.0f, 3.46447f, 20.5355f);
        p.cubicTo (2.0f, 19.0711f, 2.0f, 16.714f, 2.0f, 12.0f);
        p.closeSubPath();

        juce::PathStrokeType stroke (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
        juce::Path strokedPath;
        stroke.createStrokedPath (strokedPath, p);
        return strokedPath;
    }

    static juce::Path getResetPath()
    {
        juce::Path p;
        p.startNewSubPath (18.2577f, 3.50828f);
        p.cubicTo (18.538f, 3.62437f, 18.7207f, 3.89785f, 18.7207f, 4.20119f);
        p.lineTo (18.7207f, 8.44383f);
        p.cubicTo (18.7207f, 8.85805f, 18.3849f, 9.19383f, 17.9707f, 9.19383f);
        p.lineTo (13.728f, 9.19383f);
        p.cubicTo (13.4247f, 9.19383f, 13.1512f, 9.0111f, 13.0351f, 8.73085f);
        p.cubicTo (12.9191f, 8.45059f, 12.9832f, 8.128f, 13.1977f, 7.9135f);
        p.lineTo (14.8007f, 6.3105f);
        p.cubicTo (12.1674f, 5.20912f, 9.01606f, 5.7309f, 6.87348f, 7.87348f);
        p.cubicTo (4.04217f, 10.7048f, 4.04217f, 15.2952f, 6.87348f, 18.1265f);
        p.cubicTo (9.70478f, 20.9578f, 14.2952f, 20.9578f, 17.1265f, 18.1265f);
        p.cubicTo (18.7727f, 16.4803f, 19.4622f, 14.2401f, 19.1935f, 12.0937f);
        p.cubicTo (19.142f, 11.6827f, 19.4335f, 11.3078f, 19.8445f, 11.2563f);
        p.cubicTo (20.2555f, 11.2049f, 20.6304f, 11.4963f, 20.6819f, 11.9073f);
        p.cubicTo (21.0057f, 14.4934f, 20.1746f, 17.1997f, 18.1872f, 19.1872f);
        p.cubicTo (14.7701f, 22.6043f, 9.2299f, 22.6043f, 5.81282f, 19.1872f);
        p.cubicTo (2.39573f, 15.7701f, 2.39573f, 10.2299f, 5.81282f, 6.81282f);
        p.cubicTo (8.55119f, 4.07444f, 12.6515f, 3.5312f, 15.9309f, 5.18028f);
        p.lineTo (17.4404f, 3.67086f);
        p.cubicTo (17.6549f, 3.45637f, 17.9774f, 3.3922f, 18.2577f, 3.50828f);
        p.closeSubPath();
        return p;
    }

    // =========================================================================
    // Buscador Genérico por Nombre de Icono
    // =========================================================================
    static juce::Path getIcon (const juce::String& name)
    {
        if (name.equalsIgnoreCase ("home"))        return getHomePath();
        if (name.equalsIgnoreCase ("plus"))        return getPlusPath();
        if (name.equalsIgnoreCase ("arrow_left"))  return getBackArrowPath();
        if (name.equalsIgnoreCase ("trash"))       return getTrashPath();
        if (name.equalsIgnoreCase ("settings"))    return getSettingsPath();
        if (name.equalsIgnoreCase ("play"))        return getPlayPath();
        if (name.equalsIgnoreCase ("stop"))        return getStopPath();
        if (name.equalsIgnoreCase ("reset"))       return getResetPath();
        
        return juce::Path();
    }
};
