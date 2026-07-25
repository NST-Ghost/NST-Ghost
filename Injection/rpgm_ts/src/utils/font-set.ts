/// <reference path="../../typings/rmmv.d.ts" />
/// <reference path="../../typings/Font-Extensions.d.ts" />
// font-set.ts — Custom font loading and hook installation for RPG Maker MV/MZ.
// Ported from translator_scratch; [Translator] prefixes changed to [NST].

import { showError, showWarning } from "./error.js";
import { type FontConfig as LoaderFontConfig } from "./json-loader.js";
import pino from "./logger.js";
import { ok, Result } from "./neverthrow.js";
import { isMV, isMZ } from "./version-detection.js";

// ============================================
// Config definition
// ============================================
interface FontConfig {
  fontName: string;
  fontUrl: string;
  offsetSize?: number | undefined;
  maxSizeOffset?: number | undefined;
  dir?: string | undefined;
}

const CUSTOM_FONT_NAME = "NotoSans";
var customFontConfig: FontConfig | null = null;
var fontLoadAttempted = false;
var hooksInstalled = false;

// =============================================================================
// Logger (simplified)
// =============================================================================

var log = pino({
  level: "info",
});

// ============================================
// Font config validation (simplified)
// ============================================
function validateFontConfig(config: unknown): FontConfig | null {
  if (!config || typeof config !== "object") {
    return null;
  }

  var cfg = config as Record<string, unknown>;

  if (typeof cfg["fontName"] !== "string" || !cfg["fontName"]) {
    return null;
  }

  if (typeof cfg["fontUrl"] !== "string" || !cfg["fontUrl"]) {
    return null;
  }

  return {
    fontName: cfg["fontName"],
    fontUrl: cfg["fontUrl"],
    offsetSize:
      typeof cfg["offsetSize"] === "number" ? cfg["offsetSize"] : undefined,
    maxSizeOffset:
      typeof cfg["maxSizeOffset"] === "number"
        ? cfg["maxSizeOffset"]
        : undefined,
    dir: typeof cfg["dir"] === "string" ? cfg["dir"] : undefined,
  };
}

function getConfiguredFontName(): string {
  return customFontConfig?.fontName || CUSTOM_FONT_NAME;
}

function hasFontFaceApi(): boolean {
  return (
    typeof FontFace !== "undefined" &&
    typeof document !== "undefined" &&
    document.fonts !== undefined
  );
}

function escapeCssString(value: string): string {
  return value.replace(/\\/g, "\\\\").replace(/"/g, '\\"');
}

function addFontFaceStyle(config: FontConfig, fontUrl: string): void {
  if (typeof document === "undefined" || !document.head) {
    showError("字体加载失败", "当前环境不支持字体加载 API");
  }

  var style = document.createElement("style");
  style.type = "text/css";
  style.appendChild(
    document.createTextNode(
      '@font-face { font-family: "' +
        escapeCssString(config.fontName) +
        '"; src: url("' +
        escapeCssString(fontUrl) +
        '"); }',
    ),
  );
  document.head.appendChild(style);
  log.info(
    "[NST] Registered font via CSS @font-face:",
    config.fontName,
    fontUrl,
  );
  refreshAllWindows();
}

// ============================================
// Font file existence check
// ============================================
function checkFontFileExists(fontUrl: string): Promise<boolean> {
  return new Promise(function (resolve) {
    var xhr = new XMLHttpRequest();
    xhr.open("HEAD", fontUrl, true);
    xhr.onreadystatechange = function () {
      if (xhr.readyState === 4) {
        // 200-299 indicates success; 0 indicates local file (file://).
        resolve((xhr.status >= 200 && xhr.status < 300) || xhr.status === 0);
      }
    };
    xhr.onerror = function () {
      resolve(false);
    };
    try {
      xhr.send();
    } catch (_e) {
      // Some environments may throw.
      resolve(false);
    }
  });
}

// ============================================
// Font loading functions
// ============================================
function loadFont(config: FontConfig): Result<FontFace, Error> {
  try {
    var font = new FontFace(config.fontName, "url(" + config.fontUrl + ")");
    return ok(font);
  } catch (error) {
    var errorMsg = error instanceof Error ? error.message : String(error);
    // Explicit error: font creation failed (showError throws, does not return).
    showError(
      "字体创建失败",
      '无法创建字体 "' +
        config.fontName +
        '"\nURL: ' +
        config.fontUrl +
        "\n错误: " +
        errorMsg,
    );
  }
}

/**
 * Extract the file name from a full path.
 * e.g. "fonts/NotoSans-Regular.woff2" -> "NotoSans-Regular.woff2"
 */
function extractFileName(fontUrl: string): string {
  var parts = fontUrl.split("/");
  return parts[parts.length - 1] || fontUrl;
}

/**
 * MZ-specific font loader — uses both FontManager.load and FontFace API
 * to maximize compatibility.
 */
function loadCustomFontMZ(): void {
  if (fontLoadAttempted) return;
  fontLoadAttempted = true;

  var fontConfig: FontConfig = customFontConfig || {
    fontName: CUSTOM_FONT_NAME,
    fontUrl: "fonts/NotoSans-Regular.woff2",
    offsetSize: 0,
    maxSizeOffset: 0,
    dir: "fonts",
  };

  var validatedConfig = validateFontConfig(fontConfig);
  if (!validatedConfig) {
    showWarning("字体配置验证失败", "常规或默认字体配置无效，跳过自定义字体加载。");
    return;
  }

  var config = validatedConfig;
  // MZ uses FontManager.load(name, fileName).
  // fileName is relative to the fonts folder.
  var fileName = extractFileName(config.fontUrl);

  var FM = (
    window as unknown as {
      FontManager?: { load: (name: string, fileName: string) => void };
    }
  ).FontManager;

  try {
    // Method 1: FontManager.load (RPG Maker MZ built-in).
    if (FM && FM.load) {
      FM.load(config.fontName, fileName);
      log.info(
        "[NST] MZ FontManager.load invoked:",
        config.fontName,
        fileName,
      );
    }

    // Method 2: FontFace API or CSS @font-face fallback.
    var fontUrl = config.fontUrl;
    if (hasFontFaceApi()) {
      var font = new FontFace(config.fontName, 'url("' + fontUrl + '")');
      (document.fonts as FontFaceSetExtended).add(font);
      font.load().catch(function (error: unknown) {
        var errorMsg = error instanceof Error ? error.message : String(error);
        setTimeout(function () {
          showWarning(
            "字体加载失败",
            "无法加载字体\n" +
              "字体名称: " +
              config.fontName +
              "\n" +
              "文件名: " +
              fileName +
              "\n" +
              "错误信息: " +
              errorMsg,
          );
        }, 0);
      });
      log.info("[NST] MZ FontFace API added:", config.fontName, fontUrl);
    } else {
      addFontFaceStyle(config, fontUrl);
    }
  } catch (error) {
    var errorMsg = error instanceof Error ? error.message : String(error);
    showWarning(
      "字体加载失败",
      "无法加载字体\n" +
        "字体名称: " +
        config.fontName +
        "\n" +
        "文件名: " +
        fileName +
        "\n" +
        "错误信息: " +
        errorMsg,
    );
  }
}

/**
 * MV and fallback font loader — uses the FontFace API.
 */
function loadCustomFontFallback(config?: FontConfig): void {
  if (!config) {
    if (fontLoadAttempted) return;
    fontLoadAttempted = true;

    var fontConfig: FontConfig = customFontConfig || {
      fontName: CUSTOM_FONT_NAME,
      fontUrl: "fonts/NotoSans-Regular.woff2",
      offsetSize: 0,
      maxSizeOffset: 0,
      dir: "fonts",
    };

    var validatedConfig = validateFontConfig(fontConfig);
    if (!validatedConfig) {
      showWarning(
        "字体配置验证失败",
        "字体配置格式无效：缺少 fontName 或 fontUrl，跳过自定义字体加载。",
      );
      return;
    }
    config = validatedConfig;
  }

  // Check the font file exists first.
  checkFontFileExists(config.fontUrl).then(function (exists: boolean) {
    if (!exists) {
      setTimeout(function () {
        showWarning(
          "字体文件不存在",
          "找不到字体文件，请检查路径是否正确\n" +
            "字体名称: " +
            config!.fontName +
            "\n" +
            "字体路径: " +
            config!.fontUrl,
        );
      }, 0);
      return;
    }

    if (!hasFontFaceApi()) {
      addFontFaceStyle(config!, config!.fontUrl);
      return;
    }

    var fontResult = loadFont(config!);
    fontResult
      .map(function (font) {
        font
          .load()
          .then(function () {
            (document.fonts as FontFaceSetExtended).add(font);
            log.info("Custom font loaded:", font.family);
            refreshAllWindows();
          })
          .catch(function (error: unknown) {
            var errorMsg =
              error instanceof Error ? error.message : String(error);
            setTimeout(function () {
              showWarning(
                "字体文件加载失败",
                "无法加载字体文件\n" +
                  "字体名称: " +
                  config!.fontName +
                  "\n" +
                  "字体路径: " +
                  config!.fontUrl +
                  "\n" +
                  "错误信息: " +
                  errorMsg,
              );
            }, 0);
          });
      })
      .mapErr(function (error) {
        var errorMsg = error instanceof Error ? error.message : String(error);
        showWarning("字体创建失败", "无法创建字体对象\n错误信息: " + errorMsg);
      });
  });
}

/**
 * Universal font loading entry — selects the method based on engine type.
 */
function loadCustomFont(): void {
  if (isMZ()) {
    loadCustomFontMZ();
  } else {
    loadCustomFontFallback();
  }
}

function refreshAllWindows(): void {
  var scene = SceneManager._scene as Scene_Base & {
    _windowLayer?: {
      children: Array<{ refresh?: () => void }>;
    };
    _messageWindow?: { refresh?: () => void };
  };

  if (!scene) return;

  if (scene._windowLayer) {
    scene._windowLayer.children.forEach(function (child) {
      if (child && typeof child.refresh === "function") {
        child.refresh();
      }
    });
  }

  if (
    scene._messageWindow &&
    typeof scene._messageWindow.refresh === "function"
  ) {
    scene._messageWindow.refresh();
  }
}

// ============================================
// Hook installation
// ============================================
function installHooks(): void {
  if (hooksInstalled) return;

  if (typeof Scene_Boot === "undefined" || Scene_Boot === null) {
    console.warn("[NST] Scene_Boot undefined; cannot install font hooks");
    return;
  }

  hooksInstalled = true;

  // ============================================
  // MZ-specific API hooks
  // ============================================
  if (isMZ()) {
    log.info("[NST] Detected MZ; installing MZ font hooks");

    // MZ: Game_System.prototype.mainFontFace
    if (
      typeof Game_System !== "undefined" &&
      Game_System.prototype.mainFontFace
    ) {
      var _Game_System_mainFontFace = Game_System.prototype.mainFontFace;
      Game_System.prototype.mainFontFace = function (): string {
        var original = _Game_System_mainFontFace.call(this);
        return getConfiguredFontName() + ", " + original;
      };
    }

    // MZ: Game_System.prototype.mainFontSize
    if (
      typeof Game_System !== "undefined" &&
      Game_System.prototype.mainFontSize
    ) {
      var _Game_System_mainFontSize = Game_System.prototype.mainFontSize;
      Game_System.prototype.mainFontSize = function (): number {
        var originalSize = _Game_System_mainFontSize.call(this);
        var config = customFontConfig;
        if (config && config.offsetSize !== undefined) {
          var newSize = originalSize + config.offsetSize;
          if (config.maxSizeOffset !== undefined) {
            var maxSize = originalSize + config.maxSizeOffset;
            return Math.max(8, Math.min(newSize, maxSize));
          }
          return Math.max(8, newSize);
        }
        return originalSize;
      };
    }

    // MZ: font load hook (Scene_Boot.prototype.loadGameFonts)
    var _Scene_Boot_loadGameFonts = Scene_Boot.prototype.loadGameFonts;
    Scene_Boot.prototype.loadGameFonts = function (): void {
      _Scene_Boot_loadGameFonts.call(this);
      loadCustomFont();
      log.info("[NST] MZ font load hook executed");
    };

    // Note: we no longer block isReady; font loading happens in the background.
    // If font loading fails, showError reports it explicitly.
    log.info("[NST] MZ font hooks installed");
  }

  // ============================================
  // MV-specific API hooks
  // ============================================
  if (isMV()) {
    log.info("[NST] Detected MV; installing MV font hooks");

    // MV: Window_Base.prototype.standardFontFace
    if (
      typeof Window_Base !== "undefined" &&
      Window_Base.prototype.standardFontFace
    ) {
      var _Window_Base_standardFontFace =
        Window_Base.prototype.standardFontFace;
      Window_Base.prototype.standardFontFace = function (): string {
        var original = _Window_Base_standardFontFace.call(this);
        return getConfiguredFontName() + ", " + original;
      };
    }

    // MV: Window_Base.prototype.standardFontSize
    if (
      typeof Window_Base !== "undefined" &&
      Window_Base.prototype.standardFontSize
    ) {
      var _Window_Base_standardFontSize =
        Window_Base.prototype.standardFontSize;
      Window_Base.prototype.standardFontSize = function (): number {
        var originalSize = _Window_Base_standardFontSize.call(this);
        var config = customFontConfig;
        if (config && config.offsetSize !== undefined) {
          var newSize = originalSize + config.offsetSize;
          if (config.maxSizeOffset !== undefined) {
            var maxSize = originalSize + config.maxSizeOffset;
            return Math.max(8, Math.min(newSize, maxSize));
          }
          return Math.max(8, newSize);
        }
        return originalSize;
      };
    }

    // MV: font load hook (Scene_Boot.prototype.create)
    var _Scene_Boot_create = Scene_Boot.prototype.create;
    Scene_Boot.prototype.create = function (): void {
      _Scene_Boot_create.call(this);
      loadCustomFont();
      log.info("[NST] MV font load hook executed");
    };

    // Note: we no longer block isReady; font loading happens in the background.
    log.info("[NST] MV font hooks installed");
  }
}

// ============================================
// Public API
// ============================================

/**
 * Initialize font configuration.
 * Font hooks are installed after the config is initialized to avoid the
 * module-load-order issue that would cause a permanent skip.
 */
export function initFonts(fontConfig?: FontConfig): void {
  if (!fontConfig) {
    // When no fontConfig is provided, attempt to load from the JSON config.
    // json-loader re-exports the resolved config via getLoadedFontConfig().
    try {
      var loaded = getLoadedFontConfig();
      if (loaded) {
        fontConfig = {
          fontName: loaded.fontName,
          fontUrl: loaded.fontUrl,
          offsetSize: loaded.offsetSize,
          maxSizeOffset: loaded.maxSizeOffset,
        };
      }
    } catch (error) {
      var errorMsg = error instanceof Error ? error.message : String(error);
      showError(
        "字体配置加载失败",
        "无法从配置文件加载字体设置\n错误信息: " + errorMsg,
      );
    }
  }

  if (fontConfig) {
    customFontConfig = fontConfig;
  }

  log.info("[NST] Font config initialized");
  installHooks();
}

// ============================================
// Bridge to json-loader's resolved font config
// ============================================
// To avoid a circular import, json-loader publishes the resolved font config
// onto this module via setLoadedFontConfig at init time.
var _loadedFontConfig: LoaderFontConfig | undefined;

export function setLoadedFontConfig(cfg: LoaderFontConfig | undefined): void {
  _loadedFontConfig = cfg;
}

export function getLoadedFontConfig(): LoaderFontConfig | undefined {
  return _loadedFontConfig;
}
