// default-config.ts — Default configuration for the NST Translation Layer.
//
// This module provides the default config object that is used when the game's
// nst_translations/config.json is missing or incomplete. It mirrors the
// hardcoded hooks from the legacy NST_TranslationLayer.js so that the new
// config-driven system produces identical behaviour out of the box.
//
// The config structure matches the schema in json-loader.ts.

// =============================================================================
// Custom Hook Definitions
// =============================================================================

/**
 * These 10 hooks replicate the exact behaviour of the legacy NST plugin's
 * hardcoded prototype patching. Each entry corresponds to one of the original
 * var _o1.._o10 wrapper functions.
 */
const DEFAULT_CUSTOM_HOOKS = [
  // --- 1. Main text pipeline: Window_Base.convertEscapeCharacters ---
  {
    class: "Window_Base",
    method: "convertEscapeCharacters",
    type: "tr0" as const,
    paramIndex: 0,
    enabled: true,
    title: "Main text pipeline",
    desc: "Translates text passing through the escape-character converter.",
  },

  // --- 2. Dialogue messages: Game_Message.add ---
  {
    class: "Game_Message",
    method: "add",
    type: "tr0" as const,
    paramIndex: 0,
    enabled: true,
    title: "Dialogue message text",
    desc: "Translates dialogue message text as it is added.",
  },

  // --- 3. Speaker name: Game_Message.setSpeakerName ---
  {
    class: "Game_Message",
    method: "setSpeakerName",
    type: "tr0" as const,
    paramIndex: 0,
    enabled: true,
    title: "Speaker name",
    desc: "Translates the speaker name displayed above dialogue.",
  },

  // --- 4. Event command 101 (Show Text) ---
  {
    class: "Game_Interpreter",
    method: "command101",
    type: "trNested" as const,
    paramIndex: 0,
    nestedIndex: [4],
    minParamLength: 5,
    enabled: true,
    title: "Show Text command",
    desc: "Translates event command 101 (Show Text) text parameter.",
  },

  // --- 5. Event command 102 (Show Choices) ---
  {
    class: "Game_Interpreter",
    method: "command102",
    type: "trNestedArray" as const,
    paramIndex: 0,
    nestedIndex: [0],
    enabled: true,
    title: "Show Choices command",
    desc: "Translates event command 102 (Show Choices) choice text.",
  },

  // --- 6. Data load translation ---
  {
    class: "DataManager",
    method: "onLoad",
    type: "trData" as const,
    paramIndex: 0,
    enabled: true,
    title: "Data load translation",
    desc: "Translates text content in game data objects on load.",
  },

  // --- 7. Text rendering: Window_Base.drawText ---
  {
    class: "Window_Base",
    method: "drawText",
    type: "tr0" as const,
    paramIndex: 0,
    enabled: true,
    title: "Window text draw",
    desc: "Translates text rendered by Window_Base.drawText.",
  },

  // --- 8. Extended text rendering: Window_Base.drawTextEx ---
  {
    class: "Window_Base",
    method: "drawTextEx",
    type: "tr0" as const,
    paramIndex: 0,
    enabled: true,
    title: "Extended text draw",
    desc: "Translates text rendered by Window_Base.drawTextEx.",
  },

  // --- 9. Text width calculation: Window_Base.textWidth ---
  {
    class: "Window_Base",
    method: "textWidth",
    type: "tr0" as const,
    paramIndex: 0,
    enabled: true,
    title: "Text width calc",
    desc: "Translates text during width measurement for correct layout.",
  },

  // --- 10. Bitmap rendering: Bitmap.drawText ---
  {
    class: "Bitmap",
    method: "drawText",
    type: "tr0" as const,
    paramIndex: 0,
    enabled: true,
    title: "Bitmap text draw",
    desc: "Translates text rendered by Bitmap.drawText.",
  },

  // --- 11. Bitmap measurement: Bitmap.measureTextWidth ---
  {
    class: "Bitmap",
    method: "measureTextWidth",
    type: "tr0" as const,
    paramIndex: 0,
    enabled: true,
    title: "Bitmap text measure",
    desc: "Translates text during Bitmap.measureTextWidth.",
  },

  // --- 12. Text state creation ---
  {
    class: "Window_Base",
    method: "createTextState",
    type: "tr0" as const,
    paramIndex: 0,
    enabled: true,
    title: "Text state creation",
    desc: "Translates text in Window_Base.createTextState.",
  },
];

// =============================================================================
// Default Config Object
// =============================================================================

/**
 * The full default configuration. This is merged with (or used in place of)
 * the user's nst_translations/config.json when fields are missing.
 */
export const DEFAULT_CONFIG = {
  __customHooks__: DEFAULT_CUSTOM_HOOKS,

  __textFields__: [
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
    "terms",
    "messages",
  ],

  __textCommands__: {
    "101": [4],
    "102": [0],
    "320": [1],
    "324": [1],
    "325": [1],
    "402": [1],
    "405": [0],
  },

  __controlCharPatterns__: [
    "\\\\[VNP]\\[\\d+\\]",
    "\\\\I\\[\\d+\\]",
    "\\\\C\\[\\d+\\]",
    "\\\\G",
    "\\\\[{}]",
    "\\\\\\$",
    "\\\\[.\\|]",
    "\\\\!",
    "\\\\[><]",
    "\\\\\\^",
    "\\\\\\\\",
    "\\\\FS\\[\\d+\\]",
    "\\\\P[XY]\\[-?\\d+\\]",
    "\\\\[OT]C\\[\\d+\\]",
    "\\\\(?:MSGCORE|MSGSND)\\[[^\\]]*\\]",
  ],

  __ignorePatterns__: [
    "^.$",
    "^\\s*$",
    "^[\\d\\s.,\\-+%$/\\\\:;()\\[\\]{}=*#@!?<>~`'\"^&|_]+$",
    "^\\d+$",
  ],

  __fontConfig__: {
    offsetSize: 0,
    maxSizeOffset: 0,
    fontName: "NotoSans",
    fontUrl: "fonts/NotoSans-Regular.woff2",
  },

  __sourceLocale__: "ja",
};

export type DefaultConfig = typeof DEFAULT_CONFIG;
