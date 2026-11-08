# Easy Chorus

Touch-friendly guitar **chorus + delay + shimmer** for Linux — **LV2** and **VST3**, built with **JUCE 8**.

Three large **on/off** toggles (**Chorus**, **Delay**, **Shimmer**) plus six knobs in a 2×3 grid:
**Rate**, **Depth**, **Delay**, **Feedback**, **Shimmer**, **Mix**.

Use any combo — including **delay-only** or **shimmer-only** — via the section toggles (true per-stage bypass).

**Demo** (top-right): injects a clean plucked arpeggio so Standalone works with muted/silent input — no guitar required. Turn Demo off to use a real audio interface.

## Requirements

- Linux (tested on Ubuntu 24.04)
- CMake ≥ 3.22
- C++17 compiler (`g++` or `clang++`)
- Ninja (recommended)
- Dev packages typically needed:

```bash
sudo apt install build-essential cmake ninja-build pkg-config \
  libasound2-dev libfreetype-dev libx11-dev libxcomposite-dev \
  libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev \
  libxrender-dev libgl1-mesa-dev libcurl4-openssl-dev \
  libwebkit2gtk-4.1-dev lv2-dev
```

JUCE is fetched automatically via CMake `FetchContent` (no manual install).

## Build

```bash
cd ~/easy-chorus
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Artifacts land under `build/EasyChorus_artefacts/Release/` and are also copied into `Dist/`:

| Format | Dist path |
| --- | --- |
| VST3 | `Dist/VST3/Easy Chorus.vst3` |
| LV2  | `Dist/LV2/Easy Chorus.lv2` |

A **Standalone** binary is built as well (handy for quick listening).

## Install (user-local)

```bash
# VST3
mkdir -p ~/.vst3
cp -a "Dist/VST3/Easy Chorus.vst3" ~/.vst3/

# LV2
mkdir -p ~/.lv2
cp -a "Dist/LV2/Easy Chorus.lv2" ~/.lv2/
```

Or symlink from the build tree if you prefer not to copy.

Rescan plugins in your host (Carla, Ardour, Reaper, etc.).

## Parameters

### Demo + section toggles

| Toggle | Default | Role |
| --- | --- | --- |
| Demo | **Off** | Inject synthetic plucked arpeggio as input |
| Chorus | **On** | Hard-bypass chorus stage |
| Delay | **On** | Hard-bypass delay stage |
| Shimmer | **Off** | Hard-bypass shimmer stage |

Enable **Demo** in Standalone when you have no guitar/mic (Options → output device, then tap Demo).

When a section is off, that DSP block is **skipped** (not merely amount=0). Solo Delay or solo Shimmer works with the others off.

### Tempo / delay timing

| Control | Role |
| --- | --- |
| **Sync** | Use the DAW/host BPM when available (falls back to manual BPM in Standalone) |
| **BPM** | Manual tempo (40–240) when Sync is off; shows host BPM when Sync is on |
| **Musical** | Delay uses note divisions instead of milliseconds |
| **Note / Delay** | Free mode: ms. Musical mode: `1/1` … `1/16` (incl. dotted / triplets) |

### Presets

Use the **in-plugin** preset bar (top row, left of Demo): **`<` / `>`** or the dropdown — no Options menu needed.

Factory set includes BFMV Clean, Chorus Glow, Delay Trail, Ping Pong, Reverse Dream, Shimmer Cloud, Ethereal Wash, Starfield, High Halo, Musical 1/8, Wide Stereo.

**High sparkles / spacey tips:** open the **Shimmer** (and **Shimmer 2**) pages — set **Pitch** to **+19** or **+24**, turn **Ethereal** on, keep **Amount** moderate, and leave **Mix** around 35–45% so the dry line stays clear. Try presets **Starfield** or **Ethereal Wash**.

### UI layout

One effect page at a time via the top tabs (**Chorus / Delay / Shimmer / Delay 2 / Shimmer 2**), with large knobs and toggles.
**Output** (Waves, Mix, Master) stays visible under every page.
Signal path is serial: Chorus → Delay → Shimmer → Delay 2 → Shimmer 2 → Mix/Master.

### Delay / shimmer modes

| Toggle | Default | Role |
| --- | --- | --- |
| **Stereo** | Off | Wider dual delay (≈12 ms right offset). Off = linked mono image |
| **Ping** | Off | Ping-pong cross-feedback (L↔R). Combines with Stereo offset |
| **Reverse** | Off | Tape-style reverse echoes (overlapping reverse grains) |
| **Ethereal** | Off | Airier / wider shimmer bloom (mild RT stretch; Decay still applies) |
| **Waves** | Off | Tempo-synced swell: both delay + shimmer layers breathe in/out over 2 bars |

Delay 2 / Shimmer 2 mirror the first layers (own time/pitch/decay/modes/levels) so you can stack reverse+forward, dual pitches, etc.

Dials have **clickable step marks** around the rim — tap a mark to jump to that value (still drag for fine control).

### Knobs

| Control | Range | Default | Role |
| --- | --- | --- | --- |
| Rate | 0.1–4 Hz | 0.70 Hz | Chorus LFO (CE-style) |
| Depth | 0–100% | 35% | Chorus thickening |
| Time / Note | 50–1000 ms | 380 ms | Delay time (ms) or musical division |
| Feedback | 0–85% | 32% | Delay repeats |
| Amount | 0–100% | 25% | Pitch-shifted reverb wash amount |
| Pitch | −12…+24 st | +12 st | Shimmer feedback pitch (−1 oct … +2 oct) |
| Decay | 0.4–12 s | 3.5 s | Shimmer wash length (approx. RT60) |
| Level (per stage) | 0–100% | 100% | Independent stage wet amount |
| Mix | 0–100% | 45% | Dry / full wet-chain blend |
| Master | 0–150% | 100% | Final output level |

Chorus voices are fixed at **2** in v1 (no Voices knob).

## Starting recipe — BFMV clean

Inspired by the polished metalcore cleans on **Bullet for My Valentine — “Tears Don’t Fall”** and **“Hearts Burst Into Fire”** (Cyber Twin–era chorus + digital delay character — not a master clone).

These are also the **plugin defaults**:

| Control | Setting |
| --- | --- |
| Chorus | **On** |
| Delay | **On** |
| Shimmer | **Off** |
| Rate | **0.70 Hz** |
| Depth | **35%** |
| Delay | **380 ms** (~quarter on TDF tempo) |
| Feedback | **32%** |
| Shimmer (amount) | **5%** (ready when you enable the section) |
| Mix | **45%** |

Tips:
- Intro arpeggios: keep Delay ~370–400 ms, Feedback ~30–40%, Mix ~40–50%
- Drier crunch sections: drop Mix / Feedback, or toggle Delay off
- **Delay only**: Chorus off, Delay on, Shimmer off
- **Shimmer only**: Chorus off, Delay off, Shimmer on — raise Shimmer amount (~40–70%) and Mix
- Classic BFMV clean: leave Shimmer toggle **off**

## Signal flow

```
Input → Chorus (2-voice CE) → Delay (digital) → Shimmer (+12 st reverb) → Mix with dry → Output
```

## DSP (v1)

- **Chorus** — CE / amp-modeler thickening: ~7–18 ms Hermite taps, ±4.5 ms max mod, phase-offset LFOs
- **Delay** — clean digital 50 ms–1 s (includes 370–400 ms), feedback HPF, capped at 85%
- **Shimmer** — Valhalla-inspired pitch-shifted FDN reverb (+12 st feedback), soft bloom; size/color fixed in v1
- Global **Mix** with smoothed dry/wet; all continuous params smoothed

## Project layout

```
easy-chorus/
  CMakeLists.txt
  README.md
  Source/
    PluginProcessor.{h,cpp}
    PluginEditor.{h,cpp}
    dsp/ChorusEngine.{h,cpp}
    dsp/DelayEngine.{h,cpp}
    dsp/ShimmerEngine.{h,cpp}
  Dist/          # built plugin bundles
  Resources/
```
