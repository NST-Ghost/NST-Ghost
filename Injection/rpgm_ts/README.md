# NST Translator — TypeScript Sub-Project

This directory contains the TypeScript source for the **NST Translation Layer**,
the runtime translation plugin for RPG Maker MV/MZ games.

The TypeScript here is bundled by Rollup into a single drop-in JavaScript file
(`NST_TranslationLayer.js`) that ships inside a game's `js/plugins/` directory
and reads translations from a `nst_translations/` folder at the game root.

```
src/                     TypeScript source
├── main.ts              IIFE entry point + public debug API
├── core/
│   ├── translator.ts    Translation engine (cache, Aho-Corasick, regex, ...)
│   └── custom-hooks.ts  Config-driven runtime hook system (12 hook types)
├── utils/
│   ├── json-loader.ts   Loads nst_translations/{config,*.}.json + schema validation
│   ├── font-set.ts      Custom font loading for MV/MZ
│   ├── logger.ts        Browser-compatible logger (no Node dependency at runtime)
│   ├── error.ts         showError + Result unwrap
│   ├── neverthrow.ts    Result<T,E> type
│   ├── version-detection.ts
│   └── zod.ts           Tiny zod-like schema validation (ES5-compatible)
└── defaults/
    └── default-config.ts  Default config (replicates legacy hardcoded hooks)

typings/                 RPG Maker MV/MZ type definitions
dist/                    Build output (gitignored)
```

## Requirements

- **Node.js >= 18** and **npm**. Only needed at build time; not required at
  runtime — the generated JS file is a self-contained IIFE.

## Building

The recommended way is the repo-level helper script, which installs deps,
builds, and copies the output into `Injection/rpgm/` in one step:

```bash
# From the repository root:
./scripts/build-translator.sh           # dev build (with sourcemap)
./scripts/build-translator.sh --prod    # production build (minified, ~90 KB)
```

On Windows:

```bat
scripts\build-translator.bat
scripts\build-translator.bat --prod
```

Alternatively, run the build directly inside this directory:

```bash
cd Injection/rpgm_ts
npm ci              # install dependencies (first time only)
npm run build       # dev build
npm run build:prod  # production build
npm run typecheck   # type-check without emitting
```

The build output lands at `dist/NST_TranslationLayer.js`.

## How It Integrates With NST

- **CMake** does not invoke this build automatically (to avoid forcing every
  developer to install Node). Instead, the CMake `POST_BUILD` step checks
  whether `Injection/rpgm/NST_TranslationLayer.js` exists and copies it into
  the build tree, emitting a warning if it is missing.
- The C++ `RpgmInjectionExporter` deploys the built plugin into a game's
  `js/plugins/` directory along with generated `nst_translations/*.json`
  files when the user chooses *Deploy as Injection Layer*.
- The generated JS reads JSON (not the legacy `.txt` marker format). The C++
  exporter writes that JSON.

## Adding Hooks for New Plugins

The translation layer is **config-driven**: hooks are declared in
`nst_translations/config.json` under `__customHooks__`, not hardcoded in the
plugin. To add support for a new third-party plugin, add an entry to the
config (or to `src/defaults/default-config.ts` for the built-in default):

```json
{
  "class": "Window_MyPlugin",
  "method": "refresh",
  "type": "trThisAfter",
  "fields": ["_text"],
  "enabled": true
}
```

The 12 supported hook types are documented in `src/core/custom-hooks.ts`.

## Layout Produced in a Deployed Game

```
[Game root]/
└── nst_translations/
    ├── config.json        hooks, patterns, font, sourceLocale
    ├── Actors.json        { "source": "translation", ... }
    ├── Map001.json
    └── System.json
```
