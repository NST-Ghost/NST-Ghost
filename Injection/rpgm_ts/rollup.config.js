// rollup.config.js
// NST Translator — RPG Maker MV/MZ compatibility build configuration.
// Bundles the TypeScript source into a single IIFE plugin file with the
// RPG Maker plugin comment header so it can be dropped into js/plugins/.

import { babel } from '@rollup/plugin-babel';
import commonjs from '@rollup/plugin-commonjs';
import resolve from '@rollup/plugin-node-resolve';
import replace from '@rollup/plugin-replace';
import terser from '@rollup/plugin-terser';
import typescript from '@rollup/plugin-typescript';

// RPG Maker MV/MZ plugin comment header. The @plugindesc line is what
// RPG Maker's Plugin Manager reads. Keep this block at the very top.
const pluginBanner = `/*:
 * @target MV MZ
 * @plugindesc NST Translation Layer v1.0 — Drop-in runtime translation.
 * @author NST
 *
 * @help
 * ============================================================================
 * NST Translation Layer
 * ============================================================================
 *
 * Drop-in translation for RPG Maker MV/MZ games.
 * Works on: NW.js (PC), JoiPlay (Android), Browser, Linux, Mac
 *
 * This plugin reads translations from a nst_translations/ folder placed
 * at the game root, and hooks the RPG Maker runtime to translate text
 * on-the-fly WITHOUT modifying the original game data files.
 *
 * Layout:
 *   [Game root]/
 *   ├── nst_translations/
 *   │   ├── config.json        (hooks, patterns, font, sourceLocale)
 *   │   ├── Actors.json        (one file per game data file)
 *   │   ├── Map001.json
 *   │   └── System.json
 *   └── js/plugins/
 *       └── NST_TranslationLayer.js   (this file)
 *
 * HOW TO USE:
 *   1. Copy this file to: [Game]/js/plugins/NST_TranslationLayer.js
 *   2. Copy nst_translations/ folder to: [Game]/nst_translations/
 *   3. Add "NST_TranslationLayer" to plugins.js (or use the provided main.js)
 *   4. Play the game — translations work automatically
 *
 * Debug (F12 console):
 *   NST.TL.stats()     // show entry counts
 *   NST.TL.reload()    // reload translations
 *   NST.TL.missed()    // list untranslated source-locale text
 *
 * ============================================================================
 */
`;

const isProd = process.env.NODE_ENV === 'production';

export default {
  input: 'src/main.ts',
  output: {
    file: 'dist/NST_TranslationLayer.js',
    format: 'iife',
    name: 'NSTTranslatorPlugin',
    sourcemap: !isProd,
    banner: pluginBanner,
  },
  external: [],
  plugins: [
    replace({
      preventAssignment: true,
      'process.env.NODE_ENV': JSON.stringify(isProd ? 'production' : 'development'),
    }),
    resolve({
      browser: true,
      preferBuiltins: false,
      mainFields: ['browser', 'module', 'main'],
    }),
    commonjs({
      transformMixedEsModules: true,
    }),
    typescript({
      tsconfig: './tsconfig.json',
      sourceMap: !isProd,
    }),
    babel({
      babelHelpers: 'bundled',
      extensions: ['.js', '.ts', '.mjs'],
      // Only transpile src plus node_modules that need downleveling.
      include: ['src/**', 'node_modules/mnemonist/**', 'node_modules/ahocorasick/**'],
      configFile: './babel.config.cjs',
    }),
    // Minify in production builds to keep the bundled file small.
    ...(isProd ? [terser()] : []),
  ],
};
