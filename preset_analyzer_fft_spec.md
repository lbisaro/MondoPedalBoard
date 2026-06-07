# Módulo Preset Analyzer: Algoritmo de Procesamiento de FFT y Renderizado de Curvas

Este documento describe paso a paso cómo se calcula, suaviza en frecuencia/tiempo y se grafica la señal de audio en el módulo **Preset Analyzer** de este proyecto. Esta especificación está diseñada para ser transmitida a un agente de IA en otro proyecto (por ejemplo, para implementaciones en Python, Web o VST).

---

## 1. Etapa de Procesamiento de Audio (AnalysisEngine)

El procesamiento espectral se realiza de forma continua en el motor de audio [AnalysisEngine](file:///c:/Users/lbisa/OneDrive/Documentos/soft/Line%206/MondoPedalBoard/Source/AnalysisEngine.h#L227) con los siguientes pasos:

### 1. Ventaneo y FFT
* **Tamaño de FFT ($N$):** $2048$ muestras (`fftOrder = 11`).
* **Número de Bins ($M$):** $1024$ bins (representando frecuencias desde $0 \text{ Hz}$ hasta la frecuencia de Nyquist $f_s / 2$).
* **Función de Ventana:** Se aplica una **ventana de Hann** (`juce::dsp::WindowingFunction<float>::hann`) sobre las 2048 muestras del bloque en el dominio del tiempo para reducir el ensanchamiento espectral (leakage).
* **Transformada:** Se realiza una FFT directa real-a-magnitud.
* **Normalización:** Las magnitudes de salida se dividen por el tamaño del bloque para normalizar la amplitud:
  $$\text{mag}[i] = \frac{\text{magnitud\_raw}[i]}{N}$$

### 2. Suavizado Temporal (Attack / Decay Asimétrico)
Para lograr una respuesta fluida (que reaccione al instante ante transitorios pero decaiga lentamente simulando un analizador analógico), se aplica un suavizado temporal asimétrico sobre cada bin del espectro (`latestFFTData`):
* **Parámetros:** $\alpha_{attack} = 0.3$, $\alpha_{decay} = 0.85$.
* **Ecuación:**
  $$\text{historia}[i] = \begin{cases} 
    0.3 \cdot \text{historia}[i] + 0.7 \cdot \text{nuevo}[i] & \text{si } \text{nuevo}[i] > \text{historia}[i] \quad (\text{Attack rápido}) \\
    0.85 \cdot \text{historia}[i] + 0.15 \cdot \text{nuevo}[i] & \text{si } \text{nuevo}[i] \le \text{historia}[i] \quad (\text{Decay lento})
  \end{cases}$$

---

## 2. Etapa de UI y Suavizado Frecuencial (FrequencyGraphComponent)

El componente gráfico [FrequencyGraphComponent](file:///c:/Users/lbisa/OneDrive/Documentos/soft/Line%206/MondoPedalBoard/Source/FrequencyGraphComponent.cpp#L3) recibe el espectro suavizado en el tiempo y realiza los siguientes cálculos antes del dibujado:

### 1. Suavizado por Octavas Fraccionales (Fractional Octave Smoothing)
Para que el gráfico coincida con la percepción auditiva humana, se aplica un suavizado de **1/6 de octava** (configurable mediante `smoothingDenominator`). Esto reemplaza cada bin por el promedio móvil de los bins dentro de su banda de octava correspondiente:

1. **Ancho de banda de la octava:**
   * $\text{fraction} = \frac{1}{\text{smoothingDenominator}}$ (por defecto $\frac{1}{6}$).
   * $\text{octaveFactor} = 2^{\frac{\text{fraction}}{2}}$.
   * Factores de límite inferior y superior: $L_{low} = \frac{1}{\text{octaveFactor}}$, $L_{high} = \text{octaveFactor}$.

2. **Cálculo por Bin:**
   Para cada bin $i$ con frecuencia central $f_c = i \cdot \frac{f_s}{N}$:
   * Si $f_c < 40 \text{ Hz}$, **no se aplica suavizado** para preservar la resolución en graves.
   * Si $f_c \ge 40 \text{ Hz}$, se calcula el rango de frecuencias a promediar:
     $$f_{low} = f_c \cdot L_{low}, \quad f_{high} = f_c \cdot L_{high}$$
   * Mapeo de frecuencias a índices de bins:
     $$\text{startBin} = \left\lfloor \frac{f_{low}}{f_s / N} \right\rfloor, \quad \text{endBin} = \left\lceil \frac{f_{high}}{f_s / N} \right\rceil$$
   * El valor resultante `smoothedData[i]` es el promedio aritmético de todos los bins de magnitud desde `startBin` hasta `endBin`.

### 2. Curva Representativa Acumulada (Peak Hold Envelope)
En lugar de promediar las tramas en el tiempo (lo que disolvería la firma tonal), el Preset Analyzer registra la **envolvente máxima** (Peak Hold) de la señal suavizada. Esto crea una silueta del preset acumulando el pico histórico para cada bin de frecuencia:
$$\text{representativeCurve}[i] = \max(\text{representativeCurve}[i], \, \text{smoothedData}[i])$$

---

## 3. Renderizado y Mapeo Gráfico (paint)

Para graficar los datos en la pantalla, se realiza una conversión logarítmica sesgada de las coordenadas:

### 1. Mapeo de Coordenadas X (Logarítmico con Skew)
El oído humano percibe el tono de forma logarítmica. Para expandir el área visual asignada a las altas frecuencias y mantener una correcta proporción:
* Rango: $f_{min} = 10 \text{ Hz}$ a $f_{max} = 20000 \text{ Hz}$.
* Factor de Sesgo (`skewFactor`): $1.5$.
* Fórmula para obtener la coordenada normalizada $X_{norm} \in [0.0, 1.0]$ para una frecuencia $f$:
  $$norm = \frac{\log_{10}(f) - \log_{10}(f_{min})}{\log_{10}(f_{max}) - \log_{10}(f_{min})}$$
  $$X_{norm} = norm^{skewFactor}$$
  $$X = \text{left} + \text{width} \cdot X_{norm}$$

### 2. Mapeo de Coordenadas Y (Decibelios Lineales)
* Rango dinámico visual: $dB_{min} = -80.0 \text{ dB}$ a $dB_{max} = 0.0 \text{ dB}$.
* Para cada magnitud lineal $mag$:
  $$dB = 20 \log_{10}(mag)$$
  $$Y_{norm} = \text{map}(dB, \, dB_{min}, \, dB_{max}, \, 0.0, \, 1.0)$$ (clamped entre $0.0$ y $1.0$).
  $$Y = \text{bottom} - \text{height} \cdot Y_{norm}$$

### 3. Dibujado de Curvas en Pantalla
El componente renderiza tres capas independientes:
1. **Live FFT Curve:** Se traza una línea continua nítida (en color Cian) conectando los puntos $(X, Y)$ calculados a partir de `smoothedData`.
2. **Representative Envelope:** Utilizando `representativeCurve`, se cierra un camino poligonal (Path) con la parte inferior del gráfico ($\text{bottom}$) y se rellena con un degradado o color cian translúcido ($\alpha \approx 0.22$).
3. **Static Reference Curve:** Si se carga una firma tonal de referencia, se grafica como una línea de color verde lima.
