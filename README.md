# Apex Master – Mastering-Plugin (VST3 / AU / Standalone)

All-in-one Mastering-Kette im Stil von iZotope Ozone, gebaut mit JUCE 8.

**Signalkette:** Input → EQ → Multiband-Dynamics → Harmonic Exciter → Stereo Imager → Maximizer → Meter

| Modul | Funktionen |
|---|---|
| **EQ** | Low Cut (12 dB/Okt), Low Shelf, 2 Bells mit Q, High Shelf. Bänder direkt im Analyser ziehbar (Mausrad = Q, Doppelklick = Reset / HP an-aus) |
| **Dynamics** | 3-Band-Kompressor, phasenkorrekte Linkwitz-Riley-Weichen (LR4), Threshold / Ratio / Attack / Release / Makeup pro Band, Soft-Knee, GR-Anzeige pro Band |
| **Exciter** | Tape / Tube / Warm, 2× Oversampling, nur die erzeugten Obertöne werden zugemischt (Drive, Frequenz, Mix) |
| **Imager** | M/S-Stereobreite 0–200 %, Mono-Bass (Seitensignal unterhalb X Hz entfernt, 24 dB/Okt) |
| **Maximizer** | Look-ahead-Brickwall-Limiter (2 ms) – Gain, Ceiling, Release; Ceiling wird sample-genau eingehalten |
| **Meter** | In/Out-Peak mit Hold, Gain Reduction, LUFS Integrated / Short-Term / Momentary (EBU R128, ITU-R BS.1770), Korrelation, Spektrum-Analyser (4096er FFT, 4,5 dB/Okt Tilt) |

**Presets:** Init, Techno Punch, EDM Loud, Warm Analog, Gentle Polish, Deep / Dub

Latenz: ca. 3 ms (wird dem Host gemeldet → automatische Kompensation).

---

## Bauen ohne eigenen Compiler (GitHub Actions – empfohlen)

1. Neues GitHub-Repository anlegen und den Inhalt dieses Ordners hochladen (inkl. `.github`).
2. Tab **Actions** öffnen → Workflow „Build Apex Master“ läuft automatisch (oder „Run workflow“).
3. Nach ~10 Minuten unter dem Lauf die Artefakte **ApexMaster-Windows** bzw. **ApexMaster-macOS** herunterladen.

## Selbst bauen

Voraussetzungen: CMake ≥ 3.22, Git, und
- **Windows:** Visual Studio 2022 („Desktopentwicklung mit C++“)
- **macOS:** Xcode Command Line Tools

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

JUCE wird beim ersten Konfigurieren automatisch geladen. Ergebnis: `build/ApexMaster_artefacts/Release/VST3/Apex Master.vst3`

## Installieren

- **Windows:** `Apex Master.vst3` nach `C:\Program Files\Common Files\VST3\` kopieren
- **macOS:** nach `~/Library/Audio/Plug-Ins/VST3/` (AU: `~/Library/Audio/Plug-Ins/Components/`).
  Da das Plugin nicht signiert ist, einmal im Terminal ausführen:
  `xattr -cr ~/Library/Audio/Plug-Ins/VST3/"Apex Master.vst3"`
- **Linux:** nach `~/.vst3/`

Danach in der DAW einen Plugin-Rescan durchführen.

## Projektstruktur

```
Source/PluginProcessor.*   Parameter, Presets, Signalkette, Meter
Source/PluginEditor.*      GUI (Analyser, Module, Meter, LookAndFeel)
Source/dsp/                MasterEQ, MultibandComp, Exciter, Imager, Maximizer, LoudnessMeter
Tests/TestMain.cpp         Offline-Test (cmake -DAPEX_BUILD_TESTS=ON)
```

## Ideen für Version 2
True-Peak-Limiting (Oversampling im Maximizer), Modul-Reihenfolge per Drag & Drop, A/B-Vergleich, Mid/Side-EQ, Referenztrack-Abgleich, Dithering.

> Hinweis: Für den kommerziellen Verkauf braucht JUCE ab einem bestimmten Umsatz eine Lizenz (oder das Projekt muss unter AGPLv3 veröffentlicht werden); VST3 erfordert zudem die Zustimmung zu Steinbergs Lizenzbedingungen.
