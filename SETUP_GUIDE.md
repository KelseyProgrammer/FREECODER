# FREECODER — Claude Code Setup Walkthrough

This doc walks you through bootstrapping the FREECODER plugin using the
Pamplejuce template and Claude Code.

---

## Prerequisites

Make sure you have these installed before starting:

- **Git** (with submodule support)
- **CMake** 3.22+  (or use CLion/VS2022 which bundle it)
- **Xcode** (macOS) or **Visual Studio 2022** (Windows)
- **Claude Code** (`npm install -g @anthropic-ai/claude-code`)

---

## Step 1 — Create Your Repo from the Template

1. Go to https://github.com/sudara/pamplejuce
2. Click **"Use this template" → "Create a new repository"**
3. Name it `FREECODER`, set visibility (public or private)
4. Clone it locally:
   ```bash
   git clone https://github.com/YOUR_USERNAME/FREECODER.git
   cd FREECODER
   ```

---

## Step 2 — Initialize Submodules

```bash
git submodule update --init --recursive
```

This pulls in JUCE (the `develop` branch), CMake helpers, and the Melatonin
Inspector component. Takes a minute or two.

---

## Step 3 — Drop In the CLAUDE.md

Copy the `CLAUDE.md` file from this package into the root of your `FREECODER`
repo. This gives Claude Code all the context it needs about what you're building.

```bash
cp /path/to/CLAUDE.md ./CLAUDE.md
```

---

## Step 4 — Launch Claude Code

From inside the repo root:

```bash
claude
```

Claude Code will automatically read `CLAUDE.md` on startup and understand the
full project context.

---

## Step 5 — Run the Scaffold Prompt

Once Claude Code is running, paste this prompt to kick off the initial scaffold:

```
Read CLAUDE.md to understand the project.

Then do the following in order:

1. Edit CMakeLists.txt to rename the project from "Pamplejuce" to "FREECODER",
   set PLUGIN_CODE to "FRCD", PLUGIN_MANUFACTURER_CODE to "AMNT",
   PLUGIN_MANUFACTURER to "Ament Audio", PLUGIN_DESCRIPTION to
   "Spectral Morphing Workstation", and add juce_dsp to the
   target_link_libraries block.

2. Rename the source files PluginProcessor.h, PluginProcessor.cpp,
   PluginEditor.h, PluginEditor.cpp if they still reference "Pamplejuce" in
   class names or comments — update them to use "Freecoder" naming.

3. Create source/SpectralEngine.h and source/SpectralEngine.cpp with a stub
   class SpectralEngine that has:
   - A prepare(double sampleRate, int blockSize) method
   - A process(juce::AudioBuffer<float>& buffer) method  
   - A startRecording() / stopRecording() method pair
   - A private juce::AudioBuffer<float> donorBuffer member

4. In PluginProcessor.h/.cpp, add an AudioProcessorValueTreeState (APVTS) with
   the parameters defined in CLAUDE.md: morph, grain, formant, scatter, drywet,
   recTrigger.

5. In PluginEditor.h/.cpp, add placeholder rotary sliders for each parameter
   and a toggle button for REC, all attached via APVTS SliderAttachment /
   ButtonAttachment. Display the plugin name and version.

6. Add SpectralEngine.cpp to the sources list in CMakeLists.txt.

After all edits, show me the final CMakeLists.txt so I can verify it before
building.
```

---

## Step 6 — Configure and Build

After Claude Code finishes the scaffold, configure and do a test build:

```bash
# macOS — Xcode
cmake -B Builds -G Xcode
cmake --build Builds --target FREECODER_Standalone --config Debug

# Or cross-platform
cmake -B Builds
cmake --build Builds --target FREECODER_Standalone
```

The Standalone target is fastest for iteration — you don't need a DAW to test.

---

## Step 7 — Verify It Opens

Launch the standalone:
```bash
open Builds/FREECODER_artefacts/Debug/Standalone/FREECODER.app   # macOS
```

You should see the plugin window with placeholder knobs and the REC button.
If it builds and opens, Phase 1 is complete.

---

## Ongoing Claude Code Workflow

Use targeted prompts for each Phase 2/3 task. Examples:

**Implement donor buffer capture:**
```
In PluginProcessor::processBlock, when the recTrigger parameter is active,
write incoming audio samples into SpectralEngine's donorBuffer using a
circular write pointer. Cap the buffer at 5 seconds. Expose a getDonorFillLevel()
method returning a float 0.0–1.0.
```

**Implement FFT spectral morphing:**
```
In SpectralEngine::process, implement a basic spectral magnitude blend using
juce::dsp::FFT with a 2048-point FFT and 50% overlap Hann window. Blend the
magnitude spectrum of the live input with the magnitude spectrum of the donor
buffer frame, controlled by the morph parameter (0=dry, 1=full donor spectrum).
Reconstruct via IFFT and overlap-add.
```

---

## Tips

- Always ask Claude Code to **show you diffs before applying** on big changes
- Run the **Standalone** build target during development — faster than VST3
- Add a **Catch2 test** for SpectralEngine early (magnitude blend math is easy to unit test)
- Keep `CLAUDE.md` updated as the architecture evolves — it's Claude Code's source of truth
