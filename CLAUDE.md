# FREECODER — Claude Code Project Guide

## Project Overview

**FREECODER** is a JUCE-based VST3/AU audio plugin inspired by the Game Changer Audio RECODER pedal.
It is a **Spectral Morphing Workstation** — a sample-based effect generator that captures a short audio
recording (the "spectral donor") and uses it to imprint spectral, formant, granular, and timbral
properties onto a live input signal.

Built on the [Pamplejuce](https://github.com/sudara/pamplejuce) template (JUCE 8, CMake, Catch2,
GitHub Actions CI).

---

## Core Concept

The user records any sound (a riff, melody, noise, breath, glitch…). That recording becomes a
**spectral donor** — a living audio fingerprint. The dry input signal is then continuously
cross-pollinated with that donor through a blend of:

- **Spectral morphing** (FFT-based cross-synthesis)
- **Formant shifting** (LPC or spectral envelope transfer)
- **Granular texture** (grain scrubbing/scattering over the donor buffer)
- **Phase vocoder processing** (pitch-independent time stretching / smearing)

The result is something between a vocoder, a granular reverb, an audio prism, and a formant shifter —
but uniquely shaped by whatever the user chose to record.

---

## Plugin Identity

| Field | Value |
|---|---|
| Plugin Name | FREECODER |
| Manufacturer | Ament Audio |
| Manufacturer Code | `AMNT` |
| Plugin Code | `FRCD` |
| Version | 0.1.0 |
| Formats | VST3, AU (Standalone for dev) |
| Plugin Type | Effect (not instrument) |
| Accepts MIDI | No |
| Produces MIDI | No |
| Num Input Channels | 2 (stereo) |
| Num Output Channels | 2 (stereo) |

---

## File / Directory Structure (Pamplejuce conventions)

```
FREECODER/
├── CLAUDE.md                  ← this file
├── CMakeLists.txt             ← main build config — plugin identity lives here
├── JUCE/                      ← git submodule
├── source/
│   ├── PluginProcessor.h/.cpp ← audio engine — where all DSP lives
│   ├── PluginEditor.h/.cpp    ← UI
│   └── SpectralEngine.h/.cpp  ← core spectral morphing DSP class (to be created)
├── tests/
│   └── ...                    ← Catch2 unit tests
└── packaging/
    └── ...                    ← installers, signing (Pamplejuce defaults)
```

---

## CMakeLists.txt Customizations Required

When editing `CMakeLists.txt`, make the following changes from the Pamplejuce defaults:

1. `PROJECT_NAME` → `FREECODER`
2. `PLUGIN_NAME` → `"FREECODER"`
3. `PLUGIN_MANUFACTURER_CODE` → `"AMNT"`
4. `PLUGIN_CODE` → `"FRCD"`
5. `PLUGIN_MANUFACTURER` → `"Ament Audio"`
6. `PLUGIN_DESCRIPTION` → `"Spectral Morphing Workstation"`
7. `IS_SYNTH` → `FALSE`
8. `NEEDS_MIDI_INPUT` → `FALSE`
9. `NEEDS_MIDI_OUTPUT` → `FALSE`
10. Formats: keep `VST3 AU Standalone` (drop AUv3/AAX unless needed later)
11. Add `juce_dsp` to `target_link_libraries` — required for FFT, filters, and convolution

---

## DSP Architecture (target state for v0.1)

### Buffers
- `donorBuffer`: `juce::AudioBuffer<float>` — circular, holds the last N seconds of recorded audio
- `donorRecording`: bool flag — true while capture is active
- `donorLength`: int — number of samples captured (max ~5 seconds @ 44100 = 220500 samples)

### Processing Chain (per block)
1. **Capture path**: If `donorRecording`, write input to `donorBuffer`
2. **FFT analysis**: Run forward FFT on current input block and on a windowed frame of `donorBuffer`
3. **Spectral morphing**: Multiply/blend magnitudes; optionally transfer spectral envelope
4. **Granular layer**: Scatter grain playheads across `donorBuffer`, mix into output
5. **IFFT synthesis**: Reconstruct output signal from modified spectrum
6. **Dry/wet mix**: Blend processed signal with dry input

### Parameters (APVTS)
All parameters use `juce::AudioProcessorValueTreeState`.

| ID | Name | Range | Default | Description |
|---|---|---|---|---|
| `morph` | Morph | 0.0–1.0 | 0.5 | Spectral blend dry input ↔ donor spectrum |
| `grain` | Grain | 0.0–1.0 | 0.0 | Amount of granular texture from donor |
| `formant` | Formant | 0.0–1.0 | 0.5 | Formant envelope transfer intensity |
| `scatter` | Scatter | 0.0–1.0 | 0.3 | Grain position randomization |
| `drywet` | Dry/Wet | 0.0–1.0 | 0.8 | Final dry/wet blend |
| `recTrigger` | Record | 0/1 | 0 | Momentary — starts/stops donor capture |

---

## UI Layout (PluginEditor — target v0.1)

Simple, functional layout. No fancy graphics in v0.1 — clean JUCE LookAndFeel.

```
┌─────────────────────────────────────┐
│  FREECODER            v0.1.0        │
│                                     │
│  [● REC]   Donor: [████████░░] 80%  │
│                                     │
│  MORPH  ○    GRAIN  ○    FORMANT ○  │
│   0.50       0.00       0.50        │
│                                     │
│  SCATTER ○   DRY/WET ○              │
│   0.30        0.80                  │
└─────────────────────────────────────┘
```

- REC button triggers `recTrigger` parameter
- Donor fill bar shows `donorFillLevel` (read from processor)
- All knobs are `juce::Slider` (rotary) attached via APVTS SliderAttachment

---

## Key JUCE Modules Needed

Add these to `target_link_libraries` in CMakeLists.txt:
- `juce::juce_audio_processors`
- `juce::juce_audio_utils`
- `juce::juce_dsp`          ← FFT, windowing, IIR filters
- `juce::juce_gui_basics`
- `juce::juce_gui_extra`

---

## Development Workflow

```bash
# Initial setup (after cloning from pamplejuce template)
git submodule update --init --recursive

# Configure (Mac — generates Xcode project)
cmake -B Builds -G Xcode

# Configure (cross-platform with Ninja)
cmake -B Builds -G Ninja

# Build standalone for fast iteration
cmake --build Builds --target FREECODER_Standalone

# Run tests
cmake --build Builds --target FREECODER_Tests && ./Builds/tests/FREECODER_Tests
```

---

## Development Priorities

### Phase 1 — Scaffold ✅ COMPLETE
- [x] Rename all `Pamplejuce` → `FREECODER` in CMakeLists.txt
- [x] Set manufacturer/plugin codes (see table above)
- [x] Add `juce_dsp` to link libraries
- [x] Create `SpectralEngine.h/.cpp` in `/source`
- [x] Add APVTS parameters in `PluginProcessor.cpp`
- [x] Stub out `PluginEditor` with placeholder knobs and REC button
- [x] Confirm standalone builds and opens

### Phase 2 — Core DSP ✅ COMPLETE
- [x] Implement `donorBuffer` capture in `processBlock` (capped at 5 sec, auto-stops)
- [x] Implement FFT overlap-add loop: 2048-pt FFT, 4x overlap, Hann window
- [x] Implement spectral magnitude blending controlled by `morph` param
- [x] Wire `drywet` param to final mix
- [x] Donor read head scrubs through donor buffer per hop (continuous re-analysis)
- [x] Catch2 tests for blend math and engine state (`tests/SpectralEngineTests.cpp`)

### SpectralEngine Implementation Notes
- FFT size: 2048 (`kFFTOrder = 11`), hop: 512 (4x overlap)
- Hann analysis window (unnormalized), OLA scale = `1 / (2 * kFFTSize)`
- Donor magnitude spectrum re-analysed every hop from advancing `donorReadPos`
- Phase of live input is preserved; only magnitudes are morphed
- Introduces ~2048-sample latency (~46 ms at 44100 Hz)

### Phase 3 — Extended Features
- [ ] Granular grain engine
- [ ] Formant envelope extraction & transfer
- [ ] Scatter parameter
- [ ] Polished UI

---

## Notes & Conventions

- **No global state** — all DSP state lives in `PluginProcessor` or `SpectralEngine`
- **Thread safety** — use `juce::AbstractFifo` or atomic flags for donor capture
- **Sample rate changes** — handle `prepareToPlay` properly, re-allocate FFT buffers there
- **FFT size** — start with 2048 or 4096, 50% overlap, Hann window
- **Test-driven** — add a Catch2 test for `SpectralEngine` magnitude blending early
- **No JUCE_DISPLAY_SPLASH_SCREEN** hack needed — handled by Pamplejuce defaults
