// babel.config.cjs
// NST Translator — RPG Maker MV/MZ compatibility config.
// MV runs on NW.js 0.12.x (Chromium 41); MZ on NW.js 0.44+ (Chromium 76+).
// Target Chrome 41 for maximum MV compatibility.
module.exports = {
  presets: [
    ['@babel/preset-env', {
      targets: {
        // RPG Maker MV uses NW.js 0.12.x, based on Chromium 41.
        // Target Chrome 41 for compatibility with old MV games.
        chrome: '41'
      },
      modules: false, // Keep ES module format; let Rollup handle it.
      useBuiltIns: false, // Do not auto-inject polyfills (handled manually in main.ts).
    }]
  ],
  assumptions: {
    setPublicClassFields: true,
    privateFieldsAsProperties: true,
    noDocumentAll: true,
  },
};
