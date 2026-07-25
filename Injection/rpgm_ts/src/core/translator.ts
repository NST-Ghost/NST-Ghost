// translator.ts
// RPG Maker MV/MZ 翻译插件
/// <reference path="../../typings/ahocorasick.d.ts" />
import AhoCorasick from "ahocorasick";
import { LRUCache } from "mnemonist";
import { showError } from "../utils/error.js";
import { type Config, type LoadResult, loadAll } from "../utils/json-loader.js";
import pino from "../utils/logger.js";
import { applyHooks as applyCustomHooks } from "./custom-hooks.js";

// =============================================================================
// 类型定义
// =============================================================================

type ACSearchResult = Array<[number, string[]]>;

interface ProcessedMatch {
  start: number;
  end: number;
  pattern: string;
  translation: string;
}

interface RegexPatternEntry {
  pattern: RegExp;
  replacement: string;
}

interface ControlExtraction {
  plain: string;
  controls: ControlInfo[];
}

interface ControlInfo {
  text: string;
  index: number; // 控制符在原文中的起始位置
}

// =============================================================================
// 常量配置
// =============================================================================

const REGEX_KEY = "__regex__";
const DEFAULT_MISSED_LIMIT = 500;
const DEFAULT_MAX_DEPTH = 6;
const DEFAULT_CACHE_SIZE = 3000;
const MAX_LOOP_COUNT = 100;
const MAX_MARKERS_SIZE = 5000;

const PLACEHOLDER_BRACE = /\{(\d+)\}/g;

/**
 * 验证正则表达式标志是否有效
 * @param flags 标志字符串
 * @returns 如果标志有效返回 true
 */
function isValidRegexFlags(flags: string): boolean {
  // 只允许有效的正则表达式标志
  return /^[gimsuy]*$/.test(flags);
}

// =============================================================================
// 日志配置（简化版）
// =============================================================================

const log = pino({
  level: "info",
});

// =============================================================================
// 状态管理
// =============================================================================

let translatedObjects = new WeakSet<object>();

const state = {
  dict: new Map<string, string>(),
  sortedKeys: [] as string[],
  cache: new LRUCache<string, string>(DEFAULT_CACHE_SIZE),
  missed: new Map<string, number>(),
  config: null as Config | null,
  controlRe: null as RegExp | null,
  controlPattern: "" as string, // 存储控制符模式字符串，用于创建新正则实例
  ignoreRe: null as RegExp | null,
  regexPatterns: [] as RegexPatternEntry[],
  ready: false,
  hooked: false,
  acAutomaton: null as AhoCorasick | null,
  acPatterns: [] as string[],
  missedLimit: DEFAULT_MISSED_LIMIT,
  maxDepth: DEFAULT_MAX_DEPTH,
  // 用 LRUCache 替换 Set，避免 FIFO 淘汰时的 O(n) 遍历
  translatedMarkers: new LRUCache<string, true>(MAX_MARKERS_SIZE),
  sourceLocaleCharsRe: null as RegExp | null,
};

// =============================================================================
// 初始化
// =============================================================================

function rebuildAhoCorasick(): void {
  const patterns = state.sortedKeys;

  if (patterns.length === 0) {
    state.acAutomaton = null;
    state.acPatterns = [];
    return;
  }

  state.acPatterns = patterns;
  state.acAutomaton = new AhoCorasick(patterns);
}

function invalidateAhoCorasick(): void {
  state.acAutomaton = null;
  state.acPatterns = [];
}

function rebuildSortedKeys(): void {
  state.sortedKeys = Array.from(state.dict.keys())
    .filter((key) => !(key.startsWith("__") && key.endsWith("__")))
    .sort((a, b) => b.length - a.length);
}

/**
 * 根据源语言代码构建字符检测正则表达式
 * @param sourceLocale 源语言代码（如 'ja', 'ko', 'en' 等）
 * @returns 用于检测源语言字符的正则表达式，如果无法识别则返回 null
 */
function buildSourceLocaleRegex(sourceLocale: string): RegExp | null {
  const locale = sourceLocale.toLowerCase();

  // 日语：平假名 + 片假名（不包含CJK汉字以避免误判中文）
  if (locale === "ja" || locale === "jp" || locale === "jpn") {
    return /[\u3040-\u309F\u30A0-\u30FF]/;
  }

  // 韩语：谚文（韩文字母）
  if (locale === "ko" || locale === "kor" || locale === "kr") {
    return /[\uAC00-\uD7AF\u1100-\u11FF\u3130-\u318F]/;
  }

  // 英语：基本拉丁字母
  if (locale === "en" || locale === "eng" || locale.startsWith("en-")) {
    return /[a-zA-Z]/;
  }

  showError(
    "不支持的源语言",
    `无法识别的语言代码: "${sourceLocale}"\n支持的语言代码:\n  - 日语: ja, jp, jpn\n  - 韩语: ko, kor, kr\n  - 英语: en, eng, en-*`,
  );
}

function init(preloaded?: LoadResult): void {
  if (state.ready) return;

  const { config, translations, sourceLocale } = preloaded ?? loadAll();

  state.config = config;

  state.sourceLocaleCharsRe = buildSourceLocaleRegex(sourceLocale);

  const controlPatterns = config?.__controlCharPatterns__ ?? [];
  const ignorePatterns = config?.__ignorePatterns__ ?? [];

  const controlPattern =
    controlPatterns.length > 0 ? controlPatterns.join("|") : "";
  state.controlRe = safeCompileRegex(controlPattern, "gi");
  // 存储控制符模式字符串，用于在extractControls 中创建新的正则实例
  state.controlPattern = controlPattern;

  state.ignoreRe = safeCompileRegex(
    ignorePatterns.length > 0 ? ignorePatterns.join("|") : "",
  );

  state.dict = translations;

  rebuildSortedKeys();

  const regexData = state.dict.get(REGEX_KEY);
  if (regexData) {
    loadRegexPatterns(regexData);
    state.dict.delete(REGEX_KEY);
    rebuildSortedKeys();
  }

  rebuildAhoCorasick();

  state.ready = true;
  log.info({ count: state.dict.size }, "Translator ready");
}

function safeCompileRegex(pattern: string, flags?: string): RegExp | null {
  if (!pattern) return null;

  if (flags && !isValidRegexFlags(flags)) {
    showError(
      "正则表达式标志无效",
      `无效的正则表达式标志: "${flags}"，只允许 g, i, m, s, u, y`,
    );
  }

  try {
    return new RegExp(pattern, flags);
  } catch (e) {
    const errorMsg = e instanceof Error ? e.message : String(e);
    return showError(
      "正则表达式编译失败",
      `无法编译正则表达式: "${pattern}"\n错误: ${errorMsg}`,
    );
  }
}

function loadRegexPatterns(regexData: string): void {
  let regexArray: Array<[string, string, string?]>;

  try {
    regexArray = JSON.parse(regexData);
  } catch (e) {
    const errorMsg = e instanceof Error ? e.message : String(e);
    showError(
      "正则模式解析失败",
      `无法解析 __regex__ 配置的JSON 数据\n错误: ${errorMsg}`,
    );
  }

  if (!Array.isArray(regexArray)) {
    showError(
      "正则模式格式错误",
      `__regex__ 配置必须是数组格式，当前类型: ${typeof regexArray}`,
    );
  }

  state.regexPatterns = regexArray
    .map((entry): RegexPatternEntry | null => {
      // 验证元素数量
      if (entry.length < 2 || entry.length > 3) {
        return showError(
          "正则模式格式错误",
          `__regex__ 配置的每个模式必须包含2或3个元素，当前元素数量: ${entry.length}`,
        );
      }

      const [patternStr, replacement, flags = ""] = entry;

      // 验证 flags 是否有效
      if (flags && !isValidRegexFlags(flags)) {
        return showError(
          "正则模式标志无效",
          `__regex__ 配置的标志必须是有效的正则表达式标志（g, i, m, s, u, y），当前标志: "${flags}"`,
        );
      }

      try {
        return {
          pattern: new RegExp(patternStr, flags),
          replacement,
        };
      } catch (e) {
        const errorMsg = e instanceof Error ? e.message : String(e);
        return showError(
          "正则模式编译失败",
          `无法编译正则表达式: "${patternStr}"\n标志: "${flags}"\n错误: ${errorMsg}`,
        );
      }
    })
    .filter((x): x is RegexPatternEntry => x !== null);

  log.info({ count: state.regexPatterns.length }, "Regex patterns loaded");
}

// =============================================================================
// 控制符处理
// =============================================================================

/**
 * 从文本中提取控制符信息
 * 改进：每次创建新的正则实例避免状态污染
 */
function extractControls(text: string): ControlExtraction {
  // 如果没有配置控制符模式，直接返回原文
  if (!state.controlPattern) {
    return { plain: text, controls: [] };
  }

  // 每次创建新的正则实例，避免全局状态污染（并发安全）
  const extractRe = new RegExp(state.controlPattern, "gi");
  const replaceRe = new RegExp(state.controlPattern, "gi");

  const controls: ControlInfo[] = [];
  let match: RegExpExecArray | null;
  let loopCount = 0;

  match = extractRe.exec(text);
  while (match !== null && loopCount++ < MAX_LOOP_COUNT) {
    controls.push({
      text: match[0],
      index: match.index,
    });

    // 防止零宽匹配导致无限循环
    if (match.index === extractRe.lastIndex) {
      extractRe.lastIndex++;
    }

    match = extractRe.exec(text);
  }

  const plain = text.replace(replaceRe, "");

  return { plain, controls };
}

/**
 * 将控制符应用到翻译后的文本
 * 改进：合并 test + replace 为单次操作，避免重复匹配
 */
function applyControlPlaceholders(
  translation: string,
  controls: ControlInfo[],
): string {
  if (controls.length === 0) return translation;

  // 单次 replace 操作，同时检测是否有占位符被替换
  let hasReplacement = false;
  const result = translation.replace(PLACEHOLDER_BRACE, (match, indexStr) => {
    const index = Number.parseInt(indexStr, 10);
    if (index >= 0 && index < controls.length) {
      hasReplacement = true;
      return controls[index]?.text ?? match;
    }
    return match;
  });

  // 如果有占位符被替换，返回替换结果；否则使用重建
  return hasReplacement
    ? result
    : reconstructWithControls(translation, controls);
}

/**
 * 重建控制符：将前缀控制符放在开头，其余控制符追加到末尾
 */
function reconstructWithControls(
  translation: string,
  controls: ControlInfo[],
): string {
  if (controls.length === 0) return translation;

  const prefixControls: string[] = [];
  const suffixControls: string[] = [];

  // 识别连续出现在文本开头的控制符作为前缀
  let lastEnd = 0;
  for (const ctrl of controls) {
    if (ctrl.index === lastEnd) {
      prefixControls.push(ctrl.text);
      lastEnd = ctrl.index + ctrl.text.length;
    } else {
      break;
    }
  }

  // 剩余的控制符作为后缀
  for (let i = prefixControls.length; i < controls.length; i++) {
    suffixControls.push(controls[i]?.text ?? "");
  }

  return prefixControls.join("") + translation + suffixControls.join("");
}

// =============================================================================
// 核心翻译功能
// =============================================================================

/**
 * 检测文本是否包含源语言字符
 * 根据配置的 sourceLocale 动态判断
 */
function hasSourceLocaleChars(text: string): boolean {
  if (!state.sourceLocaleCharsRe) {
    return false;
  }
  return state.sourceLocaleCharsRe.test(text);
}

function markTranslated(text: string): void {
  state.translatedMarkers.set(text, true);
}

function translate(text: string): string {
  if (!state.ready || !text || typeof text !== "string") {
    return text;
  }

  const cached = state.cache.get(text);
  if (cached !== undefined) {
    return cached;
  }

  if (state.ignoreRe?.test(text)) {
    state.cache.set(text, text);
    return text;
  }

  const result = translateCore(text);

  // 只记录包含源语言字符的未翻译文本
  if (result === text && hasSourceLocaleChars(text)) {
    const count = state.missed.get(text);
    if (count !== undefined) {
      state.missed.set(text, count + 1);
    } else if (state.missed.size < state.missedLimit) {
      state.missed.set(text, 1);
    }
  }

  state.cache.set(text, result);
  return result;
}

function translateCore(text: string): string {
  // 1. 直接精确匹配 - O(1)
  const directMatch = state.dict.get(text);
  if (directMatch !== undefined) {
    markTranslated(directMatch);
    return directMatch;
  }

  if (state.translatedMarkers.get(text) === true) {
    return text;
  }

  // 2. 提取控制符
  const { plain, controls } = extractControls(text);
  const hasControls = controls.length > 0 && plain.length > 0 && plain !== text;

  // 3. 去除控制符后精确匹配 - O(1)
  if (hasControls) {
    const plainMatch = state.dict.get(plain);
    if (plainMatch !== undefined) {
      const result = applyControlPlaceholders(plainMatch, controls);
      markTranslated(result);
      return result;
    }
  }

  // 4. 正则模式匹配
  const regexResult = applyRegexPatterns(text);
  if (regexResult !== null) {
    markTranslated(regexResult);
    return regexResult;
  }

  // 5. 正则模式匹配 - 去除控制符后
  if (hasControls) {
    const plainRegexResult = applyRegexPatterns(plain);
    if (plainRegexResult !== null) {
      const result = applyControlPlaceholders(plainRegexResult, controls);
      markTranslated(result);
      return result;
    }
  }

  // 6. 部分匹配
  const result = partialMatchTranslate(text);
  if (result !== text) {
    markTranslated(result);
  }
  return result;
}

function applyRegexPatterns(text: string): string | null {
  for (const { pattern, replacement } of state.regexPatterns) {
    pattern.lastIndex = 0;
    const result = text.replace(pattern, replacement);
    if (result !== text) {
      return result;
    }
  }
  return null;
}

function partialMatchTranslate(text: string): string {
  if (!state.acAutomaton) {
    rebuildAhoCorasick();
    if (!state.acAutomaton) return text;
  }

  const rawMatches: ACSearchResult = state.acAutomaton.search(text);

  if (!rawMatches || rawMatches.length === 0) {
    return text;
  }

  const processedMatches: ProcessedMatch[] = [];

  for (const [endIndex, patterns] of rawMatches) {
    const longestPattern = patterns.reduce(
      (longest, p) => (p.length > longest.length ? p : longest),
      "",
    );

    if (longestPattern) {
      const translation = state.dict.get(longestPattern);
      if (translation) {
        const start = endIndex - longestPattern.length + 1;
        processedMatches.push({
          start,
          end: endIndex + 1,
          pattern: longestPattern,
          translation,
        });
      }
    }
  }

  if (processedMatches.length === 0) {
    return text;
  }

  const selectedMatches = selectNonOverlappingMatches(processedMatches);

  if (selectedMatches.length === 0) {
    return text;
  }

  let result = text;
  for (let i = selectedMatches.length - 1; i >= 0; i--) {
    const match = selectedMatches[i];
    if (match) {
      result =
        result.slice(0, match.start) +
        match.translation +
        result.slice(match.end);
    }
  }

  return result;
}

function selectNonOverlappingMatches(
  matches: ProcessedMatch[],
): ProcessedMatch[] {
  if (matches.length === 0) return [];
  if (matches.length === 1) return matches;

  const sorted = [...matches].sort((a, b) => {
    const lenDiff = b.end - b.start - (a.end - a.start);
    return lenDiff === 0 ? a.start - b.start : lenDiff;
  });

  const selected: ProcessedMatch[] = [];

  for (const match of sorted) {
    if (!hasOverlapBinarySearch(selected, match)) {
      insertSorted(selected, match);
    }
  }

  return selected;
}

function hasOverlapBinarySearch(
  selected: ProcessedMatch[],
  newMatch: ProcessedMatch,
): boolean {
  if (selected.length === 0) return false;

  const { start, end } = newMatch;

  let left = 0;
  let right = selected.length;

  while (left < right) {
    const mid = (left + right) >>> 1;
    if (selected[mid]!.end <= start) {
      left = mid + 1;
    } else {
      right = mid;
    }
  }

  if (left < selected.length && selected[left]!.start < end) {
    return true;
  }

  return false;
}

function insertSorted(selected: ProcessedMatch[], match: ProcessedMatch): void {
  let left = 0;
  let right = selected.length;

  while (left < right) {
    const mid = (left + right) >>> 1;
    if (selected[mid]!.start < match.start) {
      left = mid + 1;
    } else {
      right = mid;
    }
  }

  selected.splice(left, 0, match);
}

// =============================================================================
// 数据翻译
// =============================================================================

function translateData(data: unknown): void {
  if (!state.ready || !data) return;

  try {
    if (Array.isArray(data)) {
      for (const item of data) {
        if (item && typeof item === "object") {
          translateObject(item as Record<string, unknown>, 0);
        }
      }
    } else if (typeof data === "object") {
      translateObject(data as Record<string, unknown>, 0);
    }
  } catch (e) {
    const errorMsg = e instanceof Error ? e.message : String(e);
    showError("数据翻译错误", `翻译游戏数据时发生错误\n错误: ${errorMsg}`);
  }
}

function translateObject(obj: Record<string, unknown>, depth: number): void {
  if (depth > state.maxDepth || !obj || typeof obj !== "object") return;

  if (translatedObjects.has(obj)) return;
  translatedObjects.add(obj);

  const cfg = state.config;
  if (!cfg) return;

  const textFields = cfg.__textFields__ ?? [];

  for (const field of textFields) {
    const val = obj[field];
    if (typeof val === "string") {
      obj[field] = translate(val);
    } else if (val && typeof val === "object" && !Array.isArray(val)) {
      translateNested(val as Record<string, unknown>, depth + 1);
    }
  }

  const list = obj.list;
  if (Array.isArray(list)) {
    translateCommands(list);
  }

  const pages = obj.pages;
  if (Array.isArray(pages)) {
    for (const page of pages) {
      if (page && typeof page === "object") {
        const pageObj = page as Record<string, unknown>;
        if (Array.isArray(pageObj.list)) {
          translateCommands(pageObj.list as unknown[]);
        }
      }
    }
  }
}

function translateNested(obj: Record<string, unknown>, depth: number): void {
  if (depth > state.maxDepth || !obj || typeof obj !== "object") return;

  if (translatedObjects.has(obj)) return;
  translatedObjects.add(obj);

  for (const key of Object.keys(obj)) {
    const val = obj[key];
    if (typeof val === "string") {
      obj[key] = translate(val);
    } else if (val && typeof val === "object" && !Array.isArray(val)) {
      translateNested(val as Record<string, unknown>, depth + 1);
    }
  }
}

function translateCommands(list: unknown[]): void {
  if (!Array.isArray(list) || !state.config) return;

  const cmds = state.config.__textCommands__ ?? {};

  for (const cmd of list) {
    if (!cmd || typeof cmd !== "object") continue;

    const cmdObj = cmd as Record<string, unknown>;
    const code = cmdObj.code;
    const params = cmdObj.parameters;

    if (typeof code !== "number" || !Array.isArray(params)) continue;

    const indices = cmds[String(code)];
    if (indices) {
      for (const idx of indices) {
        if (idx >= params.length) continue;
        const p = params[idx];
        if (typeof p === "string") {
          params[idx] = translate(p);
        } else if (Array.isArray(p)) {
          for (let j = 0; j < p.length; j++) {
            if (typeof p[j] === "string") {
              p[j] = translate(p[j] as string);
            }
          }
        }
      }
    }
  }
}

// =============================================================================
// 钩子函数
// =============================================================================

/**
 * 创建用于 custom-hooks 的日志函数
 */
function createHookLogger(): (...args: unknown[]) => void {
  return (...args: unknown[]) => {
    const message = args
      .map((arg) => {
        if (typeof arg === "string") return arg;
        if (arg instanceof Error) return arg.message;
        try {
          return JSON.stringify(arg);
        } catch {
          return String(arg);
        }
      })
      .join(" ");

    log.info(message);
  };
}

function applyHooks(): void {
  if (state.hooked) return;

  const customHooks = state.config?.__customHooks__ ?? [];

  const { stats } = applyCustomHooks({
    translate,
    translateData,
    hooks: customHooks,
    logger: createHookLogger(),
  });

  state.hooked = true;
  log.info({ stats }, "Custom hooks applied");
}

// =============================================================================
// 辅助函数
// =============================================================================

function clearTranslationState(): void {
  state.cache.clear();
  state.translatedMarkers.clear();
  translatedObjects = new WeakSet<object>();
}

// =============================================================================
// 公共 API
// =============================================================================

export const Translator = {
  translate,
  init,
  hook: applyHooks,

  add(key: string, value: string): void {
    state.dict.set(key, value);
    const isMetaKey = key.startsWith("__") && key.endsWith("__");
    if (!isMetaKey) {
      rebuildSortedKeys();
    }
    clearTranslationState();
    invalidateAhoCorasick();
  },

  addBatch(entries: Array<[string, string]>): void {
    for (const [key, value] of entries) {
      state.dict.set(key, value);
    }
    rebuildSortedKeys();
    clearTranslationState();
    invalidateAhoCorasick();
  },

  remove(key: string): void {
    state.dict.delete(key);
    rebuildSortedKeys();
    clearTranslationState();
    invalidateAhoCorasick();
  },

  has(key: string): boolean {
    return state.dict.has(key);
  },

  reload(): void {
    const shouldRehook = state.hooked;
    state.dict.clear();
    state.missed.clear();
    state.regexPatterns = [];
    clearTranslationState();
    invalidateAhoCorasick();
    state.ready = false;
    state.hooked = false;
    init();
    if (shouldRehook) {
      applyHooks();
    }
  },

  getMissed(): string[] {
    return Array.from(state.missed.keys());
  },

  exportMissed(includeCount = false): string {
    const entries = Array.from(state.missed.entries()).sort(
      (a, b) => b[1] - a[1],
    );
    const obj: Record<string, string | number> = {};
    for (const [key, count] of entries) {
      obj[key] = includeCount ? count : "";
    }
    return JSON.stringify(obj, null, 2);
  },

  clearMissed(): void {
    state.missed.clear();
  },

  stats(): Record<string, unknown> {
    return {
      translations: state.dict.size,
      cached: state.cache.size,
      missed: state.missed.size,
      missedLimit: state.missedLimit,
      regexPatterns: state.regexPatterns.length,
      acBuilt: state.acAutomaton !== null,
      acPatterns: state.acPatterns.length,
      maxDepth: state.maxDepth,
      ready: state.ready,
      hooked: state.hooked,
      translatedMarkers: state.translatedMarkers.size,
    };
  },

  isReady(): boolean {
    return state.ready;
  },

  /**
   * 设置日志级别
   */
  setLogLevel(
    level: "trace" | "debug" | "info" | "warn" | "error" | "fatal" | "silent",
  ): void {
    log.level = level;
  },

  debug: {
    getConfig(): Config | null {
      return state.config;
    },

    getRegexPatterns(): RegexPatternEntry[] {
      return [...state.regexPatterns];
    },

    testTranslate(text: string): {
      result: string;
      cached: boolean;
      matched: boolean;
      controls: ControlInfo[];
    } {
      const cached = state.cache.get(text);
      const { controls } = extractControls(text);
      const result = translate(text);
      return {
        result,
        cached: cached !== undefined,
        matched: result !== text,
        controls,
      };
    },

    extractControls(text: string): ControlExtraction {
      return extractControls(text);
    },

    applyPlaceholders(translation: string, controls: ControlInfo[]): string {
      return applyControlPlaceholders(translation, controls);
    },

    getDict(): Map<string, string> {
      return new Map(state.dict);
    },

    getLogLevel(): string {
      return log.level;
    },
  },
};

// eslint-disable-next-line @typescript-eslint/no-implied-eval
const _global: Record<string, unknown> = (typeof window !== "undefined" ? window : typeof self !== "undefined" ? self : Function("return this")()) as Record<string, unknown>;
_global["Translator"] = Translator;
