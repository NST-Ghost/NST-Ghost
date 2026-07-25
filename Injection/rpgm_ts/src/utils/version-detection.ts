/// <reference path="../../typings/rmmv.d.ts" />
// version-detection.ts — RPG Maker MV/MZ version detection.
// Ported from translator_scratch.

// ============================================
// Version detection (RPG Maker MV/MZ)
// ============================================

function detectVersion(): { isMZ: boolean; isMV: boolean } {
  const utils = typeof Utils !== "undefined" && Utils !== null
    ? (Utils as { RPGMAKER_NAME?: string })
    : null;

  const isMZ = utils?.RPGMAKER_NAME === "MZ";
  const isMV = !isMZ && typeof utils?.RPGMAKER_NAME === "string";

  return { isMZ, isMV };
}

export function isMZ(): boolean {
  return detectVersion().isMZ;
}

export function isMV(): boolean {
  return detectVersion().isMV;
}

export function isRPGMaker(): boolean {
  return typeof Utils !== "undefined" && Utils !== null;
}

export function getVersionInfo(): {
  isMZ: boolean;
  isMV: boolean;
  name: string;
  engine: "RPGMaker" | "Unknown";
} {
  const { isMZ, isMV } = detectVersion();
  return {
    isMZ,
    isMV,
    name: isMZ ? "MZ" : "MV",
    engine: "RPGMaker",
  };
}
