# COOLTONE — BY ORYMY
**YMY RECORDS** · Afinador y editor de interpretación vocal · VST3 + Standalone · Windows 10/11 x64

> Versión **0.1.0 — Fase 1 (núcleo DSP)**. Esto **todavía no es el plugin**: no hay VST3, ni interfaz, ni instalador.
> Este README dice con exactitud qué existe y qué no. Nada se presenta como terminado si no lo está.

## Estado por módulo

| Módulo | Estado | Verificación |
|---|---|---|
| `ScaleEngine` (14 escalas, 12 tonalidades, notas on/off manual, ajuste a nota permitida, detección de tonalidad Krumhansl-Schmuckler) | Implementado | Tests unitarios |
| `PitchDetection` (YIN, interpolación parabólica, 44.1–96 kHz) | Implementado | Tests con senos, armónicos con fundamental débil, silencio y ruido |
| `NoteSegmenter` (pitch → notas editables, el vibrato no parte la nota) | Implementado | Tests con melodía, hueco, salto legato y vibrato |
| `WavIO` (16-bit, 24-bit y 32-bit float; lectura con validación) | Implementado | Tests de ida y vuelta, padding RIFF, archivo truncado |
| PitchCorrection, VibratoEngine, MagicVibrato, MelismaEngine | **Pendiente** (Fase 2) | — |
| FormantProcessor, BreathProcessor, VoiceExpression, AudioEngine | **Pendiente** (Fase 3) | — |
| Proyecto JUCE, piano roll, paneles, redimensionado | **Pendiente** (Fase 4) | — |
| PresetManager, A/B, Export Audio, **Drag to Playlist** | **Pendiente** (Fase 5) | — |
| README final, instalador Setup.exe, ZIP de entrega | **Pendiente** (Fase 6) | — |

## Compilar y probar

Requisitos: CMake ≥ 3.20 y un compilador C++17 (Visual Studio 2022 en Windows).

```
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

En GitHub, `.github/workflows/build.yml` hace esto automáticamente en Windows (MSVC) y en Linux (GCC con AddressSanitizer + UBSan) en cada push.

## Estructura

```
Source/Core/   Núcleo C++17 puro, sin JUCE, con pruebas
Tests/         Pruebas unitarias (arnés propio, sin dependencias)
```
Los módulos de JUCE (`Plugin/`, `DSP/`, `UI/`) se añadirán en las fases siguientes.

## Decisiones de diseño y limitaciones conocidas

- **Escala "Jazz"**: la especificación no la define; se implementa como *bebop dominante* (0 2 4 5 7 9 10 11).
- Cambiar tonalidad o escala borra los overrides manuales de notas.
- **YIN**: la función de diferencia es directa, O(W·τ) por frame. En la prueba (Linux, -O2) 30 s de audio se analizan en ~0,9 s, válido para análisis offline. Una versión con FFT queda como optimización.
- Resolución de pitch pensada para voz: 70–1000 Hz por defecto.
- La exportación a 24-bit no aplica dither todavía.
- **Drag to Playlist** (sección 20): se implementará con `juce::DragAndDropContainer::performExternalDragDropOfFiles` sobre un WAV ya renderizado y validado, con respaldo de «abrir ubicación del archivo». Su funcionamiento con FL Studio solo podrá confirmarse probándolo en FL Studio.

## Probado hasta ahora

Compilación con GCC (`-Wall -Wextra -Wpedantic`, sin avisos) y ejecución de las 16 pruebas / 93 comprobaciones, también con AddressSanitizer y UBSan, sin fallos.
**No probado todavía**: compilación con MSVC y el `CMakeLists.txt` (el entorno de desarrollo no tenía CMake). El workflow de GitHub los verificará en el primer push.
