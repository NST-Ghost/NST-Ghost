/**
 * 自定义 Hook 系统
 * 提供运行时方法拦截和翻译功能
 * 支持嵌套路径访问
 */

import { showError } from "../utils/error.js";
import type { CustomHookConfig, CustomHookType } from "../utils/json-loader.js";

declare const window: Window & Record<string, unknown>;

// ============== 类型定义 ==============

export type TranslateFunction = (text: string) => string;
export type TranslateDataFunction = (data: unknown) => void;
export type HookType = CustomHookType;
export type LoggerFunction = (...args: unknown[]) => void;

const HOOK_ORIGINAL_KEY = "__translatorHookOriginal__" as const;
const HOOK_FULL_NAME_KEY = "__translatorHookFullName__" as const;

type HookMethod = ((...args: unknown[]) => unknown) & {
  [HOOK_ORIGINAL_KEY]?: (...args: unknown[]) => unknown;
  [HOOK_FULL_NAME_KEY]?: string;
};

export interface HookStats {
  total: number;
  installed: number;
  failed: number;
}

export interface ApplyHooksOptions {
  translate: TranslateFunction;
  translateData: TranslateDataFunction;
  hooks: CustomHookConfig[];
  logger?: LoggerFunction;
  verbose?: boolean;
}

export interface ApplyHooksResult {
  manager: CustomHookManager;
  stats: HookStats;
}

// ============== 常量 ==============

const MESSAGES = {
  classNotExist: "类不存在",
  methodNotExist: "方法不存在",
  hookInstallSuccess: "Hook安装成功:",
  hookInstallError: "Hook安装错误:",
  hookExecError: "Hook执行错误:",
} as const;

function createSilentLogger(): LoggerFunction {
  return () => {};
}

function getOriginalHookMethod(
  method: (...args: unknown[]) => unknown,
): (...args: unknown[]) => unknown {
  return (method as HookMethod)[HOOK_ORIGINAL_KEY] ?? method;
}

// ============== 内部配置类型 ==============

interface InternalHookConfig {
  readonly paramIdx: number;
  readonly nestedIdx: readonly number[];
  readonly fields: readonly string[];
  readonly fullName: string;
  readonly type: NonNullable<HookType>;
  readonly minParamLen: number;
  readonly isStatic: boolean;
}

/** 处理器执行上下文 */
interface HandlerContext {
  readonly orig: (...args: unknown[]) => unknown;
  readonly args: unknown[];
  readonly cfg: InternalHookConfig;
  readonly ctx: unknown;
  readonly tr: TranslateFunction;
  readonly trData: TranslateDataFunction;
  readonly getNestedValue: (
    value: unknown,
    nestedIdx: readonly number[],
  ) => unknown;
  readonly setNestedValue: (
    value: unknown,
    nestedIdx: readonly number[],
    newValue: unknown,
  ) => void;
}

/** 处理器函数类型 */
type HookHandler = (hctx: HandlerContext) => unknown;

// ============== 兼容性检查结果 ==============

interface CompatibilityResult {
  compatible: boolean;
  reason: string;
  target: Record<string, unknown> | null;
  method: ((...args: unknown[]) => unknown) | null;
  isStatic: boolean;
}

// ============== Hook 管理器 ==============

export class CustomHookManager {
  private readonly _tr: TranslateFunction;
  private readonly _trData: TranslateDataFunction;
  private readonly _log: LoggerFunction;

  private readonly _stats: HookStats = {
    total: 0,
    installed: 0,
    failed: 0,
  };

  constructor(
    tr: TranslateFunction,
    trData: TranslateDataFunction,
    logger?: LoggerFunction,
  ) {
    this._tr = tr;
    this._trData = trData;
    this._log = logger ?? createSilentLogger();
  }

  get stats() {
    return { ...this._stats };
  }

  /**
   * 通过点号分隔的路径获取嵌套对象/函数（支持 class/function）
   * @param path 对象路径，如 "SceneManager" 或 "Window_Base"
   */
  private _getObjectByPath(path: string): Record<string, unknown> | undefined {
    if (!path || typeof path !== "string") return undefined;

    const parts = path.split(".").filter(Boolean);

    // 比 window 更通用：支持 module/不同运行环境下的全局对象
    const root: unknown =
      typeof globalThis !== "undefined" ? globalThis : (window as unknown);

    let current: unknown = root;

    for (const part of parts) {
      if (current === null || current === undefined) return undefined;

      const t = typeof current;
      // 允许 function（class/constructor 在 JS 里就是 function）
      if (t !== "object" && t !== "function") return undefined;

      current = (current as Record<string, unknown>)[part];
    }

    if (current === null || current === undefined) return undefined;

    const t = typeof current;
    return t === "object" || t === "function"
      ? (current as unknown as Record<string, unknown>)
      : undefined;
  }

  /**
   * 检测 Hook 兼容性（支持静态方法和实例方法）
   * 支持嵌套路径访问
   */
  private _checkCompatibility(h: CustomHookConfig): CompatibilityResult {
    // 使用路径解析获取目标对象
    const cls = this._getObjectByPath(h.class);

    if (!cls) {
      return {
        compatible: false,
        reason: `${MESSAGES.classNotExist}: ${h.class}`,
        target: null,
        method: null,
        isStatic: false,
      };
    }

    // 优先检查配置中指定的类型
    if (h.static === true) {
      // 明确指定为静态方法（直接属性）
      const method = cls[h.method];
      if (typeof method !== "function") {
        return {
          compatible: false,
          reason: `静态 ${MESSAGES.methodNotExist}: ${h.method}`,
          target: null,
          method: null,
          isStatic: true,
        };
      }
      return {
        compatible: true,
        reason: "",
        target: cls,
        method: method as (...args: unknown[]) => unknown,
        isStatic: true,
      };
    }

    if (h.static === false) {
      // 明确指定为实例方法（prototype 上的方法）
      const prototype = cls["prototype"] as Record<string, unknown> | undefined;
      if (!prototype) {
        return {
          compatible: false,
          reason: `${MESSAGES.classNotExist}，无 prototype`,
          target: null,
          method: null,
          isStatic: false,
        };
      }
      const method = prototype[h.method];
      if (typeof method !== "function") {
        return {
          compatible: false,
          reason: `实例 ${MESSAGES.methodNotExist}: ${h.method}`,
          target: null,
          method: null,
          isStatic: false,
        };
      }
      return {
        compatible: true,
        reason: "",
        target: prototype,
        method: method as (...args: unknown[]) => unknown,
        isStatic: false,
      };
    }

    // 未指定时自动检测：先检查直接方法（静态），再检查 prototype（实例）
    const staticMethod = cls[h.method];
    if (typeof staticMethod === "function") {
      return {
        compatible: true,
        reason: "",
        target: cls,
        method: staticMethod as (...args: unknown[]) => unknown,
        isStatic: true,
      };
    }

    const prototype = cls["prototype"] as Record<string, unknown> | undefined;
    if (prototype) {
      const instanceMethod = prototype[h.method];
      if (typeof instanceMethod === "function") {
        return {
          compatible: true,
          reason: "",
          target: prototype,
          method: instanceMethod as (...args: unknown[]) => unknown,
          isStatic: false,
        };
      }
    }

    return {
      compatible: false,
      reason: `${MESSAGES.methodNotExist}: ${h.method}`,
      target: null,
      method: null,
      isStatic: false,
    };
  }

  /** 创建内部 Hook 配置 */
  private _createConfig(
    h: CustomHookConfig,
    isStatic: boolean,
  ): InternalHookConfig {
    return Object.freeze({
      paramIdx: h.paramIndex ?? 0,
      nestedIdx: Object.freeze(h.nestedIndex ? [...h.nestedIndex] : []),
      fields: Object.freeze(h.fields ? [...h.fields] : []),
      fullName: `${h.class}.${h.method}`,
      type: h.type || "tr0",
      minParamLen: h.minParamLength ?? 0,
      isStatic,
    });
  }

  /** 验证数组索引是否有效 */
  private _isValidIndex(arr: unknown, idx: unknown): arr is unknown[] {
    return (
      Array.isArray(arr) &&
      typeof idx === "number" &&
      Number.isInteger(idx) &&
      idx >= 0 &&
      idx < arr.length
    );
  }

  /** 获取嵌套值 */
  private _getNestedValue(
    root: unknown,
    nestedIdx: readonly number[],
  ): unknown {
    let value: unknown = root;
    for (const idx of nestedIdx) {
      if (!this._isValidIndex(value, idx)) return undefined;
      value = value[idx];
    }
    return value;
  }

  /** 设置嵌套值 */
  private _setNestedValue(
    root: unknown,
    nestedIdx: readonly number[],
    newValue: unknown,
  ): void {
    if (nestedIdx.length === 0) return;

    let target: unknown = root;
    for (let i = 0; i < nestedIdx.length - 1; i++) {
      const idx = nestedIdx[i];
      if (!this._isValidIndex(target, idx)) return;
      target = target[idx as number];
    }

    const lastIdx = nestedIdx[nestedIdx.length - 1];
    if (this._isValidIndex(target, lastIdx)) {
      target[lastIdx as number] = newValue;
    }
  }

  // ============== 处理器方法 ==============

  /** tr0: 翻译指定索引的字符串参数 */
  private _handleTr0(hctx: HandlerContext): unknown {
    const { orig, args, cfg, ctx, tr } = hctx;
    const idx = cfg.paramIdx;
    if (typeof args[idx] === "string") {
      args[idx] = tr(args[idx]);
    }
    return orig.apply(ctx, args);
  }

  /** trRet: 翻译返回值 */
  private _handleTrRet(hctx: HandlerContext): unknown {
    const { orig, args, ctx, tr } = hctx;
    const ret = orig.apply(ctx, args);
    return typeof ret === "string" ? tr(ret) : ret;
  }

  /** trArray: 翻译数组参数中的所有字符串 */
  private _handleTrArray(hctx: HandlerContext): unknown {
    const { orig, args, cfg, ctx, tr } = hctx;
    const arr = args[cfg.paramIdx];
    if (Array.isArray(arr)) {
      for (let i = 0; i < arr.length; i++) {
        if (typeof arr[i] === "string") {
          arr[i] = tr(arr[i] as string);
        }
      }
    }
    return orig.apply(ctx, args);
  }

  /** trNestedArray: 翻译嵌套数组中的所有字符串 */
  private _handleTrNestedArray(hctx: HandlerContext): unknown {
    const { orig, args, cfg, ctx, tr, getNestedValue } = hctx;
    const root = args[cfg.paramIdx];
    if (
      cfg.minParamLen > 0 &&
      (!Array.isArray(root) || root.length < cfg.minParamLen)
    ) {
      return orig.apply(ctx, args);
    }
    const nestedArr = getNestedValue(root, cfg.nestedIdx);
    if (Array.isArray(nestedArr)) {
      for (let i = 0; i < nestedArr.length; i++) {
        if (typeof nestedArr[i] === "string") {
          nestedArr[i] = tr(nestedArr[i] as string);
        }
      }
    }
    return orig.apply(ctx, args);
  }

  /** trObject: 翻译对象参数的指定字段 */
  private _handleTrObject(hctx: HandlerContext): unknown {
    const { orig, args, cfg, ctx, tr } = hctx;
    const obj = args[cfg.paramIdx] as Record<string, unknown> | null;
    if (obj) {
      for (const f of cfg.fields) {
        if (typeof obj[f] === "string") {
          obj[f] = tr(obj[f]);
        }
      }
    }
    return orig.apply(ctx, args);
  }

  /** trObjectAfter: 调用原方法后翻译对象参数的指定字段 */
  private _handleTrObjectAfter(hctx: HandlerContext): unknown {
    const { orig, args, cfg, ctx, tr } = hctx;
    const result = orig.apply(ctx, args);
    const obj = args[cfg.paramIdx] as Record<string, unknown> | null;
    if (obj) {
      for (const f of cfg.fields) {
        if (typeof obj[f] === "string") {
          obj[f] = tr(obj[f]);
        }
      }
    }
    return result;
  }

  /** trRetObject: 翻译返回对象的指定字段 */
  private _handleTrRetObject(hctx: HandlerContext): unknown {
    const { orig, args, cfg, ctx, tr } = hctx;
    const result = orig.apply(ctx, args);
    if (result && typeof result === "object") {
      const obj = result as Record<string, unknown>;
      for (const f of cfg.fields) {
        if (typeof obj[f] === "string") {
          obj[f] = tr(obj[f]);
        }
      }
    }
    return result;
  }

  /** trThis: 翻译 this 上下文的指定字段 */
  private _handleTrThis(hctx: HandlerContext): unknown {
    const { orig, args, cfg, ctx, tr } = hctx;
    const context = ctx as Record<string, unknown>;
    for (const f of cfg.fields) {
      if (typeof context[f] === "string") {
        context[f] = tr(context[f]);
      }
    }
    return orig.apply(ctx, args);
  }

  /** trThisAfter: 调用原方法后翻译 this 上下文的指定字段 */
  private _handleTrThisAfter(hctx: HandlerContext): unknown {
    const { orig, args, cfg, ctx, tr } = hctx;
    const result = orig.apply(ctx, args);
    const context = ctx as Record<string, unknown>;
    for (const f of cfg.fields) {
      if (typeof context[f] === "string") {
        context[f] = tr(context[f]);
      }
    }
    return result;
  }

  /** trCommandList: 翻译命令列表中的 name 字段（RPG Maker 专用） */
  private _handleTrCommandList(hctx: HandlerContext): unknown {
    const { orig, args, ctx, tr } = hctx;
    const result = orig.apply(ctx, args);
    const context = ctx as Record<string, unknown>;
    const list = context["_list"];
    if (Array.isArray(list)) {
      for (const item of list as Record<string, unknown>[]) {
        if (item && typeof item["name"] === "string") {
          item["name"] = tr(item["name"]);
        }
      }
    }
    return result;
  }

  /** trData: 调用原方法后使用 translateData 翻译数据对象 */
  private _handleTrData(hctx: HandlerContext): unknown {
    const { orig, args, cfg, ctx, trData } = hctx;
    const result = orig.apply(ctx, args);
    const dataObj = args[cfg.paramIdx];
    if (dataObj !== null && dataObj !== undefined) {
      trData(dataObj);
    }
    return result;
  }

  /** trNested: 翻译嵌套路径指向的字符串值 */
  private _handleTrNested(hctx: HandlerContext): unknown {
    const { orig, args, cfg, ctx, tr, getNestedValue, setNestedValue } = hctx;
    const root = args[cfg.paramIdx];
    if (
      cfg.minParamLen > 0 &&
      (!Array.isArray(root) || root.length < cfg.minParamLen)
    ) {
      return orig.apply(ctx, args);
    }
    const nestedValue = getNestedValue(root, cfg.nestedIdx);
    if (typeof nestedValue === "string") {
      if (cfg.nestedIdx.length === 0) {
        args[cfg.paramIdx] = tr(nestedValue);
      } else {
        setNestedValue(root, cfg.nestedIdx, tr(nestedValue));
      }
    }
    return orig.apply(ctx, args);
  }

  /** 默认处理器: 直接调用原方法 */
  private _handleDefault(hctx: HandlerContext): unknown {
    const { orig, args, ctx } = hctx;
    return orig.apply(ctx, args);
  }

  /** 处理器映射表 */
  private readonly _handlers: ReadonlyMap<HookType, HookHandler> = new Map<
    HookType,
    HookHandler
  >([
    ["tr0", this._handleTr0.bind(this)],
    ["trRet", this._handleTrRet.bind(this)],
    ["trArray", this._handleTrArray.bind(this)],
    ["trNestedArray", this._handleTrNestedArray.bind(this)],
    ["trObject", this._handleTrObject.bind(this)],
    ["trObjectAfter", this._handleTrObjectAfter.bind(this)],
    ["trRetObject", this._handleTrRetObject.bind(this)],
    ["trThis", this._handleTrThis.bind(this)],
    ["trThisAfter", this._handleTrThisAfter.bind(this)],
    ["trCommandList", this._handleTrCommandList.bind(this)],
    ["trData", this._handleTrData.bind(this)],
    ["trNested", this._handleTrNested.bind(this)],
  ]);

  /** 执行翻译处理 */
  private _executeHandler(
    type: NonNullable<HookType>,
    orig: (...args: unknown[]) => unknown,
    args: unknown[],
    cfg: InternalHookConfig,
    ctx: unknown,
  ): unknown {
    const hctx: HandlerContext = {
      orig,
      args,
      cfg,
      ctx,
      tr: this._tr,
      trData: this._trData,
      getNestedValue: this._getNestedValue.bind(this),
      setNestedValue: this._setNestedValue.bind(this),
    };

    const handler = this._handlers.get(type) ?? this._handleDefault.bind(this);
    return handler(hctx);
  }

  /** 创建包装函数 */
  private _createWrapper(
    origMethod: (...args: unknown[]) => unknown,
    cfg: InternalHookConfig,
  ): HookMethod {
    const executeHandler = this._executeHandler.bind(this);
    const fullName = cfg.fullName;
    const hookType = cfg.type;

    const wrapper: HookMethod = function wrapper(
      this: unknown,
      ...args: unknown[]
    ): unknown {
      try {
        return executeHandler(cfg.type, origMethod, [...args], cfg, this);
      } catch (e) {
        const errorMsg = e instanceof Error ? e.message : String(e);
        // 使用 console.error 而非 showError，Hook 运行时错误不应崩溃游戏
        console.error(
          `[Translator] Hook 执行错误 "${fullName}" [${hookType}]: ${errorMsg}`,
        );
        // 降级：调用原始方法
        try {
          return origMethod.apply(this, args);
        } catch (_) {
          return undefined;
        }
      }
    };

    Object.defineProperty(wrapper, HOOK_ORIGINAL_KEY, {
      value: origMethod,
      configurable: true,
    });
    Object.defineProperty(wrapper, HOOK_FULL_NAME_KEY, {
      value: fullName,
      configurable: true,
    });

    return wrapper;
  }

  /** 批量安装 Hook */
  install(hooks: CustomHookConfig[]): void {
    if (!hooks?.length) {
      this._log("No hooks to install");
      return;
    }

    this._log(`Installing ${hooks.length} hooks...`);

    for (const h of hooks) {
      if (!h.class || !h.method) continue;

      if (h.enabled === false) {
        this._log(`Hook被禁用: ${h.class}.${h.method}`);
        continue;
      }

      this._stats.total++;
      const fullName = `${h.class}.${h.method}`;

      // 1. 兼容性检查
      const compat = this._checkCompatibility(h);
      if (!compat.compatible) {
        this._stats.failed++;
        this._log(
          `${MESSAGES.hookInstallError} ${fullName} (${compat.reason})`,
        );
        continue;
      }

      // 2. 安装 Hook
      try {
        const cfg = this._createConfig(h, compat.isStatic);
        const originalMethod = getOriginalHookMethod(compat.method!);
        const wrapper = this._createWrapper(originalMethod, cfg);

        // 替换方法（静态方法或实例方法）
        compat.target![h.method] = wrapper;

        const methodType = compat.isStatic ? "静态" : "实例";
        this._stats.installed++;
        this._log(
          `${MESSAGES.hookInstallSuccess} ${fullName} [${cfg.type}] (${methodType}方法)`,
        );
      } catch (e) {
        this._stats.failed++;
        const errorMsg = e instanceof Error ? e.message : String(e);
        showError(
          "Hook 安装失败",
          `无法安装 Hook "${fullName}"\n错误: ${errorMsg}`,
        );
      }
    }

    this._log(
      `Hook安装执行完毕: ${this._stats.installed}/${this._stats.total} installed, ${this._stats.failed} failed`,
    );
  }
}

// ============== 公共 API ==============

export function applyHooks(options: ApplyHooksOptions): ApplyHooksResult {
  const { translate, translateData, hooks, logger, verbose = true } = options;

  const effectiveLogger = verbose ? logger : createSilentLogger();

  const manager = new CustomHookManager(
    translate,
    translateData,
    effectiveLogger,
  );

  manager.install(hooks);

  return {
    manager,
    stats: manager.stats,
  };
}

export type { CustomHookConfig } from "../utils/json-loader.js";
