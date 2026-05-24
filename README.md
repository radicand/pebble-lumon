# LumonTime — A Severance-Inspired Pebble Watchface

A fully-featured Pebble Time watchface styled after **Lumon Industries** from the TV series *Severance*, featuring the iconic terminal aesthetic with real-time data and an immersive grid-based display.

## Features

### Visual Design
- **Rasterized Lumon Logo** — Rendered from the official `lumon.industries` embedded font, preserving the distinctive extended letterforms and signature teardrop "O"
- **CRT Scanlines** — Authentic retro terminal effect with vertical scan lines
- **MDR Data Grid** — Full-screen grid of pseudo-random digits (Macrodata Refinement style) with time and date embedded as grid cells
- **Authentic Palette** — Cyan-on-dark color scheme matching the Lumon Industries intranet terminal (`#BDFFFF` on `#001D2F`)

### Data Display

#### Real-Time (Live on Device)
- **Time** — Current hour and minute, displayed in random grid cells
- **Date** — Current day/month/year, displayed in random grid cells (different position than time each minute)
- **Step Count** — Reads from device health API; falls back to simulated progression
- **Battery Percentage** — Reads from device power state; falls back to simulated discharge

#### Status Row (Top)
- **Left:** `STEPS` — Daily step count formatted as `X`, `X.Xk`, or `Xk`
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

### Real Data Fallback Strategy

**Steps:**
1. Try `globalThis.health.steps`
2. Try `require("health").steps`
3. Try `require("device").health.steps`
4. Fall back to time-based simulation (~8.5k steps/day with variation)

**Battery:**
1. Try `globalThis.power.battery`
2. Try `require("device").power.battery`
3. Fall back to simulated discharge (~1% per 10 minutes)

On a real Pebble device with proper SDK support, the watchface will display actual health and power data. On emulators or unsupported environments, it gracefully falls back to realistic simulations.

### Architecture

- **Logo** — Bitmap rasterized from `lumon.industries` font, encoded as compact Poco rectangles (87 rectangles, ~40 bytes)
- **Grid** — Deterministic pseudo-random digits seeded by cell position + time; time and date cells are reserved and drawn separately with larger fonts
- **Time/Date Placement** — Pseudo-random but deterministic per minute; uses separate salts to avoid overlap
- **Rendering** — Optimized for Pebble watchdog (~6-scanline intervals, sparse grid cells to keep frame budget low)

## Inspiration

This watchface is inspired by the **Lumon Industries intranet terminal** from HBO's *Severance*. The show features a dystopian corporation with heavily segregated employees ("innies" and "outies") who work in the mysterious Macrodata Refinement (MDR) department, analyzing data on grid-based terminals without knowing what they're processing.

## Resources

- [Severance Wiki — Typography](https://www.severance.wiki/typography)
- [Lumon Industries Intranet](https://lumon.industries/intranet/terminal/)
- [Pebble SDK Documentation](https://developer.rebble.io/)

## License

This watchface is a fan project inspired by *Severance*. Use and modify freely for personal use.

---

**Status:** Production-ready for Pebble Time 2 (emery). Currently in active use. ⏰
