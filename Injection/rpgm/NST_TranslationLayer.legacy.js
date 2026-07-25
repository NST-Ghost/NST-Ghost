/*:
 * @plugindesc NST Translation Layer v2.0 — Enhanced Drop-in runtime translation
 * @author NST (Advanced Version)
 *
 * @help
 * ============================================================================
 * NST Translation Layer (Advanced)
 * ============================================================================
 *
 * Drop-in translation for RPG Maker MV/MZ games.
 * Works on: NW.js (PC), JoiPlay (Android), Browser, Linux, Mac
 *
 * Enhanced features:
 *   1. Control Character Protection: extracts codes like \V[n], \C[n] etc
 *      during translation, preventing AI or translator corruption.
 *   2. Regex Translation Patterns: supports `/pattern/flags` replacement entries.
 *   3. Substring translations: uses Aho-Corasick algorithm for partial matching.
 *   4. Font Replacer & Offset Size: custom font loading and sizing support.
 *   5. Low-level rendering hooks: drawText, textWidth, measureTextWidth.
 *   6. Missed translations log: prints untranslated text in F12 console.
 *
 * HOW TO USE:
 *   1. Copy this file to: [Game]/js/plugins/NST_TranslationLayer.js
 *   2. Copy nst_translations/ folder to: [Game]/nst_translations/
 *   3. Add "NST_TranslationLayer" to plugins.js
 *   4. Play the game — translations work automatically
 *
 * ============================================================================
 */

(function() {
    'use strict';

    // ========================================================================
    //  Configuration (edit nst_translations/config.txt to change)
    // ========================================================================
    var _cfg = {
        enabled: true,
        debug: false,
        lazyLoad: true,
        fallback: 'original',
        sourceLocale: '',
        fontName: '',
        fontUrl: '',
        fontOffsetSize: undefined,
        fontMaxSizeOffset: undefined
    };

    // ========================================================================
    //  Translation Storage
    // ========================================================================
    var _src = new Map();           // source text → translated text
    var _keys = new Map();          // key path → translated text
    var _regexPatterns = [];        // array of { pattern: RegExp, replacement: string }
    var _translatedMarkers = new Map(); // tracks already translated texts (idempotence)
    var _missed = new Map();        // tracks missed translation keys (text → count)
    var _loaded = {};               // baseName → true (loaded files tracker)
    var _dbReady = false;
    var _ready = false;

    var _controlPattern = "\\\\[VNP]\\[\\d+\\]|\\\\I\\[\\d+\\]|\\\\C\\[\\d+\\]|\\\\G|\\\\[{}]|\\\\\\$|\\\\[.\\|]|\\\\!|\\\\[><]|\\\\\\^|\\\\\\\\|\\\\FS\\[\\d+\\]|\\\\P[XY]\\[-?\\d+\\]|\\\\[OT]C\\[\\d+\\]|\\\\(?:MSGCORE|MSGSND)\\[[^\\]]*\\]";
    var _sourceLocaleRe = null;
    var _acAutomaton = null;

    function _log(m) { if (_cfg.debug) console.log('[NST] ' + m); }

    // ========================================================================
    //  Cross-Platform File Reading
    //  Supports: NW.js, JoiPlay, Electron, Browser
    // ========================================================================

    // Detect base path for the game root
    var _base = (function() {
        // NW.js desktop (RPG Maker default)
        if (typeof nw !== 'undefined' && typeof require !== 'undefined') {
            var p = require('path');
            return p.dirname(process.mainModule.filename).replace(/\\/g, '/') + '/';
        }
        // Electron
        if (typeof process !== 'undefined' && process.versions && process.versions.electron) {
            return './';
        }
        // Browser / JoiPlay / WebView — relative paths work
        return '';
    })();

    /**
     * Read file as text. Works everywhere.
     * @param {string} path - relative to game root
     * @returns {string|null}
     */
    function _read(path) {
        var full = _base + path;

        // Node.js fs (NW.js)
        if (typeof require !== 'undefined') {
            try {
                var fs = require('fs');
                if (fs.existsSync(full)) {
                    return fs.readFileSync(full, 'utf8');
                }
                return null;
            } catch(e) { /* fall through to XHR */ }
        }

        // Synchronous XHR — works on JoiPlay, browser, everything
        try {
            var x = new XMLHttpRequest();
            x.open('GET', full, false);
            x.overrideMimeType('text/plain; charset=utf-8');
            x.send(null);
            if (x.status === 200 || x.status === 0) {
                return x.responseText;
            }
        } catch(e) {}

        return null;
    }

    // ========================================================================
    //  Config Parser (config.txt)
    // ========================================================================

    function _parseConfig(txt) {
        if (!txt) return;
        var lines = txt.split('\n');
        for (var i = 0; i < lines.length; i++) {
            var ln = lines[i].trim();
            if (!ln || ln[0] === '#') continue;
            var eq = ln.indexOf('=');
            if (eq < 0) continue;
            var k = ln.substring(0, eq).trim().toLowerCase();
            var v = ln.substring(eq + 1).trim();
            if (k === 'enabled')   _cfg.enabled   = (v === 'true');
            if (k === 'debug')     _cfg.debug     = (v === 'true');
            if (k === 'lazy_load') _cfg.lazyLoad   = (v === 'true');
            if (k === 'fallback')  _cfg.fallback   = v.toLowerCase();
            if (k === 'source_locale') {
                _cfg.sourceLocale = v;
                _initSourceLocale(v);
            }
            if (k === 'font_name')            _cfg.fontName = v;
            if (k === 'font_url')             _cfg.fontUrl = v;
            if (k === 'font_offset_size')     _cfg.fontOffsetSize = parseInt(v, 10);
            if (k === 'font_max_size_offset') _cfg.fontMaxSizeOffset = parseInt(v, 10);
        }
    }

    // ========================================================================
    //  Aho-Corasick Algorithm for Substring Translations
    // ========================================================================
    function AhoCorasickNode() {
        this.children = {};
        this.failure = null;
        this.output = [];
    }

    function AhoCorasick(keywords) {
        this.root = new AhoCorasickNode();
        for (var i = 0; i < keywords.length; i++) {
            this.insert(keywords[i]);
        }
        this.buildFailureLinks();
    }

    AhoCorasick.prototype.insert = function(keyword) {
        var node = this.root;
        for (var i = 0; i < keyword.length; i++) {
            var char = keyword[i];
            if (!node.children[char]) {
                node.children[char] = new AhoCorasickNode();
            }
            node = node.children[char];
        }
        node.output.push(keyword);
    };

    AhoCorasick.prototype.buildFailureLinks = function() {
        var queue = [];
        var root = this.root;
        
        for (var char in root.children) {
            var child = root.children[char];
            child.failure = root;
            queue.push(child);
        }
        
        while (queue.length > 0) {
            var node = queue.shift();
            
            for (var char in node.children) {
                var child = node.children[char];
                var failNode = node.failure;
                
                while (failNode !== null && !failNode.children[char]) {
                    failNode = failNode.failure;
                }
                
                child.failure = failNode ? failNode.children[char] : root;
                
                if (child.failure) {
                    child.output = child.output.concat(child.failure.output);
                }
                
                queue.push(child);
            }
        }
    };

    AhoCorasick.prototype.search = function(text) {
        var results = [];
        var node = this.root;
        
        for (var i = 0; i < text.length; i++) {
            var char = text[i];
            
            while (node !== null && !node.children[char]) {
                node = node.failure;
            }
            
            node = node ? node.children[char] : this.root;
            
            if (node.output.length > 0) {
                for (var j = 0; j < node.output.length; j++) {
                    results.push({
                        end: i,
                        pattern: node.output[j]
                    });
                }
            }
        }
        return results;
    };

    function selectNonOverlappingMatches(matches, textLength) {
        matches.sort(function(a, b) {
            var lenA = a.pattern.length;
            var lenB = b.pattern.length;
            if (lenA !== lenB) return lenB - lenA; // longer first
            var startA = a.end - lenA + 1;
            var startB = b.end - lenB + 1;
            return startA - startB;
        });

        var selected = [];
        var covered = new Array(textLength);
        for (var i = 0; i < textLength; i++) covered[i] = false;

        for (var i = 0; i < matches.length; i++) {
            var m = matches[i];
            var len = m.pattern.length;
            var start = m.end - len + 1;
            var end = m.end;

            var overlap = false;
            for (var j = start; j <= end; j++) {
                if (covered[j]) {
                    overlap = true;
                    break;
                }
            }

            if (!overlap) {
                selected.push({
                    start: start,
                    end: end + 1,
                    pattern: m.pattern
                });
                for (var j = start; j <= end; j++) {
                    covered[j] = true;
                }
            }
        }

        selected.sort(function(a, b) {
            return a.start - b.start;
        });

        return selected;
    }

    function _rebuildAC() {
        var keys = [];
        _src.forEach(function(value, key) {
            if (key.length >= 2) {
                keys.push(key);
            }
        });
        keys.sort(function(a, b) { return b.length - a.length; });
        if (keys.length > 0) {
            _acAutomaton = new AhoCorasick(keys);
            _log('Rebuilt AC Automaton with ' + keys.length + ' keys.');
        } else {
            _acAutomaton = null;
        }
    }

    // ========================================================================
    //  Control Characters Extraction
    // ========================================================================
    var PLACEHOLDER_BRACE = /\{(\d+)\}/g;

    function extractControls(text) {
        if (!_controlPattern) {
            return { plain: text, controls: [] };
        }
        var extractRe = new RegExp(_controlPattern, "gi");
        var replaceRe = new RegExp(_controlPattern, "gi");
        var controls = [];
        var match;
        var loopCount = 0;
        
        while ((match = extractRe.exec(text)) !== null && loopCount++ < 100) {
            controls.push({
                text: match[0],
                index: match.index
            });
            if (match.index === extractRe.lastIndex) {
                extractRe.lastIndex++;
            }
        }
        var plain = text.replace(replaceRe, "");
        return { plain: plain, controls: controls };
    }

    function applyControlPlaceholders(translation, controls) {
        if (controls.length === 0) return translation;
        var hasReplacement = false;
        var result = translation.replace(PLACEHOLDER_BRACE, function(match, indexStr) {
            var index = parseInt(indexStr, 10);
            if (index >= 0 && index < controls.length) {
                hasReplacement = true;
                return controls[index].text;
            }
            return match;
        });
        return hasReplacement ? result : reconstructWithControls(translation, controls);
    }

    function reconstructWithControls(translation, controls) {
        if (controls.length === 0) return translation;
        var prefixControls = [];
        var suffixControls = [];
        var lastEnd = 0;
        for (var i = 0; i < controls.length; i++) {
            var ctrl = controls[i];
            if (ctrl.index === lastEnd) {
                prefixControls.push(ctrl.text);
                lastEnd = ctrl.index + ctrl.text.length;
            } else {
                break;
            }
        }
        for (var i = prefixControls.length; i < controls.length; i++) {
            suffixControls.push(controls[i].text);
        }
        return prefixControls.join("") + translation + suffixControls.join("");
    }

    // ========================================================================
    //  Source Locale Detector
    // ========================================================================
    function _initSourceLocale(locale) {
        if (!locale) {
            _sourceLocaleRe = null;
            return;
        }
        var l = locale.toLowerCase();
        if (l === 'ja' || l === 'jp' || l === 'jpn') {
            _sourceLocaleRe = /[\u3040-\u309F\u30A0-\u30FF]/; // Japanese Kana
        } else if (l === 'ko' || l === 'kor' || l === 'kr') {
            _sourceLocaleRe = /[\uAC00-\uD7AF\u1100-\u11FF\u3130-\u318F]/; // Korean Hangul
        } else if (l === 'en' || l === 'eng') {
            _sourceLocaleRe = /[a-zA-Z]/; // English
        } else {
            _sourceLocaleRe = null;
        }
    }

    function _hasSourceLocaleChars(text) {
        if (!_sourceLocaleRe) return true;
        return _sourceLocaleRe.test(text);
    }

    // ========================================================================
    //  Translation File Parser
    // ========================================================================

    function _parse(txt) {
        if (!txt) return 0;
        var n = 0;
        var S_NONE = 0, S_KEY = 1, S_ORIG = 2, S_TRANS = 3;
        var state = S_NONE, ck = '', co = '', ct = '';
        var lines = txt.split('\n');

        for (var i = 0; i < lines.length; i++) {
            var raw = lines[i];
            var t = raw.trim();

            if (state === S_NONE && (!t || t[0] === '#')) continue;

            if (t === '<<<KEY>>>')        { state = S_KEY;   ck = ''; continue; }
            if (t === '<<<ORIGINAL>>>')   { state = S_ORIG;  co = ''; continue; }
            if (t === '<<<TRANSLATED>>>') { state = S_TRANS;  ct = ''; continue; }
            if (t === '<<<END>>>') {
                var orig  = co.replace(/\n$/, '');
                var trans = ct.replace(/\n$/, '');
                if (orig && trans) {
                    // Detect if orig is a RegExp
                    var rx = /^\/(.+)\/([gimsuy]*)$/.exec(orig);
                    if (rx) {
                        try {
                            _regexPatterns.push({
                                pattern: new RegExp(rx[1], rx[2]),
                                replacement: trans
                            });
                        } catch(e) {
                            console.error('[NST] Invalid RegExp entry: ' + orig, e);
                        }
                    } else {
                        _src.set(orig, trans);
                        if (ck) _keys.set(ck.trim(), trans);
                    }
                    n++;
                }
                state = S_NONE; ck = ''; co = ''; ct = '';
                continue;
            }

            if (state === S_KEY)   ck += (ck ? '\n' : '') + raw;
            if (state === S_ORIG)  co += (co ? '\n' : '') + raw;
            if (state === S_TRANS) ct += (ct ? '\n' : '') + raw;
        }
        return n;
    }

    // ========================================================================
    //  File Loading
    // ========================================================================

    function _loadFile(name) {
        if (_loaded[name]) return;
        var txt = _read('nst_translations/' + name + '.txt');
        if (txt !== null) {
            var c = _parse(txt);
            _loaded[name] = true;
            _log('Loaded ' + name + '.txt (' + c + ' entries)');
            if (c > 0) {
                _rebuildAC();
            }
        }
    }

    function _loadDB() {
        if (_dbReady) return;
        var files = [
            'Actors','Classes','Skills','Items','Weapons','Armors',
            'Enemies','Troops','States','CommonEvents','System','_inline'
        ];
        for (var i = 0; i < files.length; i++) _loadFile(files[i]);
        _dbReady = true;
    }

    function _loadMap(id) {
        if (id <= 0) return;
        var s = String(id);
        while (s.length < 3) s = '0' + s;
        _loadFile('Map' + s);
    }

    function _loadAll() {
        _loadDB();
        // On NW.js, scan directory for all .txt files
        if (typeof require !== 'undefined') {
            try {
                var fs = require('fs');
                var dir = _base + 'nst_translations/';
                if (fs.existsSync(dir)) {
                    var all = fs.readdirSync(dir);
                    for (var i = 0; i < all.length; i++) {
                        if (/\.txt$/i.test(all[i]) && all[i] !== 'config.txt') {
                            _loadFile(all[i].replace(/\.txt$/i, ''));
                        }
                    }
                }
            } catch(e) {}
        }
    }

    // ========================================================================
    //  Font System
    // ========================================================================
    var _fontLoadAttempted = false;
    var _fontHooksInstalled = false;

    function _loadFont(fontConfig) {
        if (_fontLoadAttempted) return;
        _fontLoadAttempted = true;

        var fontUrl = _cfg.fontUrl;
        
        if (typeof FontFace !== 'undefined' && document.fonts) {
            try {
                var font = new FontFace(fontConfig.fontName, 'url("' + fontUrl + '")');
                document.fonts.add(font);
                font.load().then(function() {
                    _log('Custom font loaded successfully: ' + fontConfig.fontName);
                    _refreshWindows();
                }).catch(function(e) {
                    console.error('[NST] Font loading failed:', e);
                    _addFontFaceStyle(fontConfig, fontUrl);
                });
            } catch(e) {
                _addFontFaceStyle(fontConfig, fontUrl);
            }
        } else {
            _addFontFaceStyle(fontConfig, fontUrl);
        }

        _installFontHooks();
    }

    function _addFontFaceStyle(config, fontUrl) {
        if (typeof document === 'undefined' || !document.head) return;
        try {
            var style = document.createElement('style');
            style.type = 'text/css';
            style.appendChild(document.createTextNode(
                '@font-face { font-family: "' + config.fontName.replace(/"/g, '\\"') + '"; src: url("' + fontUrl.replace(/"/g, '\\"') + '"); }'
            ));
            document.head.appendChild(style);
            _log('Custom font injected via CSS @font-face: ' + config.fontName);
            _refreshWindows();
        } catch(e) {
            console.error('[NST] CSS Font Face insertion failed:', e);
        }
    }

    function _refreshWindows() {
        if (typeof SceneManager === 'undefined' || !SceneManager._scene) return;
        var scene = SceneManager._scene;
        if (scene._windowLayer && scene._windowLayer.children) {
            scene._windowLayer.children.forEach(function(child) {
                if (child && typeof child.refresh === 'function') {
                    try { child.refresh(); } catch(e) {}
                }
            });
        }
        if (scene._messageWindow && typeof scene._messageWindow.refresh === 'function') {
            try { scene._messageWindow.refresh(); } catch(e) {}
        }
    }

    function _installFontHooks() {
        if (_fontHooksInstalled) return;
        _fontHooksInstalled = true;

        // MZ Main Font Face
        if (typeof Game_System !== 'undefined' && Game_System.prototype.mainFontFace) {
            var origMainFontFace = Game_System.prototype.mainFontFace;
            Game_System.prototype.mainFontFace = function() {
                var original = origMainFontFace.call(this);
                if (_cfg.fontName) {
                    return _cfg.fontName + ', ' + original;
                }
                return original;
            };
        }

        // MZ Main Font Size
        if (typeof Game_System !== 'undefined' && Game_System.prototype.mainFontSize) {
            var origMainFontSize = Game_System.prototype.mainFontSize;
            Game_System.prototype.mainFontSize = function() {
                var originalSize = origMainFontSize.call(this);
                if (_cfg.fontOffsetSize !== undefined) {
                    var newSize = originalSize + _cfg.fontOffsetSize;
                    if (_cfg.fontMaxSizeOffset !== undefined) {
                        var maxSize = originalSize + _cfg.fontMaxSizeOffset;
                        return Math.max(8, Math.min(newSize, maxSize));
                    }
                    return Math.max(8, newSize);
                }
                return originalSize;
            };
        }

        // MV Font Face
        if (typeof Window_Base !== 'undefined' && Window_Base.prototype.standardFontFace) {
            var origStandardFontFace = Window_Base.prototype.standardFontFace;
            Window_Base.prototype.standardFontFace = function() {
                var original = origStandardFontFace.call(this);
                if (_cfg.fontName) {
                    return _cfg.fontName + ', ' + original;
                }
                return original;
            };
        }

        // MV Font Size
        if (typeof Window_Base !== 'undefined' && Window_Base.prototype.standardFontSize) {
            var origStandardFontSize = Window_Base.prototype.standardFontSize;
            Window_Base.prototype.standardFontSize = function() {
                var originalSize = origStandardFontSize.call(this);
                if (_cfg.fontOffsetSize !== undefined) {
                    var newSize = originalSize + _cfg.fontOffsetSize;
                    if (_cfg.fontMaxSizeOffset !== undefined) {
                        var maxSize = originalSize + _cfg.fontMaxSizeOffset;
                        return Math.max(8, Math.min(newSize, maxSize));
                    }
                    return Math.max(8, newSize);
                }
                return originalSize;
            };
        }
    }

    // ========================================================================
    //  Translation Pipeline
    // ========================================================================

    function markTranslated(text) {
        if (!text) return;
        if (_translatedMarkers.size > 5000) {
            _translatedMarkers.clear();
        }
        _translatedMarkers.set(text, true);
    }

    function _applyRegex(text) {
        for (var i = 0; i < _regexPatterns.length; i++) {
            var item = _regexPatterns[i];
            item.pattern.lastIndex = 0;
            var replaced = text.replace(item.pattern, item.replacement);
            if (replaced !== text) {
                return replaced;
            }
        }
        return null;
    }

    function partialMatchTranslate(text) {
        if (!_acAutomaton) return text;
        var matches = _acAutomaton.search(text);
        if (!matches || matches.length === 0) return text;

        var selected = selectNonOverlappingMatches(matches, text.length);
        if (selected.length === 0) return text;

        var result = text;
        for (var i = selected.length - 1; i >= 0; i--) {
            var match = selected[i];
            var translation = _src.get(match.pattern);
            if (translation) {
                result = result.substring(0, match.start) + translation + result.substring(match.end);
            }
        }
        return result;
    }

    function _recordMissed(text) {
        if (!_cfg.enabled || !text) return;
        if (!_hasSourceLocaleChars(text)) return;
        var count = _missed.get(text) || 0;
        if (_missed.size < 1000) {
            _missed.set(text, count + 1);
        }
    }

    function _tr(text, key) {
        if (!_cfg.enabled || !text) return text;
        if (_translatedMarkers.has(text)) return text;

        var result = text;

        // 1. Precise Key Match (high priority)
        if (key && _keys.has(key)) {
            result = _keys.get(key);
            markTranslated(result);
            return result;
        }

        // 2. Exact Match
        if (_src.has(text)) {
            result = _src.get(text);
            markTranslated(result);
            return result;
        }

        // 3. Control Character Extraction
        var ext = extractControls(text);
        var hasControls = ext.controls.length > 0 && ext.plain.length > 0 && ext.plain !== text;
        if (hasControls) {
            if (_src.has(ext.plain)) {
                result = applyControlPlaceholders(_src.get(ext.plain), ext.controls);
                markTranslated(result);
                return result;
            }
        }

        // 4. Regex Patterns
        var rxMatch = _applyRegex(text);
        if (rxMatch !== null) {
            markTranslated(rxMatch);
            return rxMatch;
        }
        if (hasControls) {
            var rxPlainMatch = _applyRegex(ext.plain);
            if (rxPlainMatch !== null) {
                result = applyControlPlaceholders(rxPlainMatch, ext.controls);
                markTranslated(result);
                return result;
            }
        }

        // 5. Aho-Corasick Substring Matching
        if (_hasSourceLocaleChars(text)) {
            var acMatch = partialMatchTranslate(text);
            if (acMatch !== text) {
                markTranslated(acMatch);
                return acMatch;
            }
            if (hasControls) {
                var acPlainMatch = partialMatchTranslate(ext.plain);
                if (acPlainMatch !== ext.plain) {
                    result = applyControlPlaceholders(acPlainMatch, ext.controls);
                    markTranslated(result);
                    return result;
                }
            }
        }

        // 6. Record Missed Translation
        if (result === text) {
            _recordMissed(text);
        }

        // Fallbacks
        if (result === text) {
            if (_cfg.fallback === 'empty') return '';
            if (_cfg.fallback === '[untranslated]' && _hasSourceLocaleChars(text)) {
                return '[UNTRANSLATED] ' + text;
            }
        }

        return result;
    }

    // ========================================================================
    //  Static Data Translation (Database)
    // ========================================================================
    var _textFields = [
        "name", "description", "displayName", "nickname", "profile",
        "message1", "message2", "message3", "message4", "gameTitle", "terms", "messages"
    ];

    function _translateData(data) {
        if (!data) return;
        try {
            if (Array.isArray(data)) {
                for (var i = 0; i < data.length; i++) {
                    if (data[i] && typeof data[i] === 'object') {
                        _translateObject(data[i], 0);
                    }
                }
            } else if (typeof data === 'object') {
                _translateObject(data, 0);
            }
        } catch(e) {
            console.error('[NST] Error translating database object:', e);
        }
    }

    function _translateObject(obj, depth) {
        if (depth > 6 || !obj || typeof obj !== 'object') return;
        
        for (var i = 0; i < _textFields.length; i++) {
            var field = _textFields[i];
            if (typeof obj[field] === 'string') {
                obj[field] = _tr(obj[field]);
            } else if (obj[field] && typeof obj[field] === 'object' && !Array.isArray(obj[field])) {
                _translateNested(obj[field], depth + 1);
            }
        }

        if (Array.isArray(obj.list)) {
            _translateCommands(obj.list);
        }

        if (Array.isArray(obj.pages)) {
            for (var p = 0; p < obj.pages.length; p++) {
                var page = obj.pages[p];
                if (page && typeof page === 'object' && Array.isArray(page.list)) {
                    _translateCommands(page.list);
                }
            }
        }
    }

    function _translateNested(obj, depth) {
        if (depth > 6 || !obj || typeof obj !== 'object') return;
        var keys = Object.keys(obj);
        for (var i = 0; i < keys.length; i++) {
            var key = keys[i];
            var val = obj[key];
            if (typeof val === 'string') {
                obj[key] = _tr(val);
            } else if (val && typeof val === 'object' && !Array.isArray(val)) {
                _translateNested(val, depth + 1);
            }
        }
    }

    var _textCommands = {
        "101": [4],
        "102": [0],
        "320": [1],
        "324": [1],
        "325": [1],
        "402": [1],
        "405": [0]
    };

    function _translateCommands(list) {
        for (var i = 0; i < list.length; i++) {
            var cmd = list[i];
            if (!cmd || typeof cmd !== 'object') continue;
            var code = String(cmd.code);
            var params = cmd.parameters;
            if (!code || !Array.isArray(params)) continue;
            
            var indices = _textCommands[code];
            if (indices) {
                for (var k = 0; k < indices.length; k++) {
                    var idx = indices[k];
                    if (idx >= params.length) continue;
                    var p = params[idx];
                    if (typeof p === 'string') {
                        params[idx] = _tr(p);
                    } else if (Array.isArray(p)) {
                        for (var j = 0; j < p.length; j++) {
                            if (typeof p[j] === 'string') {
                                p[j] = _tr(p[j]);
                            }
                        }
                    }
                }
            }
        }
    }

    // ========================================================================
    //  Initialization
    // ========================================================================

    function _init() {
        if (_ready) return;
        _ready = true;

        _parseConfig(_read('nst_translations/config.txt'));

        if (!_cfg.enabled) {
            console.log('[NST] Translation disabled.');
            return;
        }

        _loadDB();
        if (!_cfg.lazyLoad) _loadAll();

        if (_cfg.fontName && _cfg.fontUrl) {
            _loadFont({
                fontName: _cfg.fontName,
                fontUrl: _cfg.fontUrl,
                offsetSize: _cfg.fontOffsetSize,
                maxSizeOffset: _cfg.fontMaxSizeOffset
            });
        }

        console.log('[NST] Translation Layer ready. ' +
            _src.size + ' source entries, ' + _keys.size + ' key entries, ' + _regexPatterns.length + ' regex entries.');
    }

    // ========================================================================
    //  RPG Maker Hooks
    // ========================================================================

    // --- 1. Main text pipeline ---
    var _o1 = Window_Base.prototype.convertEscapeCharacters;
    Window_Base.prototype.convertEscapeCharacters = function(text) {
        if (_cfg.enabled && text) {
            text = _tr(text);
        }
        return _o1.call(this, text);
    };

    // --- 2. Dialogue messages ---
    var _o2 = Window_Message.prototype.startMessage;
    Window_Message.prototype.startMessage = function() {
        if (_cfg.enabled && $gameMessage && $gameMessage._texts) {
            var full = $gameMessage._texts.join('\n');
            var out = _tr(full);
            if (out !== full) {
                $gameMessage._texts = out.split('\n');
            }
        }
        _o2.call(this);
    };

    // --- 3. Choice list ---
    if (typeof Window_ChoiceList !== 'undefined') {
        var _o3 = Window_ChoiceList.prototype.makeCommandList;
        Window_ChoiceList.prototype.makeCommandList = function() {
            _o3.call(this);
            if (_cfg.enabled && this._list) {
                for (var i = 0; i < this._list.length; i++) {
                    if (this._list[i] && this._list[i].name) {
                        this._list[i].name = _tr(this._list[i].name);
                    }
                }
            }
        };
    }

    // --- 4. Map change → lazy load ---
    var _o4 = Game_Map.prototype.setup;
    Game_Map.prototype.setup = function(mapId) {
        _o4.call(this, mapId);
        if (_cfg.enabled && _cfg.lazyLoad) _loadMap(mapId);
    };

    // --- 5. Database init & data hooks ---
    var _o5 = DataManager.loadDatabase;
    DataManager.loadDatabase = function() {
        _o5.call(this);
        _init();
    };

    var _oOnLoad = DataManager.onLoad;
    DataManager.onLoad = function(object) {
        if (_cfg.enabled && object) {
            _translateData(object);
        }
        _oOnLoad.call(this, object);
    };

    // --- 6. Scrolling text ---
    if (typeof Window_ScrollText !== 'undefined') {
        var _o6 = Window_ScrollText.prototype.startMessage;
        Window_ScrollText.prototype.startMessage = function() {
            if (_cfg.enabled && $gameMessage && $gameMessage._texts) {
                var full = $gameMessage._texts.join('\n');
                var out = _tr(full);
                if (out !== full) $gameMessage._texts = out.split('\n');
            }
            _o6.call(this);
        };
    }

    // --- 7. Actor name/nickname/profile ---
    var _o7a = Game_Actor.prototype.name;
    Game_Actor.prototype.name = function() {
        var r = _o7a.call(this);
        return _cfg.enabled && r ? _tr(r) : r;
    };

    var _o7b = Game_Actor.prototype.nickname;
    Game_Actor.prototype.nickname = function() {
        var r = _o7b.call(this);
        return _cfg.enabled && r ? _tr(r) : r;
    };

    var _o7c = Game_Actor.prototype.profile;
    Game_Actor.prototype.profile = function() {
        var r = _o7c.call(this);
        return _cfg.enabled && r ? _tr(r) : r;
    };

    // --- 8. Item/Skill/Weapon/Armor names ---
    var _o8 = Window_Base.prototype.drawItemName;
    Window_Base.prototype.drawItemName = function(item, x, y, width) {
        if (_cfg.enabled && item && item.name) {
            var copy = Object.create(item);
            copy.name = _tr(item.name);
            if (item.description) copy.description = _tr(item.description);
            return _o8.call(this, copy, x, y, width);
        }
        return _o8.call(this, item, x, y, width);
    };

    // --- 9. Help window (descriptions) ---
    if (typeof Window_Help !== 'undefined') {
        var _o9 = Window_Help.prototype.setItem;
        Window_Help.prototype.setItem = function(item) {
            if (_cfg.enabled && item && item.description) {
                var copy = Object.create(item);
                copy.description = _tr(item.description);
                return _o9.call(this, copy);
            }
            return _o9.call(this, item);
        };
    }

    // --- 10. Low-level Text Rendering Hooks ---
    function _hookMethod(obj, name, paramIndex) {
        if (!obj || !obj.prototype || typeof obj.prototype[name] !== 'function') return;
        var orig = obj.prototype[name];
        obj.prototype[name] = function() {
            var args = Array.prototype.slice.call(arguments);
            if (_cfg.enabled && typeof args[paramIndex] === 'string') {
                args[paramIndex] = _tr(args[paramIndex]);
            }
            return orig.apply(this, args);
        };
    }

    if (typeof Window_Base !== 'undefined') {
        _hookMethod(Window_Base, 'drawText', 0);
        _hookMethod(Window_Base, 'drawTextEx', 0);
        _hookMethod(Window_Base, 'textWidth', 0);
        _hookMethod(Window_Base, 'createTextState', 0);
    }
    if (typeof Bitmap !== 'undefined') {
        _hookMethod(Bitmap, 'drawText', 0);
        _hookMethod(Bitmap, 'measureTextWidth', 0);
    }

    // ========================================================================
    //  Public API (F12 console)
    // ========================================================================

    window.NST = window.NST || {};
    window.NST.TL = {
        translate: _tr,
        reload: function() {
            _src.clear(); _keys.clear(); _regexPatterns = []; _translatedMarkers.clear(); _missed.clear();
            _loaded = {}; _dbReady = false; _ready = false;
            _init();
            console.log('[NST] Reloaded.');
        },
        stats: function() {
            console.log('[NST] Sources: ' + _src.size +
                        ', Keys: ' + _keys.size +
                        ', Regexes: ' + _regexPatterns.length +
                        ', Files: ' + Object.keys(_loaded).join(', '));
        },
        missed: function() {
            var list = [];
            _missed.forEach(function(count, text) {
                list.push({ text: text, count: count });
            });
            list.sort(function(a, b) { return b.count - a.count; });
            console.log('[NST] Missed translations (top 100):');
            for (var i = 0; i < Math.min(list.length, 100); i++) {
                console.log('Count: ' + list[i].count + ' | Text: ' + list[i].text);
            }
        }
    };

})();
