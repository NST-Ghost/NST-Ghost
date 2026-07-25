// RPG Maker MZ 扩展类型定义

interface FontFaceSetExtended extends FontFaceSet {
  add(font: FontFace): void;
}

interface Game_System {
  mainFontFace(): string;
  mainFontSize(): number;
}

interface Scene_Boot {
  loadGameFonts(): void;
}

interface Utils {
  RPGMAKER_NAME: string;
}