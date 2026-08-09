# RPG Maker Translation Safety & Preservation Rules

## 1. Plugin Command Key Prefixes Masking
RPG Maker MV/MZ plugins (e.g., `TextPicture`, `PictureCallCommon`, `DText`, `FilterController`) rely on exact Japanese keyword prefixes in event parameters:
- `X座標 = <val>`
- `Y座標 = <val>`
- `テキスト = <val>`
- `スクリプト = <val>`
- `スイッチ = <val>`
- `変数 = <val>`
- `ピクチャID = <val>`
- `透明度 = <val>`
- `拡大率 = <val>`

**Rule:** `RpgmControlMasker` MUST mask these key prefixes (`X座標 = `, `テキスト = `, etc.) as placeholders before translation, and restore them 100% identically post-translation. Never allow translation services to translate the parameter key prefix.

## 2. Empty Translation Fallback Guard
If a translation service returns an empty string `""` for a non-empty source text entry:
- **Rule:** Do NOT save `""` into the database or target game data files.
- Automatically fall back to the original `sourceText` to prevent undefined picture parameters or unhandled JS exceptions that result in a game startup black screen.

## 3. Escape Code & Control Code Integrity
Preserve all RPG Maker control codes (`\V[...]`, `\N[...]`, `\C[...]`, `\I[...]`, `\FS[...]`, `\eval{...}`) strictly using immutable `__NST_TAG_n__` placeholders.
