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
| Plugin Type | Effect + MIDI Instrument |
| Accepts MIDI | Yes |
| Produces MIDI | No |
| Num Input Channels | 2 (stereo) |
| Num Output Channels | 2 (stereo) |

---

## File / Directory Structure (Pamplejuce conventions)

```
FREECODER/
├── FREECODER FILES/
│   └── CLAUDE.md              ← mirror of project guide (kept outside pamplejuce/)
└── pamplejuce/                ← active project root (Pamplejuce template)
    ├── CLAUDE.md              ← this file
    ├── CMakeLists.txt         ← main build config — plugin identity lives here
    ├── JUCE/                  ← git submodule
    ├── source/
    │   ├── PluginProcessor.h/.cpp  ← audio engine, APVTS, state I/O
    │   ├── PluginEditor.h/.cpp     ← UI (pedal-style, FreecoderLookAndFeel)
    │   └── SpectralEngine.h/.cpp   ← all DSP: FFT OLA, granular, formant, phase vocoder
    ├── tests/
    │   ├── Catch2Main.cpp
    │   ├── PluginBasics.cpp
    │   └── SpectralEngineTests.cpp
    └── packaging/
        └── ...                ← installers, signing (Pamplejuce defaults)
```

---

## CMakeLists.txt Configuration (as shipped)

All Pamplejuce defaults have been overridden. Current values:

| Variable | Value |
|---|---|
| `PROJECT_NAME` | `FREECODER` |
| `PLUGIN_NAME` | `"FREECODER"` |
| `PLUGIN_MANUFACTURER_CODE` | `"AMNT"` |
| `PLUGIN_CODE` | `"FRCD"` |
| `PLUGIN_MANUFACTURER` | `"Ament Audio"` |
| `PLUGIN_DESCRIPTION` | `"Spectral Morphing Workstation"` |
| `IS_SYNTH` | `FALSE` |
| `NEEDS_MIDI_INPUT` | `TRUE` |
| `NEEDS_MIDI_OUTPUT` | `FALSE` |
| Formats | `VST3 AU Standalone` |

`juce_dsp` is included in `target_link_libraries` for FFT, windowing, filters, and the output limiter.

---

## DSP Architecture (as shipped v0.1)

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
| `morph` | Morph | 0.0–1.0 | 0.5 | Phrase loop (0) ↔ spectral freeze (1) |
| `grain` | Grain | 0.0–1.0 | 0.0 | Amount of granular texture from donor |
| `formant` | Formant | 0.0–1.0 | 0.5 | Formant envelope transfer intensity |
| `scatter` | Scatter | 0.0–1.0 | 0.3 | Grain position randomization |
| `drywet` | Dry/Wet | 0.0–1.0 | 0.8 | Final dry/wet blend |
| `pitch` | Pitch | -12–+12 | 0 | Phrase loop pitch shift in semitones |
| `recTrigger` | Record | 0/1 | 0 | Toggle — starts/stops donor capture |
| `engage` | Engage | 0/1 | 0 | Toggle — activates spectral freeze + phrase |
| `reverse` | Reverse | 0/1 | 0 | Toggle — plays phrase loop backwards |
| `midiMode` | MIDI Mode | 0/1 | 0 | Effect mode (0) vs MIDI instrument mode (1) |
| `rootNote` | Root Note | 0–127 | 60 | MIDI note that maps to 0 semitone pitch shift |

---

## UI Layout (PluginEditor — as shipped v0.1)

Pedal-inspired aesthetic: black background, neon green accents, `FreecoderLookAndFeel`.

```
┌─────────────────────────────────────────────┐
│  FREECODER                        v0.1.0    │
│                                             │
│  phrase [━━━━━━━━━━━━━━━━━━━━━━] spectral   │  MORPH  (LinearHorizontal)
│         [━━━━━━━━━━━━━━━━━━━━━━]            │  DRY/WET (LinearHorizontal)
│                                             │
│  ┌─────────────────────────────────────┐   │
│  │  ∿∿∿∿  donor waveform display  ∿∿∿∿ │   │  live donor buffer, drawn by timer
│  └─────────────────────────────────────┘   │
│                                             │
│  [GRAIN] [SCATTER] [FORMANT]  [PITCH]       │  RotaryVerticalDrag pad sliders
│                                             │
│  [● REC]          [ENGAGE]   [REVERSE]      │  footswitch TextButtons
└─────────────────────────────────────────────┘
```

- MORPH and DRY/WET: `LinearHorizontal`, `NoTextBox`, strip-slider style
- GRAIN / SCATTER / FORMANT / PITCH: `RotaryVerticalDrag`, `NoTextBox`
- REC / ENGAGE / REVERSE: `juce::TextButton`, attached via APVTS `ButtonAttachment`
- Waveform display: redraws every timer tick from `getDonorBuffer()` / `getDonorLength()`
- Tiny `[i]` inspect button (melatonin inspector, dev only, tucked in corner)

---

## Key JUCE Modules (in `target_link_libraries`)

- `juce::juce_audio_processors`
- `juce::juce_audio_utils`
- `juce::juce_dsp`          ← FFT, windowing, IIR filters, Limiter
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

### Phase 3 — Extended Features ✅ COMPLETE
- [x] Granular grain engine (stereo, scatter-randomised, Hann-windowed)
- [x] Formant envelope extraction & transfer (spectral envelope via prefix-sum box filter)
- [x] Scatter parameter (grain position randomisation)
- [x] Pedal-inspired UI: black + neon green, pad sliders, footswitches

### Phase 4 — Phase Vocoder + ENGAGE ✅ COMPLETE
- [x] True frequency estimation (phase deviation per-bin)
- [x] ENGAGE = spectral freeze with self-sustaining phase accumulators
- [x] 15 ms crossfade on engage/disengage transitions
- [x] Dry passthrough (gate) when not engaged
- [x] Play Phrase: raw donor loop blended with spectral freeze via MORPH
- [x] Freeze position: phrase loop anchors to donor analysis position at moment of engage
- [x] Latency reporting (`setLatencySamples`) and donor state persistence (getStateInformation)

### Phase 5 — Polish + Shippability ✅ COMPLETE
- [x] Parameter smoothing (20 ms SmoothedValue for all params, 15 ms engage crossfade)
- [x] Output limiter (juce::dsp::Limiter, -1 dBFS ceiling)
- [x] Stereo grain engine (per-channel donor reads)
- [x] PITCH pad: ±12 semitones, fractional-interpolated phrase loop resampling
- [x] REVERSE toggle: phrase loop plays backwards
- [x] Waveform display: live donor buffer drawn in centre display
- [x] MORPH sublabel: `phrase ↔ spectral` endpoint labels
- [x] AU validation: `auval -v aufx Frcd Amnt` — all tests pass
- [x] CI: Windows/macOS/Linux builds, pluginval level 10, graceful signing bypass
- [x] Version: 0.1.0

### SpectralEngine Implementation Notes
- FFT size: 2048 (`kFFTOrder = 11`), hop: 512 (4x overlap)
- Hann analysis window (unnormalized), OLA scale = `1 / (2 * kFFTSize)`
- Donor magnitude spectrum re-analysed every hop from advancing `donorReadPos`
- Phase of live input is preserved in spectral morph; donor phase runs free when frozen
- Introduces ~2048-sample latency (~46 ms at 44100 Hz), reported to host
- PITCH: `rate = pow(2, semitones/12)`, fractional `phraseReadPosF` with linear interp
- REVERSE: decrements `phraseReadPosF` by rate each sample, wraps around donorLength

### Phase 6 — v0.2: Preset System + Visualizer (build first)

**Strategic direction:** FREECODER's biggest differentiator is becoming a **MIDI-triggered playable instrument** — record any sound, play it chromatically with spectral freeze, granular texture, and formant transfer responding to MIDI in real-time. No other plugin combines the donor-recording workflow with MIDI pitch control + spectral freeze. The effect plugin market is saturated; the instrument market is where the novelty and profit are.

**Build order:** Preset system → Visualizer → MIDI instrument mode → Custom graphics

#### Step 1 — Preset System ✅ COMPLETE
- [x] `PresetManager` class: save/load APVTS XML + donor buffer to `~/Documents/Ament Audio/FREECODER/Presets/`
- [x] 10 factory presets (Init, Shimmer Pad, Grain Cloud, Formant Choir, Glitch Freeze, Deep Freeze, Subtle Texture, Octave Up, Octave Down, Scatter Storm)
- [x] Preset browser strip in the UI: prev/next arrows, preset name display, save button
- [x] Donor buffer included in user preset save (factory presets are param-only, preserving donor buffer)

#### Step 2 — Visualizer ✅ COMPLETE
- [x] Replace static waveform display with a dual-layer live FFT spectrum display
- [x] Bottom layer: donor magnitude spectrum (green, filled)
- [x] Top layer: live input magnitude spectrum (dim white outline) — shows morph happening in real-time
- [x] CriticalSection tryEnter — audio thread skips update if UI is reading (no RT blocking)
- [x] dB scale (-60 to 0 dBFS), 256 display bins, peak-hold within each bin
- [x] Update rate: 30 Hz via existing timer; display persists last frame when idle

#### Step 3 — MIDI Instrument Mode ✅ COMPLETE
- [x] Accept MIDI input (`NEEDS_MIDI_INPUT TRUE` in CMakeLists.txt)
- [x] `noteOn`: triggers ENGAGE + sets pitch relative to root note; last-note priority
- [x] `noteOff`: disengages only if the released note matches the held note
- [x] Mono (last-note priority) — `midiCurrentNote` audio-thread-only state
- [x] `rootNote` param (0–127, default 60 = C4) — controls the "home" pitch
- [x] `midiMode` bool param + EFFECT/MIDI toggle button in UI
- [x] Effect mode preserves all pre-existing behaviour exactly
- [x] `isBusesLayoutSupported` accepts zero inputs (pure instrument layout)
- [x] `midiNoteToName()` helper; root note displayed as e.g. "ROOT: C4" in MIDI utility row

#### Step 4 — Custom Graphics
- [ ] Logo / wordmark (vector SVG → BinaryData)
- [ ] Proper pedal faceplate texture (replace dot-grid background)
- [ ] Custom knob skin for pad sliders (replace fill-rect with rendered knob image)
- [ ] Footswitch rubber cap look (replace ellipse with bitmap or layered paint)

#### Tech debt (ongoing)
- [x] `setStateInformation` bounds check on `donorLen` — fixed in v0.1.1
- [x] `computeEnvelope` stack allocation → `mutable envelopePrefix` member — fixed in v0.1.1
- [ ] Replace `juce::TextButton` footswitches with custom painted toggle components
- [ ] SIMD-optimise magnitude blend loop in `processFFTFrame`

---

## Notes & Conventions

- **No global state** — all DSP state lives in `PluginProcessor` or `SpectralEngine`
- **Thread safety** — donor buffer reads in UI (waveform display) are best-effort, no lock needed for display
- **Sample rate changes** — handled in `prepareToPlay`, all SmoothedValues re-initialised
- **FFT size** — 2048-pt, 4x overlap (hop = 512), Hann window
- **Test-driven** — Catch2 tests in `tests/SpectralEngineTests.cpp`; em dashes replaced with `-` for Windows CTest compatibility
- **No JUCE_DISPLAY_SPLASH_SCREEN** hack needed — handled by Pamplejuce defaults
- **AU codes**: plist uses lowercase (`Frcd`/`Amnt`), so run `auval -v aufx Frcd Amnt` (not uppercase)
