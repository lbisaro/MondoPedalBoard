# MondoPedalBoard & MondoHelixAnalyzer 🎸📊

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)](#)
[![Platform](https://img.shields.io/badge/platform-Windows%20ASIO-blue.svg)](#)
[![Framework](https://img.shields.io/badge/framework-JUCE-orange.svg)](#)
[![Tone Guide](https://img.shields.io/badge/tone--guide-Helix%20Calibrated-purple.svg)](helix_tone_sculpting_guide.md)

**MondoHelixAnalyzer** (integrado en **MondoPedalBoard**) es una suite de diagnóstico y calibración tímbrica científica en tiempo real y offline diseñada para guitarristas e ingenieros de sonido que utilizan procesadores de la familia **Line 6 Helix** (*Helix Floor, LT, Rack, Control y Helix Native*).

El sistema analiza y compara la señal de tu guitarra real con perfiles acústicos de tonos legendarios, permitiéndote esculpir presets perfectos a través de métricas dinámicas y espectrales tridimensionales.

---

## 🧭 Enlace Rápido al Manual de Calibración

Si buscas el proceso paso a paso para modelar tus presets físicos de la Helix usando esta aplicación, consulta nuestra guía dedicada en la raíz de este repositorio:

👉 **[Guía de Calibración de Tono en Line 6 Helix](helix_tone_sculpting_guide.md)** *(Aprende a correlacionar Bass, Treble, Presence e IRs con los medidores de la app).*

---

## ✨ Características Principales

La suite de MondoHelixAnalyzer está dividida en dos módulos robustos y profesionales:

### 1. Real-Time Preset Analyzer ⚡
Un tablero (dashboard) de control interactivo en tiempo real estructurado en una **grilla simétrica premium de 4 columnas** que procesa la señal de retorno de tu Helix:
*   **Target Profile Configurator:** Modifica dinámicamente las tolerancias del analizador según tres perfiles base: `AMBIENT`, `RHYTHM` o `LEAD`, o carga curvas de referencia personalizadas.
*   **Loudness (EBU R128):** Medición integrada en LUFS para nivelar la salida del patch de forma óptima sin clipping digital.
*   **Dynamics (PLR):** Monitoreo del *Peak-to-Loudness Ratio* en tiempo real para calibrar compresores y saturaciones de válvulas.
*   **La Trilogía Tímbrica Espectral (en %):**
    *   **Tonal Body (%):** Energía acumulada en graves y medios-bajos (`100 Hz - 500 Hz`) para dar solidez sin retumbar.
    *   **Tonal Cut (%):** Claridad en medios-altos (`2 kHz - 5 kHz`) para cortar en la mezcla grupal.
    *   **Brightness Band Energy (%):** ¡Nueva métrica inteligente! Mide el porcentaje de agudos en una banda de frecuencia **que se desplaza dinámicamente** según tu target profile (`600Hz-1500Hz` en Ambient, `1200Hz-2000Hz` en Rhythm, y `1900Hz-2700Hz` en Lead).
*   **Spectral Brightness (Hz):** Centroide espectral físico absoluto en Hertz para ubicar el centro de gravedad tonal de tu preset.
*   **Guitar (Live) Mode:** Calibra de forma interactiva tocando tu instrumento real. Silencia la inyección USB hacia la Helix para priorizar el jack físico de la guitarra, mientras analiza y monitorea el retorno estéreo en tiempo real.

### 2. Offline Samples Analyzer 💾
Carga archivos de audio comercial (`.wav` o `.mp3`) de tus producciones de guitarra favoritas, ejecútalos en un hilo secundario y extrae su ADN tímbrico:
*   **Cálculo Offline:** Mide y almacena de forma instantánea el volumen integrado, el PLR dinámico, el centroide en Hz y los porcentajes espectrales ideales (incluyendo el nuevo brillo adaptable).
*   **Persistencia JSON:** Guarda los análisis bajo archivos extendidos de perfil `.gtr_analysis`.
*   **Visualización Síncrona:** Una interfaz idéntica de 4 columnas para contrastar la muestra de referencia directamente antes de cargarla en el Preset Analyzer en vivo.

---

## 🛠️ Arquitectura y Tecnologías Utilizadas

*   **Audio Engine (DSP Core):** Escrito en **C++** nativo de alto rendimiento utilizando el framework **JUCE** para manejo de búferes de audio ASIO de ultrabaja latencia.
*   **Filtros y FFT:** Procesamiento digital de señales mediante transformadas rápidas de Fourier (FFT) y promediado exponencial (EMA) para actualizaciones de UI suaves y estables.
*   **Visualizer:** Gráficos vectoriales vectorizados con aceleración nativa y renderizado de curvas de respuesta espectral FFT históricas (líneas amarillas y verdes de referencia).
*   **Internal Emulator:** Incluye un emulador interno de guitarra sintética (Karplus-Strong) y modelado físico RLC de micrófonos (*Guitar Emulator*) para calibración automatizada desatendida.

---

## 🚀 Instrucciones de Compilación y Configuración

El proyecto está configurado con **CMake** y preparado para generar ejecutables independientes (*Standalone*) y plugins de audio (*VST3*).

### Requisitos Previos:
1.  **Visual Studio 2022** (con soporte de desarrollo de escritorio en C++).
2.  **CMake** instalado en tu sistema.
3.  Controlador **ASIO de Line 6 Helix** (o controlador de audio multicanal ASIO configurado en Windows).

### Compilación desde Visual Studio (Recomendado):
1.  Navega a la carpeta `/build` en tu terminal o IDE.
2.  Abre el archivo de solución de Visual Studio:
    ```bash
    MondoHelixAnalyzer.sln
    ```
3.  Selecciona tu objetivo de compilación (ej. `MondoHelixAnalyzer_Standalone` o `MondoHelixAnalyzer_All`) en configuración `Debug` o `Release`.
4.  Presiona **F5** o haz clic en **Compilar solución**.
5.  ¡El ejecutable se generará en la carpeta `build/MondoHelixAnalyzer_artefacts/` listo para usarse!

---

## 📂 Estructura de Directorios

```
MondoPedalBoard/
├── Source/                             # Código fuente de C++ (JUCE)
│   ├── AnalysisEngine.h                # Algoritmos DSP en tiempo real (FFT, PLR, LUFS)
│   ├── SamplesOfflineAnalyzer.h        # Lógica de análisis de muestras en hilos secundarios
│   ├── PresetAnalyzerViewComponent.cpp # Interfaz gráfica en vivo (4 columnas simétricas)
│   ├── SamplesAnalyzerViewComponent.cpp# Interfaz gráfica offline (Muestras de referencia)
│   └── TargetProfiles.h                # Constantes y rangos de calibración espectral
├── artifacts/                          # Guías, reportes y recursos de la suite
├── build/                              # Configuración de compilación MSBuild/Visual Studio
├── helix_tone_sculpting_guide.md       # 🧭 Guía de Calibración de Tono en la Helix
└── README.md                           # 📖 Página Principal de GitHub (Este archivo)
```

---

## 🤝 Contribuciones

Si tienes sugerencias para mejorar el algoritmo de cálculo de energía tímbrica, optimizar el consumo de la FFT o agregar nuevos perfiles ideales de guitarra, ¡siéntete libre de abrir un Pull Request o crear un Issue en este repositorio!

*Diseñado científicamente por guitarristas para guitarristas.* 🎸⚡
