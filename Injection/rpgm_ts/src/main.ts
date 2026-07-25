// main.ts — NST Translation Layer plugin entry.
// Supports RPG Maker MV/MZ.
//
// This file is the IIFE entry that Rollup bundles into a single
// NST_TranslationLayer.js file for dropping into js/plugins/.

import "core-js/modules/es.array.from.js";
import "core-js/modules/es.array.iterator.js";
import "core-js/modules/es.map.js";
import "core-js/modules/es.promise.js";
import "core-js/modules/es.string.ends-with.js";
import "core-js/modules/es.string.raw.js";
import "core-js/modules/es.string.starts-with.js";
import "core-js/modules/es.weak-set.js";

import { Translator } from "./core/translator.js";
import { initFonts } from "./utils/font-set.js";
import { loadAll, loadMap, reloadAll } from "./utils/json-loader.js";
import pino from "./utils/logger.js";

const log = pino({
  level: "info",
});

// =============================================================================
// Global type declarations for RPG Maker hooks
// =============================================================================

declare const Scene_Boot:
  | {
      prototype: {
        start(): void;
      };
    }
  | undefined;

declare const Game_Map:
  | {
      prototype: {
        setup(mapId: number): void;
      };
    }
  | undefined;

declare const window: Window & Record<string, unknown>;

// =============================================================================
// Initialization
// =============================================================================

const _preloaded = loadAll();
initFonts(_preloaded.fontConfig);

let translatorInitialized = false;

function initializeTranslator(): void {
  if (translatorInitialized) return;
  Translator.init(_preloaded);
  Translator.hook();
  translatorInitialized = true;
  log.info("Translator initialized");
}

// Initialize immediately on plugin load.
initializeTranslator();

// RPG Maker MV/MZ boot hook — re-initialize after the engine is ready.
if (typeof Scene_Boot !== "undefined" && Scene_Boot !== null) {
  const origSceneBootStart = Scene_Boot.prototype.start;
  Scene_Boot.prototype.start = function () {
    initializeTranslator();
    origSceneBootStart.call(this);
  };
  log.info("RPG Maker boot hook installed");
}

// =============================================================================
// Map-change lazy load hook
// =============================================================================

if (typeof Game_Map !== "undefined" && Game_Map !== null) {
  const origGameMapSetup = Game_Map.prototype.setup;
  Game_Map.prototype.setup = function (mapId: number) {
    origGameMapSetup.call(this, mapId);
    loadMap(mapId);
  };
  log.info("Map lazy-load hook installed");
}

log.info("Plugin loaded");

// =============================================================================
// Public Debug API (F12 console)
// =============================================================================

window.NST = window.NST || {};
(window.NST as Record<string, unknown>).TL = {
  /**
   * Translate a string (same as the internal pipeline).
   */
  translate: Translator.translate,

  /**
   * Reload all translations and config from disk.
   */
  reload: function () {
    var result = reloadAll();
    Translator.init(result);
    log.info("Translations reloaded. " + result.translations.size + " entries.");
  },

  /**
   * Show statistics about loaded translations.
   */
  stats: function () {
    var s = Translator.stats();
    console.log("[NST] === Translation Stats ===");
    console.log("  Source entries: " + (s.translations as number));
    console.log("  Cached: " + (s.cached as number));
    console.log("  Missed: " + (s.missed as number));
    console.log("  Regex patterns: " + (s.regexPatterns as number));
    console.log("  AC patterns: " + (s.acPatterns as number));
    console.log("  Ready: " + (s.ready as boolean));
    console.log("  Hooked: " + (s.hooked as boolean));
  },

  /**
   * List the top 100 missed (untranslated) source-locale strings.
   */
  missed: function () {
    var missed = Translator.getMissed();
    console.log("[NST] === Missed Translations (top 100) ===");
    for (var i = 0; i < Math.min(missed.length, 100); i++) {
      console.log("  [" + i + "] " + missed[i]);
    }
    console.log("  Total missed: " + missed.length);
  },

  /**
   * Export missed translations as a JSON string (for feeding back to the tool).
   */
  exportMissed: function (includeCount = false) {
    console.log(Translator.exportMissed(includeCount));
  },
};
