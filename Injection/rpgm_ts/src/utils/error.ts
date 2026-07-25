// error.ts — Error handling utilities.
// Uses alert() for explicit reporting to ensure maximum compatibility across
// NW.js, JoiPlay, and browsers. Ported from translator_scratch.

import { Result } from "./neverthrow.js";

// =============================================================================
// RPG Maker global type declarations
// =============================================================================

declare const AudioManager: {
  stopAll(): void;
} | undefined;

declare const SceneManager: {
  stop(): void;
} | undefined;

// =============================================================================
// Error display
// =============================================================================

/**
 * Attempt to gracefully stop the game's audio and scene before throwing.
 */
function tryStopGame(): void {
  if (typeof AudioManager !== "undefined") {
    try {
      AudioManager.stopAll();
    } catch (_) {
      // Ignore errors.
    }
  }

  if (typeof SceneManager !== "undefined") {
    try {
      SceneManager.stop();
    } catch (_) {
      // Ignore errors.
    }
  }
}

/**
 * Display an error and abort execution.
 * Uses an alert popup for maximum compatibility, then throws.
 */
export function showError(name: string, message: string): never {
  var fullMessage = "[NST] " + name + "\n" + message;

  // Always emit to console.
  console.error(fullMessage);

  // Attempt to stop the game cleanly.
  tryStopGame();

  // Show the error via alert.
  if (typeof alert === "function") {
    try {
      alert("[NST] " + name + "\n\n" + message);
    } catch (_) {
      // Ignore alert failures.
    }
  }

  throw new Error(fullMessage);
}

/**
 * Display a non-fatal warning.
 * Does not abort execution; only writes to the console.
 */
export function showWarning(name: string, message: string): void {
  var fullMessage = "[NST Warning] " + name + "\n" + message;
  console.warn(fullMessage);
}

// =============================================================================
// Result unwrap
// =============================================================================

/**
 * Unwrap a Result. If it is an error, display the error and abort.
 */
export function unwrap<T>(result: Result<T, string>, context: string): T {
  if (result.isErr()) {
    showError(context, result.error);
  }
  return result.value;
}
