# LumonTime — A Severance-Inspired Pebble Watchface

A fully-featured Pebble Time watchface styled after **Lumon Industries** from the TV series *Severance*, featuring the iconic terminal aesthetic with real-time data and an immersive grid-based display.

## Features

### Visual Design
- **Rasterized Lumon Logo** — Rendered from the official `lumon.industries` embedded font, preserving the distinctive extended letterforms and signature teardrop "O"
- **MDR Data Grid** — Full-screen grid of pseudo-random digits (Macrodata Refinement style); time and date replace grid cells at randomized positions each minute
- **Bright Cyan Palette** — High-contrast cyan-on-dark color scheme inspired by Lumon Industries intranet terminals

### Data Display

#### Real-Time (Live on Device)
- **Time** — Current hour and minute, replaces grid cells at a randomized position each minute
- **Date** — Current day/month/year, replaces grid cells at a randomized position (guaranteed different row from time)
- **Step Count** — Unavailable (Alloy SDK does not expose Pebble Health API)
- **Battery Percentage** — Real-time battery level via Alloy `embedded:sensor/Battery`

#### Status Row (Top)
- **Left:** `STEPS` — Displays `--` (not currently available in Alloy SDK)
- **Right:** Rotating **MDR file codename** (one of 20 from the show), or `PRAISE KIER` at the top of each hour

#### Badge Row (Bottom)
- **Left:** `BAT` — Battery percentage
- **Right:** `DEPT MDR` — Department badge

## Building & Installing

### Prerequisites
- [Pebble SDK](https://developer.rebble.io/) installed
- Pebble Time 2 (emery) or compatible device

### Build
```bash
pebble build
```

### Install on Emulator
```bash
pebble install --emulator emery
```

### Install on Device
Connect your Pebble device and run:
```bash
pebble install
```

### Live Screenshot
```bash
pebble screenshot --emulator emery
```

## Technical Details

### Data Sources

**Battery** — Uses Alloy's documented `embedded:sensor/Battery` API for real-time device power state.

**Steps** — Not currently available; the Alloy/Moddable SDK does not expose the Pebble Health API (`health_service_sum_today`) to watchface JavaScript code.

### Architecture

- **Logo** — Bitmap rasterized from `lumon.industries` font, encoded as compact Poco rectangles (87 rectangles, ~40 bytes)
- **Grid** — Deterministic pseudo-random digits seeded by cell position + time
- **Date/Time Overlay** — Pseudo-random deterministic placement per minute; occupies grid cells instead of overlaying them; uses separate salts to ensure date and time never occupy the same row
- **Rendering** — Optimized to minimize frame updates; grid cells are skipped where date/time text appears

## Inspiration

This watchface is inspired by the **Lumon Industries MDR terminal** from *Severance*. The show features a dystopian corporation with heavily segregated employees ("innies" and "outies") who work in the mysterious Macrodata Refinement (MDR) department, analyzing data on grid-based terminals without knowing what they're processing.

## Resources

- [Severance Wiki — Typography](https://www.severance.wiki/typography)
- [Lumon Industries Intranet](https://lumon.industries/intranet/terminal/)
- [Pebble SDK Documentation](https://developer.rebble.io/)

## License

This watchface is a fan project inspired by *Severance*. Use and modify freely for personal use.

---

**Status:** Production-ready for Pebble Time 2 (emery). Currently in active use. ⏰
