// neverthrow.ts — Simplified Result type implementation.
// ES5-compatible, following Suckless & KISS principles.
// Ported from translator_scratch.

/**
 * Result type — represents the outcome of an operation that may succeed or fail.
 * Replaces try-catch with type-safe error handling.
 */
export type Result<T, E> = OkResult<T, E> | ErrResult<T, E>;

export interface OkResult<T, E> {
  readonly _tag: "Ok";
  readonly value: T;
  readonly error: undefined;
  isOk(): this is OkResult<T, E>;
  isErr(): this is ErrResult<T, E>;
  map<U>(fn: (value: T) => U): Result<U, E>;
  mapErr<F>(fn: (error: E) => F): Result<T, F>;
}

export interface ErrResult<T, E> {
  readonly _tag: "Err";
  readonly value: undefined;
  readonly error: E;
  isOk(): this is OkResult<T, E>;
  isErr(): this is ErrResult<T, E>;
  map<U>(fn: (value: T) => U): Result<U, E>;
  mapErr<F>(fn: (error: E) => F): Result<T, F>;
}

export function ok<T, E>(value: T): OkResult<T, E> {
  const result: OkResult<T, E> = {
    _tag: "Ok",
    value,
    error: undefined,
    isOk(): this is OkResult<T, E> {
      return true;
    },
    isErr(): this is ErrResult<T, E> {
      return false;
    },
    map<U>(fn: (value: T) => U): Result<U, E> {
      return ok(fn(value));
    },
    mapErr<F>(_fn: (error: E) => F): Result<T, F> {
      return this as unknown as Result<T, F>;
    },
  };
  return result;
}

export function err<T, E>(error: E): ErrResult<T, E> {
  const result: ErrResult<T, E> = {
    _tag: "Err",
    value: undefined,
    error,
    isOk(): this is OkResult<T, E> {
      return false;
    },
    isErr(): this is ErrResult<T, E> {
      return true;
    },
    map<U>(_fn: (value: T) => U): Result<U, E> {
      return this as unknown as Result<U, E>;
    },
    mapErr<F>(fn: (error: E) => F): Result<T, F> {
      return err(fn(error));
    },
  };
  return result;
}
