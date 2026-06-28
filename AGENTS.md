# Agent guide — LumonTime

Pebble Time 2 (emery) watchface. Native Pebble C SDK with companion JS and Clay config.

## Source layout

| Path | Purpose |
| --- | --- |
| `src/c/mdbl.c` | Watchface logic, rendering, animation, health/battery |
| `src/pkjs/config.json` | Clay settings page definition |
| `src/pkjs/index.js` | Phone-side companion: Clay config, settings sync |
| `outline-rules/` | ast-grep outline extractors for C and JavaScript |

Build with `pebble build`. Install with `pebble install --emulator emery`.

## Code navigation with ast-grep outline

Use [ast-grep outline](https://ast-grep.github.io/blog/ast-grep-outline.html) for a structural first pass before reading full files. It returns symbol names, types, and line ranges so you can open the smallest useful slice.

Bundled ast-grep rules do not cover this project's C code well. This repo ships custom extractors in `outline-rules/` and npm scripts that load them.

### Commands

```bash
npm install

# File or directory structure (default: local top-level symbols, digest view)
npm run outline -- src/c/mdbl.c
npm run outline -- src

# Directory summary (grouped symbol names)
npm run outline -- src --view names

# Imports / dependencies only
npm run outline -- src/c/mdbl.c --items imports

# Drill into one symbol
npm run outline -- src/c/mdbl.c --match init --view expanded

# Machine-readable output (line ranges for targeted reads)
npm run outline:json -- src
```

Pass paths after `--`. Additional flags go to `ast-grep outline` directly.

### When to use outline

1. **Unfamiliar file** — run outline on the file before reading it whole.
2. **Find entry points** — outline a directory with `--view names`, then `--match` on a candidate symbol.
3. **Trace dependencies** — `--items imports` on a file or subtree.
4. **Targeted edit** — `--match <symbol> --view expanded` to see members/signatures, then read only those line ranges.

Outline is local and syntax-based. It does not resolve types, follow references, or answer "who calls X". Use full file reads or search for that.

### What the custom rules extract

**C** (`outline-rules/c.yml`): `#include`, externs, typedef structs/enums, static globals, forward declarations, function definitions.

**JavaScript** (`outline-rules/javascript.yml`): top-level `var` constants, `Pebble.addEventListener(...)` handlers, plus bundled rules for `function` declarations.

## Linting with ast-grep

```bash
npm run lint:ast    # scan src/ against rules in rules/
npm run test:ast    # run rule snapshot tests in rule-tests/
```

Add new scan rules under `rules/`. Project config is in `sgconfig.yml`.

## Conventions

- Pebble C watchface code lives in a single translation unit (`src/c/mdbl.c`).
- Settings flow: Clay `config.json` → `webviewclosed` → `localStorage` → AppMessage `ANIM_MODE` → C persist key.
- Prefer minimal diffs. Match existing naming and layout patterns in `mdbl.c`.
