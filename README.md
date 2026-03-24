# FREECODER
**Spectral Morphing Workstation** — VST3 · AU · CLAP

by [Ament Audio](https://chrisament.gumroad.com/l/kvbani)

---

## What It Does

Record any sound — a chord, a breath, a riff. That recording becomes a **spectral donor**: a living audio fingerprint. Your live input is then continuously cross-pollinated with it through spectral morphing, formant transfer, granular texture, and phase vocoder processing.

The result is something between a vocoder, a granular reverb, and an audio prism — uniquely shaped by whatever you chose to record. Play it as an effect or a **polyphonic MIDI instrument**.

---

## Features

- **Spectral Freeze** — FFT-based cross-synthesis blends your live signal with the donor's spectral fingerprint in real time
- **Granular Texture** — Scatter-randomised grain engine scrubs the donor buffer independently of freeze
- **Formant Transfer** — Spectral envelope extraction maps the donor's vowel character onto your input
- **Phrase Loop** — Loop and pitch-shift the recorded phrase with fractional-interpolated resampling
- **MIDI Instrument Mode** — 8-voice polyphony, ADSR envelope, root note mapping
- **3 Donor Slots** — Hot-swap between three independent recordings live
- **WAV Import / Export** — Bring in any audio file as a donor, or export your capture
- **Live Spectrum + Tuner** — Dual-layer FFT visualizer with built-in pitch tuner

---

## Download

**[Buy — $29](https://chrisament.gumroad.com/l/kvbani)** · 7-day honor system trial available via [GitHub Releases](https://github.com/KelseyProgrammer/FREECODER/releases/latest)

**[Product Page](https://kelseyProgrammer.github.io/FREECODER/freecoder.html)** · **[User Manual](https://kelseyProgrammer.github.io/FREECODER/manual.html)**

---

## Install

| Format | Path |
|--------|------|
| VST3 | `~/Library/Audio/Plug-Ins/VST3/` |
| AU | `~/Library/Audio/Plug-Ins/Components/` |
| CLAP | `~/Library/Audio/Plug-Ins/CLAP/` |

> **Unsigned build:** right-click the plugin → Open → Allow, or run:
> ```bash
> xattr -cr ~/Library/Audio/Plug-Ins/VST3/FREECODER.vst3
> ```

---

## Specs

| | |
|---|---|
| Formats | VST3 · AU · CLAP (macOS) · VST3 (Windows/Linux) |
| Plugin type | Effect + MIDI Instrument |
| I/O | Stereo in → Stereo out |
| Latency | ~46 ms (2048-sample FFT @ 44.1 kHz), reported to host |
| FFT | 2048-pt, 4× overlap-add, Hann window |
| Polyphony | 8 voices (MIDI mode) |
| Donor length | Up to 5 seconds · 3 slots |
| Requires | macOS 10.13+ · 64-bit DAW |
| Version | 0.2.12 |

---

## Building from Source

```bash
git submodule update --init --recursive
cmake -B BuildsNinja -G Ninja
cmake --build BuildsNinja --target FREECODER_Standalone
```

Run tests:
```bash
cmake --build BuildsNinja --target FREECODER_Tests && ./BuildsNinja/Tests/FREECODER_Tests
```

---

## License

© 2026 Ament Audio. All rights reserved.
