// json-loader.ts — JSON Loading & Schema Validation (NST-adapted).
//
// This loader reads translations from the nst_translations/ folder at the
// game root (not from data/ like translator_scratch). Layout:
//
//   nst_translations/
//   ├── config.json      (hooks, patterns, font, sourceLocale)
//   ├── Actors.json      (per-game-data-file translation maps)
//   ├── Map001.json
//   └── System.json
//
// Translation files are loaded lazily by default: the base database files
// are loaded at init, and map-specific files are loaded on map change.

import { unwrap } from "./error.js";
import { err, ok, Result } from "./neverthrow.js";
import { z, infer as zodInfer, ZodType } from "./zod.js";
import pino from "./logger.js";
import { setLoadedFontConfig } from "./font-set.js";
import { DEFAULT_CONFIG } from "../defaults/default-config.js";

// NW.js provides require() as a global. Declared here so TypeScript does not
// complain about it in files that need Node.js fs access at runtime.
declare const require: {
  (id: string): unknown;
  cache: Record<string, unknown>;
};

// Minimal type for the Node.js fs module subset we actually use.
interface NodeFs {
  existsSync(path: string): boolean;
  readFileSync(path: string, encoding: string): string;
  readdirSync(path: string): string[];
}

// =============================================================================
// Schemas
// =============================================================================

const CUSTOM_HOOK_TYPES = [
  "tr0",
  "trRet",
  "trArray",
  "trNestedArray",
  "trObject",
  "trObjectAfter",
  "trRetObject",
  "trThis",
  "trThisAfter",
  "trCommandList",
  "trData",
  "trNested",
] as const;

const CustomHookTypeSchema = z.enum(CUSTOM_HOOK_TYPES);

type CustomHookTypeValue = (typeof CUSTOM_HOOK_TYPES)[number];

interface CustomHookConfigData {
  class: string;
  method: string;
  type?: CustomHookTypeValue | undefined;
  fields?: string[] | undefined;
  paramIndex?: number | undefined;
  nestedIndex?: number[] | undefined;
  minParamLength?: number | undefined;
  enabled?: boolean | undefined;
  static?: boolean | undefined;
  title?: string | undefined;
  desc?: string | undefined;
}

type TextCommandsData = Record<string, number[]>;
type TranslationObject = { translation: string };
type TranslationsData = Record<
  string,
  string | TranslationObject
>;

const HOOK_TYPES_REQUIRING_FIELDS: Record<string, true> = {
  trObject: true,
  trObjectAfter: true,
  trRetObject: true,
  trThis: true,
  trThisAfter: true,
};

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === "object" && value !== null && !Array.isArray(value);
}

function isNonNegativeInteger(value: unknown): value is number {
  return (
    typeof value === "number" &&
    isFinite(value) &&
    Math.floor(value) === value &&
    value >= 0
  );
}

function requireNonEmptyString(value: unknown, path: string): string {
  if (typeof value !== "string" || value.length === 0) {
    throw new Error(path + " must be a non-empty string");
  }
  return value;
}

function readOptionalString(value: unknown, path: string): string | undefined {
  if (value === undefined) return undefined;
  if (typeof value !== "string") {
    throw new Error(path + " must be a string");
  }
  return value;
}

function readOptionalBoolean(value: unknown, path: string): boolean | undefined {
  if (value === undefined) return undefined;
  if (typeof value !== "boolean") {
    throw new Error(path + " must be a boolean");
  }
  return value;
}

function readOptionalIndex(value: unknown, path: string): number | undefined {
  if (value === undefined) return undefined;
  if (!isNonNegativeInteger(value)) {
    throw new Error(path + " must be a non-negative integer");
  }
  return value;
}

function readOptionalStringArray(
  value: unknown,
  path: string,
): string[] | undefined {
  if (value === undefined) return undefined;
  if (!Array.isArray(value)) {
    throw new Error(path + " must be an array");
  }
  return value.map((item, index) =>
    requireNonEmptyString(item, path + "[" + index + "]"),
  );
}

function readOptionalIndexArray(
  value: unknown,
  path: string,
): number[] | undefined {
  if (value === undefined) return undefined;
  if (!Array.isArray(value)) {
    throw new Error(path + " must be an array");
  }
  return value.map((item, index) => {
    if (!isNonNegativeInteger(item)) {
      throw new Error(path + "[" + index + "] must be a non-negative integer");
    }
    return item;
  });
}

function isCustomHookType(value: string): value is CustomHookTypeValue {
  return CUSTOM_HOOK_TYPES.indexOf(value as CustomHookTypeValue) !== -1;
}

function parseCustomHookConfig(data: unknown): CustomHookConfigData {
  if (!isRecord(data)) {
    throw new Error("custom hook must be an object");
  }

  const typeValue = readOptionalString(data["type"], "custom hook type");
  if (typeValue !== undefined && !isCustomHookType(typeValue)) {
    throw new Error("custom hook type is invalid: " + typeValue);
  }

  const result: CustomHookConfigData = {
    class: requireNonEmptyString(data["class"], "custom hook class"),
    method: requireNonEmptyString(data["method"], "custom hook method"),
  };

  if (typeValue !== undefined) result.type = typeValue;

  const fields = readOptionalStringArray(data["fields"], "custom hook fields");
  if (fields !== undefined) result.fields = fields;

  const paramIndex = readOptionalIndex(
    data["paramIndex"],
    "custom hook paramIndex",
  );
  if (paramIndex !== undefined) result.paramIndex = paramIndex;

  const nestedIndex = readOptionalIndexArray(
    data["nestedIndex"],
    "custom hook nestedIndex",
  );
  if (nestedIndex !== undefined) result.nestedIndex = nestedIndex;

  const minParamLength = readOptionalIndex(
    data["minParamLength"],
    "custom hook minParamLength",
  );
  if (minParamLength !== undefined) result.minParamLength = minParamLength;

  const enabled = readOptionalBoolean(data["enabled"], "custom hook enabled");
  if (enabled !== undefined) result.enabled = enabled;

  const isStatic = readOptionalBoolean(data["static"], "custom hook static");
  if (isStatic !== undefined) result.static = isStatic;

  const title = readOptionalString(data["title"], "custom hook title");
  if (title !== undefined) result.title = title;

  const desc = readOptionalString(data["desc"], "custom hook desc");
  if (desc !== undefined) result.desc = desc;

  const effectiveType = result.type ?? "tr0";
  if (
    HOOK_TYPES_REQUIRING_FIELDS[effectiveType] &&
    (!result.fields || result.fields.length === 0)
  ) {
    throw new Error("custom hook type " + effectiveType + " requires fields");
  }

  return result;
}

function parseTextCommands(data: unknown): TextCommandsData {
  if (!isRecord(data)) {
    throw new Error("__textCommands__ must be an object");
  }

  const result: TextCommandsData = {};
  for (const key of Object.keys(data)) {
    if (!/^\d+$/.test(key)) {
      throw new Error(
        "__textCommands__ key must be a numeric command code: " + key,
      );
    }
    const indices = readOptionalIndexArray(data[key], "__textCommands__." + key);
    result[key] = indices ?? [];
  }
  return result;
}

function parseTranslationsData(data: unknown): TranslationsData {
  if (!isRecord(data)) {
    throw new Error("translations must be an object");
  }

  const result: TranslationsData = {};
  for (const key of Object.keys(data)) {
    const value = data[key];

    // Keys starting with "_" are comments / disabled entries — skip them.
    // Meta keys (__x__) are NOT skipped here; they are handled by the caller.
    if (key.charAt(0) === "_" && !(key.startsWith("__") && key.endsWith("__"))) {
      continue;
    }

    if (typeof value === "string") {
      result[key] = value;
      continue;
    }

    if (isRecord(value) && typeof value["translation"] === "string") {
      result[key] = { translation: value["translation"] };
      continue;
    }

    throw new Error(
      "translation entry " + key + " must be a string or { translation: string }",
    );
  }
  return result;
}

const CustomHookConfigSchema = z.custom<CustomHookConfigData>(
  parseCustomHookConfig,
);

const TextCommandsSchema = z.custom<TextCommandsData>(parseTextCommands);

export const FontConfigSchema = z.object({
  offsetSize: z.number().default(0),
  maxSizeOffset: z.number().default(0),
  fontName: z.string().min(1).default("NotoSans"),
  fontUrl: z.string().min(1).default("NotoSans-Regular.woff2"),
});

export const ConfigSchema = z.object({
  __textFields__: z
    .array(z.string().min(1))
    .default([
      "name",
      "description",
      "displayName",
      "nickname",
      "profile",
      "message1",
      "message2",
      "message3",
      "message4",
      "gameTitle",
    ]),
  __textCommands__: TextCommandsSchema.default({
    "101": [4],
    "102": [0],
    "401": [0],
    "405": [0],
  }),
  __controlCharPatterns__: z
    .array(z.string().min(1))
    .default(["\\\\[A-Za-z]+(?:\\\\[\\\\d+\\\\])?"]),
  __ignorePatterns__: z
    .array(z.string().min(1))
    .default([String.raw`^(?:\s*|\d+|[\s\d.,]+)$`]),
  __fontConfig__: FontConfigSchema.optional(),
  __customHooks__: z.array(CustomHookConfigSchema).optional(),
  __sourceLocale__: z.string().min(1).default(String.raw`ja`),
});

export const TranslationsSchema = z.custom<TranslationsData>(
  parseTranslationsData,
);

// =============================================================================
// Types
// =============================================================================

export type Config = zodInfer<typeof ConfigSchema>;
export type Translations = zodInfer<typeof TranslationsSchema>;
export type FontConfig = zodInfer<typeof FontConfigSchema>;
export type CustomHookConfig = zodInfer<typeof CustomHookConfigSchema>;
export type CustomHookType = zodInfer<typeof CustomHookTypeSchema>;

export interface LoadResult {
  config: Config;
  translations: Map<string, string>;
  fontConfig: FontConfig | undefined;
  customHooks: CustomHookConfig[] | undefined;
  sourceLocale: string;
}

// =============================================================================
// Path Utilities
// =============================================================================

/**
 * Detect the game root directory.
 * RPG Maker MV/MZ supports two directory structures:
 *
 * Structure 1 (standard):
 *   Game root/
 *   ├── data/          <- data files
 *   ├── js/
 *   │   └── plugins/   <- plugins directory
 *   └── index.html
 *
 * Structure 2 (www subdirectory):
 *   Game root/
 *   ├── www/
 *   │   ├── data/      <- data files
 *   │   ├── js/
 *   │   │   └── plugins/
 *   │   └── index.html
 *   └── Game.exe
 *
 * We look for nst_translations/ relative to the game root.
 */
function getBasePath(): string {
  // Try Node.js path resolution if running inside NW.js or Electron
  var glob = (typeof globalThis !== "undefined" ? globalThis : typeof window !== "undefined" ? window : {}) as any;
  if (typeof glob.require !== "undefined" && typeof glob.process !== "undefined" && glob.process.mainModule) {
    try {
      var path = glob.require("path");
      var base = path.dirname(glob.process.mainModule.filename);
      if (base) return base;
    } catch (_) {}
  }

  if (typeof window !== "undefined") {
    const href = window.location.href;
    const basePath = getDirectoryPath(href);
    if (basePath && basePath !== ".") return basePath;

    const currentScript = document.currentScript as HTMLScriptElement | null;
    if (currentScript && currentScript.src) {
      // Go up 2 levels from js/plugins/<this>.js -> game root.
      return getDirectoryPath(getDirectoryPath(getDirectoryPath(currentScript.src)));
    }

    const scripts = document.getElementsByTagName("script");
    if (scripts.length > 0) {
      const lastScript = scripts[scripts.length - 1];
      if (lastScript && lastScript.src) {
        return getDirectoryPath(getDirectoryPath(getDirectoryPath(lastScript.src)));
      }
    }
  }
  return ".";
}

function getDirectoryPath(filePath: string): string {
  const cleanPath = (filePath.split("?")[0] ?? filePath).split("#")[0] ?? filePath;
  let lastSlash = cleanPath.lastIndexOf("/");
  if (lastSlash === -1) lastSlash = cleanPath.lastIndexOf("\\");
  return lastSlash > 0 ? cleanPath.substring(0, lastSlash) : ".";
}

function joinPath(base: string, relative: string): string {
  const normalizedBase = base.replace(/[/\\]+$/, "");
  const normalizedRelative = relative.replace(/^[/\\]+/, "");
  return normalizedBase + "/" + normalizedRelative;
}

// Export joinPath for potential use by other modules.
export { joinPath };

// =============================================================================
// Cross-Platform File Reading
// =============================================================================

function toLocalPath(path: string): string {
  if (path.indexOf("file://") === 0) {
    var decoded = decodeURIComponent(path.substring(7));
    if (decoded.match(/^\/[a-zA-Z]:/)) {
      return decoded.substring(1);
    }
    return decoded;
  }
  return path;
}

/**
 * Read a file as text. Works on NW.js, JoiPlay, Electron, and browsers.
 * @param filePath - path relative to game root
 * @returns file content or null
 */
function readFileSync(filePath: string): string | null {
  var full = getBasePath() + "/" + filePath;

  // Node.js fs (NW.js desktop).
  if (typeof require !== "undefined") {
    try {
      var fs = require("fs") as NodeFs;
      var localPath = toLocalPath(full);
      if (fs.existsSync(localPath)) {
        return fs.readFileSync(localPath, "utf8");
      }
      return null;
    } catch (_e) { /* fall through to XHR */ }
  }

  // Synchronous XHR — works on JoiPlay, browsers, and everything else.
  try {
    var x = new XMLHttpRequest();
    x.open("GET", full, false);
    x.overrideMimeType("application/json; charset=utf-8");
    x.send(null);
    return (x.status === 200 || x.status === 0) ? x.responseText : null;
  } catch (_e) {
    return null;
  }
}

// =============================================================================
// JSON Loader
// =============================================================================

const log = pino({ level: "info" });

function loadJsonSync<T>(filePath: string, schema: ZodType<T>): Result<T, string> {
  try {
    const fileContent = readFileSync(filePath);
    if (!fileContent) return err("File not found or unreadable: " + filePath);

    let jsonData: unknown;
    try {
      jsonData = JSON.parse(fileContent);
    } catch (parseError) {
      const parseMsg = parseError instanceof Error ? parseError.message : String(parseError);
      return err("JSON parse failed: " + filePath + "\nError: " + parseMsg);
    }

    const parsed = schema.safeParse(jsonData);
    if (!parsed.success) {
      const errorMsg = parsed.error ? parsed.error.message : "Unknown validation error";
      return err("Validation failed: " + filePath + "\nError: " + errorMsg);
    }

    return ok(parsed.data as T);
  } catch (e) {
    const message = e instanceof Error ? e.message : String(e);
    return err("Unknown error loading: " + filePath + "\nError: " + message);
  }
}

// =============================================================================
// Translation Parsing
// =============================================================================

/**
 * Parse a translation data object into a source→translated Map.
 * Supports both simple { source: translation } and extended { source: { translation } } formats.
 * Entries with keys starting with "_" (but not "__x__") are treated as comments and skipped.
 */
function parseTranslations(data: Translations): Map<string, string> {
  const map = new Map<string, string>();

  for (const key of Object.keys(data)) {
    const value = data[key];

    // Skip comment/disabled entries (keys starting with "_" but not meta-keys).
    if (key.charAt(0) === "_" && !(key.startsWith("__") && key.endsWith("__"))) {
      continue;
    }

    let text: string | null = null;
    if (typeof value === "string") {
      text = value;
    } else if (!Array.isArray(value) && value && typeof value === "object") {
      text = (value as { translation: string }).translation;
    }

    // Skip entries where source equals translation (no-op).
    if (text && text !== key) {
      map.set(key, text);
    }
  }

  // Sort entries by key length (longest first) for optimal partial matching.
  const entries = Array.from(map.entries()).sort((a, b) => b[0].length - a[0].length);
  return new Map(entries);
}

// =============================================================================
// Lazy-Load State
// =============================================================================

var _loadedFiles: Record<string, boolean> = {};
var _baseTranslations: Map<string, string> = new Map();

/** Database files that are always loaded at init. */
const DB_FILES = [
  "Actors", "Classes", "Skills", "Items", "Weapons", "Armors",
  "Enemies", "Troops", "States", "CommonEvents", "System", "_inline",
];

/**
 * Load a single translation JSON file and merge its entries into the
 * shared translation map. Called both at init (for DB files) and lazily
 * (for map files on map change).
 */
function loadTranslationFile(fileName: string): number {
  if (_loadedFiles[fileName]) return 0;

  var txt = readFileSync("nst_translations/" + fileName + ".json");
  if (txt === null) return 0;

  var parsed = loadJsonSync<Translations>(
    "nst_translations/" + fileName + ".json",
    TranslationsSchema,
  );

  if (parsed.isErr()) {
    log.warn("Failed to load " + fileName + ".json: " + parsed.error);
    return 0;
  }

  var translations = parseTranslations(parsed.value);
  translations.forEach(function (value, key) {
    _baseTranslations.set(key, value);
  });

  _loadedFiles[fileName] = true;
  log.info("Loaded " + fileName + ".json (" + translations.size + " entries)");
  return translations.size;
}

// =============================================================================
// Public API
// =============================================================================

/**
 * Load all database translation files and the config.
 * This is the primary initialization entry point.
 */
export function loadAll(): LoadResult {
  const loadedConfigResult = loadJsonSync<any>("nst_translations/config.json", z.custom<any>((val) => val));
  const loadedConfig = unwrap(loadedConfigResult, "Config Load Failed");

  const merged = {
    ...DEFAULT_CONFIG,
    ...loadedConfig,
    __textCommands__: loadedConfig?.__textCommands__
      ? { ...DEFAULT_CONFIG.__textCommands__, ...loadedConfig.__textCommands__ }
      : DEFAULT_CONFIG.__textCommands__,
    __fontConfig__: loadedConfig?.__fontConfig__
      ? { ...DEFAULT_CONFIG.__fontConfig__, ...loadedConfig.__fontConfig__ }
      : DEFAULT_CONFIG.__fontConfig__,
  };

  const parsed = ConfigSchema.safeParse(merged);
  if (!parsed.success) {
    throw new Error(parsed.error ? parsed.error.message : "Unknown validation error");
  }
  const config = parsed.data!;

  // Publish font config to the font-set module.
  setLoadedFontConfig(config.__fontConfig__);

  // Load all database files.
  _baseTranslations = new Map();
  _loadedFiles = {};

  var totalEntries = 0;
  for (var i = 0; i < DB_FILES.length; i++) {
    var dbFile = DB_FILES[i];
    if (dbFile !== undefined) {
      totalEntries += loadTranslationFile(dbFile);
    }
  }

  // On NW.js, scan the nst_translations/ directory for any additional .json files.
  if (typeof require !== "undefined") {
    try {
      var fs = require("fs") as NodeFs;
      var dir = toLocalPath(getBasePath() + "/nst_translations/");
      if (fs.existsSync(dir)) {
        var all = fs.readdirSync(dir);
        for (var j = 0; j < all.length; j++) {
          var fname = all[j];
          if (fname !== undefined && /\.json$/i.test(fname) && fname !== "config.json") {
            var baseName = fname.replace(/\.json$/i, "");
            totalEntries += loadTranslationFile(baseName);
          }
        }
      }
    } catch (_e) { /* ignore */ }
  }

  log.info({
    configEntries: Object.keys(config).length,
    translationEntries: _baseTranslations.size,
    filesLoaded: Object.keys(_loadedFiles).length,
  }, "Load complete");

  return {
    config,
    translations: _baseTranslations,
    fontConfig: config.__fontConfig__,
    customHooks: config.__customHooks__,
    sourceLocale: config.__sourceLocale__,
  };
}

/**
 * Load a map-specific translation file. Call this when the game changes maps.
 * @param mapId - the map number (e.g. 1, 2, 3)
 */
export function loadMap(mapId: number): void {
  if (mapId <= 0) return;
  var s = String(mapId);
  while (s.length < 3) s = "0" + s;
  loadTranslationFile("Map" + s);
}

/**
 * Reload everything from disk. Useful for the F12 debug console.
 */
export function reloadAll(): LoadResult {
  _baseTranslations = new Map();
  _loadedFiles = {};
  return loadAll();
}

/**
 * Get the current translation map (read-only reference).
 * Used by the debug API.
 */
export function getTranslations(): ReadonlyMap<string, string> {
  return _baseTranslations;
}
