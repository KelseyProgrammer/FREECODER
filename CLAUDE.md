# FREECODER — Claude Code Project Guide

## Project Overview

**FREECODER** is a JUCE-based VST3/AU/CLAP audio plugin — a **Spectral Morphing Workstation**.
It captures a short audio recording (the "spectral donor") and uses it to imprint spectral, formant,
granular, and timbral properties onto a live input signal. It also operates as a polyphonic MIDI
instrument: record any sound, then play it chromatically.

Built on the [Pamplejuce](https://github.com/sudara/pamplejuce) template (JUCE 8, CMake, Catch2, GitHub Actions CI).

---

## Plugin Identity

| Field | Value |
|---|---|
| Version | 0.2.8 |
| Plugin Name | FREECODER |
| Manufacturer | Ament Audio |
| Manufacturer Code | `Amnt` |
| Plugin Code | `Frcd` |
| Bundle ID | `com.amentaudio.freecoder` |
| Formats | VST3, AU, CLAP, Standalone |
| IS_SYNTH | FALSE |
| NEEDS_MIDI_INPUT | TRUE |
| Channels | Stereo in + stereo out |

AU validation: `auval -v aumf Frcd Amnt` (type is `aumf` — music effect — because NEEDS_MIDI_INPUT is TRUE)

---

## File Structure

```
pamplejuce/
├── CMakeLists.txt          — plugin identity, modules, build config
├── VERSION                 — semver string read by CMake
├── source/
│   ├── PluginProcessor.h/.cpp   — APVTS params, processBlock, state I/O
│   ├── PluginEditor.h/.cpp      — UI: FreecoderLookAndFeel, all controls
│   ├── SpectralEngine.h/.cpp    — all DSP: FFT OLA, granular, phase vocoder, MIDI voices
│   ├── FootswitchButton.h       — self-painting juce::Button subclass (REC + FREEZE)
│   └── PresetManager.h/.cpp     — factory + user preset save/load
├── assets/
│   └── images/
│       ├── freecoder_logo.svg   — wordmark SVG (compiled into BinaryData)
│       └── pamplejuce.png       — template asset (unused)
├── docs/
│   ├── index.html               — GitHub Pages landing page
│   └── manual.html              — full user manual
├── tests/
│   ├── Catch2Main.cpp
│   ├── PluginBasics.cpp
│   └── SpectralEngineTests.cpp
└── modules/
    ├── clap-juce-extensions/    — CLAP format support
    └── melatonin_inspector/     — dev UI inspector (debug only)
```

---

## Parameters (APVTS)

| ID | Type | Range | Default | Description |
|---|---|---|---|---|
| `morph` | float | 0–1 | 0.5 | Phrase loop (0) ↔ spectral freeze (1) |
| `grain` | float | 0–1 | 0.0 | Granular texture amount |
| `formant` | float | 0–1 | 0.5 | Formant envelope transfer intensity; in PHRASE mode, cross-synthesises the phrase with the live input's spectral envelope (phrase takes on live input's timbral character) via a second OLA path |
| `scatter` | float | 0–1 | 0.3 | Grain position randomisation + phrase playhead jitter (in PHRASE mode) |
| `drywet` | float | 0–1 | 0.8 | Final dry/wet blend |
| `pitch` | float | -12–+12 | 0.0 | Phrase pitch shift (semitones) |
| `recTrigger` | bool | — | false | Edge-triggered: starts/stops donor capture |
| `engage` | bool | — | false | Spectral freeze engage |
| `phraseEngage` | bool | — | false | Phrase loop engage (independent of freeze) |
| `reverse` | bool | — | false | Play phrase loop backwards |
| `recLength` | float | 1–5 | 5.0 | Max record length, snapped to {1,2,3,5} sec |
| `autoEngage` | bool | — | false | Auto-engages freeze when recording stops |
| `midiMode` | bool | — | false | Effect mode (false) vs MIDI instrument mode (true) |
| `rootNote` | int | 0–127 | 60 | MIDI root note (C4 = 60) for pitch reference |
| `latch` | bool | — | false | MIDI: hold notes without sustain pedal |
| `effectAdsr` | bool | — | false | Enable ADSR shaping of engage output in effect mode |
| `adsrAttack` | float | 0.001–5.0 | 0.01 | ADSR attack (s) — shared by effect + MIDI modes |
| `adsrDecay` | float | 0.001–5.0 | 0.20 | ADSR decay (s) |
| `adsrSustain` | float | 0–1 | 1.0 | ADSR sustain level |
| `adsrRelease` | float | 0.001–10.0 | 0.50 | ADSR release (s) |

---

## DSP Architecture (SpectralEngine)

**Constants:**
- FFT: 2048-pt (`kFFTOrder=11`), hop 512 (4× overlap), Hann window
- Latency: 2048 samples (~46 ms @ 44.1 kHz), reported via `setLatencySamples`
- Grains: 16 max, 2048 samples each (`kMaxGrains`, `kGrainSamples`)
- MIDI voices: 8 polyphonic (`kMaxVoices`)
- Donor buffer: ~5 sec max (`kMaxDonorSamples = 220500`)
- Donor slots: 3 (A/B/C) (`kNumDonorSlots`)
- Visualiser bins: 256 (`kVisBins`)
- Waveform overview points: 512 (`kWavePoints`)

**Processing chain per block:**
1. Capture path: if recording, write input → donorBuffer (edge-triggered, auto-stops at recLength)
2. FFT overlap-add: forward FFT on input + donor frame
3. Spectral morph: blend magnitudes (morph param), optionally transfer formant envelope
4. Phase vocoder: true frequency estimation; ENGAGE locks phase accumulators (spectral freeze)
5. Granular layer: 16 Hann-windowed grains scatter across donorBuffer, mixed into output
6. MIDI voices (midiMode only): each active voice has its own phrasePos + ADSR, mixed before dry/wet
7. IFFT synthesis, OLA accumulate → output
8. Dry/wet blend
9. Output limiter (juce::dsp::Limiter, -1 dBFS ceiling)

**Phrase formant path (PHRASE mode, formant > 0) — cross-synthesis:**
- Per-channel `phraseFormantRing` (kFFTSize circular buffer) captures raw phrase output samples each block
- Every hop, `processPhraseFftFrame()` runs FFT on the ring, applies `liveEnvelope / phraseEnvelope` shaping (cross-synthesis: phrase takes on live input's timbral character), OLA-accumulates into `phraseFormantQueue`
- `liveEnvelope` is populated by the just-completed `processFFTFrame` passes; `blendedMag` is used as scratch for the phrase envelope
- Mixer blends: constant-power crossfade `rawPhrase * sqrt(1-formant) + shapedPhrase * sqrt(formant)`; raw phrase used as fallback during 2048-sample warmup
- State cleared in `setPhraseEngage(false)` and the deferred OLA flush

**Phrase scatter (PHRASE mode, scatter > 0):**
- At each hop boundary while `phraseEngaged`, jitters `phraseReadPosF` by `±scatter × donorLength × 0.25`
- Scatter jump and loop-wrap both trigger a 512-sample crossfade via `scatterOldPosF` / `scatterFadeCount` to eliminate clicks

**Donor slots (A/B/C):**
- `setActiveSlot(n)` switches the working donorBuffer to slot n
- `donorSlots[n]` holds pre-allocated snapshot buffers — slot switch never allocates
- State is persisted via `getStateInformation` (all 3 slots saved as raw float PCM)

**Thread safety:**
- Visualiser: `CriticalSection visLock` — audio thread calls `tryEnter`, skips if UI is reading
- Tuner: `CriticalSection tunerLock` — same pattern
- Waveform: `CriticalSection waveformLock` — same pattern; playhead baked in at read time from `waveformPlayheadAtomic`
- Donor slot requests: `std::atomic<int> requestedSlot`, applied at block boundary
- Auto-engage: `std::atomic<bool> autoEngagePending`, consumed once by processBlock

---

## UI Components (PluginEditor)

**Sliders:**
- `morphSlider`, `drywetSlider` — LinearHorizontal strip sliders (top of panel)
- `grainSlider`, `scatterSlider`, `formantSlider`, `pitchSlider` — RotaryVerticalDrag pads
- `recLengthSlider` — LinearHorizontal, near REC button
- `rootNoteSlider` — LinearHorizontal, MIDI utility row

**Buttons:**
- `recButton` (REC), `engageButton` (FREEZE) — **FootswitchButton** instances (self-painting, no LookAndFeel override)
- `reverseButton` (REVERSE), `phraseButton` (PHRASE) — TextButton
- `modeButton` (EFFECT ↔ MIDI toggle)
- `autoEngageButton` (AUTO), `effectAdsrButton` (ADSR), `latchButton` (LATCH)
- `slotButtonA/B/C` (A / B / C donor slots)
- `exportButton` (EXP), `importButton` (IMP) — WAV export/import of active donor slot
- `prevPresetButton` (<), `nextPresetButton` (>), `savePresetButton` (SAVE)
- `inspectButton` (i) — melatonin dev inspector

**ADSR sliders** (adsrAttackSlider / adsrDecaySlider / adsrSustainSlider / adsrReleaseSlider):
- Visible and active in both MIDI mode and effect mode (when effectAdsr is on)

**Display area:** Split into two regions:
- Upper ~60%: dual-layer FFT spectrum (donor = green filled, live input = dim white outline), 15 Hz refresh, dB scale −60 to 0 dBFS, grid lines at −12/−24/−36/−48 dBFS
- Lower ~40%: waveform overview (peak-normalised donor buffer, white playhead line when PHRASE engaged)

**Logo:** SVG loaded from `BinaryData::freecoder_logo_svg` in constructor via `juce::Drawable::createFromSVG()`; drawn proportionally in `paint()` with text fallback.

**Window:** Resizable — `setResizeLimits(420, 480, 900, 1000)`. All layout in `resized()` is fully
proportional to `W` and `H`. All paint() positions reference component bounds or compute from
`headerH`/`contentY` local vars (proportional to H).

**Tuner:** FFT-based fundamental detection updated every hop on ch 0; result exposed via
`getTunerResult()`.

---

## Preset System (PresetManager)

- Factory presets: 12 presets, param-only (donor buffer preserved as-is)
  - Covers: Init, Deep Freeze, Shimmer Freeze, Glitch Freeze, Phrase Loop, Phrase+Formant,
    Phrase Scatter, Grain Cloud, Octave Shimmer, Pitch Down, MIDI Pad, MIDI Pluck
  - `FactoryPreset` struct includes: morph, grain, formant, scatter, drywet, pitch,
    phraseEngage, midiMode, reverse, adsrAttack, adsrDecay, adsrSustain, adsrRelease
- User presets: full binary (APVTS XML + all 3 donor slot buffers), saved as `*.freecoder`
- Location: `~/Documents/Ament Audio/FREECODER/Presets/`
- Navigation: `nextPreset()`, `previousPreset()`, `loadPresetAtIndex()`
- Save: `promptSavePreset()` — async native dialog, safe from message thread

---

## Development Workflow

**Use Ninja + Standalone for all day-to-day iteration.**

```bash
# One-time setup
git submodule update --init --recursive
cmake -B BuildsNinja -G Ninja        # configure once

# Daily loop (~10–60 sec incremental rebuild)
cmake --build BuildsNinja --target FREECODER_Standalone

# Run unit tests
cmake --build BuildsNinja --target FREECODER_Tests && ./BuildsNinja/Tests/FREECODER_Tests

# Full plugin build (AU + VST3 + CLAP + Standalone) — only before merging to main
cmake --build BuildsNinja
```

**CI runs only on:** pushes to `main`, version tags (`v*`), and PRs into `main`.
Feature branch pushes trigger no CI — develop freely.

**Release flow:**
1. Merge feature branch → `main` (CI validates all platforms)
2. `git tag v0.2.2 && git push --tags` → CI builds + creates GitHub Release automatically

**Do not use `Builds/`** (Xcode generator, 10× larger). Use `BuildsNinja/` exclusively.

---

## What Remains

### High priority (v0.3)
- **SIMD** — ~~done~~: Pass 1 (blend) uses `FloatVectorOperations` (guaranteed SIMD); Pass 2 (formant) split into 3 sub-passes so energy reductions and gain application each auto-vectorise independently
- **Parameter smoothing on preset load** — ~~done~~: `requestSnapSmoothers()` is called after `replaceState()` in all three paths (factory preset, user preset, DAW state restore); `snapSmoothers()` snaps all SmoothedValues on the next audio block
- **Test coverage** — ~~done~~: `SpectralEngineTests.cpp` covers blend math, state management, waveform snapshot, scatter crossfade, MIDI voice steal (16 test cases, 163 872 assertions)
- **AU validation** — ~~done~~: `auval -v aumf Frcd Amnt` passes clean (confirmed 0.2.5)

### Medium priority
- **Code signing / notarization** — plugin is currently unsigned; users must manually allow via `xattr -cr` or right-click → Open. Requires Apple Developer account + CI secrets for automated signing
- **Windows testing** — CI builds Windows VST3 but it has not been manually tested in a DAW
- **Mono input handling** — ~~done~~: `isBusesLayoutSupported` accepts mono in + stereo out; `processBlock` upmixes ch 0 → ch 1 before engine processing
- **Resizable window persistence** — ~~done~~: `savedEditorWidth/Height` saved in `getStateInformation`, restored in `setStateInformation`; editor reads them at construction via `setSize()`

### Lower priority / polish
- **FootswitchButton for PHRASE + REVERSE** — currently still TextButton; would complete the visual consistency of all four footswitches
- **Slot button indicator** — slot buttons A/B/C don't visually indicate which slot has a recording; adding a dot or dim glow when `donorSlotHasData(n)` would improve usability
- **Label layout at small sizes** — footswitch label text can overlap at minimum window width (420px); clamp or hide sub-labels below a threshold
- **Preset name display** — current name shown in header; no indication of unsaved changes (dirty state)

---

## Notes & Conventions

- No global state — all DSP state lives in `PluginProcessor` or `SpectralEngine`
- `isBusesLayoutSupported` requires matching stereo in + stereo out (donor recording always needs audio input)
- Sample rate changes handled in `prepareToPlay` — all SmoothedValues re-initialised
- `COPY_PLUGIN_AFTER_BUILD TRUE` in CMakeLists — plugin auto-installs to `~/Library/Audio/Plug-Ins` on Mac after every build
- Em dashes replaced with `-` in test strings for Windows CTest compatibility
- clangd reports false-positive `juce` undeclared errors — ignore; actual cmake/ninja builds are always clean
