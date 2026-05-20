/*:
 * @plugindesc NST Translation Layer v1.0 — Drop-in runtime translation
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
 * HOW TO USE:
 *   1. Copy this file to: [Game]/js/plugins/NST_TranslationLayer.js
 *   2. Copy nst_translations/ folder to: [Game]/nst_translations/
 *   3. Add "NST_TranslationLayer" to plugins.js (see below)
 *   4. Play the game — translations work automatically
 *
 * ADDING TO plugins.js:
 *   Open js/plugins.js and add this line to the array:
 *   {"name":"NST_TranslationLayer","status":true,"description":"NST","parameters":{}}
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
        fallback: 'original'
    };

    // ========================================================================
    //  Translation Storage
    // ========================================================================
    var _src = new Map();       // source text → translated text
    var _keys = new Map();      // key path → translated text
    var _loaded = {};           // baseName → true (loaded files tracker)
    var _dbReady = false;
    var _ready = false;

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
            x.send();
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
        }
    }

    // ========================================================================
    //  Translation File Parser
    //
    //  Format:
    //    <<<KEY>>>           (optional — for precise matching)
    //    events[5].parameters[0]
    //    <<<ORIGINAL>>>
    //    こんにちは
    //    <<<TRANSLATED>>>
    //    สวัสดี
    //    <<<END>>>
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
                    _src.set(orig, trans);
                    if (ck) _keys.set(ck.trim(), trans);
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
    //  Translation Lookup
    // ========================================================================

    function _tr(text, key) {
        if (!_cfg.enabled || !text) return text;
        // Key match first
        if (key && _keys.has(key)) return _keys.get(key);
        // Source match
        if (_src.has(text)) return _src.get(text);
        // Fallback
        if (_cfg.fallback === 'empty') return '';
        if (_cfg.fallback === '[untranslated]') return '[UNTRANSLATED] ' + text;
        return text;
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

        console.log('[NST] Translation Layer ready. ' +
            _src.size + ' source entries, ' + _keys.size + ' key entries.');
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

    // --- 5. Database init ---
    var _o5 = DataManager.loadDatabase;
    DataManager.loadDatabase = function() {
        _o5.call(this);
        _init();
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

    // ========================================================================
    //  Public API (F12 console)
    // ========================================================================

    window.NST = window.NST || {};
    window.NST.TL = {
        translate: _tr,
        reload: function() {
            _src.clear(); _keys.clear();
            _loaded = {}; _dbReady = false; _ready = false;
            _init();
            console.log('[NST] Reloaded.');
        },
        stats: function() {
            console.log('[NST] Sources: ' + _src.size +
                        ', Keys: ' + _keys.size +
                        ', Files: ' + Object.keys(_loaded).join(', '));
        }
    };

})();
