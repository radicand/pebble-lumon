# LumonTime — A Severance-Inspired Pebble Watchface

A fully-featured Pebble Time watchface styled after **Lumon Industries** from the TV series *Severance*, featuring the iconic terminal aesthetic with real-time data and an immersive grid-based display.

**[Download on the Pebble App Store](https://apps.repebble.com/87e9be27ecf34f248efa8235)**

## Features

### Visual Design
- **Rasterized Lumon Logo** — Rendered from the official `lumon.industries` embedded font, preserving the distinctive extended letterforms and signature teardrop "O"
- **MDR Data Grid** — Full-screen grid of pseudo-random digits (Macrodata Refinement style); time and date replace grid cells at randomized positions each minute
- **Bright Cyan Palette** — High-contrast cyan-on-dark color scheme inspired by Lumon Industries intranet terminals

### Data Display

#### Real-Time (Live on Device)
- **Time** — Current hour and minute, replaces grid cells at a randomized position each minute
- **Date** — Current day/month/year, replaces grid cells at a randomized position (guaranteed different row from time)
- **Step Count** — Real-time daily step total via Pebble C `HealthService`
- **Battery Percentage** — Real-time battery level via Pebble C `BatteryStateService`

#### Status Row (Top)
- **Left:** `STEPS` — Displays the current daily step count, or `--` if health data is unavailable or not permitted
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

**Battery** — Uses Pebble C `battery_state_service_peek()` and `battery_state_service_subscribe()` for real-time device power state. The displayed value is `BatteryChargeState.charge_percent`.

**Steps** — Uses Pebble C `HealthService`. The watchface checks `health_service_metric_accessible(HealthMetricStepCount, time_start_of_today(), time(NULL))` before reading `health_service_sum_today(HealthMetricStepCount)`. Health events are subscribed so the display can refresh when movement data changes.

### Architecture

- **Native Pebble C SDK** — The watchface is now a `native` Pebble project; the previous Alloy/Moddable runtime dependency has been removed.
- **Logo** — Bitmap rasterized from `lumon.industries` font, encoded as compact Pebble C rectangles
- **Grid** — Deterministic pseudo-random digits seeded by cell position + time
- **Date/Time Overlay** — Pseudo-random deterministic placement per minute; occupies grid cells instead of overlaying them; uses separate salts to ensure date and time never occupy the same row
- **Rendering** — A custom layer redraws on minute ticks, health events, and battery changes; grid cells are skipped where date/time text appears

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
