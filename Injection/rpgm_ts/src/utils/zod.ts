// zod.ts - 简化的 Schema 验证实现
// ES5 兼容，遵循 Suckless & KISS 原则

// =============================================================================
// 类型定义
// =============================================================================

export interface ParseResult<T> {
  success: boolean;
  data?: T;
  error?: ZodError;
}

export interface ZodError {
  message: string;
  issues: Array<{ path: string[]; message: string }>;
}

export interface ZodType<T> {
  _type: T;
  parse(data: unknown): T;
  safeParse(data: unknown): ParseResult<T>;
  optional(): ZodOptional<T>;
  default(defaultValue: T): ZodDefault<T>;
}

export interface ZodOptional<T> extends ZodType<T | undefined> {
  _isOptional: true;
}

export interface ZodDefault<T> extends ZodType<T> {
  _defaultValue: T;
}

export interface ZodString extends ZodType<string> {
  min(length: number): ZodString;
}

export interface ZodNumber extends ZodType<number> {}

export interface ZodBoolean extends ZodType<boolean> {}

export interface ZodArray<T> extends ZodType<T[]> {}

export interface ZodCustom<T> extends ZodType<T> {}

export interface ZodEnum<T extends readonly string[]> extends ZodType<T[number]> {
  options: T;
}

// =============================================================================
// 辅助函数
// =============================================================================

function createError(message: string, path?: string[]): ZodError {
  return {
    message: message,
    issues: [{ path: path || [], message: message }],
  };
}

function isObject(value: unknown): value is Record<string, unknown> {
  return typeof value === "object" && value !== null && !Array.isArray(value);
}

// =============================================================================
// Schema 实现
// =============================================================================

function createStringSchema(minLength?: number): ZodString {
  var schema: ZodString = {
    _type: "" as string,
    parse: function (data: unknown): string {
      if (typeof data !== "string") {
        throw new TypeError("Expected string, got " + typeof data);
      }
      if (minLength !== undefined && data.length < minLength) {
        throw new Error("String must be at least " + minLength + " characters");
      }
      return data;
    },
    safeParse: function (data: unknown): ParseResult<string> {
      try {
        return { success: true, data: this.parse(data) };
      } catch (e) {
        var msg = e instanceof Error ? e.message : String(e);
        return { success: false, error: createError(msg) };
      }
    },
    optional: function (): ZodOptional<string> {
      return createOptionalSchema(this);
    },
    default: function (defaultValue: string): ZodDefault<string> {
      return createDefaultSchema(this, defaultValue);
    },
    min: function (length: number): ZodString {
      return createStringSchema(length);
    },
  };
  return schema;
}

function createNumberSchema(): ZodNumber {
  var schema: ZodNumber = {
    _type: 0 as number,
    parse: function (data: unknown): number {
      if (typeof data !== "number") {
        throw new Error("Expected number, got " + typeof data);
      }
      return data;
    },
    safeParse: function (data: unknown): ParseResult<number> {
      try {
        return { success: true, data: this.parse(data) };
      } catch (e) {
        var msg = e instanceof Error ? e.message : String(e);
        return { success: false, error: createError(msg) };
      }
    },
    optional: function (): ZodOptional<number> {
      return createOptionalSchema(this);
    },
    default: function (defaultValue: number): ZodDefault<number> {
      return createDefaultSchema(this, defaultValue);
    },
  };
  return schema;
}

function createBooleanSchema(): ZodBoolean {
  var schema: ZodBoolean = {
    _type: false as boolean,
    parse: function (data: unknown): boolean {
      if (typeof data !== "boolean") {
        throw new Error("Expected boolean, got " + typeof data);
      }
      return data;
    },
    safeParse: function (data: unknown): ParseResult<boolean> {
      try {
        return { success: true, data: this.parse(data) };
      } catch (e) {
        var msg = e instanceof Error ? e.message : String(e);
        return { success: false, error: createError(msg) };
      }
    },
    optional: function (): ZodOptional<boolean> {
      return createOptionalSchema(this);
    },
    default: function (defaultValue: boolean): ZodDefault<boolean> {
      return createDefaultSchema(this, defaultValue);
    },
  };
  return schema;
}

function createArraySchema<T>(itemSchema: ZodType<T>): ZodArray<T> {
  var schema: ZodArray<T> = {
    _type: [] as T[],
    parse: function (data: unknown): T[] {
      if (!Array.isArray(data)) {
        throw new Error("Expected array, got " + typeof data);
      }
      var result: T[] = [];
      for (var i = 0; i < data.length; i++) {
        result.push(itemSchema.parse(data[i]));
      }
      return result;
    },
    safeParse: function (data: unknown): ParseResult<T[]> {
      try {
        return { success: true, data: this.parse(data) };
      } catch (e) {
        var msg = e instanceof Error ? e.message : String(e);
        return { success: false, error: createError(msg) };
      }
    },
    optional: function (): ZodOptional<T[]> {
      return createOptionalSchema(this);
    },
    default: function (defaultValue: T[]): ZodDefault<T[]> {
      return createDefaultSchema(this, defaultValue);
    },
  };
  return schema;
}

function createCustomSchema<T>(parser: (data: unknown) => T): ZodCustom<T> {
  var schema: ZodCustom<T> = {
    _type: undefined as T,
    parse: function (data: unknown): T {
      return parser(data);
    },
    safeParse: function (data: unknown): ParseResult<T> {
      try {
        return { success: true, data: this.parse(data) };
      } catch (e) {
        var msg = e instanceof Error ? e.message : String(e);
        return { success: false, error: createError(msg) };
      }
    },
    optional: function (): ZodOptional<T> {
      return createOptionalSchema(this);
    },
    default: function (defaultValue: T): ZodDefault<T> {
      return createDefaultSchema(this, defaultValue);
    },
  };
  return schema;
}

function createOptionalSchema<T>(innerSchema: ZodType<T>): ZodOptional<T> {
  var schema = {
    _type: undefined as T | undefined,
    _isOptional: true as const,
    parse: function (data: unknown): T | undefined {
      if (data === undefined || data === null) {
        return undefined;
      }
      return innerSchema.parse(data);
    },
    safeParse: function (data: unknown): ParseResult<T | undefined> {
      try {
        return { success: true, data: this.parse(data) };
      } catch (e) {
        var msg = e instanceof Error ? e.message : String(e);
        return { success: false, error: createError(msg) };
      }
    },
    optional: function (): ZodOptional<T | undefined> {
      return this as unknown as ZodOptional<T | undefined>;
    },
    default: function (defaultValue: T | undefined): ZodDefault<T | undefined> {
      return createDefaultSchema(this, defaultValue);
    },
  };
  return schema;
}

function createDefaultSchema<T>(
  innerSchema: ZodType<T>,
  defaultValue: T
): ZodDefault<T> {
  var schema = {
    _type: defaultValue,
    _defaultValue: defaultValue,
    parse: function (data: unknown): T {
      if (data === undefined || data === null) {
        return defaultValue;
      }
      return innerSchema.parse(data);
    },
    safeParse: function (data: unknown): ParseResult<T> {
      try {
        return { success: true, data: this.parse(data) };
      } catch (e) {
        var msg = e instanceof Error ? e.message : String(e);
        return { success: false, error: createError(msg) };
      }
    },
    optional: function (): ZodOptional<T> {
      return createOptionalSchema(this);
    },
    default: function (newDefault: T): ZodDefault<T> {
      return createDefaultSchema(innerSchema, newDefault);
    },
  };
  return schema;
}

// =============================================================================
// Object Schema
// =============================================================================

type ZodRawShape = Record<string, ZodType<unknown>>;

type InferShape<T extends ZodRawShape> = {
  [K in keyof T]: T[K]["_type"];
};

export interface ZodObject<T extends ZodRawShape> extends ZodType<InferShape<T>> {
  shape: T;
}

function createObjectSchema<T extends ZodRawShape>(shape: T): ZodObject<T> {
  var schema: ZodObject<T> = {
    _type: {} as InferShape<T>,
    shape: shape,
    parse: function (data: unknown): InferShape<T> {
      if (!isObject(data)) {
        throw new Error("Expected object, got " + typeof data);
      }
      var result = {} as Record<string, unknown>;
      var keys = Object.keys(shape);
      for (var i = 0; i < keys.length; i++) {
        var key = keys[i];
        if (key === undefined) continue;
        var fieldSchema = shape[key];
        if (fieldSchema === undefined) continue;
        var value = data[key];
        result[key] = fieldSchema.parse(value);
      }
      return result as InferShape<T>;
    },
    safeParse: function (data: unknown): ParseResult<InferShape<T>> {
      try {
        return { success: true, data: this.parse(data) };
      } catch (e) {
        var msg = e instanceof Error ? e.message : String(e);
        return { success: false, error: createError(msg) };
      }
    },
    optional: function (): ZodOptional<InferShape<T>> {
      return createOptionalSchema(this);
    },
    default: function (defaultValue: InferShape<T>): ZodDefault<InferShape<T>> {
      return createDefaultSchema(this, defaultValue);
    },
  };
  return schema;
}

// =============================================================================
// Record Schema
// =============================================================================

export interface ZodRecord<K extends string, V> extends ZodType<Record<K, V>> {}

function createRecordSchema<V>(
  _keySchema: ZodType<string>,
  valueSchema: ZodType<V>
): ZodRecord<string, V> {
  var schema: ZodRecord<string, V> = {
    _type: {} as Record<string, V>,
    parse: function (data: unknown): Record<string, V> {
      if (!isObject(data)) {
        throw new Error("Expected object, got " + typeof data);
      }
      var result = {} as Record<string, V>;
      var keys = Object.keys(data);
      for (var i = 0; i < keys.length; i++) {
        var key = keys[i];
        if (key === undefined) continue;
        result[key] = valueSchema.parse(data[key]);
      }
      return result;
    },
    safeParse: function (data: unknown): ParseResult<Record<string, V>> {
      try {
        return { success: true, data: this.parse(data) };
      } catch (e) {
        var msg = e instanceof Error ? e.message : String(e);
        return { success: false, error: createError(msg) };
      }
    },
    optional: function (): ZodOptional<Record<string, V>> {
      return createOptionalSchema(this);
    },
    default: function (defaultValue: Record<string, V>): ZodDefault<Record<string, V>> {
      return createDefaultSchema(this, defaultValue);
    },
  };
  return schema;
}

// =============================================================================
// Union Schema
// =============================================================================

export interface ZodUnion<T extends ZodType<unknown>[]>
  extends ZodType<T[number]["_type"]> {}

function createUnionSchema<T extends ZodType<unknown>[]>(
  schemas: T
): ZodUnion<T> {
  var schema: ZodUnion<T> = {
    _type: undefined as T[number]["_type"],
    parse: function (data: unknown): T[number]["_type"] {
      var errors: string[] = [];
      for (var i = 0; i < schemas.length; i++) {
        var s = schemas[i];
        if (s === undefined) continue;
        var result = s.safeParse(data);
        if (result.success) {
          return result.data;
        }
        errors.push(result.error?.message || "Unknown error");
      }
      throw new Error("No matching schema: " + errors.join(", "));
    },
    safeParse: function (data: unknown): ParseResult<T[number]["_type"]> {
      try {
        return { success: true, data: this.parse(data) };
      } catch (e) {
        var msg = e instanceof Error ? e.message : String(e);
        return { success: false, error: createError(msg) };
      }
    },
    optional: function (): ZodOptional<T[number]["_type"]> {
      return createOptionalSchema(this);
    },
    default: function (
      defaultValue: T[number]["_type"]
    ): ZodDefault<T[number]["_type"]> {
      return createDefaultSchema(this, defaultValue);
    },
  };
  return schema;
}

// =============================================================================
// Tuple Schema
// =============================================================================

type InferTuple<T extends ZodType<unknown>[]> = {
  [K in keyof T]: T[K] extends ZodType<infer U> ? U : never;
};

export interface ZodTuple<T extends ZodType<unknown>[]>
  extends ZodType<InferTuple<T>> {}

function createTupleSchema<T extends ZodType<unknown>[]>(
  schemas: T
): ZodTuple<T> {
  var schema: ZodTuple<T> = {
    _type: [] as unknown as InferTuple<T>,
    parse: function (data: unknown): InferTuple<T> {
      if (!Array.isArray(data)) {
        throw new Error("Expected array, got " + typeof data);
      }
      if (data.length !== schemas.length) {
        throw new Error(
          "Expected exactly " + schemas.length + " elements, got " + data.length
        );
      }
      var result: unknown[] = [];
      for (var i = 0; i < schemas.length; i++) {
        var s = schemas[i];
        if (s === undefined) continue;
        result.push(s.parse(data[i]));
      }
      return result as InferTuple<T>;
    },
    safeParse: function (data: unknown): ParseResult<InferTuple<T>> {
      try {
        return { success: true, data: this.parse(data) };
      } catch (e) {
        var msg = e instanceof Error ? e.message : String(e);
        return { success: false, error: createError(msg) };
      }
    },
    optional: function (): ZodOptional<InferTuple<T>> {
      return createOptionalSchema(this);
    },
    default: function (defaultValue: InferTuple<T>): ZodDefault<InferTuple<T>> {
      return createDefaultSchema(this, defaultValue);
    },
  };
  return schema;
}

// =============================================================================
// Enum Schema
// =============================================================================

function createEnumSchema<T extends readonly string[]>(
  values: T
): ZodEnum<T> {
  var schema: ZodEnum<T> = {
    _type: values[0] as T[number],
    options: values,
    parse: function (data: unknown): T[number] {
      if (typeof data !== "string") {
        throw new Error("Expected string, got " + typeof data);
      }
      for (var i = 0; i < values.length; i++) {
        if (values[i] === data) {
          return data as T[number];
        }
      }
      throw new Error(
        "Invalid enum value: " + data + ". Expected one of: " + values.join(", ")
      );
    },
    safeParse: function (data: unknown): ParseResult<T[number]> {
      try {
        return { success: true, data: this.parse(data) };
      } catch (e) {
        var msg = e instanceof Error ? e.message : String(e);
        return { success: false, error: createError(msg) };
      }
    },
    optional: function (): ZodOptional<T[number]> {
      return createOptionalSchema(this);
    },
    default: function (defaultValue: T[number]): ZodDefault<T[number]> {
      return createDefaultSchema(this, defaultValue);
    },
  };
  return schema;
}

// =============================================================================
// 公共 API - 模拟 zod 的 z 命名空间
// =============================================================================

export var z = {
  string: function(): ZodString {
    return createStringSchema();
  },
  number: function (): ZodNumber {
    return createNumberSchema();
  },
  boolean: function (): ZodBoolean {
    return createBooleanSchema();
  },
  array: function <T>(itemSchema: ZodType<T>): ZodArray<T> {
    return createArraySchema(itemSchema);
  },
  custom: function <T>(parser: (data: unknown) => T): ZodCustom<T> {
    return createCustomSchema(parser);
  },
  object: function <T extends ZodRawShape>(shape: T): ZodObject<T> {
    return createObjectSchema(shape);
  },
  record: function <V>(
    keySchema: ZodType<string>,
    valueSchema: ZodType<V>
  ): ZodRecord<string, V> {
    return createRecordSchema(keySchema, valueSchema);
  },
  union: function <T extends ZodType<unknown>[]>(schemas: T): ZodUnion<T> {
    return createUnionSchema(schemas);
  },
  tuple: function <T extends ZodType<unknown>[]>(schemas: T): ZodTuple<T> {
    return createTupleSchema(schemas);
  },
  enum: function <T extends readonly string[]>(values: T): ZodEnum<T> {
    return createEnumSchema(values);
  },
};

// 类型推断辅助
export type infer<T extends ZodType<unknown>> = T["_type"];
