// logger.ts — Simple browser-compatible logging module.
// Replaces pino to avoid a Node.js dependency at runtime.
// Ported from translator_scratch.

type LogLevel = 'trace' | 'debug' | 'info' | 'warn' | 'error' | 'fatal' | 'silent';

interface LoggerOptions {
  level?: LogLevel;
  name?: string;
}

interface Logger {
  level: LogLevel;
  trace: (msg: string | object, ...args: unknown[]) => void;
  debug: (msg: string | object, ...args: unknown[]) => void;
  info: (msg: string | object, ...args: unknown[]) => void;
  warn: (msg: string | object, ...args: unknown[]) => void;
  error: (msg: string | object, ...args: unknown[]) => void;
  fatal: (msg: string | object, ...args: unknown[]) => void;
}

const LOG_LEVELS: Record<LogLevel, number> = {
  trace: 10,
  debug: 20,
  info: 30,
  warn: 40,
  error: 50,
  fatal: 60,
  silent: 100,
};

function createLogger(options: LoggerOptions = {}): Logger {
  let currentLevel: LogLevel = options.level || 'info';
  const prefix = options.name ? `[${options.name}]` : '[NST]';

  function shouldLog(level: LogLevel): boolean {
    return LOG_LEVELS[level] >= LOG_LEVELS[currentLevel];
  }

  function formatMessage(msg: string | object, args: unknown[]): string {
    if (typeof msg === 'object') {
      try {
        return JSON.stringify(msg) + (args.length > 0 ? ' ' + args.join(' ') : '');
      } catch {
        return String(msg) + (args.length > 0 ? ' ' + args.join(' ') : '');
      }
    }
    return msg + (args.length > 0 ? ' ' + args.join(' ') : '');
  }

  const logger: Logger = {
    get level(): LogLevel {
      return currentLevel;
    },
    set level(newLevel: LogLevel) {
      currentLevel = newLevel;
    },

    trace(msg: string | object, ...args: unknown[]): void {
      if (shouldLog('trace')) {
        console.log(`${prefix} [TRACE]`, formatMessage(msg, args));
      }
    },

    debug(msg: string | object, ...args: unknown[]): void {
      if (shouldLog('debug')) {
        console.log(`${prefix} [DEBUG]`, formatMessage(msg, args));
      }
    },

    info(msg: string | object, ...args: unknown[]): void {
      if (shouldLog('info')) {
        console.log(`${prefix} [INFO]`, formatMessage(msg, args));
      }
    },

    warn(msg: string | object, ...args: unknown[]): void {
      if (shouldLog('warn')) {
        console.warn(`${prefix} [WARN]`, formatMessage(msg, args));
      }
    },

    error(msg: string | object, ...args: unknown[]): void {
      if (shouldLog('error')) {
        console.error(`${prefix} [ERROR]`, formatMessage(msg, args));
      }
    },

    fatal(msg: string | object, ...args: unknown[]): void {
      if (shouldLog('fatal')) {
        console.error(`${prefix} [FATAL]`, formatMessage(msg, args));
      }
    },
  };

  return logger;
}

// Default export: a factory function mimicking pino's API.
export default function pino(options: LoggerOptions = {}): Logger {
  return createLogger(options);
}

export { createLogger, Logger, LoggerOptions, LogLevel };
