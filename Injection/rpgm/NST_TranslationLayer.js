/*:
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

(function () {
	'use strict';

	var commonjsGlobal = typeof globalThis !== 'undefined' ? globalThis : typeof window !== 'undefined' ? window : typeof global !== 'undefined' ? global : typeof self !== 'undefined' ? self : {};

	function getDefaultExportFromCjs (x) {
		return x && x.__esModule && Object.prototype.hasOwnProperty.call(x, 'default') ? x['default'] : x;
	}

	var es_array_from = {};

	var globalThis_1;
	var hasRequiredGlobalThis;

	function requireGlobalThis () {
		if (hasRequiredGlobalThis) return globalThis_1;
		hasRequiredGlobalThis = 1;
		var check = function (it) {
		  return it && it.Math === Math && it;
		};

		// https://github.com/zloirock/core-js/issues/86#issuecomment-115759028
		globalThis_1 =
		  // eslint-disable-next-line es/no-global-this -- safe
		  check(typeof globalThis == 'object' && globalThis) ||
		  check(typeof window == 'object' && window) ||
		  // eslint-disable-next-line no-restricted-globals -- safe
		  check(typeof self == 'object' && self) ||
		  check(typeof commonjsGlobal == 'object' && commonjsGlobal) ||
		  check(typeof globalThis_1 == 'object' && globalThis_1) ||
		  // eslint-disable-next-line no-new-func -- fallback
		  (function () { return this; })() || Function('return this')();
		return globalThis_1;
	}

	var objectGetOwnPropertyDescriptor = {};

	var fails;
	var hasRequiredFails;

	function requireFails () {
		if (hasRequiredFails) return fails;
		hasRequiredFails = 1;
		fails = function (exec) {
		  try {
		    return !!exec();
		  } catch (error) {
		    return true;
		  }
		};
		return fails;
	}

	var descriptors;
	var hasRequiredDescriptors;

	function requireDescriptors () {
		if (hasRequiredDescriptors) return descriptors;
		hasRequiredDescriptors = 1;
		var fails = requireFails();

		// Detect IE8's incomplete defineProperty implementation
		descriptors = !fails(function () {
		  // eslint-disable-next-line es/no-object-defineproperty -- required for testing
		  return Object.defineProperty({}, 1, { get: function () { return 7; } })[1] !== 7;
		});
		return descriptors;
	}

	var functionBindNative;
	var hasRequiredFunctionBindNative;

	function requireFunctionBindNative () {
		if (hasRequiredFunctionBindNative) return functionBindNative;
		hasRequiredFunctionBindNative = 1;
		var fails = requireFails();

		functionBindNative = !fails(function () {
		  // eslint-disable-next-line es/no-function-prototype-bind -- safe
		  var test = function () { /* empty */ }.bind();
		  // eslint-disable-next-line no-prototype-builtins -- safe
		  return typeof test != 'function' || test.hasOwnProperty('prototype');
		});
		return functionBindNative;
	}

	var functionCall;
	var hasRequiredFunctionCall;

	function requireFunctionCall () {
		if (hasRequiredFunctionCall) return functionCall;
		hasRequiredFunctionCall = 1;
		var NATIVE_BIND = requireFunctionBindNative();

		var call = Function.prototype.call;
		// eslint-disable-next-line es/no-function-prototype-bind -- safe
		functionCall = NATIVE_BIND ? call.bind(call) : function () {
		  return call.apply(call, arguments);
		};
		return functionCall;
	}

	var objectPropertyIsEnumerable = {};

	var hasRequiredObjectPropertyIsEnumerable;

	function requireObjectPropertyIsEnumerable () {
		if (hasRequiredObjectPropertyIsEnumerable) return objectPropertyIsEnumerable;
		hasRequiredObjectPropertyIsEnumerable = 1;
		var $propertyIsEnumerable = {}.propertyIsEnumerable;
		// eslint-disable-next-line es/no-object-getownpropertydescriptor -- safe
		var getOwnPropertyDescriptor = Object.getOwnPropertyDescriptor;

		// Nashorn ~ JDK8 bug
		var NASHORN_BUG = getOwnPropertyDescriptor && !$propertyIsEnumerable.call({ 1: 2 }, 1);

		// `Object.prototype.propertyIsEnumerable` method implementation
		// https://tc39.es/ecma262/#sec-object.prototype.propertyisenumerable
		objectPropertyIsEnumerable.f = NASHORN_BUG ? function propertyIsEnumerable(V) {
		  var descriptor = getOwnPropertyDescriptor(this, V);
		  return !!descriptor && descriptor.enumerable;
		} : $propertyIsEnumerable;
		return objectPropertyIsEnumerable;
	}

	var createPropertyDescriptor;
	var hasRequiredCreatePropertyDescriptor;

	function requireCreatePropertyDescriptor () {
		if (hasRequiredCreatePropertyDescriptor) return createPropertyDescriptor;
		hasRequiredCreatePropertyDescriptor = 1;
		createPropertyDescriptor = function (bitmap, value) {
		  return {
		    enumerable: !(bitmap & 1),
		    configurable: !(bitmap & 2),
		    writable: !(bitmap & 4),
		    value: value
		  };
		};
		return createPropertyDescriptor;
	}

	var functionUncurryThis;
	var hasRequiredFunctionUncurryThis;

	function requireFunctionUncurryThis () {
		if (hasRequiredFunctionUncurryThis) return functionUncurryThis;
		hasRequiredFunctionUncurryThis = 1;
		var NATIVE_BIND = requireFunctionBindNative();

		var FunctionPrototype = Function.prototype;
		var call = FunctionPrototype.call;
		// eslint-disable-next-line es/no-function-prototype-bind -- safe
		var uncurryThisWithBind = NATIVE_BIND && FunctionPrototype.bind.bind(call, call);

		functionUncurryThis = NATIVE_BIND ? uncurryThisWithBind : function (fn) {
		  return function () {
		    return call.apply(fn, arguments);
		  };
		};
		return functionUncurryThis;
	}

	var classofRaw;
	var hasRequiredClassofRaw;

	function requireClassofRaw () {
		if (hasRequiredClassofRaw) return classofRaw;
		hasRequiredClassofRaw = 1;
		var uncurryThis = requireFunctionUncurryThis();

		var toString = uncurryThis({}.toString);
		var stringSlice = uncurryThis(''.slice);

		classofRaw = function (it) {
		  return stringSlice(toString(it), 8, -1);
		};
		return classofRaw;
	}

	var indexedObject;
	var hasRequiredIndexedObject;

	function requireIndexedObject () {
		if (hasRequiredIndexedObject) return indexedObject;
		hasRequiredIndexedObject = 1;
		var uncurryThis = requireFunctionUncurryThis();
		var fails = requireFails();
		var classof = requireClassofRaw();

		var $Object = Object;
		var split = uncurryThis(''.split);

		// fallback for non-array-like ES3 and non-enumerable old V8 strings
		indexedObject = fails(function () {
		  // throws an error in rhino, see https://github.com/mozilla/rhino/issues/346
		  // eslint-disable-next-line no-prototype-builtins -- safe
		  return !$Object('z').propertyIsEnumerable(0);
		}) ? function (it) {
		  return classof(it) === 'String' ? split(it, '') : $Object(it);
		} : $Object;
		return indexedObject;
	}

	var isNullOrUndefined;
	var hasRequiredIsNullOrUndefined;

	function requireIsNullOrUndefined () {
		if (hasRequiredIsNullOrUndefined) return isNullOrUndefined;
		hasRequiredIsNullOrUndefined = 1;
		// we can't use just `it == null` since of `document.all` special case
		// https://tc39.es/ecma262/#sec-IsHTMLDDA-internal-slot-aec
		isNullOrUndefined = function (it) {
		  return it === null || it === undefined;
		};
		return isNullOrUndefined;
	}

	var requireObjectCoercible;
	var hasRequiredRequireObjectCoercible;

	function requireRequireObjectCoercible () {
		if (hasRequiredRequireObjectCoercible) return requireObjectCoercible;
		hasRequiredRequireObjectCoercible = 1;
		var isNullOrUndefined = requireIsNullOrUndefined();

		var $TypeError = TypeError;

		// `RequireObjectCoercible` abstract operation
		// https://tc39.es/ecma262/#sec-requireobjectcoercible
		requireObjectCoercible = function (it) {
		  if (isNullOrUndefined(it)) throw new $TypeError("Can't call method on " + it);
		  return it;
		};
		return requireObjectCoercible;
	}

	var toIndexedObject;
	var hasRequiredToIndexedObject;

	function requireToIndexedObject () {
		if (hasRequiredToIndexedObject) return toIndexedObject;
		hasRequiredToIndexedObject = 1;
		// toObject with fallback for non-array-like ES3 strings
		var IndexedObject = requireIndexedObject();
		var requireObjectCoercible = requireRequireObjectCoercible();

		toIndexedObject = function (it) {
		  return IndexedObject(requireObjectCoercible(it));
		};
		return toIndexedObject;
	}

	var isCallable;
	var hasRequiredIsCallable;

	function requireIsCallable () {
		if (hasRequiredIsCallable) return isCallable;
		hasRequiredIsCallable = 1;
		// https://tc39.es/ecma262/#sec-IsHTMLDDA-internal-slot
		var documentAll = typeof document == 'object' && document.all;

		// `IsCallable` abstract operation
		// https://tc39.es/ecma262/#sec-iscallable
		// eslint-disable-next-line unicorn/no-typeof-undefined -- required for testing
		isCallable = typeof documentAll == 'undefined' && documentAll !== undefined ? function (argument) {
		  return typeof argument == 'function' || argument === documentAll;
		} : function (argument) {
		  return typeof argument == 'function';
		};
		return isCallable;
	}

	var isObject$1;
	var hasRequiredIsObject;

	function requireIsObject () {
		if (hasRequiredIsObject) return isObject$1;
		hasRequiredIsObject = 1;
		var isCallable = requireIsCallable();

		isObject$1 = function (it) {
		  return typeof it == 'object' ? it !== null : isCallable(it);
		};
		return isObject$1;
	}

	var getBuiltIn;
	var hasRequiredGetBuiltIn;

	function requireGetBuiltIn () {
		if (hasRequiredGetBuiltIn) return getBuiltIn;
		hasRequiredGetBuiltIn = 1;
		var globalThis = requireGlobalThis();
		var isCallable = requireIsCallable();

		var aFunction = function (argument) {
		  return isCallable(argument) ? argument : undefined;
		};

		getBuiltIn = function (namespace, method) {
		  return arguments.length < 2 ? aFunction(globalThis[namespace]) : globalThis[namespace] && globalThis[namespace][method];
		};
		return getBuiltIn;
	}

	var objectIsPrototypeOf;
	var hasRequiredObjectIsPrototypeOf;

	function requireObjectIsPrototypeOf () {
		if (hasRequiredObjectIsPrototypeOf) return objectIsPrototypeOf;
		hasRequiredObjectIsPrototypeOf = 1;
		var uncurryThis = requireFunctionUncurryThis();

		objectIsPrototypeOf = uncurryThis({}.isPrototypeOf);
		return objectIsPrototypeOf;
	}

	var environmentUserAgent;
	var hasRequiredEnvironmentUserAgent;

	function requireEnvironmentUserAgent () {
		if (hasRequiredEnvironmentUserAgent) return environmentUserAgent;
		hasRequiredEnvironmentUserAgent = 1;
		var globalThis = requireGlobalThis();

		var navigator = globalThis.navigator;
		var userAgent = navigator && navigator.userAgent;

		environmentUserAgent = userAgent ? String(userAgent) : '';
		return environmentUserAgent;
	}

	var environmentV8Version;
	var hasRequiredEnvironmentV8Version;

	function requireEnvironmentV8Version () {
		if (hasRequiredEnvironmentV8Version) return environmentV8Version;
		hasRequiredEnvironmentV8Version = 1;
		var globalThis = requireGlobalThis();
		var userAgent = requireEnvironmentUserAgent();

		var process = globalThis.process;
		var Deno = globalThis.Deno;
		var versions = process && process.versions || Deno && Deno.version;
		var v8 = versions && versions.v8;
		var match, version;

		if (v8) {
		  match = v8.split('.');
		  // in old Chrome, versions of V8 isn't V8 = Chrome / 10
		  // but their correct versions are not interesting for us
		  version = match[0] > 0 && match[0] < 4 ? 1 : +(match[0] + match[1]);
		}

		// BrowserFS NodeJS `process` polyfill incorrectly set `.v8` to `0.0`
		// so check `userAgent` even if `.v8` exists, but 0
		if (!version && userAgent) {
		  match = userAgent.match(/Edge\/(\d+)/);
		  if (!match || match[1] >= 74) {
		    match = userAgent.match(/Chrome\/(\d+)/);
		    if (match) version = +match[1];
		  }
		}

		environmentV8Version = version;
		return environmentV8Version;
	}

	var symbolConstructorDetection;
	var hasRequiredSymbolConstructorDetection;

	function requireSymbolConstructorDetection () {
		if (hasRequiredSymbolConstructorDetection) return symbolConstructorDetection;
		hasRequiredSymbolConstructorDetection = 1;
		/* eslint-disable es/no-symbol -- required for testing */
		var V8_VERSION = requireEnvironmentV8Version();
		var fails = requireFails();
		var globalThis = requireGlobalThis();

		var $String = globalThis.String;

		// eslint-disable-next-line es/no-object-getownpropertysymbols -- required for testing
		symbolConstructorDetection = !!Object.getOwnPropertySymbols && !fails(function () {
		  var symbol = Symbol('symbol detection');
		  // Chrome 38 Symbol has incorrect toString conversion
		  // `get-own-property-symbols` polyfill symbols converted to object are not Symbol instances
		  // nb: Do not call `String` directly to avoid this being optimized out to `symbol+''` which will,
		  // of course, fail.
		  return !$String(symbol) || !(Object(symbol) instanceof Symbol) ||
		    // Chrome 38-40 symbols are not inherited from DOM collections prototypes to instances
		    !Symbol.sham && V8_VERSION && V8_VERSION < 41;
		});
		return symbolConstructorDetection;
	}

	var useSymbolAsUid;
	var hasRequiredUseSymbolAsUid;

	function requireUseSymbolAsUid () {
		if (hasRequiredUseSymbolAsUid) return useSymbolAsUid;
		hasRequiredUseSymbolAsUid = 1;
		/* eslint-disable es/no-symbol -- required for testing */
		var NATIVE_SYMBOL = requireSymbolConstructorDetection();

		useSymbolAsUid = NATIVE_SYMBOL &&
		  !Symbol.sham &&
		  typeof Symbol.iterator == 'symbol';
		return useSymbolAsUid;
	}

	var isSymbol;
	var hasRequiredIsSymbol;

	function requireIsSymbol () {
		if (hasRequiredIsSymbol) return isSymbol;
		hasRequiredIsSymbol = 1;
		var getBuiltIn = requireGetBuiltIn();
		var isCallable = requireIsCallable();
		var isPrototypeOf = requireObjectIsPrototypeOf();
		var USE_SYMBOL_AS_UID = requireUseSymbolAsUid();

		var $Object = Object;

		isSymbol = USE_SYMBOL_AS_UID ? function (it) {
		  return typeof it == 'symbol';
		} : function (it) {
		  var $Symbol = getBuiltIn('Symbol');
		  return isCallable($Symbol) && isPrototypeOf($Symbol.prototype, $Object(it));
		};
		return isSymbol;
	}

	var tryToString;
	var hasRequiredTryToString;

	function requireTryToString () {
		if (hasRequiredTryToString) return tryToString;
		hasRequiredTryToString = 1;
		var $String = String;

		tryToString = function (argument) {
		  try {
		    return $String(argument);
		  } catch (error) {
		    return 'Object';
		  }
		};
		return tryToString;
	}

	var aCallable;
	var hasRequiredACallable;

	function requireACallable () {
		if (hasRequiredACallable) return aCallable;
		hasRequiredACallable = 1;
		var isCallable = requireIsCallable();
		var tryToString = requireTryToString();

		var $TypeError = TypeError;

		// `Assert: IsCallable(argument) is true`
		aCallable = function (argument) {
		  if (isCallable(argument)) return argument;
		  throw new $TypeError(tryToString(argument) + ' is not a function');
		};
		return aCallable;
	}

	var getMethod;
	var hasRequiredGetMethod;

	function requireGetMethod () {
		if (hasRequiredGetMethod) return getMethod;
		hasRequiredGetMethod = 1;
		var aCallable = requireACallable();
		var isNullOrUndefined = requireIsNullOrUndefined();

		// `GetMethod` abstract operation
		// https://tc39.es/ecma262/#sec-getmethod
		getMethod = function (V, P) {
		  var func = V[P];
		  return isNullOrUndefined(func) ? undefined : aCallable(func);
		};
		return getMethod;
	}

	var ordinaryToPrimitive;
	var hasRequiredOrdinaryToPrimitive;

	function requireOrdinaryToPrimitive () {
		if (hasRequiredOrdinaryToPrimitive) return ordinaryToPrimitive;
		hasRequiredOrdinaryToPrimitive = 1;
		var call = requireFunctionCall();
		var isCallable = requireIsCallable();
		var isObject = requireIsObject();

		var $TypeError = TypeError;

		// `OrdinaryToPrimitive` abstract operation
		// https://tc39.es/ecma262/#sec-ordinarytoprimitive
		ordinaryToPrimitive = function (input, pref) {
		  var fn, val;
		  if (pref === 'string' && isCallable(fn = input.toString) && !isObject(val = call(fn, input))) return val;
		  if (isCallable(fn = input.valueOf) && !isObject(val = call(fn, input))) return val;
		  if (pref !== 'string' && isCallable(fn = input.toString) && !isObject(val = call(fn, input))) return val;
		  throw new $TypeError("Can't convert object to primitive value");
		};
		return ordinaryToPrimitive;
	}

	var sharedStore = {exports: {}};

	var isPure;
	var hasRequiredIsPure;

	function requireIsPure () {
		if (hasRequiredIsPure) return isPure;
		hasRequiredIsPure = 1;
		isPure = false;
		return isPure;
	}

	var defineGlobalProperty;
	var hasRequiredDefineGlobalProperty;

	function requireDefineGlobalProperty () {
		if (hasRequiredDefineGlobalProperty) return defineGlobalProperty;
		hasRequiredDefineGlobalProperty = 1;
		var globalThis = requireGlobalThis();

		// eslint-disable-next-line es/no-object-defineproperty -- safe
		var defineProperty = Object.defineProperty;

		defineGlobalProperty = function (key, value) {
		  try {
		    defineProperty(globalThis, key, { value: value, configurable: true, writable: true });
		  } catch (error) {
		    globalThis[key] = value;
		  } return value;
		};
		return defineGlobalProperty;
	}

	var hasRequiredSharedStore;

	function requireSharedStore () {
		if (hasRequiredSharedStore) return sharedStore.exports;
		hasRequiredSharedStore = 1;
		var IS_PURE = requireIsPure();
		var globalThis = requireGlobalThis();
		var defineGlobalProperty = requireDefineGlobalProperty();

		var SHARED = '__core-js_shared__';
		var store = sharedStore.exports = globalThis[SHARED] || defineGlobalProperty(SHARED, {});

		(store.versions || (store.versions = [])).push({
		  version: '3.49.0',
		  mode: IS_PURE ? 'pure' : 'global',
		  copyright: '© 2013–2025 Denis Pushkarev (zloirock.ru), 2025–2026 CoreJS Company (core-js.io). All rights reserved.',
		  license: 'https://github.com/zloirock/core-js/blob/v3.49.0/LICENSE',
		  source: 'https://github.com/zloirock/core-js'
		});
		return sharedStore.exports;
	}

	var shared;
	var hasRequiredShared;

	function requireShared () {
		if (hasRequiredShared) return shared;
		hasRequiredShared = 1;
		var store = requireSharedStore();

		shared = function (key, value) {
		  return store[key] || (store[key] = value || {});
		};
		return shared;
	}

	var toObject;
	var hasRequiredToObject;

	function requireToObject () {
		if (hasRequiredToObject) return toObject;
		hasRequiredToObject = 1;
		var requireObjectCoercible = requireRequireObjectCoercible();

		var $Object = Object;

		// `ToObject` abstract operation
		// https://tc39.es/ecma262/#sec-toobject
		toObject = function (argument) {
		  return $Object(requireObjectCoercible(argument));
		};
		return toObject;
	}

	var hasOwnProperty_1;
	var hasRequiredHasOwnProperty;

	function requireHasOwnProperty () {
		if (hasRequiredHasOwnProperty) return hasOwnProperty_1;
		hasRequiredHasOwnProperty = 1;
		var uncurryThis = requireFunctionUncurryThis();
		var toObject = requireToObject();

		var hasOwnProperty = uncurryThis({}.hasOwnProperty);

		// `HasOwnProperty` abstract operation
		// https://tc39.es/ecma262/#sec-hasownproperty
		// eslint-disable-next-line es/no-object-hasown -- safe
		hasOwnProperty_1 = Object.hasOwn || function hasOwn(it, key) {
		  return hasOwnProperty(toObject(it), key);
		};
		return hasOwnProperty_1;
	}

	var uid;
	var hasRequiredUid;

	function requireUid () {
		if (hasRequiredUid) return uid;
		hasRequiredUid = 1;
		var uncurryThis = requireFunctionUncurryThis();

		var id = 0;
		var postfix = Math.random();
		var toString = uncurryThis(1.1.toString);

		uid = function (key) {
		  return 'Symbol(' + (key === undefined ? '' : key) + ')_' + toString(++id + postfix, 36);
		};
		return uid;
	}

	var wellKnownSymbol;
	var hasRequiredWellKnownSymbol;

	function requireWellKnownSymbol () {
		if (hasRequiredWellKnownSymbol) return wellKnownSymbol;
		hasRequiredWellKnownSymbol = 1;
		var globalThis = requireGlobalThis();
		var shared = requireShared();
		var hasOwn = requireHasOwnProperty();
		var uid = requireUid();
		var NATIVE_SYMBOL = requireSymbolConstructorDetection();
		var USE_SYMBOL_AS_UID = requireUseSymbolAsUid();

		var Symbol = globalThis.Symbol;
		var WellKnownSymbolsStore = shared('wks');
		var createWellKnownSymbol = USE_SYMBOL_AS_UID ? Symbol['for'] || Symbol : Symbol && Symbol.withoutSetter || uid;

		wellKnownSymbol = function (name) {
		  if (!hasOwn(WellKnownSymbolsStore, name)) {
		    WellKnownSymbolsStore[name] = NATIVE_SYMBOL && hasOwn(Symbol, name)
		      ? Symbol[name]
		      : createWellKnownSymbol('Symbol.' + name);
		  } return WellKnownSymbolsStore[name];
		};
		return wellKnownSymbol;
	}

	var toPrimitive;
	var hasRequiredToPrimitive;

	function requireToPrimitive () {
		if (hasRequiredToPrimitive) return toPrimitive;
		hasRequiredToPrimitive = 1;
		var call = requireFunctionCall();
		var isObject = requireIsObject();
		var isSymbol = requireIsSymbol();
		var getMethod = requireGetMethod();
		var ordinaryToPrimitive = requireOrdinaryToPrimitive();
		var wellKnownSymbol = requireWellKnownSymbol();

		var $TypeError = TypeError;
		var TO_PRIMITIVE = wellKnownSymbol('toPrimitive');

		// `ToPrimitive` abstract operation
		// https://tc39.es/ecma262/#sec-toprimitive
		toPrimitive = function (input, pref) {
		  if (!isObject(input) || isSymbol(input)) return input;
		  var exoticToPrim = getMethod(input, TO_PRIMITIVE);
		  var result;
		  if (exoticToPrim) {
		    if (pref === undefined) pref = 'default';
		    result = call(exoticToPrim, input, pref);
		    if (!isObject(result) || isSymbol(result)) return result;
		    throw new $TypeError("Can't convert object to primitive value");
		  }
		  if (pref === undefined) pref = 'number';
		  return ordinaryToPrimitive(input, pref);
		};
		return toPrimitive;
	}

	var toPropertyKey;
	var hasRequiredToPropertyKey;

	function requireToPropertyKey () {
		if (hasRequiredToPropertyKey) return toPropertyKey;
		hasRequiredToPropertyKey = 1;
		var toPrimitive = requireToPrimitive();
		var isSymbol = requireIsSymbol();

		// `ToPropertyKey` abstract operation
		// https://tc39.es/ecma262/#sec-topropertykey
		toPropertyKey = function (argument) {
		  var key = toPrimitive(argument, 'string');
		  return isSymbol(key) ? key : key + '';
		};
		return toPropertyKey;
	}

	var documentCreateElement;
	var hasRequiredDocumentCreateElement;

	function requireDocumentCreateElement () {
		if (hasRequiredDocumentCreateElement) return documentCreateElement;
		hasRequiredDocumentCreateElement = 1;
		var globalThis = requireGlobalThis();
		var isObject = requireIsObject();

		var document = globalThis.document;
		// typeof document.createElement is 'object' in old IE
		var EXISTS = isObject(document) && isObject(document.createElement);

		documentCreateElement = function (it) {
		  return EXISTS ? document.createElement(it) : {};
		};
		return documentCreateElement;
	}

	var ie8DomDefine;
	var hasRequiredIe8DomDefine;

	function requireIe8DomDefine () {
		if (hasRequiredIe8DomDefine) return ie8DomDefine;
		hasRequiredIe8DomDefine = 1;
		var DESCRIPTORS = requireDescriptors();
		var fails = requireFails();
		var createElement = requireDocumentCreateElement();

		// Thanks to IE8 for its funny defineProperty
		ie8DomDefine = !DESCRIPTORS && !fails(function () {
		  // eslint-disable-next-line es/no-object-defineproperty -- required for testing
		  return Object.defineProperty(createElement('div'), 'a', {
		    get: function () { return 7; }
		  }).a !== 7;
		});
		return ie8DomDefine;
	}

	var hasRequiredObjectGetOwnPropertyDescriptor;

	function requireObjectGetOwnPropertyDescriptor () {
		if (hasRequiredObjectGetOwnPropertyDescriptor) return objectGetOwnPropertyDescriptor;
		hasRequiredObjectGetOwnPropertyDescriptor = 1;
		var DESCRIPTORS = requireDescriptors();
		var call = requireFunctionCall();
		var propertyIsEnumerableModule = requireObjectPropertyIsEnumerable();
		var createPropertyDescriptor = requireCreatePropertyDescriptor();
		var toIndexedObject = requireToIndexedObject();
		var toPropertyKey = requireToPropertyKey();
		var hasOwn = requireHasOwnProperty();
		var IE8_DOM_DEFINE = requireIe8DomDefine();

		// eslint-disable-next-line es/no-object-getownpropertydescriptor -- safe
		var $getOwnPropertyDescriptor = Object.getOwnPropertyDescriptor;

		// `Object.getOwnPropertyDescriptor` method
		// https://tc39.es/ecma262/#sec-object.getownpropertydescriptor
		objectGetOwnPropertyDescriptor.f = DESCRIPTORS ? $getOwnPropertyDescriptor : function getOwnPropertyDescriptor(O, P) {
		  O = toIndexedObject(O);
		  P = toPropertyKey(P);
		  if (IE8_DOM_DEFINE) try {
		    return $getOwnPropertyDescriptor(O, P);
		  } catch (error) { /* empty */ }
		  if (hasOwn(O, P)) return createPropertyDescriptor(!call(propertyIsEnumerableModule.f, O, P), O[P]);
		};
		return objectGetOwnPropertyDescriptor;
	}

	var objectDefineProperty = {};

	var v8PrototypeDefineBug;
	var hasRequiredV8PrototypeDefineBug;

	function requireV8PrototypeDefineBug () {
		if (hasRequiredV8PrototypeDefineBug) return v8PrototypeDefineBug;
		hasRequiredV8PrototypeDefineBug = 1;
		var DESCRIPTORS = requireDescriptors();
		var fails = requireFails();

		// V8 ~ Chrome 36-
		// https://bugs.chromium.org/p/v8/issues/detail?id=3334
		v8PrototypeDefineBug = DESCRIPTORS && fails(function () {
		  // eslint-disable-next-line es/no-object-defineproperty -- required for testing
		  return Object.defineProperty(function () { /* empty */ }, 'prototype', {
		    value: 42,
		    writable: false
		  }).prototype !== 42;
		});
		return v8PrototypeDefineBug;
	}

	var anObject;
	var hasRequiredAnObject;

	function requireAnObject () {
		if (hasRequiredAnObject) return anObject;
		hasRequiredAnObject = 1;
		var isObject = requireIsObject();

		var $String = String;
		var $TypeError = TypeError;

		// `Assert: Type(argument) is Object`
		anObject = function (argument) {
		  if (isObject(argument)) return argument;
		  throw new $TypeError($String(argument) + ' is not an object');
		};
		return anObject;
	}

	var hasRequiredObjectDefineProperty;

	function requireObjectDefineProperty () {
		if (hasRequiredObjectDefineProperty) return objectDefineProperty;
		hasRequiredObjectDefineProperty = 1;
		var DESCRIPTORS = requireDescriptors();
		var IE8_DOM_DEFINE = requireIe8DomDefine();
		var V8_PROTOTYPE_DEFINE_BUG = requireV8PrototypeDefineBug();
		var anObject = requireAnObject();
		var toPropertyKey = requireToPropertyKey();

		var $TypeError = TypeError;
		// eslint-disable-next-line es/no-object-defineproperty -- safe
		var $defineProperty = Object.defineProperty;
		// eslint-disable-next-line es/no-object-getownpropertydescriptor -- safe
		var $getOwnPropertyDescriptor = Object.getOwnPropertyDescriptor;
		var ENUMERABLE = 'enumerable';
		var CONFIGURABLE = 'configurable';
		var WRITABLE = 'writable';

		// `Object.defineProperty` method
		// https://tc39.es/ecma262/#sec-object.defineproperty
		objectDefineProperty.f = DESCRIPTORS ? V8_PROTOTYPE_DEFINE_BUG ? function defineProperty(O, P, Attributes) {
		  anObject(O);
		  P = toPropertyKey(P);
		  anObject(Attributes);
		  if (typeof O === 'function' && P === 'prototype' && 'value' in Attributes && WRITABLE in Attributes && !Attributes[WRITABLE]) {
		    var current = $getOwnPropertyDescriptor(O, P);
		    if (current && current[WRITABLE]) {
		      O[P] = Attributes.value;
		      Attributes = {
		        configurable: CONFIGURABLE in Attributes ? Attributes[CONFIGURABLE] : current[CONFIGURABLE],
		        enumerable: ENUMERABLE in Attributes ? Attributes[ENUMERABLE] : current[ENUMERABLE],
		        writable: false
		      };
		    }
		  } return $defineProperty(O, P, Attributes);
		} : $defineProperty : function defineProperty(O, P, Attributes) {
		  anObject(O);
		  P = toPropertyKey(P);
		  anObject(Attributes);
		  if (IE8_DOM_DEFINE) try {
		    return $defineProperty(O, P, Attributes);
		  } catch (error) { /* empty */ }
		  if ('get' in Attributes || 'set' in Attributes) throw new $TypeError('Accessors not supported');
		  if ('value' in Attributes) O[P] = Attributes.value;
		  return O;
		};
		return objectDefineProperty;
	}

	var createNonEnumerableProperty;
	var hasRequiredCreateNonEnumerableProperty;

	function requireCreateNonEnumerableProperty () {
		if (hasRequiredCreateNonEnumerableProperty) return createNonEnumerableProperty;
		hasRequiredCreateNonEnumerableProperty = 1;
		var DESCRIPTORS = requireDescriptors();
		var definePropertyModule = requireObjectDefineProperty();
		var createPropertyDescriptor = requireCreatePropertyDescriptor();

		createNonEnumerableProperty = DESCRIPTORS ? function (object, key, value) {
		  return definePropertyModule.f(object, key, createPropertyDescriptor(1, value));
		} : function (object, key, value) {
		  object[key] = value;
		  return object;
		};
		return createNonEnumerableProperty;
	}

	var makeBuiltIn = {exports: {}};

	var functionName;
	var hasRequiredFunctionName;

	function requireFunctionName () {
		if (hasRequiredFunctionName) return functionName;
		hasRequiredFunctionName = 1;
		var DESCRIPTORS = requireDescriptors();
		var hasOwn = requireHasOwnProperty();

		var FunctionPrototype = Function.prototype;
		// eslint-disable-next-line es/no-object-getownpropertydescriptor -- safe
		var getDescriptor = DESCRIPTORS && Object.getOwnPropertyDescriptor;

		var EXISTS = hasOwn(FunctionPrototype, 'name');
		// additional protection from minified / mangled / dropped function names
		var PROPER = EXISTS && function something() { /* empty */ }.name === 'something';
		var CONFIGURABLE = EXISTS && (!DESCRIPTORS || (DESCRIPTORS && getDescriptor(FunctionPrototype, 'name').configurable));

		functionName = {
		  EXISTS: EXISTS,
		  PROPER: PROPER,
		  CONFIGURABLE: CONFIGURABLE
		};
		return functionName;
	}

	var inspectSource;
	var hasRequiredInspectSource;

	function requireInspectSource () {
		if (hasRequiredInspectSource) return inspectSource;
		hasRequiredInspectSource = 1;
		var uncurryThis = requireFunctionUncurryThis();
		var isCallable = requireIsCallable();
		var store = requireSharedStore();

		var functionToString = uncurryThis(Function.toString);

		// this helper broken in `core-js@3.4.1-3.4.4`, so we can't use `shared` helper
		if (!isCallable(store.inspectSource)) {
		  store.inspectSource = function (it) {
		    return functionToString(it);
		  };
		}

		inspectSource = store.inspectSource;
		return inspectSource;
	}

	var weakMapBasicDetection;
	var hasRequiredWeakMapBasicDetection;

	function requireWeakMapBasicDetection () {
		if (hasRequiredWeakMapBasicDetection) return weakMapBasicDetection;
		hasRequiredWeakMapBasicDetection = 1;
		var globalThis = requireGlobalThis();
		var isCallable = requireIsCallable();

		var WeakMap = globalThis.WeakMap;

		weakMapBasicDetection = isCallable(WeakMap) && /native code/.test(String(WeakMap));
		return weakMapBasicDetection;
	}

	var sharedKey;
	var hasRequiredSharedKey;

	function requireSharedKey () {
		if (hasRequiredSharedKey) return sharedKey;
		hasRequiredSharedKey = 1;
		var shared = requireShared();
		var uid = requireUid();

		var keys = shared('keys');

		sharedKey = function (key) {
		  return keys[key] || (keys[key] = uid(key));
		};
		return sharedKey;
	}

	var hiddenKeys;
	var hasRequiredHiddenKeys;

	function requireHiddenKeys () {
		if (hasRequiredHiddenKeys) return hiddenKeys;
		hasRequiredHiddenKeys = 1;
		hiddenKeys = {};
		return hiddenKeys;
	}

	var internalState;
	var hasRequiredInternalState;

	function requireInternalState () {
		if (hasRequiredInternalState) return internalState;
		hasRequiredInternalState = 1;
		var NATIVE_WEAK_MAP = requireWeakMapBasicDetection();
		var globalThis = requireGlobalThis();
		var isObject = requireIsObject();
		var createNonEnumerableProperty = requireCreateNonEnumerableProperty();
		var hasOwn = requireHasOwnProperty();
		var shared = requireSharedStore();
		var sharedKey = requireSharedKey();
		var hiddenKeys = requireHiddenKeys();

		var OBJECT_ALREADY_INITIALIZED = 'Object already initialized';
		var TypeError = globalThis.TypeError;
		var WeakMap = globalThis.WeakMap;
		var set, get, has;

		var enforce = function (it) {
		  return has(it) ? get(it) : set(it, {});
		};

		var getterFor = function (TYPE) {
		  return function (it) {
		    var state;
		    if (!isObject(it) || (state = get(it)).type !== TYPE) {
		      throw new TypeError('Incompatible receiver, ' + TYPE + ' required');
		    } return state;
		  };
		};

		if (NATIVE_WEAK_MAP || shared.state) {
		  var store = shared.state || (shared.state = new WeakMap());
		  /* eslint-disable no-self-assign -- prototype methods protection */
		  store.get = store.get;
		  store.has = store.has;
		  store.set = store.set;
		  /* eslint-enable no-self-assign -- prototype methods protection */
		  set = function (it, metadata) {
		    if (store.has(it)) throw new TypeError(OBJECT_ALREADY_INITIALIZED);
		    metadata.facade = it;
		    store.set(it, metadata);
		    return metadata;
		  };
		  get = function (it) {
		    return store.get(it) || {};
		  };
		  has = function (it) {
		    return store.has(it);
		  };
		} else {
		  var STATE = sharedKey('state');
		  hiddenKeys[STATE] = true;
		  set = function (it, metadata) {
		    if (hasOwn(it, STATE)) throw new TypeError(OBJECT_ALREADY_INITIALIZED);
		    metadata.facade = it;
		    createNonEnumerableProperty(it, STATE, metadata);
		    return metadata;
		  };
		  get = function (it) {
		    return hasOwn(it, STATE) ? it[STATE] : {};
		  };
		  has = function (it) {
		    return hasOwn(it, STATE);
		  };
		}

		internalState = {
		  set: set,
		  get: get,
		  has: has,
		  enforce: enforce,
		  getterFor: getterFor
		};
		return internalState;
	}

	var hasRequiredMakeBuiltIn;

	function requireMakeBuiltIn () {
		if (hasRequiredMakeBuiltIn) return makeBuiltIn.exports;
		hasRequiredMakeBuiltIn = 1;
		var uncurryThis = requireFunctionUncurryThis();
		var fails = requireFails();
		var isCallable = requireIsCallable();
		var hasOwn = requireHasOwnProperty();
		var DESCRIPTORS = requireDescriptors();
		var CONFIGURABLE_FUNCTION_NAME = requireFunctionName().CONFIGURABLE;
		var inspectSource = requireInspectSource();
		var InternalStateModule = requireInternalState();

		var enforceInternalState = InternalStateModule.enforce;
		var getInternalState = InternalStateModule.get;
		var $String = String;
		// eslint-disable-next-line es/no-object-defineproperty -- safe
		var defineProperty = Object.defineProperty;
		var stringSlice = uncurryThis(''.slice);
		var replace = uncurryThis(''.replace);
		var join = uncurryThis([].join);

		var CONFIGURABLE_LENGTH = DESCRIPTORS && !fails(function () {
		  return defineProperty(function () { /* empty */ }, 'length', { value: 8 }).length !== 8;
		});

		var TEMPLATE = String(String).split('String');

		var makeBuiltIn$1 = makeBuiltIn.exports = function (value, name, options) {
		  if (stringSlice($String(name), 0, 7) === 'Symbol(') {
		    name = '[' + replace($String(name), /^Symbol\(([^)]*)\).*$/, '$1') + ']';
		  }
		  if (options && options.getter) name = 'get ' + name;
		  if (options && options.setter) name = 'set ' + name;
		  if (!hasOwn(value, 'name') || (CONFIGURABLE_FUNCTION_NAME && value.name !== name)) {
		    if (DESCRIPTORS) defineProperty(value, 'name', { value: name, configurable: true });
		    else value.name = name;
		  }
		  if (CONFIGURABLE_LENGTH && options && hasOwn(options, 'arity') && value.length !== options.arity) {
		    defineProperty(value, 'length', { value: options.arity });
		  }
		  try {
		    if (options && hasOwn(options, 'constructor') && options.constructor) {
		      if (DESCRIPTORS) defineProperty(value, 'prototype', { writable: false });
		    // in V8 ~ Chrome 53, prototypes of some methods, like `Array.prototype.values`, are non-writable
		    } else if (value.prototype) value.prototype = undefined;
		  } catch (error) { /* empty */ }
		  var state = enforceInternalState(value);
		  if (!hasOwn(state, 'source')) {
		    state.source = join(TEMPLATE, typeof name == 'string' ? name : '');
		  } return value;
		};

		// add fake Function#toString for correct work wrapped methods / constructors with methods like LoDash isNative
		// eslint-disable-next-line no-extend-native -- required
		Function.prototype.toString = makeBuiltIn$1(function toString() {
		  return isCallable(this) && getInternalState(this).source || inspectSource(this);
		}, 'toString');
		return makeBuiltIn.exports;
	}

	var defineBuiltIn;
	var hasRequiredDefineBuiltIn;

	function requireDefineBuiltIn () {
		if (hasRequiredDefineBuiltIn) return defineBuiltIn;
		hasRequiredDefineBuiltIn = 1;
		var isCallable = requireIsCallable();
		var definePropertyModule = requireObjectDefineProperty();
		var makeBuiltIn = requireMakeBuiltIn();
		var defineGlobalProperty = requireDefineGlobalProperty();

		defineBuiltIn = function (O, key, value, options) {
		  if (!options) options = {};
		  var simple = options.enumerable;
		  var name = options.name !== undefined ? options.name : key;
		  if (isCallable(value)) makeBuiltIn(value, name, options);
		  if (options.global) {
		    if (simple) O[key] = value;
		    else defineGlobalProperty(key, value);
		  } else {
		    try {
		      if (!options.unsafe) delete O[key];
		      else if (O[key]) simple = true;
		    } catch (error) { /* empty */ }
		    if (simple) O[key] = value;
		    else definePropertyModule.f(O, key, {
		      value: value,
		      enumerable: false,
		      configurable: !options.nonConfigurable,
		      writable: !options.nonWritable
		    });
		  } return O;
		};
		return defineBuiltIn;
	}

	var objectGetOwnPropertyNames = {};

	var mathTrunc;
	var hasRequiredMathTrunc;

	function requireMathTrunc () {
		if (hasRequiredMathTrunc) return mathTrunc;
		hasRequiredMathTrunc = 1;
		var ceil = Math.ceil;
		var floor = Math.floor;

		// `Math.trunc` method
		// https://tc39.es/ecma262/#sec-math.trunc
		// eslint-disable-next-line es/no-math-trunc -- safe
		mathTrunc = Math.trunc || function trunc(x) {
		  var n = +x;
		  return (n > 0 ? floor : ceil)(n);
		};
		return mathTrunc;
	}

	var toIntegerOrInfinity;
	var hasRequiredToIntegerOrInfinity;

	function requireToIntegerOrInfinity () {
		if (hasRequiredToIntegerOrInfinity) return toIntegerOrInfinity;
		hasRequiredToIntegerOrInfinity = 1;
		var trunc = requireMathTrunc();

		// `ToIntegerOrInfinity` abstract operation
		// https://tc39.es/ecma262/#sec-tointegerorinfinity
		toIntegerOrInfinity = function (argument) {
		  var number = +argument;
		  // eslint-disable-next-line no-self-compare -- NaN check
		  return number !== number || number === 0 ? 0 : trunc(number);
		};
		return toIntegerOrInfinity;
	}

	var toAbsoluteIndex;
	var hasRequiredToAbsoluteIndex;

	function requireToAbsoluteIndex () {
		if (hasRequiredToAbsoluteIndex) return toAbsoluteIndex;
		hasRequiredToAbsoluteIndex = 1;
		var toIntegerOrInfinity = requireToIntegerOrInfinity();

		var max = Math.max;
		var min = Math.min;

		// Helper for a popular repeating case of the spec:
		// Let integer be ? ToInteger(index).
		// If integer < 0, let result be max((length + integer), 0); else let result be min(integer, length).
		toAbsoluteIndex = function (index, length) {
		  var integer = toIntegerOrInfinity(index);
		  return integer < 0 ? max(integer + length, 0) : min(integer, length);
		};
		return toAbsoluteIndex;
	}

	var toLength;
	var hasRequiredToLength;

	function requireToLength () {
		if (hasRequiredToLength) return toLength;
		hasRequiredToLength = 1;
		var toIntegerOrInfinity = requireToIntegerOrInfinity();

		var min = Math.min;

		// `ToLength` abstract operation
		// https://tc39.es/ecma262/#sec-tolength
		toLength = function (argument) {
		  var len = toIntegerOrInfinity(argument);
		  return len > 0 ? min(len, 0x1FFFFFFFFFFFFF) : 0; // 2 ** 53 - 1 == 9007199254740991
		};
		return toLength;
	}

	var lengthOfArrayLike;
	var hasRequiredLengthOfArrayLike;

	function requireLengthOfArrayLike () {
		if (hasRequiredLengthOfArrayLike) return lengthOfArrayLike;
		hasRequiredLengthOfArrayLike = 1;
		var toLength = requireToLength();

		// `LengthOfArrayLike` abstract operation
		// https://tc39.es/ecma262/#sec-lengthofarraylike
		lengthOfArrayLike = function (obj) {
		  return toLength(obj.length);
		};
		return lengthOfArrayLike;
	}

	var arrayIncludes;
	var hasRequiredArrayIncludes;

	function requireArrayIncludes () {
		if (hasRequiredArrayIncludes) return arrayIncludes;
		hasRequiredArrayIncludes = 1;
		var toIndexedObject = requireToIndexedObject();
		var toAbsoluteIndex = requireToAbsoluteIndex();
		var lengthOfArrayLike = requireLengthOfArrayLike();

		// `Array.prototype.{ indexOf, includes }` methods implementation
		var createMethod = function (IS_INCLUDES) {
		  return function ($this, el, fromIndex) {
		    var O = toIndexedObject($this);
		    var length = lengthOfArrayLike(O);
		    if (length === 0) return !IS_INCLUDES && -1;
		    var index = toAbsoluteIndex(fromIndex, length);
		    var value;
		    // Array#includes uses SameValueZero equality algorithm
		    // eslint-disable-next-line no-self-compare -- NaN check
		    if (IS_INCLUDES && el !== el) while (length > index) {
		      value = O[index++];
		      // eslint-disable-next-line no-self-compare -- NaN check
		      if (value !== value) return true;
		    // Array#indexOf ignores holes, Array#includes - not
		    } else for (;length > index; index++) {
		      if ((IS_INCLUDES || index in O) && O[index] === el) return IS_INCLUDES || index || 0;
		    } return !IS_INCLUDES && -1;
		  };
		};

		arrayIncludes = {
		  // `Array.prototype.includes` method
		  // https://tc39.es/ecma262/#sec-array.prototype.includes
		  includes: createMethod(true),
		  // `Array.prototype.indexOf` method
		  // https://tc39.es/ecma262/#sec-array.prototype.indexof
		  indexOf: createMethod(false)
		};
		return arrayIncludes;
	}

	var objectKeysInternal;
	var hasRequiredObjectKeysInternal;

	function requireObjectKeysInternal () {
		if (hasRequiredObjectKeysInternal) return objectKeysInternal;
		hasRequiredObjectKeysInternal = 1;
		var uncurryThis = requireFunctionUncurryThis();
		var hasOwn = requireHasOwnProperty();
		var toIndexedObject = requireToIndexedObject();
		var indexOf = requireArrayIncludes().indexOf;
		var hiddenKeys = requireHiddenKeys();

		var push = uncurryThis([].push);

		objectKeysInternal = function (object, names) {
		  var O = toIndexedObject(object);
		  var i = 0;
		  var result = [];
		  var key;
		  for (key in O) !hasOwn(hiddenKeys, key) && hasOwn(O, key) && push(result, key);
		  // Don't enum bug & hidden keys
		  while (names.length > i) if (hasOwn(O, key = names[i++])) {
		    ~indexOf(result, key) || push(result, key);
		  }
		  return result;
		};
		return objectKeysInternal;
	}

	var enumBugKeys;
	var hasRequiredEnumBugKeys;

	function requireEnumBugKeys () {
		if (hasRequiredEnumBugKeys) return enumBugKeys;
		hasRequiredEnumBugKeys = 1;
		// IE8- don't enum bug keys
		enumBugKeys = [
		  'constructor',
		  'hasOwnProperty',
		  'isPrototypeOf',
		  'propertyIsEnumerable',
		  'toLocaleString',
		  'toString',
		  'valueOf'
		];
		return enumBugKeys;
	}

	var hasRequiredObjectGetOwnPropertyNames;

	function requireObjectGetOwnPropertyNames () {
		if (hasRequiredObjectGetOwnPropertyNames) return objectGetOwnPropertyNames;
		hasRequiredObjectGetOwnPropertyNames = 1;
		var internalObjectKeys = requireObjectKeysInternal();
		var enumBugKeys = requireEnumBugKeys();

		var hiddenKeys = enumBugKeys.concat('length', 'prototype');

		// `Object.getOwnPropertyNames` method
		// https://tc39.es/ecma262/#sec-object.getownpropertynames
		// eslint-disable-next-line es/no-object-getownpropertynames -- safe
		objectGetOwnPropertyNames.f = Object.getOwnPropertyNames || function getOwnPropertyNames(O) {
		  return internalObjectKeys(O, hiddenKeys);
		};
		return objectGetOwnPropertyNames;
	}

	var objectGetOwnPropertySymbols = {};

	var hasRequiredObjectGetOwnPropertySymbols;

	function requireObjectGetOwnPropertySymbols () {
		if (hasRequiredObjectGetOwnPropertySymbols) return objectGetOwnPropertySymbols;
		hasRequiredObjectGetOwnPropertySymbols = 1;
		// eslint-disable-next-line es/no-object-getownpropertysymbols -- safe
		objectGetOwnPropertySymbols.f = Object.getOwnPropertySymbols;
		return objectGetOwnPropertySymbols;
	}

	var ownKeys$1;
	var hasRequiredOwnKeys;

	function requireOwnKeys () {
		if (hasRequiredOwnKeys) return ownKeys$1;
		hasRequiredOwnKeys = 1;
		var getBuiltIn = requireGetBuiltIn();
		var uncurryThis = requireFunctionUncurryThis();
		var getOwnPropertyNamesModule = requireObjectGetOwnPropertyNames();
		var getOwnPropertySymbolsModule = requireObjectGetOwnPropertySymbols();
		var anObject = requireAnObject();

		var concat = uncurryThis([].concat);

		// all object keys, includes non-enumerable and symbols
		ownKeys$1 = getBuiltIn('Reflect', 'ownKeys') || function ownKeys(it) {
		  var keys = getOwnPropertyNamesModule.f(anObject(it));
		  var getOwnPropertySymbols = getOwnPropertySymbolsModule.f;
		  return getOwnPropertySymbols ? concat(keys, getOwnPropertySymbols(it)) : keys;
		};
		return ownKeys$1;
	}

	var copyConstructorProperties;
	var hasRequiredCopyConstructorProperties;

	function requireCopyConstructorProperties () {
		if (hasRequiredCopyConstructorProperties) return copyConstructorProperties;
		hasRequiredCopyConstructorProperties = 1;
		var hasOwn = requireHasOwnProperty();
		var ownKeys = requireOwnKeys();
		var getOwnPropertyDescriptorModule = requireObjectGetOwnPropertyDescriptor();
		var definePropertyModule = requireObjectDefineProperty();

		copyConstructorProperties = function (target, source, exceptions) {
		  var keys = ownKeys(source);
		  var defineProperty = definePropertyModule.f;
		  var getOwnPropertyDescriptor = getOwnPropertyDescriptorModule.f;
		  for (var i = 0; i < keys.length; i++) {
		    var key = keys[i];
		    if (!hasOwn(target, key) && !(exceptions && hasOwn(exceptions, key))) {
		      defineProperty(target, key, getOwnPropertyDescriptor(source, key));
		    }
		  }
		};
		return copyConstructorProperties;
	}

	var isForced_1;
	var hasRequiredIsForced;

	function requireIsForced () {
		if (hasRequiredIsForced) return isForced_1;
		hasRequiredIsForced = 1;
		var fails = requireFails();
		var isCallable = requireIsCallable();

		var replacement = /#|\.prototype\./;

		var isForced = function (feature, detection) {
		  var value = data[normalize(feature)];
		  return value === POLYFILL ? true
		    : value === NATIVE ? false
		    : isCallable(detection) ? fails(detection)
		    : !!detection;
		};

		var normalize = isForced.normalize = function (string) {
		  return String(string).replace(replacement, '.').toLowerCase();
		};

		var data = isForced.data = {};
		var NATIVE = isForced.NATIVE = 'N';
		var POLYFILL = isForced.POLYFILL = 'P';

		isForced_1 = isForced;
		return isForced_1;
	}

	var _export;
	var hasRequired_export;

	function require_export () {
		if (hasRequired_export) return _export;
		hasRequired_export = 1;
		var globalThis = requireGlobalThis();
		var getOwnPropertyDescriptor = requireObjectGetOwnPropertyDescriptor().f;
		var createNonEnumerableProperty = requireCreateNonEnumerableProperty();
		var defineBuiltIn = requireDefineBuiltIn();
		var defineGlobalProperty = requireDefineGlobalProperty();
		var copyConstructorProperties = requireCopyConstructorProperties();
		var isForced = requireIsForced();

		/*
		  options.target         - name of the target object
		  options.global         - target is the global object
		  options.stat           - export as static methods of target
		  options.proto          - export as prototype methods of target
		  options.real           - real prototype method for the `pure` version
		  options.forced         - export even if the native feature is available
		  options.bind           - bind methods to the target, required for the `pure` version
		  options.wrap           - wrap constructors to preventing global pollution, required for the `pure` version
		  options.unsafe         - use the simple assignment of property instead of delete + defineProperty
		  options.sham           - add a flag to not completely full polyfills
		  options.enumerable     - export as enumerable property
		  options.dontCallGetSet - prevent calling a getter on target
		  options.name           - the .name of the function if it does not match the key
		*/
		_export = function (options, source) {
		  var TARGET = options.target;
		  var GLOBAL = options.global;
		  var STATIC = options.stat;
		  var FORCED, target, key, targetProperty, sourceProperty, descriptor;
		  if (GLOBAL) {
		    target = globalThis;
		  } else if (STATIC) {
		    target = globalThis[TARGET] || defineGlobalProperty(TARGET, {});
		  } else {
		    target = globalThis[TARGET] && globalThis[TARGET].prototype;
		  }
		  if (target) for (key in source) {
		    sourceProperty = source[key];
		    if (options.dontCallGetSet) {
		      descriptor = getOwnPropertyDescriptor(target, key);
		      targetProperty = descriptor && descriptor.value;
		    } else targetProperty = target[key];
		    FORCED = isForced(GLOBAL ? key : TARGET + (STATIC ? '.' : '#') + key, options.forced);
		    // contained in target
		    if (!FORCED && targetProperty !== undefined) {
		      if (typeof sourceProperty == typeof targetProperty) continue;
		      copyConstructorProperties(sourceProperty, targetProperty);
		    }
		    // add a flag to not completely full polyfills
		    if (options.sham || (targetProperty && targetProperty.sham)) {
		      createNonEnumerableProperty(sourceProperty, 'sham', true);
		    }
		    defineBuiltIn(target, key, sourceProperty, options);
		  }
		};
		return _export;
	}

	var functionUncurryThisClause;
	var hasRequiredFunctionUncurryThisClause;

	function requireFunctionUncurryThisClause () {
		if (hasRequiredFunctionUncurryThisClause) return functionUncurryThisClause;
		hasRequiredFunctionUncurryThisClause = 1;
		var classofRaw = requireClassofRaw();
		var uncurryThis = requireFunctionUncurryThis();

		functionUncurryThisClause = function (fn) {
		  // Nashorn bug:
		  //   https://github.com/zloirock/core-js/issues/1128
		  //   https://github.com/zloirock/core-js/issues/1130
		  if (classofRaw(fn) === 'Function') return uncurryThis(fn);
		};
		return functionUncurryThisClause;
	}

	var functionBindContext;
	var hasRequiredFunctionBindContext;

	function requireFunctionBindContext () {
		if (hasRequiredFunctionBindContext) return functionBindContext;
		hasRequiredFunctionBindContext = 1;
		var uncurryThis = requireFunctionUncurryThisClause();
		var aCallable = requireACallable();
		var NATIVE_BIND = requireFunctionBindNative();

		var bind = uncurryThis(uncurryThis.bind);

		// optional / simple context binding
		functionBindContext = function (fn, that) {
		  aCallable(fn);
		  return that === undefined ? fn : NATIVE_BIND ? bind(fn, that) : function (/* ...args */) {
		    return fn.apply(that, arguments);
		  };
		};
		return functionBindContext;
	}

	var iteratorClose;
	var hasRequiredIteratorClose;

	function requireIteratorClose () {
		if (hasRequiredIteratorClose) return iteratorClose;
		hasRequiredIteratorClose = 1;
		var call = requireFunctionCall();
		var anObject = requireAnObject();
		var getMethod = requireGetMethod();

		iteratorClose = function (iterator, kind, value) {
		  var innerResult, innerError;
		  anObject(iterator);
		  try {
		    innerResult = getMethod(iterator, 'return');
		    if (!innerResult) {
		      if (kind === 'throw') throw value;
		      return value;
		    }
		    innerResult = call(innerResult, iterator);
		  } catch (error) {
		    innerError = true;
		    innerResult = error;
		  }
		  if (kind === 'throw') throw value;
		  if (innerError) throw innerResult;
		  anObject(innerResult);
		  return value;
		};
		return iteratorClose;
	}

	var callWithSafeIterationClosing;
	var hasRequiredCallWithSafeIterationClosing;

	function requireCallWithSafeIterationClosing () {
		if (hasRequiredCallWithSafeIterationClosing) return callWithSafeIterationClosing;
		hasRequiredCallWithSafeIterationClosing = 1;
		var anObject = requireAnObject();
		var iteratorClose = requireIteratorClose();

		// call something on iterator step with safe closing on error
		callWithSafeIterationClosing = function (iterator, fn, value, ENTRIES) {
		  try {
		    return ENTRIES ? fn(anObject(value)[0], value[1]) : fn(value);
		  } catch (error) {
		    iteratorClose(iterator, 'throw', error);
		  }
		};
		return callWithSafeIterationClosing;
	}

	var iterators;
	var hasRequiredIterators;

	function requireIterators () {
		if (hasRequiredIterators) return iterators;
		hasRequiredIterators = 1;
		iterators = {};
		return iterators;
	}

	var isArrayIteratorMethod;
	var hasRequiredIsArrayIteratorMethod;

	function requireIsArrayIteratorMethod () {
		if (hasRequiredIsArrayIteratorMethod) return isArrayIteratorMethod;
		hasRequiredIsArrayIteratorMethod = 1;
		var wellKnownSymbol = requireWellKnownSymbol();
		var Iterators = requireIterators();

		var ITERATOR = wellKnownSymbol('iterator');
		var ArrayPrototype = Array.prototype;

		// check on default Array iterator
		isArrayIteratorMethod = function (it) {
		  return it !== undefined && (Iterators.Array === it || ArrayPrototype[ITERATOR] === it);
		};
		return isArrayIteratorMethod;
	}

	var toStringTagSupport;
	var hasRequiredToStringTagSupport;

	function requireToStringTagSupport () {
		if (hasRequiredToStringTagSupport) return toStringTagSupport;
		hasRequiredToStringTagSupport = 1;
		var wellKnownSymbol = requireWellKnownSymbol();

		var TO_STRING_TAG = wellKnownSymbol('toStringTag');
		var test = {};
		// eslint-disable-next-line unicorn/no-immediate-mutation -- ES3 syntax limitation
		test[TO_STRING_TAG] = 'z';

		toStringTagSupport = String(test) === '[object z]';
		return toStringTagSupport;
	}

	var classof;
	var hasRequiredClassof;

	function requireClassof () {
		if (hasRequiredClassof) return classof;
		hasRequiredClassof = 1;
		var TO_STRING_TAG_SUPPORT = requireToStringTagSupport();
		var isCallable = requireIsCallable();
		var classofRaw = requireClassofRaw();
		var wellKnownSymbol = requireWellKnownSymbol();

		var TO_STRING_TAG = wellKnownSymbol('toStringTag');
		var $Object = Object;

		// ES3 wrong here
		var CORRECT_ARGUMENTS = classofRaw(function () { return arguments; }()) === 'Arguments';

		// fallback for IE11 Script Access Denied error
		var tryGet = function (it, key) {
		  try {
		    return it[key];
		  } catch (error) { /* empty */ }
		};

		// getting tag from ES6+ `Object.prototype.toString`
		classof = TO_STRING_TAG_SUPPORT ? classofRaw : function (it) {
		  var O, tag, result;
		  return it === undefined ? 'Undefined' : it === null ? 'Null'
		    // @@toStringTag case
		    : typeof (tag = tryGet(O = $Object(it), TO_STRING_TAG)) == 'string' ? tag
		    // builtinTag case
		    : CORRECT_ARGUMENTS ? classofRaw(O)
		    // ES3 arguments fallback
		    : (result = classofRaw(O)) === 'Object' && isCallable(O.callee) ? 'Arguments' : result;
		};
		return classof;
	}

	var isConstructor;
	var hasRequiredIsConstructor;

	function requireIsConstructor () {
		if (hasRequiredIsConstructor) return isConstructor;
		hasRequiredIsConstructor = 1;
		var uncurryThis = requireFunctionUncurryThis();
		var fails = requireFails();
		var isCallable = requireIsCallable();
		var classof = requireClassof();
		var getBuiltIn = requireGetBuiltIn();
		var inspectSource = requireInspectSource();

		var noop = function () { /* empty */ };
		var construct = getBuiltIn('Reflect', 'construct');
		var constructorRegExp = /^\s*(?:class|function)\b/;
		var exec = uncurryThis(constructorRegExp.exec);
		var INCORRECT_TO_STRING = !constructorRegExp.test(noop);

		var isConstructorModern = function isConstructor(argument) {
		  if (!isCallable(argument)) return false;
		  try {
		    construct(noop, [], argument);
		    return true;
		  } catch (error) {
		    return false;
		  }
		};

		var isConstructorLegacy = function isConstructor(argument) {
		  if (!isCallable(argument)) return false;
		  switch (classof(argument)) {
		    case 'AsyncFunction':
		    case 'GeneratorFunction':
		    case 'AsyncGeneratorFunction': return false;
		  }
		  try {
		    // we can't check .prototype since constructors produced by .bind haven't it
		    // `Function#toString` throws on some built-it function in some legacy engines
		    // (for example, `DOMQuad` and similar in FF41-)
		    return INCORRECT_TO_STRING || !!exec(constructorRegExp, inspectSource(argument));
		  } catch (error) {
		    return true;
		  }
		};

		isConstructorLegacy.sham = true;

		// `IsConstructor` abstract operation
		// https://tc39.es/ecma262/#sec-isconstructor
		isConstructor = !construct || fails(function () {
		  var called;
		  return isConstructorModern(isConstructorModern.call)
		    || !isConstructorModern(Object)
		    || !isConstructorModern(function () { called = true; })
		    || called;
		}) ? isConstructorLegacy : isConstructorModern;
		return isConstructor;
	}

	var createProperty;
	var hasRequiredCreateProperty;

	function requireCreateProperty () {
		if (hasRequiredCreateProperty) return createProperty;
		hasRequiredCreateProperty = 1;
		var DESCRIPTORS = requireDescriptors();
		var definePropertyModule = requireObjectDefineProperty();
		var createPropertyDescriptor = requireCreatePropertyDescriptor();

		createProperty = function (object, key, value) {
		  if (DESCRIPTORS) definePropertyModule.f(object, key, createPropertyDescriptor(0, value));
		  else object[key] = value;
		};
		return createProperty;
	}

	var isArray;
	var hasRequiredIsArray;

	function requireIsArray () {
		if (hasRequiredIsArray) return isArray;
		hasRequiredIsArray = 1;
		var classof = requireClassofRaw();

		// `IsArray` abstract operation
		// https://tc39.es/ecma262/#sec-isarray
		// eslint-disable-next-line es/no-array-isarray -- safe
		isArray = Array.isArray || function isArray(argument) {
		  return classof(argument) === 'Array';
		};
		return isArray;
	}

	var arraySetLength;
	var hasRequiredArraySetLength;

	function requireArraySetLength () {
		if (hasRequiredArraySetLength) return arraySetLength;
		hasRequiredArraySetLength = 1;
		var DESCRIPTORS = requireDescriptors();
		var isArray = requireIsArray();

		var $TypeError = TypeError;
		// eslint-disable-next-line es/no-object-getownpropertydescriptor -- safe
		var getOwnPropertyDescriptor = Object.getOwnPropertyDescriptor;

		// Safari < 13 does not throw an error in this case
		var SILENT_ON_NON_WRITABLE_LENGTH_SET = DESCRIPTORS && !function () {
		  // makes no sense without proper strict mode support
		  if (this !== undefined) return true;
		  try {
		    // eslint-disable-next-line es/no-object-defineproperty -- safe
		    Object.defineProperty([], 'length', { writable: false }).length = 1;
		  } catch (error) {
		    return error instanceof TypeError;
		  }
		}();

		arraySetLength = SILENT_ON_NON_WRITABLE_LENGTH_SET ? function (O, length) {
		  if (isArray(O) && !getOwnPropertyDescriptor(O, 'length').writable) {
		    throw new $TypeError('Cannot set read only .length');
		  } return O.length = length;
		} : function (O, length) {
		  return O.length = length;
		};
		return arraySetLength;
	}

	var getIteratorMethod;
	var hasRequiredGetIteratorMethod;

	function requireGetIteratorMethod () {
		if (hasRequiredGetIteratorMethod) return getIteratorMethod;
		hasRequiredGetIteratorMethod = 1;
		var classof = requireClassof();
		var getMethod = requireGetMethod();
		var isNullOrUndefined = requireIsNullOrUndefined();
		var Iterators = requireIterators();
		var wellKnownSymbol = requireWellKnownSymbol();

		var ITERATOR = wellKnownSymbol('iterator');

		getIteratorMethod = function (it) {
		  if (!isNullOrUndefined(it)) return getMethod(it, ITERATOR)
		    || getMethod(it, '@@iterator')
		    || Iterators[classof(it)];
		};
		return getIteratorMethod;
	}

	var getIterator;
	var hasRequiredGetIterator;

	function requireGetIterator () {
		if (hasRequiredGetIterator) return getIterator;
		hasRequiredGetIterator = 1;
		var call = requireFunctionCall();
		var aCallable = requireACallable();
		var anObject = requireAnObject();
		var tryToString = requireTryToString();
		var getIteratorMethod = requireGetIteratorMethod();

		var $TypeError = TypeError;

		getIterator = function (argument, usingIterator) {
		  var iteratorMethod = arguments.length < 2 ? getIteratorMethod(argument) : usingIterator;
		  if (aCallable(iteratorMethod)) return anObject(call(iteratorMethod, argument));
		  throw new $TypeError(tryToString(argument) + ' is not iterable');
		};
		return getIterator;
	}

	var arrayFrom;
	var hasRequiredArrayFrom;

	function requireArrayFrom () {
		if (hasRequiredArrayFrom) return arrayFrom;
		hasRequiredArrayFrom = 1;
		var bind = requireFunctionBindContext();
		var call = requireFunctionCall();
		var toObject = requireToObject();
		var callWithSafeIterationClosing = requireCallWithSafeIterationClosing();
		var isArrayIteratorMethod = requireIsArrayIteratorMethod();
		var isConstructor = requireIsConstructor();
		var lengthOfArrayLike = requireLengthOfArrayLike();
		var createProperty = requireCreateProperty();
		var setArrayLength = requireArraySetLength();
		var getIterator = requireGetIterator();
		var getIteratorMethod = requireGetIteratorMethod();
		var iteratorClose = requireIteratorClose();

		var $Array = Array;

		// `Array.from` method implementation
		// https://tc39.es/ecma262/#sec-array.from
		arrayFrom = function from(arrayLike /* , mapfn = undefined, thisArg = undefined */) {
		  var IS_CONSTRUCTOR = isConstructor(this);
		  var argumentsLength = arguments.length;
		  var mapfn = argumentsLength > 1 ? arguments[1] : undefined;
		  var mapping = mapfn !== undefined;
		  if (mapping) mapfn = bind(mapfn, argumentsLength > 2 ? arguments[2] : undefined);
		  var O = toObject(arrayLike);
		  var iteratorMethod = getIteratorMethod(O);
		  var index = 0;
		  var length, result, step, iterator, next, value;
		  // if the target is not iterable or it's an array with the default iterator - use a simple case
		  if (iteratorMethod && !(this === $Array && isArrayIteratorMethod(iteratorMethod))) {
		    result = IS_CONSTRUCTOR ? new this() : [];
		    iterator = getIterator(O, iteratorMethod);
		    next = iterator.next;
		    for (;!(step = call(next, iterator)).done; index++) {
		      value = mapping ? callWithSafeIterationClosing(iterator, mapfn, [step.value, index], true) : step.value;
		      try {
		        createProperty(result, index, value);
		      } catch (error) {
		        iteratorClose(iterator, 'throw', error);
		      }
		    }
		  } else {
		    length = lengthOfArrayLike(O);
		    result = IS_CONSTRUCTOR ? new this(length) : $Array(length);
		    for (;length > index; index++) {
		      value = mapping ? mapfn(O[index], index) : O[index];
		      createProperty(result, index, value);
		    }
		  }
		  setArrayLength(result, index);
		  return result;
		};
		return arrayFrom;
	}

	var checkCorrectnessOfIteration;
	var hasRequiredCheckCorrectnessOfIteration;

	function requireCheckCorrectnessOfIteration () {
		if (hasRequiredCheckCorrectnessOfIteration) return checkCorrectnessOfIteration;
		hasRequiredCheckCorrectnessOfIteration = 1;
		var wellKnownSymbol = requireWellKnownSymbol();

		var ITERATOR = wellKnownSymbol('iterator');
		var SAFE_CLOSING = false;

		try {
		  var called = 0;
		  var iteratorWithReturn = {
		    next: function () {
		      return { done: !!called++ };
		    },
		    'return': function () {
		      SAFE_CLOSING = true;
		    }
		  };
		  // eslint-disable-next-line unicorn/no-immediate-mutation -- ES3 syntax limitation
		  iteratorWithReturn[ITERATOR] = function () {
		    return this;
		  };
		  // eslint-disable-next-line es/no-array-from, no-throw-literal -- required for testing
		  Array.from(iteratorWithReturn, function () { throw 2; });
		} catch (error) { /* empty */ }

		checkCorrectnessOfIteration = function (exec, SKIP_CLOSING) {
		  try {
		    if (!SKIP_CLOSING && !SAFE_CLOSING) return false;
		  } catch (error) { return false; } // workaround of old WebKit + `eval` bug
		  var ITERATION_SUPPORT = false;
		  try {
		    var object = {};
		    // eslint-disable-next-line unicorn/no-immediate-mutation -- ES3 syntax limitation
		    object[ITERATOR] = function () {
		      return {
		        next: function () {
		          return { done: ITERATION_SUPPORT = true };
		        }
		      };
		    };
		    exec(object);
		  } catch (error) { /* empty */ }
		  return ITERATION_SUPPORT;
		};
		return checkCorrectnessOfIteration;
	}

	var hasRequiredEs_array_from;

	function requireEs_array_from () {
		if (hasRequiredEs_array_from) return es_array_from;
		hasRequiredEs_array_from = 1;
		var $ = require_export();
		var from = requireArrayFrom();
		var checkCorrectnessOfIteration = requireCheckCorrectnessOfIteration();

		var INCORRECT_ITERATION = !checkCorrectnessOfIteration(function (iterable) {
		  // eslint-disable-next-line es/no-array-from -- required for testing
		  Array.from(iterable);
		});

		// `Array.from` method
		// https://tc39.es/ecma262/#sec-array.from
		$({ target: 'Array', stat: true, forced: INCORRECT_ITERATION }, {
		  from: from
		});
		return es_array_from;
	}

	requireEs_array_from();

	var objectDefineProperties = {};

	var objectKeys;
	var hasRequiredObjectKeys;

	function requireObjectKeys () {
		if (hasRequiredObjectKeys) return objectKeys;
		hasRequiredObjectKeys = 1;
		var internalObjectKeys = requireObjectKeysInternal();
		var enumBugKeys = requireEnumBugKeys();

		// `Object.keys` method
		// https://tc39.es/ecma262/#sec-object.keys
		// eslint-disable-next-line es/no-object-keys -- safe
		objectKeys = Object.keys || function keys(O) {
		  return internalObjectKeys(O, enumBugKeys);
		};
		return objectKeys;
	}

	var hasRequiredObjectDefineProperties;

	function requireObjectDefineProperties () {
		if (hasRequiredObjectDefineProperties) return objectDefineProperties;
		hasRequiredObjectDefineProperties = 1;
		var DESCRIPTORS = requireDescriptors();
		var V8_PROTOTYPE_DEFINE_BUG = requireV8PrototypeDefineBug();
		var definePropertyModule = requireObjectDefineProperty();
		var anObject = requireAnObject();
		var toIndexedObject = requireToIndexedObject();
		var objectKeys = requireObjectKeys();

		// `Object.defineProperties` method
		// https://tc39.es/ecma262/#sec-object.defineproperties
		// eslint-disable-next-line es/no-object-defineproperties -- safe
		objectDefineProperties.f = DESCRIPTORS && !V8_PROTOTYPE_DEFINE_BUG ? Object.defineProperties : function defineProperties(O, Properties) {
		  anObject(O);
		  var props = toIndexedObject(Properties);
		  var keys = objectKeys(Properties);
		  var length = keys.length;
		  var index = 0;
		  var key;
		  while (length > index) definePropertyModule.f(O, key = keys[index++], props[key]);
		  return O;
		};
		return objectDefineProperties;
	}

	var html;
	var hasRequiredHtml;

	function requireHtml () {
		if (hasRequiredHtml) return html;
		hasRequiredHtml = 1;
		var getBuiltIn = requireGetBuiltIn();

		html = getBuiltIn('document', 'documentElement');
		return html;
	}

	var objectCreate;
	var hasRequiredObjectCreate;

	function requireObjectCreate () {
		if (hasRequiredObjectCreate) return objectCreate;
		hasRequiredObjectCreate = 1;
		/* global ActiveXObject -- old IE, WSH */
		var anObject = requireAnObject();
		var definePropertiesModule = requireObjectDefineProperties();
		var enumBugKeys = requireEnumBugKeys();
		var hiddenKeys = requireHiddenKeys();
		var html = requireHtml();
		var documentCreateElement = requireDocumentCreateElement();
		var sharedKey = requireSharedKey();

		var GT = '>';
		var LT = '<';
		var PROTOTYPE = 'prototype';
		var SCRIPT = 'script';
		var IE_PROTO = sharedKey('IE_PROTO');

		var EmptyConstructor = function () { /* empty */ };

		var scriptTag = function (content) {
		  return LT + SCRIPT + GT + content + LT + '/' + SCRIPT + GT;
		};

		// Create object with fake `null` prototype: use ActiveX Object with cleared prototype
		var NullProtoObjectViaActiveX = function (activeXDocument) {
		  activeXDocument.write(scriptTag(''));
		  activeXDocument.close();
		  var temp = activeXDocument.parentWindow.Object;
		  // eslint-disable-next-line no-useless-assignment -- avoid memory leak
		  activeXDocument = null;
		  return temp;
		};

		// Create object with fake `null` prototype: use iframe Object with cleared prototype
		var NullProtoObjectViaIFrame = function () {
		  // Thrash, waste and sodomy: IE GC bug
		  var iframe = documentCreateElement('iframe');
		  var JS = 'java' + SCRIPT + ':';
		  var iframeDocument;
		  iframe.style.display = 'none';
		  html.appendChild(iframe);
		  // https://github.com/zloirock/core-js/issues/475
		  iframe.src = String(JS);
		  iframeDocument = iframe.contentWindow.document;
		  iframeDocument.open();
		  iframeDocument.write(scriptTag('document.F=Object'));
		  iframeDocument.close();
		  return iframeDocument.F;
		};

		// Check for document.domain and active x support
		// No need to use active x approach when document.domain is not set
		// see https://github.com/es-shims/es5-shim/issues/150
		// variation of https://github.com/kitcambridge/es5-shim/commit/4f738ac066346
		// avoid IE GC bug
		var activeXDocument;
		var NullProtoObject = function () {
		  try {
		    activeXDocument = new ActiveXObject('htmlfile');
		  } catch (error) { /* ignore */ }
		  NullProtoObject = typeof document != 'undefined'
		    ? document.domain && activeXDocument
		      ? NullProtoObjectViaActiveX(activeXDocument) // old IE
		      : NullProtoObjectViaIFrame()
		    : NullProtoObjectViaActiveX(activeXDocument); // WSH
		  var length = enumBugKeys.length;
		  while (length--) delete NullProtoObject[PROTOTYPE][enumBugKeys[length]];
		  return NullProtoObject();
		};

		hiddenKeys[IE_PROTO] = true;

		// `Object.create` method
		// https://tc39.es/ecma262/#sec-object.create
		// eslint-disable-next-line es/no-object-create -- safe
		objectCreate = Object.create || function create(O, Properties) {
		  var result;
		  if (O !== null) {
		    EmptyConstructor[PROTOTYPE] = anObject(O);
		    result = new EmptyConstructor();
		    EmptyConstructor[PROTOTYPE] = null;
		    // add "__proto__" for Object.getPrototypeOf polyfill
		    result[IE_PROTO] = O;
		  } else result = NullProtoObject();
		  return Properties === undefined ? result : definePropertiesModule.f(result, Properties);
		};
		return objectCreate;
	}

	var addToUnscopables;
	var hasRequiredAddToUnscopables;

	function requireAddToUnscopables () {
		if (hasRequiredAddToUnscopables) return addToUnscopables;
		hasRequiredAddToUnscopables = 1;
		var wellKnownSymbol = requireWellKnownSymbol();
		var create = requireObjectCreate();
		var defineProperty = requireObjectDefineProperty().f;

		var UNSCOPABLES = wellKnownSymbol('unscopables');
		var ArrayPrototype = Array.prototype;

		// Array.prototype[@@unscopables]
		// https://tc39.es/ecma262/#sec-array.prototype-@@unscopables
		if (ArrayPrototype[UNSCOPABLES] === undefined) {
		  defineProperty(ArrayPrototype, UNSCOPABLES, {
		    configurable: true,
		    value: create(null)
		  });
		}

		// add a key to Array.prototype[@@unscopables]
		addToUnscopables = function (key) {
		  ArrayPrototype[UNSCOPABLES][key] = true;
		};
		return addToUnscopables;
	}

	var correctPrototypeGetter;
	var hasRequiredCorrectPrototypeGetter;

	function requireCorrectPrototypeGetter () {
		if (hasRequiredCorrectPrototypeGetter) return correctPrototypeGetter;
		hasRequiredCorrectPrototypeGetter = 1;
		var fails = requireFails();

		correctPrototypeGetter = !fails(function () {
		  function F() { /* empty */ }
		  F.prototype.constructor = null;
		  // eslint-disable-next-line es/no-object-getprototypeof -- required for testing
		  return Object.getPrototypeOf(new F()) !== F.prototype;
		});
		return correctPrototypeGetter;
	}

	var objectGetPrototypeOf;
	var hasRequiredObjectGetPrototypeOf;

	function requireObjectGetPrototypeOf () {
		if (hasRequiredObjectGetPrototypeOf) return objectGetPrototypeOf;
		hasRequiredObjectGetPrototypeOf = 1;
		var hasOwn = requireHasOwnProperty();
		var isCallable = requireIsCallable();
		var toObject = requireToObject();
		var sharedKey = requireSharedKey();
		var CORRECT_PROTOTYPE_GETTER = requireCorrectPrototypeGetter();

		var IE_PROTO = sharedKey('IE_PROTO');
		var $Object = Object;
		var ObjectPrototype = $Object.prototype;

		// `Object.getPrototypeOf` method
		// https://tc39.es/ecma262/#sec-object.getprototypeof
		// eslint-disable-next-line es/no-object-getprototypeof -- safe
		objectGetPrototypeOf = CORRECT_PROTOTYPE_GETTER ? $Object.getPrototypeOf : function (O) {
		  var object = toObject(O);
		  if (hasOwn(object, IE_PROTO)) return object[IE_PROTO];
		  var constructor = object.constructor;
		  if (isCallable(constructor) && object instanceof constructor) {
		    return constructor.prototype;
		  } return object instanceof $Object ? ObjectPrototype : null;
		};
		return objectGetPrototypeOf;
	}

	var iteratorsCore;
	var hasRequiredIteratorsCore;

	function requireIteratorsCore () {
		if (hasRequiredIteratorsCore) return iteratorsCore;
		hasRequiredIteratorsCore = 1;
		var fails = requireFails();
		var isCallable = requireIsCallable();
		var isObject = requireIsObject();
		var create = requireObjectCreate();
		var getPrototypeOf = requireObjectGetPrototypeOf();
		var defineBuiltIn = requireDefineBuiltIn();
		var wellKnownSymbol = requireWellKnownSymbol();
		var IS_PURE = requireIsPure();

		var ITERATOR = wellKnownSymbol('iterator');
		var BUGGY_SAFARI_ITERATORS = false;

		// `%IteratorPrototype%` object
		// https://tc39.es/ecma262/#sec-%iteratorprototype%-object
		var IteratorPrototype, PrototypeOfArrayIteratorPrototype, arrayIterator;

		/* eslint-disable es/no-array-prototype-keys -- safe */
		if ([].keys) {
		  arrayIterator = [].keys();
		  // Safari 8 has buggy iterators w/o `next`
		  if (!('next' in arrayIterator)) BUGGY_SAFARI_ITERATORS = true;
		  else {
		    PrototypeOfArrayIteratorPrototype = getPrototypeOf(getPrototypeOf(arrayIterator));
		    if (PrototypeOfArrayIteratorPrototype !== Object.prototype) IteratorPrototype = PrototypeOfArrayIteratorPrototype;
		  }
		}

		var NEW_ITERATOR_PROTOTYPE = !isObject(IteratorPrototype) || fails(function () {
		  var test = {};
		  // FF44- legacy iterators case
		  return IteratorPrototype[ITERATOR].call(test) !== test;
		});

		if (NEW_ITERATOR_PROTOTYPE) IteratorPrototype = {};
		else if (IS_PURE) IteratorPrototype = create(IteratorPrototype);

		// `%IteratorPrototype%[@@iterator]()` method
		// https://tc39.es/ecma262/#sec-%iteratorprototype%-@@iterator
		if (!isCallable(IteratorPrototype[ITERATOR])) {
		  defineBuiltIn(IteratorPrototype, ITERATOR, function () {
		    return this;
		  });
		}

		iteratorsCore = {
		  IteratorPrototype: IteratorPrototype,
		  BUGGY_SAFARI_ITERATORS: BUGGY_SAFARI_ITERATORS
		};
		return iteratorsCore;
	}

	var setToStringTag;
	var hasRequiredSetToStringTag;

	function requireSetToStringTag () {
		if (hasRequiredSetToStringTag) return setToStringTag;
		hasRequiredSetToStringTag = 1;
		var defineProperty = requireObjectDefineProperty().f;
		var hasOwn = requireHasOwnProperty();
		var wellKnownSymbol = requireWellKnownSymbol();

		var TO_STRING_TAG = wellKnownSymbol('toStringTag');

		setToStringTag = function (target, TAG, STATIC) {
		  if (target && !STATIC) target = target.prototype;
		  if (target && !hasOwn(target, TO_STRING_TAG)) {
		    defineProperty(target, TO_STRING_TAG, { configurable: true, value: TAG });
		  }
		};
		return setToStringTag;
	}

	var iteratorCreateConstructor;
	var hasRequiredIteratorCreateConstructor;

	function requireIteratorCreateConstructor () {
		if (hasRequiredIteratorCreateConstructor) return iteratorCreateConstructor;
		hasRequiredIteratorCreateConstructor = 1;
		var IteratorPrototype = requireIteratorsCore().IteratorPrototype;
		var create = requireObjectCreate();
		var createPropertyDescriptor = requireCreatePropertyDescriptor();
		var setToStringTag = requireSetToStringTag();
		var Iterators = requireIterators();

		var returnThis = function () { return this; };

		iteratorCreateConstructor = function (IteratorConstructor, NAME, next, ENUMERABLE_NEXT) {
		  var TO_STRING_TAG = NAME + ' Iterator';
		  IteratorConstructor.prototype = create(IteratorPrototype, { next: createPropertyDescriptor(+!ENUMERABLE_NEXT, next) });
		  setToStringTag(IteratorConstructor, TO_STRING_TAG, false, true);
		  Iterators[TO_STRING_TAG] = returnThis;
		  return IteratorConstructor;
		};
		return iteratorCreateConstructor;
	}

	var functionUncurryThisAccessor;
	var hasRequiredFunctionUncurryThisAccessor;

	function requireFunctionUncurryThisAccessor () {
		if (hasRequiredFunctionUncurryThisAccessor) return functionUncurryThisAccessor;
		hasRequiredFunctionUncurryThisAccessor = 1;
		var uncurryThis = requireFunctionUncurryThis();
		var aCallable = requireACallable();

		functionUncurryThisAccessor = function (object, key, method) {
		  try {
		    // eslint-disable-next-line es/no-object-getownpropertydescriptor -- safe
		    return uncurryThis(aCallable(Object.getOwnPropertyDescriptor(object, key)[method]));
		  } catch (error) { /* empty */ }
		};
		return functionUncurryThisAccessor;
	}

	var isPossiblePrototype;
	var hasRequiredIsPossiblePrototype;

	function requireIsPossiblePrototype () {
		if (hasRequiredIsPossiblePrototype) return isPossiblePrototype;
		hasRequiredIsPossiblePrototype = 1;
		var isObject = requireIsObject();

		isPossiblePrototype = function (argument) {
		  return isObject(argument) || argument === null;
		};
		return isPossiblePrototype;
	}

	var aPossiblePrototype;
	var hasRequiredAPossiblePrototype;

	function requireAPossiblePrototype () {
		if (hasRequiredAPossiblePrototype) return aPossiblePrototype;
		hasRequiredAPossiblePrototype = 1;
		var isPossiblePrototype = requireIsPossiblePrototype();

		var $String = String;
		var $TypeError = TypeError;

		aPossiblePrototype = function (argument) {
		  if (isPossiblePrototype(argument)) return argument;
		  throw new $TypeError("Can't set " + $String(argument) + ' as a prototype');
		};
		return aPossiblePrototype;
	}

	var objectSetPrototypeOf;
	var hasRequiredObjectSetPrototypeOf;

	function requireObjectSetPrototypeOf () {
		if (hasRequiredObjectSetPrototypeOf) return objectSetPrototypeOf;
		hasRequiredObjectSetPrototypeOf = 1;
		/* eslint-disable no-proto -- safe */
		var uncurryThisAccessor = requireFunctionUncurryThisAccessor();
		var isObject = requireIsObject();
		var requireObjectCoercible = requireRequireObjectCoercible();
		var aPossiblePrototype = requireAPossiblePrototype();

		// `Object.setPrototypeOf` method
		// https://tc39.es/ecma262/#sec-object.setprototypeof
		// Works with __proto__ only. Old v8 can't work with null proto objects.
		// eslint-disable-next-line es/no-object-setprototypeof -- safe
		objectSetPrototypeOf = Object.setPrototypeOf || ('__proto__' in {} ? function () {
		  var CORRECT_SETTER = false;
		  var test = {};
		  var setter;
		  try {
		    setter = uncurryThisAccessor(Object.prototype, '__proto__', 'set');
		    setter(test, []);
		    CORRECT_SETTER = test instanceof Array;
		  } catch (error) { /* empty */ }
		  return function setPrototypeOf(O, proto) {
		    requireObjectCoercible(O);
		    aPossiblePrototype(proto);
		    if (!isObject(O)) return O;
		    if (CORRECT_SETTER) setter(O, proto);
		    else O.__proto__ = proto;
		    return O;
		  };
		}() : undefined);
		return objectSetPrototypeOf;
	}

	var iteratorDefine;
	var hasRequiredIteratorDefine;

	function requireIteratorDefine () {
		if (hasRequiredIteratorDefine) return iteratorDefine;
		hasRequiredIteratorDefine = 1;
		var $ = require_export();
		var call = requireFunctionCall();
		var IS_PURE = requireIsPure();
		var FunctionName = requireFunctionName();
		var isCallable = requireIsCallable();
		var createIteratorConstructor = requireIteratorCreateConstructor();
		var getPrototypeOf = requireObjectGetPrototypeOf();
		var setPrototypeOf = requireObjectSetPrototypeOf();
		var setToStringTag = requireSetToStringTag();
		var createNonEnumerableProperty = requireCreateNonEnumerableProperty();
		var defineBuiltIn = requireDefineBuiltIn();
		var wellKnownSymbol = requireWellKnownSymbol();
		var Iterators = requireIterators();
		var IteratorsCore = requireIteratorsCore();

		var PROPER_FUNCTION_NAME = FunctionName.PROPER;
		var CONFIGURABLE_FUNCTION_NAME = FunctionName.CONFIGURABLE;
		var IteratorPrototype = IteratorsCore.IteratorPrototype;
		var BUGGY_SAFARI_ITERATORS = IteratorsCore.BUGGY_SAFARI_ITERATORS;
		var ITERATOR = wellKnownSymbol('iterator');
		var KEYS = 'keys';
		var VALUES = 'values';
		var ENTRIES = 'entries';

		var returnThis = function () { return this; };

		iteratorDefine = function (Iterable, NAME, IteratorConstructor, next, DEFAULT, IS_SET, FORCED) {
		  createIteratorConstructor(IteratorConstructor, NAME, next);

		  var getIterationMethod = function (KIND) {
		    if (KIND === DEFAULT && defaultIterator) return defaultIterator;
		    if (!BUGGY_SAFARI_ITERATORS && KIND && KIND in IterablePrototype) return IterablePrototype[KIND];

		    switch (KIND) {
		      case KEYS: return function keys() { return new IteratorConstructor(this, KIND); };
		      case VALUES: return function values() { return new IteratorConstructor(this, KIND); };
		      case ENTRIES: return function entries() { return new IteratorConstructor(this, KIND); };
		    }

		    return function () { return new IteratorConstructor(this); };
		  };

		  var TO_STRING_TAG = NAME + ' Iterator';
		  var INCORRECT_VALUES_NAME = false;
		  var IterablePrototype = Iterable.prototype;
		  var nativeIterator = IterablePrototype[ITERATOR]
		    || IterablePrototype['@@iterator']
		    || DEFAULT && IterablePrototype[DEFAULT];
		  var defaultIterator = !BUGGY_SAFARI_ITERATORS && nativeIterator || getIterationMethod(DEFAULT);
		  var anyNativeIterator = NAME === 'Array' ? IterablePrototype.entries || nativeIterator : nativeIterator;
		  var CurrentIteratorPrototype, methods, KEY;

		  // fix native
		  if (anyNativeIterator) {
		    CurrentIteratorPrototype = getPrototypeOf(anyNativeIterator.call(new Iterable()));
		    if (CurrentIteratorPrototype !== Object.prototype && CurrentIteratorPrototype.next) {
		      if (!IS_PURE && getPrototypeOf(CurrentIteratorPrototype) !== IteratorPrototype) {
		        if (setPrototypeOf) {
		          setPrototypeOf(CurrentIteratorPrototype, IteratorPrototype);
		        } else if (!isCallable(CurrentIteratorPrototype[ITERATOR])) {
		          defineBuiltIn(CurrentIteratorPrototype, ITERATOR, returnThis);
		        }
		      }
		      // Set @@toStringTag to native iterators
		      setToStringTag(CurrentIteratorPrototype, TO_STRING_TAG, true, true);
		      if (IS_PURE) Iterators[TO_STRING_TAG] = returnThis;
		    }
		  }

		  // fix Array.prototype.{ values, @@iterator }.name in V8 / FF
		  if (PROPER_FUNCTION_NAME && DEFAULT === VALUES && nativeIterator && nativeIterator.name !== VALUES) {
		    if (!IS_PURE && CONFIGURABLE_FUNCTION_NAME) {
		      createNonEnumerableProperty(IterablePrototype, 'name', VALUES);
		    } else {
		      INCORRECT_VALUES_NAME = true;
		      defaultIterator = function values() { return call(nativeIterator, this); };
		    }
		  }

		  // export additional methods
		  if (DEFAULT) {
		    methods = {
		      values: getIterationMethod(VALUES),
		      keys: IS_SET ? defaultIterator : getIterationMethod(KEYS),
		      entries: getIterationMethod(ENTRIES)
		    };
		    if (FORCED) for (KEY in methods) {
		      if (BUGGY_SAFARI_ITERATORS || INCORRECT_VALUES_NAME || !(KEY in IterablePrototype)) {
		        defineBuiltIn(IterablePrototype, KEY, methods[KEY]);
		      }
		    } else $({ target: NAME, proto: true, forced: BUGGY_SAFARI_ITERATORS || INCORRECT_VALUES_NAME }, methods);
		  }

		  // define iterator
		  if ((!IS_PURE || FORCED) && IterablePrototype[ITERATOR] !== defaultIterator) {
		    defineBuiltIn(IterablePrototype, ITERATOR, defaultIterator, { name: DEFAULT });
		  }
		  Iterators[NAME] = defaultIterator;

		  return methods;
		};
		return iteratorDefine;
	}

	var createIterResultObject;
	var hasRequiredCreateIterResultObject;

	function requireCreateIterResultObject () {
		if (hasRequiredCreateIterResultObject) return createIterResultObject;
		hasRequiredCreateIterResultObject = 1;
		// `CreateIterResultObject` abstract operation
		// https://tc39.es/ecma262/#sec-createiterresultobject
		createIterResultObject = function (value, done) {
		  return { value: value, done: done };
		};
		return createIterResultObject;
	}

	var es_array_iterator;
	var hasRequiredEs_array_iterator;

	function requireEs_array_iterator () {
		if (hasRequiredEs_array_iterator) return es_array_iterator;
		hasRequiredEs_array_iterator = 1;
		var toIndexedObject = requireToIndexedObject();
		var addToUnscopables = requireAddToUnscopables();
		var Iterators = requireIterators();
		var InternalStateModule = requireInternalState();
		var defineProperty = requireObjectDefineProperty().f;
		var defineIterator = requireIteratorDefine();
		var createIterResultObject = requireCreateIterResultObject();
		var IS_PURE = requireIsPure();
		var DESCRIPTORS = requireDescriptors();

		var ARRAY_ITERATOR = 'Array Iterator';
		var setInternalState = InternalStateModule.set;
		var getInternalState = InternalStateModule.getterFor(ARRAY_ITERATOR);

		// `Array.prototype.entries` method
		// https://tc39.es/ecma262/#sec-array.prototype.entries
		// `Array.prototype.keys` method
		// https://tc39.es/ecma262/#sec-array.prototype.keys
		// `Array.prototype.values` method
		// https://tc39.es/ecma262/#sec-array.prototype.values
		// `Array.prototype[@@iterator]` method
		// https://tc39.es/ecma262/#sec-array.prototype-@@iterator
		// `CreateArrayIterator` internal method
		// https://tc39.es/ecma262/#sec-createarrayiterator
		es_array_iterator = defineIterator(Array, 'Array', function (iterated, kind) {
		  setInternalState(this, {
		    type: ARRAY_ITERATOR,
		    target: toIndexedObject(iterated), // target
		    index: 0,                          // next index
		    kind: kind                         // kind
		  });
		// `%ArrayIteratorPrototype%.next` method
		// https://tc39.es/ecma262/#sec-%arrayiteratorprototype%.next
		}, function () {
		  var state = getInternalState(this);
		  var target = state.target;
		  var index = state.index++;
		  if (!target || index >= target.length) {
		    state.target = null;
		    return createIterResultObject(undefined, true);
		  }
		  switch (state.kind) {
		    case 'keys': return createIterResultObject(index, false);
		    case 'values': return createIterResultObject(target[index], false);
		  } return createIterResultObject([index, target[index]], false);
		}, 'values');

		// argumentsList[@@iterator] is %ArrayProto_values%
		// https://tc39.es/ecma262/#sec-createunmappedargumentsobject
		// https://tc39.es/ecma262/#sec-createmappedargumentsobject
		var values = Iterators.Arguments = Iterators.Array;

		// https://tc39.es/ecma262/#sec-array.prototype-@@unscopables
		addToUnscopables('keys');
		addToUnscopables('values');
		addToUnscopables('entries');

		// V8 ~ Chrome 45- bug
		if (!IS_PURE && DESCRIPTORS && values.name !== 'values') try {
		  defineProperty(values, 'name', { value: 'values' });
		} catch (error) { /* empty */ }
		return es_array_iterator;
	}

	requireEs_array_iterator();

	var es_map = {};

	var es_map_constructor = {};

	var internalMetadata = {exports: {}};

	var objectGetOwnPropertyNamesExternal = {};

	var arraySlice;
	var hasRequiredArraySlice;

	function requireArraySlice () {
		if (hasRequiredArraySlice) return arraySlice;
		hasRequiredArraySlice = 1;
		var uncurryThis = requireFunctionUncurryThis();

		arraySlice = uncurryThis([].slice);
		return arraySlice;
	}

	var hasRequiredObjectGetOwnPropertyNamesExternal;

	function requireObjectGetOwnPropertyNamesExternal () {
		if (hasRequiredObjectGetOwnPropertyNamesExternal) return objectGetOwnPropertyNamesExternal;
		hasRequiredObjectGetOwnPropertyNamesExternal = 1;
		/* eslint-disable es/no-object-getownpropertynames -- safe */
		var classof = requireClassofRaw();
		var toIndexedObject = requireToIndexedObject();
		var $getOwnPropertyNames = requireObjectGetOwnPropertyNames().f;
		var arraySlice = requireArraySlice();

		var windowNames = typeof window == 'object' && window && Object.getOwnPropertyNames
		  ? Object.getOwnPropertyNames(window) : [];

		var getWindowNames = function (it) {
		  try {
		    return $getOwnPropertyNames(it);
		  } catch (error) {
		    return arraySlice(windowNames);
		  }
		};

		// fallback for IE11 buggy Object.getOwnPropertyNames with iframe and window
		objectGetOwnPropertyNamesExternal.f = function getOwnPropertyNames(it) {
		  return windowNames && classof(it) === 'Window'
		    ? getWindowNames(it)
		    : $getOwnPropertyNames(toIndexedObject(it));
		};
		return objectGetOwnPropertyNamesExternal;
	}

	var arrayBufferNonExtensible;
	var hasRequiredArrayBufferNonExtensible;

	function requireArrayBufferNonExtensible () {
		if (hasRequiredArrayBufferNonExtensible) return arrayBufferNonExtensible;
		hasRequiredArrayBufferNonExtensible = 1;
		// FF26- bug: ArrayBuffers are non-extensible, but Object.isExtensible does not report it
		var fails = requireFails();

		arrayBufferNonExtensible = fails(function () {
		  if (typeof ArrayBuffer == 'function') {
		    var buffer = new ArrayBuffer(8);
		    // eslint-disable-next-line es/no-object-isextensible, es/no-object-defineproperty -- safe
		    if (Object.isExtensible(buffer)) Object.defineProperty(buffer, 'a', { value: 8 });
		  }
		});
		return arrayBufferNonExtensible;
	}

	var objectIsExtensible;
	var hasRequiredObjectIsExtensible;

	function requireObjectIsExtensible () {
		if (hasRequiredObjectIsExtensible) return objectIsExtensible;
		hasRequiredObjectIsExtensible = 1;
		var fails = requireFails();
		var isObject = requireIsObject();
		var classof = requireClassofRaw();
		var ARRAY_BUFFER_NON_EXTENSIBLE = requireArrayBufferNonExtensible();

		// eslint-disable-next-line es/no-object-isextensible -- safe
		var $isExtensible = Object.isExtensible;
		var FAILS_ON_PRIMITIVES = fails(function () { });

		// `Object.isExtensible` method
		// https://tc39.es/ecma262/#sec-object.isextensible
		objectIsExtensible = (FAILS_ON_PRIMITIVES || ARRAY_BUFFER_NON_EXTENSIBLE) ? function isExtensible(it) {
		  if (!isObject(it)) return false;
		  if (ARRAY_BUFFER_NON_EXTENSIBLE && classof(it) === 'ArrayBuffer') return false;
		  return $isExtensible ? $isExtensible(it) : true;
		} : $isExtensible;
		return objectIsExtensible;
	}

	var freezing;
	var hasRequiredFreezing;

	function requireFreezing () {
		if (hasRequiredFreezing) return freezing;
		hasRequiredFreezing = 1;
		var fails = requireFails();

		freezing = !fails(function () {
		  // eslint-disable-next-line es/no-object-isextensible, es/no-object-preventextensions -- required for testing
		  return Object.isExtensible(Object.preventExtensions({}));
		});
		return freezing;
	}

	var hasRequiredInternalMetadata;

	function requireInternalMetadata () {
		if (hasRequiredInternalMetadata) return internalMetadata.exports;
		hasRequiredInternalMetadata = 1;
		var $ = require_export();
		var uncurryThis = requireFunctionUncurryThis();
		var hiddenKeys = requireHiddenKeys();
		var isObject = requireIsObject();
		var hasOwn = requireHasOwnProperty();
		var defineProperty = requireObjectDefineProperty().f;
		var getOwnPropertyNamesModule = requireObjectGetOwnPropertyNames();
		var getOwnPropertyNamesExternalModule = requireObjectGetOwnPropertyNamesExternal();
		var isExtensible = requireObjectIsExtensible();
		var uid = requireUid();
		var FREEZING = requireFreezing();

		var REQUIRED = false;
		var METADATA = uid('meta');
		var id = 0;

		var setMetadata = function (it) {
		  defineProperty(it, METADATA, { value: {
		    objectID: 'O' + id++, // object ID
		    weakData: {}          // weak collections IDs
		  } });
		};

		var fastKey = function (it, create) {
		  // return a primitive with prefix
		  if (!isObject(it)) return typeof it == 'symbol' ? it : (typeof it == 'string' ? 'S' : 'P') + it;
		  if (!hasOwn(it, METADATA)) {
		    // can't set metadata to uncaught frozen object
		    if (!isExtensible(it)) return 'F';
		    // not necessary to add metadata
		    if (!create) return 'E';
		    // add missing metadata
		    setMetadata(it);
		  // return object ID
		  } return it[METADATA].objectID;
		};

		var getWeakData = function (it, create) {
		  if (!hasOwn(it, METADATA)) {
		    // can't set metadata to uncaught frozen object
		    if (!isExtensible(it)) return true;
		    // not necessary to add metadata
		    if (!create) return false;
		    // add missing metadata
		    setMetadata(it);
		  // return the store of weak collections IDs
		  } return it[METADATA].weakData;
		};

		// add metadata on freeze-family methods calling
		var onFreeze = function (it) {
		  if (FREEZING && REQUIRED && isExtensible(it) && !hasOwn(it, METADATA)) setMetadata(it);
		  return it;
		};

		var enable = function () {
		  meta.enable = function () { /* empty */ };
		  REQUIRED = true;
		  var getOwnPropertyNames = getOwnPropertyNamesModule.f;
		  var splice = uncurryThis([].splice);
		  var test = {};
		  // eslint-disable-next-line unicorn/no-immediate-mutation -- ES3 syntax limitation
		  test[METADATA] = 1;

		  // prevent exposing of metadata key
		  if (getOwnPropertyNames(test).length) {
		    getOwnPropertyNamesModule.f = function (it) {
		      var result = getOwnPropertyNames(it);
		      for (var i = 0, length = result.length; i < length; i++) {
		        if (result[i] === METADATA) {
		          splice(result, i, 1);
		          break;
		        }
		      } return result;
		    };

		    $({ target: 'Object', stat: true, forced: true }, {
		      getOwnPropertyNames: getOwnPropertyNamesExternalModule.f
		    });
		  }
		};

		var meta = internalMetadata.exports = {
		  enable: enable,
		  fastKey: fastKey,
		  getWeakData: getWeakData,
		  onFreeze: onFreeze
		};

		hiddenKeys[METADATA] = true;
		return internalMetadata.exports;
	}

	var iterate;
	var hasRequiredIterate;

	function requireIterate () {
		if (hasRequiredIterate) return iterate;
		hasRequiredIterate = 1;
		var bind = requireFunctionBindContext();
		var call = requireFunctionCall();
		var anObject = requireAnObject();
		var tryToString = requireTryToString();
		var isArrayIteratorMethod = requireIsArrayIteratorMethod();
		var lengthOfArrayLike = requireLengthOfArrayLike();
		var isPrototypeOf = requireObjectIsPrototypeOf();
		var getIterator = requireGetIterator();
		var getIteratorMethod = requireGetIteratorMethod();
		var iteratorClose = requireIteratorClose();

		var $TypeError = TypeError;

		var Result = function (stopped, result) {
		  this.stopped = stopped;
		  this.result = result;
		};

		var ResultPrototype = Result.prototype;

		iterate = function (iterable, unboundFunction, options) {
		  var that = options && options.that;
		  var AS_ENTRIES = !!(options && options.AS_ENTRIES);
		  var IS_RECORD = !!(options && options.IS_RECORD);
		  var IS_ITERATOR = !!(options && options.IS_ITERATOR);
		  var INTERRUPTED = !!(options && options.INTERRUPTED);
		  var fn = bind(unboundFunction, that);
		  var iterator, iterFn, index, length, result, next, step;

		  var stop = function (condition) {
		    var $iterator = iterator;
		    iterator = undefined;
		    if ($iterator) iteratorClose($iterator, 'normal');
		    return new Result(true, condition);
		  };

		  var callFn = function (value) {
		    if (AS_ENTRIES) {
		      anObject(value);
		      return INTERRUPTED ? fn(value[0], value[1], stop) : fn(value[0], value[1]);
		    } return INTERRUPTED ? fn(value, stop) : fn(value);
		  };

		  if (IS_RECORD) {
		    iterator = iterable.iterator;
		  } else if (IS_ITERATOR) {
		    iterator = iterable;
		  } else {
		    iterFn = getIteratorMethod(iterable);
		    if (!iterFn) throw new $TypeError(tryToString(iterable) + ' is not iterable');
		    // optimisation for array iterators
		    if (isArrayIteratorMethod(iterFn)) {
		      for (index = 0, length = lengthOfArrayLike(iterable); length > index; index++) {
		        result = callFn(iterable[index]);
		        if (result && isPrototypeOf(ResultPrototype, result)) return result;
		      } return new Result(false);
		    }
		    iterator = getIterator(iterable, iterFn);
		  }

		  next = IS_RECORD ? iterable.next : iterator.next;
		  while (!(step = call(next, iterator)).done) {
		    // `IteratorValue` errors should propagate without closing the iterator
		    var value = step.value;
		    try {
		      result = callFn(value);
		    } catch (error) {
		      if (iterator) iteratorClose(iterator, 'throw', error);
		      else throw error;
		    }
		    if (typeof result == 'object' && result && isPrototypeOf(ResultPrototype, result)) return result;
		  } return new Result(false);
		};
		return iterate;
	}

	var anInstance;
	var hasRequiredAnInstance;

	function requireAnInstance () {
		if (hasRequiredAnInstance) return anInstance;
		hasRequiredAnInstance = 1;
		var isPrototypeOf = requireObjectIsPrototypeOf();

		var $TypeError = TypeError;

		anInstance = function (it, Prototype) {
		  if (isPrototypeOf(Prototype, it)) return it;
		  throw new $TypeError('Incorrect invocation');
		};
		return anInstance;
	}

	var inheritIfRequired;
	var hasRequiredInheritIfRequired;

	function requireInheritIfRequired () {
		if (hasRequiredInheritIfRequired) return inheritIfRequired;
		hasRequiredInheritIfRequired = 1;
		var isCallable = requireIsCallable();
		var isObject = requireIsObject();
		var setPrototypeOf = requireObjectSetPrototypeOf();

		// makes subclassing work correct for wrapped built-ins
		inheritIfRequired = function ($this, dummy, Wrapper) {
		  var NewTarget, NewTargetPrototype;
		  if (
		    // it can work only with native `setPrototypeOf`
		    setPrototypeOf &&
		    // we haven't completely correct pre-ES6 way for getting `new.target`, so use this
		    isCallable(NewTarget = dummy.constructor) &&
		    NewTarget !== Wrapper &&
		    isObject(NewTargetPrototype = NewTarget.prototype) &&
		    NewTargetPrototype !== Wrapper.prototype
		  ) setPrototypeOf($this, NewTargetPrototype);
		  return $this;
		};
		return inheritIfRequired;
	}

	var collection;
	var hasRequiredCollection;

	function requireCollection () {
		if (hasRequiredCollection) return collection;
		hasRequiredCollection = 1;
		var $ = require_export();
		var globalThis = requireGlobalThis();
		var uncurryThis = requireFunctionUncurryThis();
		var isForced = requireIsForced();
		var defineBuiltIn = requireDefineBuiltIn();
		var InternalMetadataModule = requireInternalMetadata();
		var iterate = requireIterate();
		var anInstance = requireAnInstance();
		var isCallable = requireIsCallable();
		var isNullOrUndefined = requireIsNullOrUndefined();
		var isObject = requireIsObject();
		var fails = requireFails();
		var checkCorrectnessOfIteration = requireCheckCorrectnessOfIteration();
		var setToStringTag = requireSetToStringTag();
		var inheritIfRequired = requireInheritIfRequired();

		collection = function (CONSTRUCTOR_NAME, wrapper, common) {
		  var IS_MAP = CONSTRUCTOR_NAME.indexOf('Map') !== -1;
		  var IS_WEAK = CONSTRUCTOR_NAME.indexOf('Weak') !== -1;
		  var ADDER = IS_MAP ? 'set' : 'add';
		  var NativeConstructor = globalThis[CONSTRUCTOR_NAME];
		  var NativePrototype = NativeConstructor && NativeConstructor.prototype;
		  var Constructor = NativeConstructor;
		  var exported = {};

		  var fixMethod = function (KEY) {
		    var uncurriedNativeMethod = uncurryThis(NativePrototype[KEY]);
		    defineBuiltIn(NativePrototype, KEY,
		      KEY === 'add' ? function add(value) {
		        uncurriedNativeMethod(this, value === 0 ? 0 : value);
		        return this;
		      } : KEY === 'delete' ? function (key) {
		        return IS_WEAK && !isObject(key) ? false : uncurriedNativeMethod(this, key === 0 ? 0 : key);
		      } : KEY === 'get' ? function get(key) {
		        return IS_WEAK && !isObject(key) ? undefined : uncurriedNativeMethod(this, key === 0 ? 0 : key);
		      } : KEY === 'has' ? function has(key) {
		        return IS_WEAK && !isObject(key) ? false : uncurriedNativeMethod(this, key === 0 ? 0 : key);
		      } : function set(key, value) {
		        uncurriedNativeMethod(this, key === 0 ? 0 : key, value);
		        return this;
		      }
		    );
		  };

		  var REPLACE = isForced(
		    CONSTRUCTOR_NAME,
		    !isCallable(NativeConstructor) || !(IS_WEAK || NativePrototype.forEach && !fails(function () {
		      new NativeConstructor().entries().next();
		    }))
		  );

		  if (REPLACE) {
		    // create collection constructor
		    Constructor = common.getConstructor(wrapper, CONSTRUCTOR_NAME, IS_MAP, ADDER);
		    InternalMetadataModule.enable();
		  } else if (isForced(CONSTRUCTOR_NAME, true)) {
		    var instance = new Constructor();
		    // early implementations not supports chaining
		    var HASNT_CHAINING = instance[ADDER](IS_WEAK ? {} : -0, 1) !== instance;
		    // V8 ~ Chromium 40- weak-collections throws on primitives, but should return false
		    var THROWS_ON_PRIMITIVES = fails(function () { instance.has(1); });
		    // most early implementations doesn't supports iterables, most modern - not close it correctly
		    // eslint-disable-next-line no-new -- required for testing
		    var ACCEPT_ITERABLES = checkCorrectnessOfIteration(function (iterable) { new NativeConstructor(iterable); });
		    // for early implementations -0 and +0 not the same
		    var BUGGY_ZERO = !IS_WEAK && fails(function () {
		      // V8 ~ Chromium 42- fails only with 5+ elements
		      var $instance = new NativeConstructor();
		      var index = 5;
		      while (index--) $instance[ADDER](index, index);
		      return !$instance.has(-0);
		    });

		    if (!ACCEPT_ITERABLES) {
		      Constructor = wrapper(function (dummy, iterable) {
		        anInstance(dummy, NativePrototype);
		        var that = inheritIfRequired(new NativeConstructor(), dummy, Constructor);
		        if (!isNullOrUndefined(iterable)) iterate(iterable, that[ADDER], { that: that, AS_ENTRIES: IS_MAP });
		        return that;
		      });
		      Constructor.prototype = NativePrototype;
		      NativePrototype.constructor = Constructor;
		    }

		    if (THROWS_ON_PRIMITIVES || BUGGY_ZERO) {
		      fixMethod('delete');
		      fixMethod('has');
		      IS_MAP && fixMethod('get');
		    }

		    if (BUGGY_ZERO || HASNT_CHAINING) fixMethod(ADDER);

		    // weak collections should not contains .clear method
		    if (IS_WEAK && NativePrototype.clear) delete NativePrototype.clear;
		  }

		  exported[CONSTRUCTOR_NAME] = Constructor;
		  $({ global: true, constructor: true, forced: Constructor !== NativeConstructor }, exported);

		  setToStringTag(Constructor, CONSTRUCTOR_NAME);

		  if (!IS_WEAK) common.setStrong(Constructor, CONSTRUCTOR_NAME, IS_MAP);

		  return Constructor;
		};
		return collection;
	}

	var defineBuiltInAccessor;
	var hasRequiredDefineBuiltInAccessor;

	function requireDefineBuiltInAccessor () {
		if (hasRequiredDefineBuiltInAccessor) return defineBuiltInAccessor;
		hasRequiredDefineBuiltInAccessor = 1;
		var makeBuiltIn = requireMakeBuiltIn();
		var defineProperty = requireObjectDefineProperty();

		defineBuiltInAccessor = function (target, name, descriptor) {
		  if (descriptor.get) makeBuiltIn(descriptor.get, name, { getter: true });
		  if (descriptor.set) makeBuiltIn(descriptor.set, name, { setter: true });
		  return defineProperty.f(target, name, descriptor);
		};
		return defineBuiltInAccessor;
	}

	var defineBuiltIns;
	var hasRequiredDefineBuiltIns;

	function requireDefineBuiltIns () {
		if (hasRequiredDefineBuiltIns) return defineBuiltIns;
		hasRequiredDefineBuiltIns = 1;
		var defineBuiltIn = requireDefineBuiltIn();

		defineBuiltIns = function (target, src, options) {
		  for (var key in src) defineBuiltIn(target, key, src[key], options);
		  return target;
		};
		return defineBuiltIns;
	}

	var setSpecies;
	var hasRequiredSetSpecies;

	function requireSetSpecies () {
		if (hasRequiredSetSpecies) return setSpecies;
		hasRequiredSetSpecies = 1;
		var getBuiltIn = requireGetBuiltIn();
		var defineBuiltInAccessor = requireDefineBuiltInAccessor();
		var wellKnownSymbol = requireWellKnownSymbol();
		var DESCRIPTORS = requireDescriptors();

		var SPECIES = wellKnownSymbol('species');

		setSpecies = function (CONSTRUCTOR_NAME) {
		  var Constructor = getBuiltIn(CONSTRUCTOR_NAME);

		  if (DESCRIPTORS && Constructor && !Constructor[SPECIES]) {
		    defineBuiltInAccessor(Constructor, SPECIES, {
		      configurable: true,
		      get: function () { return this; }
		    });
		  }
		};
		return setSpecies;
	}

	var collectionStrong;
	var hasRequiredCollectionStrong;

	function requireCollectionStrong () {
		if (hasRequiredCollectionStrong) return collectionStrong;
		hasRequiredCollectionStrong = 1;
		var create = requireObjectCreate();
		var defineBuiltInAccessor = requireDefineBuiltInAccessor();
		var defineBuiltIns = requireDefineBuiltIns();
		var bind = requireFunctionBindContext();
		var anInstance = requireAnInstance();
		var isNullOrUndefined = requireIsNullOrUndefined();
		var iterate = requireIterate();
		var defineIterator = requireIteratorDefine();
		var createIterResultObject = requireCreateIterResultObject();
		var setSpecies = requireSetSpecies();
		var DESCRIPTORS = requireDescriptors();
		var fastKey = requireInternalMetadata().fastKey;
		var InternalStateModule = requireInternalState();

		var setInternalState = InternalStateModule.set;
		var internalStateGetterFor = InternalStateModule.getterFor;

		collectionStrong = {
		  getConstructor: function (wrapper, CONSTRUCTOR_NAME, IS_MAP, ADDER) {
		    var Constructor = wrapper(function (that, iterable) {
		      anInstance(that, Prototype);
		      setInternalState(that, {
		        type: CONSTRUCTOR_NAME,
		        index: create(null),
		        first: null,
		        last: null,
		        size: 0
		      });
		      if (!DESCRIPTORS) that.size = 0;
		      if (!isNullOrUndefined(iterable)) iterate(iterable, that[ADDER], { that: that, AS_ENTRIES: IS_MAP });
		    });

		    var Prototype = Constructor.prototype;

		    var getInternalState = internalStateGetterFor(CONSTRUCTOR_NAME);

		    var define = function (that, key, value) {
		      var state = getInternalState(that);
		      var entry = getEntry(that, key);
		      var previous, index;
		      // change existing entry
		      if (entry) {
		        entry.value = value;
		      // create new entry
		      } else {
		        state.last = entry = {
		          index: index = fastKey(key, true),
		          key: key,
		          value: value,
		          previous: previous = state.last,
		          next: null,
		          removed: false
		        };
		        if (!state.first) state.first = entry;
		        if (previous) previous.next = entry;
		        if (DESCRIPTORS) state.size++;
		        else that.size++;
		        // add to index
		        if (index !== 'F') state.index[index] = entry;
		      } return that;
		    };

		    var getEntry = function (that, key) {
		      var state = getInternalState(that);
		      // fast case
		      var index = fastKey(key);
		      var entry;
		      if (index !== 'F') return state.index[index];
		      // frozen object case
		      for (entry = state.first; entry; entry = entry.next) {
		        if (entry.key === key) return entry;
		      }
		    };

		    defineBuiltIns(Prototype, {
		      // `{ Map, Set }.prototype.clear()` methods
		      // https://tc39.es/ecma262/#sec-map.prototype.clear
		      // https://tc39.es/ecma262/#sec-set.prototype.clear
		      clear: function clear() {
		        var that = this;
		        var state = getInternalState(that);
		        var entry = state.first;
		        while (entry) {
		          entry.removed = true;
		          if (entry.previous) entry.previous = entry.previous.next = null;
		          entry = entry.next;
		        }
		        state.first = state.last = null;
		        state.index = create(null);
		        if (DESCRIPTORS) state.size = 0;
		        else that.size = 0;
		      },
		      // `{ Map, Set }.prototype.delete(key)` methods
		      // https://tc39.es/ecma262/#sec-map.prototype.delete
		      // https://tc39.es/ecma262/#sec-set.prototype.delete
		      'delete': function (key) {
		        var that = this;
		        var state = getInternalState(that);
		        var entry = getEntry(that, key);
		        if (entry) {
		          var next = entry.next;
		          var prev = entry.previous;
		          delete state.index[entry.index];
		          entry.removed = true;
		          if (prev) prev.next = next;
		          if (next) next.previous = prev;
		          if (state.first === entry) state.first = next;
		          if (state.last === entry) state.last = prev;
		          if (DESCRIPTORS) state.size--;
		          else that.size--;
		        } return !!entry;
		      },
		      // `{ Map, Set }.prototype.forEach(callbackfn, thisArg = undefined)` methods
		      // https://tc39.es/ecma262/#sec-map.prototype.foreach
		      // https://tc39.es/ecma262/#sec-set.prototype.foreach
		      forEach: function forEach(callbackfn /* , that = undefined */) {
		        var state = getInternalState(this);
		        var boundFunction = bind(callbackfn, arguments.length > 1 ? arguments[1] : undefined);
		        var entry;
		        while (entry = entry ? entry.next : state.first) {
		          boundFunction(entry.value, entry.key, this);
		          // revert to the last existing entry
		          while (entry && entry.removed) entry = entry.previous;
		        }
		      },
		      // `{ Map, Set}.prototype.has(key)` methods
		      // https://tc39.es/ecma262/#sec-map.prototype.has
		      // https://tc39.es/ecma262/#sec-set.prototype.has
		      has: function has(key) {
		        return !!getEntry(this, key);
		      }
		    });

		    defineBuiltIns(Prototype, IS_MAP ? {
		      // `Map.prototype.get(key)` method
		      // https://tc39.es/ecma262/#sec-map.prototype.get
		      get: function get(key) {
		        var entry = getEntry(this, key);
		        return entry && entry.value;
		      },
		      // `Map.prototype.set(key, value)` method
		      // https://tc39.es/ecma262/#sec-map.prototype.set
		      set: function set(key, value) {
		        return define(this, key === 0 ? 0 : key, value);
		      }
		    } : {
		      // `Set.prototype.add(value)` method
		      // https://tc39.es/ecma262/#sec-set.prototype.add
		      add: function add(value) {
		        return define(this, value = value === 0 ? 0 : value, value);
		      }
		    });
		    if (DESCRIPTORS) defineBuiltInAccessor(Prototype, 'size', {
		      configurable: true,
		      get: function () {
		        return getInternalState(this).size;
		      }
		    });
		    return Constructor;
		  },
		  setStrong: function (Constructor, CONSTRUCTOR_NAME, IS_MAP) {
		    var ITERATOR_NAME = CONSTRUCTOR_NAME + ' Iterator';
		    var getInternalCollectionState = internalStateGetterFor(CONSTRUCTOR_NAME);
		    var getInternalIteratorState = internalStateGetterFor(ITERATOR_NAME);
		    // `{ Map, Set }.prototype.{ keys, values, entries, @@iterator }()` methods
		    // https://tc39.es/ecma262/#sec-map.prototype.entries
		    // https://tc39.es/ecma262/#sec-map.prototype.keys
		    // https://tc39.es/ecma262/#sec-map.prototype.values
		    // https://tc39.es/ecma262/#sec-map.prototype-@@iterator
		    // https://tc39.es/ecma262/#sec-set.prototype.entries
		    // https://tc39.es/ecma262/#sec-set.prototype.keys
		    // https://tc39.es/ecma262/#sec-set.prototype.values
		    // https://tc39.es/ecma262/#sec-set.prototype-@@iterator
		    defineIterator(Constructor, CONSTRUCTOR_NAME, function (iterated, kind) {
		      setInternalState(this, {
		        type: ITERATOR_NAME,
		        target: iterated,
		        state: getInternalCollectionState(iterated),
		        kind: kind,
		        last: null
		      });
		    }, function () {
		      var state = getInternalIteratorState(this);
		      var kind = state.kind;
		      var entry = state.last;
		      // revert to the last existing entry
		      while (entry && entry.removed) entry = entry.previous;
		      // get next entry
		      if (!state.target || !(state.last = entry = entry ? entry.next : state.state.first)) {
		        // or finish the iteration
		        state.target = null;
		        return createIterResultObject(undefined, true);
		      }
		      // return step by kind
		      if (kind === 'keys') return createIterResultObject(entry.key, false);
		      if (kind === 'values') return createIterResultObject(entry.value, false);
		      return createIterResultObject([entry.key, entry.value], false);
		    }, IS_MAP ? 'entries' : 'values', !IS_MAP, true);

		    // `{ Map, Set }.prototype[@@species]` accessors
		    // https://tc39.es/ecma262/#sec-get-map-@@species
		    // https://tc39.es/ecma262/#sec-get-set-@@species
		    setSpecies(CONSTRUCTOR_NAME);
		  }
		};
		return collectionStrong;
	}

	var hasRequiredEs_map_constructor;

	function requireEs_map_constructor () {
		if (hasRequiredEs_map_constructor) return es_map_constructor;
		hasRequiredEs_map_constructor = 1;
		var collection = requireCollection();
		var collectionStrong = requireCollectionStrong();

		// `Map` constructor
		// https://tc39.es/ecma262/#sec-map-objects
		collection('Map', function (init) {
		  return function Map() { return init(this, arguments.length ? arguments[0] : undefined); };
		}, collectionStrong);
		return es_map_constructor;
	}

	var hasRequiredEs_map;

	function requireEs_map () {
		if (hasRequiredEs_map) return es_map;
		hasRequiredEs_map = 1;
		// TODO: Remove this module from `core-js@4` since it's replaced to module below
		requireEs_map_constructor();
		return es_map;
	}

	requireEs_map();

	var es_promise = {};

	var es_promise_constructor = {};

	var environment;
	var hasRequiredEnvironment;

	function requireEnvironment () {
		if (hasRequiredEnvironment) return environment;
		hasRequiredEnvironment = 1;
		/* global Bun, Deno -- detection */
		var globalThis = requireGlobalThis();
		var userAgent = requireEnvironmentUserAgent();
		var classof = requireClassofRaw();

		var userAgentStartsWith = function (string) {
		  return userAgent.slice(0, string.length) === string;
		};

		environment = (function () {
		  if (userAgentStartsWith('Bun/')) return 'BUN';
		  if (userAgentStartsWith('Cloudflare-Workers')) return 'CLOUDFLARE';
		  if (userAgentStartsWith('Deno/')) return 'DENO';
		  if (userAgentStartsWith('Node.js/')) return 'NODE';
		  if (globalThis.Bun && typeof Bun.version == 'string') return 'BUN';
		  if (globalThis.Deno && typeof Deno.version == 'object') return 'DENO';
		  if (classof(globalThis.process) === 'process') return 'NODE';
		  if (globalThis.window && globalThis.document) return 'BROWSER';
		  return 'REST';
		})();
		return environment;
	}

	var environmentIsNode;
	var hasRequiredEnvironmentIsNode;

	function requireEnvironmentIsNode () {
		if (hasRequiredEnvironmentIsNode) return environmentIsNode;
		hasRequiredEnvironmentIsNode = 1;
		var ENVIRONMENT = requireEnvironment();

		environmentIsNode = ENVIRONMENT === 'NODE';
		return environmentIsNode;
	}

	var path;
	var hasRequiredPath;

	function requirePath () {
		if (hasRequiredPath) return path;
		hasRequiredPath = 1;
		var globalThis = requireGlobalThis();

		path = globalThis;
		return path;
	}

	var aConstructor;
	var hasRequiredAConstructor;

	function requireAConstructor () {
		if (hasRequiredAConstructor) return aConstructor;
		hasRequiredAConstructor = 1;
		var isConstructor = requireIsConstructor();
		var tryToString = requireTryToString();

		var $TypeError = TypeError;

		// `Assert: IsConstructor(argument) is true`
		aConstructor = function (argument) {
		  if (isConstructor(argument)) return argument;
		  throw new $TypeError(tryToString(argument) + ' is not a constructor');
		};
		return aConstructor;
	}

	var speciesConstructor;
	var hasRequiredSpeciesConstructor;

	function requireSpeciesConstructor () {
		if (hasRequiredSpeciesConstructor) return speciesConstructor;
		hasRequiredSpeciesConstructor = 1;
		var anObject = requireAnObject();
		var aConstructor = requireAConstructor();
		var isNullOrUndefined = requireIsNullOrUndefined();
		var wellKnownSymbol = requireWellKnownSymbol();

		var SPECIES = wellKnownSymbol('species');

		// `SpeciesConstructor` abstract operation
		// https://tc39.es/ecma262/#sec-speciesconstructor
		speciesConstructor = function (O, defaultConstructor) {
		  var C = anObject(O).constructor;
		  var S;
		  return C === undefined || isNullOrUndefined(S = anObject(C)[SPECIES]) ? defaultConstructor : aConstructor(S);
		};
		return speciesConstructor;
	}

	var functionApply;
	var hasRequiredFunctionApply;

	function requireFunctionApply () {
		if (hasRequiredFunctionApply) return functionApply;
		hasRequiredFunctionApply = 1;
		var NATIVE_BIND = requireFunctionBindNative();

		var FunctionPrototype = Function.prototype;
		var apply = FunctionPrototype.apply;
		var call = FunctionPrototype.call;

		// eslint-disable-next-line es/no-function-prototype-bind, es/no-reflect -- safe
		functionApply = typeof Reflect == 'object' && Reflect.apply || (NATIVE_BIND ? call.bind(apply) : function () {
		  return call.apply(apply, arguments);
		});
		return functionApply;
	}

	var validateArgumentsLength;
	var hasRequiredValidateArgumentsLength;

	function requireValidateArgumentsLength () {
		if (hasRequiredValidateArgumentsLength) return validateArgumentsLength;
		hasRequiredValidateArgumentsLength = 1;
		var $TypeError = TypeError;

		validateArgumentsLength = function (passed, required) {
		  if (passed < required) throw new $TypeError('Not enough arguments');
		  return passed;
		};
		return validateArgumentsLength;
	}

	var environmentIsIos;
	var hasRequiredEnvironmentIsIos;

	function requireEnvironmentIsIos () {
		if (hasRequiredEnvironmentIsIos) return environmentIsIos;
		hasRequiredEnvironmentIsIos = 1;
		var userAgent = requireEnvironmentUserAgent();

		environmentIsIos = /ipad|iphone|ipod/i.test(userAgent) && /applewebkit/i.test(userAgent);
		return environmentIsIos;
	}

	var task;
	var hasRequiredTask;

	function requireTask () {
		if (hasRequiredTask) return task;
		hasRequiredTask = 1;
		var globalThis = requireGlobalThis();
		var apply = requireFunctionApply();
		var bind = requireFunctionBindContext();
		var isCallable = requireIsCallable();
		var hasOwn = requireHasOwnProperty();
		var fails = requireFails();
		var html = requireHtml();
		var arraySlice = requireArraySlice();
		var createElement = requireDocumentCreateElement();
		var validateArgumentsLength = requireValidateArgumentsLength();
		var IS_IOS = requireEnvironmentIsIos();
		var IS_NODE = requireEnvironmentIsNode();

		var set = globalThis.setImmediate;
		var clear = globalThis.clearImmediate;
		var process = globalThis.process;
		var Dispatch = globalThis.Dispatch;
		var Function = globalThis.Function;
		var MessageChannel = globalThis.MessageChannel;
		var String = globalThis.String;
		var counter = 0;
		var queue = {};
		var ONREADYSTATECHANGE = 'onreadystatechange';
		var $location, defer, channel, port;

		fails(function () {
		  // Deno throws a ReferenceError on `location` access without `--location` flag
		  $location = globalThis.location;
		});

		var run = function (id) {
		  if (hasOwn(queue, id)) {
		    var fn = queue[id];
		    delete queue[id];
		    fn();
		  }
		};

		var runner = function (id) {
		  return function () {
		    run(id);
		  };
		};

		var eventListener = function (event) {
		  run(event.data);
		};

		var globalPostMessageDefer = function (id) {
		  // old engines have not location.origin
		  globalThis.postMessage(String(id), $location.protocol + '//' + $location.host);
		};

		// Node.js 0.9+ & IE10+ has setImmediate, otherwise:
		if (!set || !clear) {
		  set = function setImmediate(handler) {
		    validateArgumentsLength(arguments.length, 1);
		    var fn = isCallable(handler) ? handler : Function(handler);
		    var args = arraySlice(arguments, 1);
		    queue[++counter] = function () {
		      apply(fn, undefined, args);
		    };
		    defer(counter);
		    return counter;
		  };
		  clear = function clearImmediate(id) {
		    delete queue[id];
		  };
		  // Node.js 0.8-
		  if (IS_NODE) {
		    defer = function (id) {
		      process.nextTick(runner(id));
		    };
		  // Sphere (JS game engine) Dispatch API
		  } else if (Dispatch && Dispatch.now) {
		    defer = function (id) {
		      Dispatch.now(runner(id));
		    };
		  // Browsers with MessageChannel, includes WebWorkers
		  // except iOS - https://github.com/zloirock/core-js/issues/624
		  } else if (MessageChannel && !IS_IOS) {
		    channel = new MessageChannel();
		    port = channel.port2;
		    channel.port1.onmessage = eventListener;
		    defer = bind(port.postMessage, port);
		  // Browsers with postMessage, skip WebWorkers
		  // IE8 has postMessage, but it's sync & typeof its postMessage is 'object'
		  } else if (
		    globalThis.addEventListener &&
		    isCallable(globalThis.postMessage) &&
		    !globalThis.importScripts &&
		    $location && $location.protocol !== 'file:' &&
		    !fails(globalPostMessageDefer)
		  ) {
		    defer = globalPostMessageDefer;
		    globalThis.addEventListener('message', eventListener, false);
		  // IE8-
		  } else if (ONREADYSTATECHANGE in createElement('script')) {
		    defer = function (id) {
		      html.appendChild(createElement('script'))[ONREADYSTATECHANGE] = function () {
		        html.removeChild(this);
		        run(id);
		      };
		    };
		  // Rest old browsers
		  } else {
		    defer = function (id) {
		      setTimeout(runner(id), 0);
		    };
		  }
		}

		task = {
		  set: set,
		  clear: clear
		};
		return task;
	}

	var safeGetBuiltIn;
	var hasRequiredSafeGetBuiltIn;

	function requireSafeGetBuiltIn () {
		if (hasRequiredSafeGetBuiltIn) return safeGetBuiltIn;
		hasRequiredSafeGetBuiltIn = 1;
		var globalThis = requireGlobalThis();
		var DESCRIPTORS = requireDescriptors();

		// eslint-disable-next-line es/no-object-getownpropertydescriptor -- safe
		var getOwnPropertyDescriptor = Object.getOwnPropertyDescriptor;

		// Avoid NodeJS experimental warning
		safeGetBuiltIn = function (name) {
		  if (!DESCRIPTORS) return globalThis[name];
		  var descriptor = getOwnPropertyDescriptor(globalThis, name);
		  return descriptor && descriptor.value;
		};
		return safeGetBuiltIn;
	}

	var queue;
	var hasRequiredQueue;

	function requireQueue () {
		if (hasRequiredQueue) return queue;
		hasRequiredQueue = 1;
		var Queue = function () {
		  this.head = null;
		  this.tail = null;
		};

		Queue.prototype = {
		  add: function (item) {
		    var entry = { item: item, next: null };
		    var tail = this.tail;
		    if (tail) tail.next = entry;
		    else this.head = entry;
		    this.tail = entry;
		  },
		  get: function () {
		    var entry = this.head;
		    if (entry) {
		      var next = this.head = entry.next;
		      if (next === null) this.tail = null;
		      return entry.item;
		    }
		  }
		};

		queue = Queue;
		return queue;
	}

	var environmentIsIosPebble;
	var hasRequiredEnvironmentIsIosPebble;

	function requireEnvironmentIsIosPebble () {
		if (hasRequiredEnvironmentIsIosPebble) return environmentIsIosPebble;
		hasRequiredEnvironmentIsIosPebble = 1;
		var userAgent = requireEnvironmentUserAgent();

		environmentIsIosPebble = /ipad|iphone|ipod/i.test(userAgent) && typeof Pebble != 'undefined';
		return environmentIsIosPebble;
	}

	var environmentIsWebosWebkit;
	var hasRequiredEnvironmentIsWebosWebkit;

	function requireEnvironmentIsWebosWebkit () {
		if (hasRequiredEnvironmentIsWebosWebkit) return environmentIsWebosWebkit;
		hasRequiredEnvironmentIsWebosWebkit = 1;
		var userAgent = requireEnvironmentUserAgent();

		environmentIsWebosWebkit = /web0s(?!.*chrome)/i.test(userAgent);
		return environmentIsWebosWebkit;
	}

	var microtask_1;
	var hasRequiredMicrotask;

	function requireMicrotask () {
		if (hasRequiredMicrotask) return microtask_1;
		hasRequiredMicrotask = 1;
		var globalThis = requireGlobalThis();
		var safeGetBuiltIn = requireSafeGetBuiltIn();
		var bind = requireFunctionBindContext();
		var macrotask = requireTask().set;
		var Queue = requireQueue();
		var IS_IOS = requireEnvironmentIsIos();
		var IS_IOS_PEBBLE = requireEnvironmentIsIosPebble();
		var IS_WEBOS_WEBKIT = requireEnvironmentIsWebosWebkit();
		var IS_NODE = requireEnvironmentIsNode();

		var MutationObserver = globalThis.MutationObserver || globalThis.WebKitMutationObserver;
		var document = globalThis.document;
		var process = globalThis.process;
		var Promise = globalThis.Promise;
		var microtask = safeGetBuiltIn('queueMicrotask');
		var notify, toggle, node, promise, then;

		// modern engines have queueMicrotask method
		if (!microtask) {
		  var queue = new Queue();

		  var flush = function () {
		    var parent, fn;
		    if (IS_NODE && (parent = process.domain)) parent.exit();
		    while (fn = queue.get()) try {
		      fn();
		    } catch (error) {
		      if (queue.head) notify();
		      throw error;
		    }
		    if (parent) parent.enter();
		  };

		  // browsers with MutationObserver, except iOS - https://github.com/zloirock/core-js/issues/339
		  // also except WebOS Webkit https://github.com/zloirock/core-js/issues/898
		  if (!IS_IOS && !IS_NODE && !IS_WEBOS_WEBKIT && MutationObserver && document) {
		    toggle = true;
		    node = document.createTextNode('');
		    new MutationObserver(flush).observe(node, { characterData: true });
		    notify = function () {
		      node.data = toggle = !toggle;
		    };
		  // environments with maybe non-completely correct, but existent Promise
		  } else if (!IS_IOS_PEBBLE && Promise && Promise.resolve) {
		    // Promise.resolve without an argument throws an error in LG WebOS 2
		    promise = Promise.resolve(undefined);
		    // workaround of WebKit ~ iOS Safari 10.1 bug
		    promise.constructor = Promise;
		    then = bind(promise.then, promise);
		    notify = function () {
		      then(flush);
		    };
		  // Node.js without promises
		  } else if (IS_NODE) {
		    notify = function () {
		      process.nextTick(flush);
		    };
		  // for other environments - macrotask based on:
		  // - setImmediate
		  // - MessageChannel
		  // - window.postMessage
		  // - onreadystatechange
		  // - setTimeout
		  } else {
		    // `webpack` dev server bug on IE global methods - use bind(fn, global)
		    macrotask = bind(macrotask, globalThis);
		    notify = function () {
		      macrotask(flush);
		    };
		  }

		  microtask = function (fn) {
		    if (!queue.head) notify();
		    queue.add(fn);
		  };
		}

		microtask_1 = microtask;
		return microtask_1;
	}

	var hostReportErrors;
	var hasRequiredHostReportErrors;

	function requireHostReportErrors () {
		if (hasRequiredHostReportErrors) return hostReportErrors;
		hasRequiredHostReportErrors = 1;
		hostReportErrors = function (a, b) {
		  try {
		    // eslint-disable-next-line no-console -- safe
		    arguments.length === 1 ? console.error(a) : console.error(a, b);
		  } catch (error) { /* empty */ }
		};
		return hostReportErrors;
	}

	var perform;
	var hasRequiredPerform;

	function requirePerform () {
		if (hasRequiredPerform) return perform;
		hasRequiredPerform = 1;
		perform = function (exec) {
		  try {
		    return { error: false, value: exec() };
		  } catch (error) {
		    return { error: true, value: error };
		  }
		};
		return perform;
	}

	var promiseNativeConstructor;
	var hasRequiredPromiseNativeConstructor;

	function requirePromiseNativeConstructor () {
		if (hasRequiredPromiseNativeConstructor) return promiseNativeConstructor;
		hasRequiredPromiseNativeConstructor = 1;
		var globalThis = requireGlobalThis();

		promiseNativeConstructor = globalThis.Promise;
		return promiseNativeConstructor;
	}

	var promiseConstructorDetection;
	var hasRequiredPromiseConstructorDetection;

	function requirePromiseConstructorDetection () {
		if (hasRequiredPromiseConstructorDetection) return promiseConstructorDetection;
		hasRequiredPromiseConstructorDetection = 1;
		var globalThis = requireGlobalThis();
		var NativePromiseConstructor = requirePromiseNativeConstructor();
		var isCallable = requireIsCallable();
		var isForced = requireIsForced();
		var inspectSource = requireInspectSource();
		var wellKnownSymbol = requireWellKnownSymbol();
		var ENVIRONMENT = requireEnvironment();
		var IS_PURE = requireIsPure();
		var V8_VERSION = requireEnvironmentV8Version();

		var NativePromisePrototype = NativePromiseConstructor && NativePromiseConstructor.prototype;
		var SPECIES = wellKnownSymbol('species');
		var SUBCLASSING = false;
		var NATIVE_PROMISE_REJECTION_EVENT = isCallable(globalThis.PromiseRejectionEvent);

		var FORCED_PROMISE_CONSTRUCTOR = isForced('Promise', function () {
		  var PROMISE_CONSTRUCTOR_SOURCE = inspectSource(NativePromiseConstructor);
		  var GLOBAL_CORE_JS_PROMISE = PROMISE_CONSTRUCTOR_SOURCE !== String(NativePromiseConstructor);
		  // V8 6.6 (Node 10 and Chrome 66) have a bug with resolving custom thenables
		  // https://bugs.chromium.org/p/chromium/issues/detail?id=830565
		  // We can't detect it synchronously, so just check versions
		  if (!GLOBAL_CORE_JS_PROMISE && V8_VERSION === 66) return true;
		  // We need Promise#{ catch, finally } in the pure version for preventing prototype pollution
		  if (IS_PURE && !(NativePromisePrototype['catch'] && NativePromisePrototype['finally'])) return true;
		  // We can't use @@species feature detection in V8 since it causes
		  // deoptimization and performance degradation
		  // https://github.com/zloirock/core-js/issues/679
		  if (!V8_VERSION || V8_VERSION < 51 || !/native code/.test(PROMISE_CONSTRUCTOR_SOURCE)) {
		    // Detect correctness of subclassing with @@species support
		    var promise = new NativePromiseConstructor(function (resolve) { resolve(1); });
		    var FakePromise = function (exec) {
		      exec(function () { /* empty */ }, function () { /* empty */ });
		    };
		    var constructor = promise.constructor = {};
		    constructor[SPECIES] = FakePromise;
		    SUBCLASSING = promise.then(function () { /* empty */ }) instanceof FakePromise;
		    if (!SUBCLASSING) return true;
		  // Unhandled rejections tracking support, NodeJS Promise without it fails @@species test
		  } return !GLOBAL_CORE_JS_PROMISE && (ENVIRONMENT === 'BROWSER' || ENVIRONMENT === 'DENO') && !NATIVE_PROMISE_REJECTION_EVENT;
		});

		promiseConstructorDetection = {
		  CONSTRUCTOR: FORCED_PROMISE_CONSTRUCTOR,
		  REJECTION_EVENT: NATIVE_PROMISE_REJECTION_EVENT,
		  SUBCLASSING: SUBCLASSING
		};
		return promiseConstructorDetection;
	}

	var newPromiseCapability = {};

	var hasRequiredNewPromiseCapability;

	function requireNewPromiseCapability () {
		if (hasRequiredNewPromiseCapability) return newPromiseCapability;
		hasRequiredNewPromiseCapability = 1;
		var aCallable = requireACallable();

		var $TypeError = TypeError;

		var PromiseCapability = function (C) {
		  var resolve, reject;
		  this.promise = new C(function ($$resolve, $$reject) {
		    if (resolve !== undefined || reject !== undefined) throw new $TypeError('Bad Promise constructor');
		    resolve = $$resolve;
		    reject = $$reject;
		  });
		  this.resolve = aCallable(resolve);
		  this.reject = aCallable(reject);
		};

		// `NewPromiseCapability` abstract operation
		// https://tc39.es/ecma262/#sec-newpromisecapability
		newPromiseCapability.f = function (C) {
		  return new PromiseCapability(C);
		};
		return newPromiseCapability;
	}

	var hasRequiredEs_promise_constructor;

	function requireEs_promise_constructor () {
		if (hasRequiredEs_promise_constructor) return es_promise_constructor;
		hasRequiredEs_promise_constructor = 1;
		var $ = require_export();
		var IS_PURE = requireIsPure();
		var IS_NODE = requireEnvironmentIsNode();
		var globalThis = requireGlobalThis();
		var path = requirePath();
		var call = requireFunctionCall();
		var defineBuiltIn = requireDefineBuiltIn();
		var setPrototypeOf = requireObjectSetPrototypeOf();
		var setToStringTag = requireSetToStringTag();
		var setSpecies = requireSetSpecies();
		var aCallable = requireACallable();
		var isCallable = requireIsCallable();
		var isObject = requireIsObject();
		var anInstance = requireAnInstance();
		var speciesConstructor = requireSpeciesConstructor();
		var task = requireTask().set;
		var microtask = requireMicrotask();
		var hostReportErrors = requireHostReportErrors();
		var perform = requirePerform();
		var Queue = requireQueue();
		var InternalStateModule = requireInternalState();
		var NativePromiseConstructor = requirePromiseNativeConstructor();
		var PromiseConstructorDetection = requirePromiseConstructorDetection();
		var newPromiseCapabilityModule = requireNewPromiseCapability();

		var PROMISE = 'Promise';
		var FORCED_PROMISE_CONSTRUCTOR = PromiseConstructorDetection.CONSTRUCTOR;
		var NATIVE_PROMISE_REJECTION_EVENT = PromiseConstructorDetection.REJECTION_EVENT;
		var NATIVE_PROMISE_SUBCLASSING = PromiseConstructorDetection.SUBCLASSING;
		var getInternalPromiseState = InternalStateModule.getterFor(PROMISE);
		var setInternalState = InternalStateModule.set;
		var NativePromisePrototype = NativePromiseConstructor && NativePromiseConstructor.prototype;
		var PromiseConstructor = NativePromiseConstructor;
		var PromisePrototype = NativePromisePrototype;
		var TypeError = globalThis.TypeError;
		var document = globalThis.document;
		var process = globalThis.process;
		var newPromiseCapability = newPromiseCapabilityModule.f;
		var newGenericPromiseCapability = newPromiseCapability;

		var DISPATCH_EVENT = !!(document && document.createEvent && globalThis.dispatchEvent);
		var UNHANDLED_REJECTION = 'unhandledrejection';
		var REJECTION_HANDLED = 'rejectionhandled';
		var PENDING = 0;
		var FULFILLED = 1;
		var REJECTED = 2;
		var HANDLED = 1;
		var UNHANDLED = 2;

		var Internal, OwnPromiseCapability, PromiseWrapper, nativeThen;

		// helpers
		var isThenable = function (it) {
		  var then;
		  return isObject(it) && isCallable(then = it.then) ? then : false;
		};

		var callReaction = function (reaction, state) {
		  var value = state.value;
		  var ok = state.state === FULFILLED;
		  var handler = ok ? reaction.ok : reaction.fail;
		  var resolve = reaction.resolve;
		  var reject = reaction.reject;
		  var domain = reaction.domain;
		  var result, then, exited;
		  try {
		    if (handler) {
		      if (!ok) {
		        if (state.rejection === UNHANDLED) onHandleUnhandled(state);
		        state.rejection = HANDLED;
		      }
		      if (handler === true) result = value;
		      else {
		        if (domain) domain.enter();
		        result = handler(value); // can throw
		        if (domain) {
		          domain.exit();
		          exited = true;
		        }
		      }
		      if (result === reaction.promise) {
		        reject(new TypeError('Promise-chain cycle'));
		      } else if (then = isThenable(result)) {
		        call(then, result, resolve, reject);
		      } else resolve(result);
		    } else reject(value);
		  } catch (error) {
		    if (domain && !exited) domain.exit();
		    reject(error);
		  }
		};

		var notify = function (state, isReject) {
		  if (state.notified) return;
		  state.notified = true;
		  microtask(function () {
		    var reactions = state.reactions;
		    var reaction;
		    while (reaction = reactions.get()) {
		      callReaction(reaction, state);
		    }
		    state.notified = false;
		    if (isReject && !state.rejection) onUnhandled(state);
		  });
		};

		var dispatchEvent = function (name, promise, reason) {
		  var event, handler;
		  if (DISPATCH_EVENT) {
		    event = document.createEvent('Event');
		    event.promise = promise;
		    event.reason = reason;
		    event.initEvent(name, false, true);
		    globalThis.dispatchEvent(event);
		  } else event = { promise: promise, reason: reason };
		  if (!NATIVE_PROMISE_REJECTION_EVENT && (handler = globalThis['on' + name])) handler(event);
		  else if (name === UNHANDLED_REJECTION) hostReportErrors('Unhandled promise rejection', reason);
		};

		var onUnhandled = function (state) {
		  call(task, globalThis, function () {
		    var promise = state.facade;
		    var value = state.value;
		    var IS_UNHANDLED = isUnhandled(state);
		    var result;
		    if (IS_UNHANDLED) {
		      result = perform(function () {
		        if (IS_NODE) {
		          process.emit('unhandledRejection', value, promise);
		        } else dispatchEvent(UNHANDLED_REJECTION, promise, value);
		      });
		      // Browsers should not trigger `rejectionHandled` event if it was handled here, NodeJS - should
		      state.rejection = IS_NODE || isUnhandled(state) ? UNHANDLED : HANDLED;
		      if (result.error) throw result.value;
		    }
		  });
		};

		var isUnhandled = function (state) {
		  return state.rejection !== HANDLED && !state.parent;
		};

		var onHandleUnhandled = function (state) {
		  call(task, globalThis, function () {
		    var promise = state.facade;
		    if (IS_NODE) {
		      process.emit('rejectionHandled', promise);
		    } else dispatchEvent(REJECTION_HANDLED, promise, state.value);
		  });
		};

		var bind = function (fn, state, unwrap) {
		  return function (value) {
		    fn(state, value, unwrap);
		  };
		};

		var internalReject = function (state, value, unwrap) {
		  if (state.done) return;
		  state.done = true;
		  if (unwrap) state = unwrap;
		  state.value = value;
		  state.state = REJECTED;
		  notify(state, true);
		};

		var internalResolve = function (state, value, unwrap) {
		  if (state.done) return;
		  state.done = true;
		  if (unwrap) state = unwrap;
		  try {
		    if (state.facade === value) throw new TypeError("Promise can't be resolved itself");
		    var then = isThenable(value);
		    if (then) {
		      microtask(function () {
		        var wrapper = { done: false };
		        try {
		          call(then, value,
		            bind(internalResolve, wrapper, state),
		            bind(internalReject, wrapper, state)
		          );
		        } catch (error) {
		          internalReject(wrapper, error, state);
		        }
		      });
		    } else {
		      state.value = value;
		      state.state = FULFILLED;
		      notify(state, false);
		    }
		  } catch (error) {
		    internalReject({ done: false }, error, state);
		  }
		};

		// constructor polyfill
		if (FORCED_PROMISE_CONSTRUCTOR) {
		  // 25.4.3.1 Promise(executor)
		  PromiseConstructor = function Promise(executor) {
		    anInstance(this, PromisePrototype);
		    aCallable(executor);
		    call(Internal, this);
		    var state = getInternalPromiseState(this);
		    try {
		      executor(bind(internalResolve, state), bind(internalReject, state));
		    } catch (error) {
		      internalReject(state, error);
		    }
		  };

		  PromisePrototype = PromiseConstructor.prototype;

		  // eslint-disable-next-line no-unused-vars -- required for `.length`
		  Internal = function Promise(executor) {
		    setInternalState(this, {
		      type: PROMISE,
		      done: false,
		      notified: false,
		      parent: false,
		      reactions: new Queue(),
		      rejection: false,
		      state: PENDING,
		      value: null
		    });
		  };

		  // `Promise.prototype.then` method
		  // https://tc39.es/ecma262/#sec-promise.prototype.then
		  Internal.prototype = defineBuiltIn(PromisePrototype, 'then', function then(onFulfilled, onRejected) {
		    var state = getInternalPromiseState(this);
		    var reaction = newPromiseCapability(speciesConstructor(this, PromiseConstructor));
		    state.parent = true;
		    reaction.ok = isCallable(onFulfilled) ? onFulfilled : true;
		    reaction.fail = isCallable(onRejected) && onRejected;
		    reaction.domain = IS_NODE ? process.domain : undefined;
		    if (state.state === PENDING) state.reactions.add(reaction);
		    else microtask(function () {
		      callReaction(reaction, state);
		    });
		    return reaction.promise;
		  });

		  OwnPromiseCapability = function () {
		    var promise = new Internal();
		    var state = getInternalPromiseState(promise);
		    this.promise = promise;
		    this.resolve = bind(internalResolve, state);
		    this.reject = bind(internalReject, state);
		  };

		  newPromiseCapabilityModule.f = newPromiseCapability = function (C) {
		    return C === PromiseConstructor || C === PromiseWrapper
		      ? new OwnPromiseCapability(C)
		      : newGenericPromiseCapability(C);
		  };

		  if (!IS_PURE && isCallable(NativePromiseConstructor) && NativePromisePrototype !== Object.prototype) {
		    nativeThen = NativePromisePrototype.then;

		    if (!NATIVE_PROMISE_SUBCLASSING) {
		      // make `Promise#then` return a polyfilled `Promise` for native promise-based APIs
		      defineBuiltIn(NativePromisePrototype, 'then', function then(onFulfilled, onRejected) {
		        var that = this;
		        return new PromiseConstructor(function (resolve, reject) {
		          call(nativeThen, that, resolve, reject);
		        }).then(onFulfilled, onRejected);
		      // https://github.com/zloirock/core-js/issues/640
		      }, { unsafe: true });
		    }

		    // make `.constructor === Promise` work for native promise-based APIs
		    try {
		      delete NativePromisePrototype.constructor;
		    } catch (error) { /* empty */ }

		    // make `instanceof Promise` work for native promise-based APIs
		    if (setPrototypeOf) {
		      setPrototypeOf(NativePromisePrototype, PromisePrototype);
		    }
		  }
		}

		// `Promise` constructor
		// https://tc39.es/ecma262/#sec-promise-executor
		$({ global: true, constructor: true, wrap: true, forced: FORCED_PROMISE_CONSTRUCTOR }, {
		  Promise: PromiseConstructor
		});

		PromiseWrapper = path.Promise;

		setToStringTag(PromiseConstructor, PROMISE, false, true);
		setSpecies(PROMISE);
		return es_promise_constructor;
	}

	var es_promise_all = {};

	var promiseStaticsIncorrectIteration;
	var hasRequiredPromiseStaticsIncorrectIteration;

	function requirePromiseStaticsIncorrectIteration () {
		if (hasRequiredPromiseStaticsIncorrectIteration) return promiseStaticsIncorrectIteration;
		hasRequiredPromiseStaticsIncorrectIteration = 1;
		var NativePromiseConstructor = requirePromiseNativeConstructor();
		var checkCorrectnessOfIteration = requireCheckCorrectnessOfIteration();
		var FORCED_PROMISE_CONSTRUCTOR = requirePromiseConstructorDetection().CONSTRUCTOR;

		promiseStaticsIncorrectIteration = FORCED_PROMISE_CONSTRUCTOR || !checkCorrectnessOfIteration(function (iterable) {
		  NativePromiseConstructor.all(iterable).then(undefined, function () { /* empty */ });
		});
		return promiseStaticsIncorrectIteration;
	}

	var hasRequiredEs_promise_all;

	function requireEs_promise_all () {
		if (hasRequiredEs_promise_all) return es_promise_all;
		hasRequiredEs_promise_all = 1;
		var $ = require_export();
		var call = requireFunctionCall();
		var aCallable = requireACallable();
		var newPromiseCapabilityModule = requireNewPromiseCapability();
		var perform = requirePerform();
		var iterate = requireIterate();
		var PROMISE_STATICS_INCORRECT_ITERATION = requirePromiseStaticsIncorrectIteration();

		// `Promise.all` method
		// https://tc39.es/ecma262/#sec-promise.all
		$({ target: 'Promise', stat: true, forced: PROMISE_STATICS_INCORRECT_ITERATION }, {
		  all: function all(iterable) {
		    var C = this;
		    var capability = newPromiseCapabilityModule.f(C);
		    var resolve = capability.resolve;
		    var reject = capability.reject;
		    var result = perform(function () {
		      var $promiseResolve = aCallable(C.resolve);
		      var values = [];
		      var counter = 0;
		      var remaining = 1;
		      iterate(iterable, function (promise) {
		        var index = counter++;
		        var alreadyCalled = false;
		        remaining++;
		        call($promiseResolve, C, promise).then(function (value) {
		          if (alreadyCalled) return;
		          alreadyCalled = true;
		          values[index] = value;
		          --remaining || resolve(values);
		        }, reject);
		      });
		      --remaining || resolve(values);
		    });
		    if (result.error) reject(result.value);
		    return capability.promise;
		  }
		});
		return es_promise_all;
	}

	var es_promise_catch = {};

	var hasRequiredEs_promise_catch;

	function requireEs_promise_catch () {
		if (hasRequiredEs_promise_catch) return es_promise_catch;
		hasRequiredEs_promise_catch = 1;
		var $ = require_export();
		var IS_PURE = requireIsPure();
		var FORCED_PROMISE_CONSTRUCTOR = requirePromiseConstructorDetection().CONSTRUCTOR;
		var NativePromiseConstructor = requirePromiseNativeConstructor();
		var getBuiltIn = requireGetBuiltIn();
		var isCallable = requireIsCallable();
		var defineBuiltIn = requireDefineBuiltIn();

		var NativePromisePrototype = NativePromiseConstructor && NativePromiseConstructor.prototype;

		// `Promise.prototype.catch` method
		// https://tc39.es/ecma262/#sec-promise.prototype.catch
		$({ target: 'Promise', proto: true, forced: FORCED_PROMISE_CONSTRUCTOR, real: true }, {
		  'catch': function (onRejected) {
		    return this.then(undefined, onRejected);
		  }
		});

		// makes sure that native promise-based APIs `Promise#catch` properly works with patched `Promise#then`
		if (!IS_PURE && isCallable(NativePromiseConstructor)) {
		  var method = getBuiltIn('Promise').prototype['catch'];
		  if (NativePromisePrototype['catch'] !== method) {
		    defineBuiltIn(NativePromisePrototype, 'catch', method, { unsafe: true });
		  }
		}
		return es_promise_catch;
	}

	var es_promise_race = {};

	var hasRequiredEs_promise_race;

	function requireEs_promise_race () {
		if (hasRequiredEs_promise_race) return es_promise_race;
		hasRequiredEs_promise_race = 1;
		var $ = require_export();
		var call = requireFunctionCall();
		var aCallable = requireACallable();
		var newPromiseCapabilityModule = requireNewPromiseCapability();
		var perform = requirePerform();
		var iterate = requireIterate();
		var PROMISE_STATICS_INCORRECT_ITERATION = requirePromiseStaticsIncorrectIteration();

		// `Promise.race` method
		// https://tc39.es/ecma262/#sec-promise.race
		$({ target: 'Promise', stat: true, forced: PROMISE_STATICS_INCORRECT_ITERATION }, {
		  race: function race(iterable) {
		    var C = this;
		    var capability = newPromiseCapabilityModule.f(C);
		    var reject = capability.reject;
		    var result = perform(function () {
		      var $promiseResolve = aCallable(C.resolve);
		      iterate(iterable, function (promise) {
		        call($promiseResolve, C, promise).then(capability.resolve, reject);
		      });
		    });
		    if (result.error) reject(result.value);
		    return capability.promise;
		  }
		});
		return es_promise_race;
	}

	var es_promise_reject = {};

	var hasRequiredEs_promise_reject;

	function requireEs_promise_reject () {
		if (hasRequiredEs_promise_reject) return es_promise_reject;
		hasRequiredEs_promise_reject = 1;
		var $ = require_export();
		var newPromiseCapabilityModule = requireNewPromiseCapability();
		var FORCED_PROMISE_CONSTRUCTOR = requirePromiseConstructorDetection().CONSTRUCTOR;

		// `Promise.reject` method
		// https://tc39.es/ecma262/#sec-promise.reject
		$({ target: 'Promise', stat: true, forced: FORCED_PROMISE_CONSTRUCTOR }, {
		  reject: function reject(r) {
		    var capability = newPromiseCapabilityModule.f(this);
		    var capabilityReject = capability.reject;
		    capabilityReject(r);
		    return capability.promise;
		  }
		});
		return es_promise_reject;
	}

	var es_promise_resolve = {};

	var promiseResolve;
	var hasRequiredPromiseResolve;

	function requirePromiseResolve () {
		if (hasRequiredPromiseResolve) return promiseResolve;
		hasRequiredPromiseResolve = 1;
		var anObject = requireAnObject();
		var isObject = requireIsObject();
		var newPromiseCapability = requireNewPromiseCapability();

		promiseResolve = function (C, x) {
		  anObject(C);
		  if (isObject(x) && x.constructor === C) return x;
		  var promiseCapability = newPromiseCapability.f(C);
		  var resolve = promiseCapability.resolve;
		  resolve(x);
		  return promiseCapability.promise;
		};
		return promiseResolve;
	}

	var hasRequiredEs_promise_resolve;

	function requireEs_promise_resolve () {
		if (hasRequiredEs_promise_resolve) return es_promise_resolve;
		hasRequiredEs_promise_resolve = 1;
		var $ = require_export();
		var getBuiltIn = requireGetBuiltIn();
		var IS_PURE = requireIsPure();
		var NativePromiseConstructor = requirePromiseNativeConstructor();
		var FORCED_PROMISE_CONSTRUCTOR = requirePromiseConstructorDetection().CONSTRUCTOR;
		var promiseResolve = requirePromiseResolve();

		var PromiseConstructorWrapper = getBuiltIn('Promise');
		var CHECK_WRAPPER = IS_PURE && !FORCED_PROMISE_CONSTRUCTOR;

		// `Promise.resolve` method
		// https://tc39.es/ecma262/#sec-promise.resolve
		$({ target: 'Promise', stat: true, forced: IS_PURE || FORCED_PROMISE_CONSTRUCTOR }, {
		  resolve: function resolve(x) {
		    return promiseResolve(CHECK_WRAPPER && this === PromiseConstructorWrapper ? NativePromiseConstructor : this, x);
		  }
		});
		return es_promise_resolve;
	}

	var hasRequiredEs_promise;

	function requireEs_promise () {
		if (hasRequiredEs_promise) return es_promise;
		hasRequiredEs_promise = 1;
		// TODO: Remove this module from `core-js@4` since it's split to modules listed below
		requireEs_promise_constructor();
		requireEs_promise_all();
		requireEs_promise_catch();
		requireEs_promise_race();
		requireEs_promise_reject();
		requireEs_promise_resolve();
		return es_promise;
	}

	requireEs_promise();

	var es_string_endsWith = {};

	var toString;
	var hasRequiredToString;

	function requireToString () {
		if (hasRequiredToString) return toString;
		hasRequiredToString = 1;
		var classof = requireClassof();

		var $String = String;

		toString = function (argument) {
		  if (classof(argument) === 'Symbol') throw new TypeError('Cannot convert a Symbol value to a string');
		  return $String(argument);
		};
		return toString;
	}

	var isRegexp;
	var hasRequiredIsRegexp;

	function requireIsRegexp () {
		if (hasRequiredIsRegexp) return isRegexp;
		hasRequiredIsRegexp = 1;
		var isObject = requireIsObject();
		var classof = requireClassofRaw();
		var wellKnownSymbol = requireWellKnownSymbol();

		var MATCH = wellKnownSymbol('match');

		// `IsRegExp` abstract operation
		// https://tc39.es/ecma262/#sec-isregexp
		isRegexp = function (it) {
		  var isRegExp;
		  return isObject(it) && ((isRegExp = it[MATCH]) !== undefined ? !!isRegExp : classof(it) === 'RegExp');
		};
		return isRegexp;
	}

	var notARegexp;
	var hasRequiredNotARegexp;

	function requireNotARegexp () {
		if (hasRequiredNotARegexp) return notARegexp;
		hasRequiredNotARegexp = 1;
		var isRegExp = requireIsRegexp();

		var $TypeError = TypeError;

		notARegexp = function (it) {
		  if (isRegExp(it)) {
		    throw new $TypeError("The method doesn't accept regular expressions");
		  } return it;
		};
		return notARegexp;
	}

	var correctIsRegexpLogic;
	var hasRequiredCorrectIsRegexpLogic;

	function requireCorrectIsRegexpLogic () {
		if (hasRequiredCorrectIsRegexpLogic) return correctIsRegexpLogic;
		hasRequiredCorrectIsRegexpLogic = 1;
		var wellKnownSymbol = requireWellKnownSymbol();

		var MATCH = wellKnownSymbol('match');

		correctIsRegexpLogic = function (METHOD_NAME) {
		  var regexp = /./;
		  try {
		    '/./'[METHOD_NAME](regexp);
		  } catch (error1) {
		    try {
		      regexp[MATCH] = false;
		      return '/./'[METHOD_NAME](regexp);
		    } catch (error2) { /* empty */ }
		  } return false;
		};
		return correctIsRegexpLogic;
	}

	var hasRequiredEs_string_endsWith;

	function requireEs_string_endsWith () {
		if (hasRequiredEs_string_endsWith) return es_string_endsWith;
		hasRequiredEs_string_endsWith = 1;
		var $ = require_export();
		var uncurryThis = requireFunctionUncurryThisClause();
		var getOwnPropertyDescriptor = requireObjectGetOwnPropertyDescriptor().f;
		var toLength = requireToLength();
		var toString = requireToString();
		var notARegExp = requireNotARegexp();
		var requireObjectCoercible = requireRequireObjectCoercible();
		var correctIsRegExpLogic = requireCorrectIsRegexpLogic();
		var IS_PURE = requireIsPure();

		var slice = uncurryThis(''.slice);
		var min = Math.min;

		var CORRECT_IS_REGEXP_LOGIC = correctIsRegExpLogic('endsWith');
		// https://github.com/zloirock/core-js/pull/702
		var MDN_POLYFILL_BUG = !IS_PURE && !CORRECT_IS_REGEXP_LOGIC && !!function () {
		  var descriptor = getOwnPropertyDescriptor(String.prototype, 'endsWith');
		  return descriptor && !descriptor.writable;
		}();

		// `String.prototype.endsWith` method
		// https://tc39.es/ecma262/#sec-string.prototype.endswith
		$({ target: 'String', proto: true, forced: !MDN_POLYFILL_BUG && !CORRECT_IS_REGEXP_LOGIC }, {
		  endsWith: function endsWith(searchString /* , endPosition = @length */) {
		    var that = toString(requireObjectCoercible(this));
		    notARegExp(searchString);
		    var search = toString(searchString);
		    var endPosition = arguments.length > 1 ? arguments[1] : undefined;
		    var len = that.length;
		    var end = endPosition === undefined ? len : min(toLength(endPosition), len);
		    return slice(that, end - search.length, end) === search;
		  }
		});
		return es_string_endsWith;
	}

	requireEs_string_endsWith();

	var es_string_raw = {};

	var hasRequiredEs_string_raw;

	function requireEs_string_raw () {
		if (hasRequiredEs_string_raw) return es_string_raw;
		hasRequiredEs_string_raw = 1;
		var $ = require_export();
		var uncurryThis = requireFunctionUncurryThis();
		var toIndexedObject = requireToIndexedObject();
		var toObject = requireToObject();
		var toString = requireToString();
		var lengthOfArrayLike = requireLengthOfArrayLike();

		var push = uncurryThis([].push);
		var join = uncurryThis([].join);

		// `String.raw` method
		// https://tc39.es/ecma262/#sec-string.raw
		$({ target: 'String', stat: true }, {
		  raw: function raw(template) {
		    var rawTemplate = toIndexedObject(toObject(template).raw);
		    var literalSegments = lengthOfArrayLike(rawTemplate);
		    if (!literalSegments) return '';
		    var argumentsLength = arguments.length;
		    var elements = [];
		    var i = 0;
		    while (true) {
		      push(elements, toString(rawTemplate[i++]));
		      if (i === literalSegments) return join(elements, '');
		      if (i < argumentsLength) push(elements, toString(arguments[i]));
		    }
		  }
		});
		return es_string_raw;
	}

	requireEs_string_raw();

	var es_string_startsWith = {};

	var hasRequiredEs_string_startsWith;

	function requireEs_string_startsWith () {
		if (hasRequiredEs_string_startsWith) return es_string_startsWith;
		hasRequiredEs_string_startsWith = 1;
		var $ = require_export();
		var uncurryThis = requireFunctionUncurryThisClause();
		var getOwnPropertyDescriptor = requireObjectGetOwnPropertyDescriptor().f;
		var toLength = requireToLength();
		var toString = requireToString();
		var notARegExp = requireNotARegexp();
		var requireObjectCoercible = requireRequireObjectCoercible();
		var correctIsRegExpLogic = requireCorrectIsRegexpLogic();
		var IS_PURE = requireIsPure();

		var stringSlice = uncurryThis(''.slice);
		var min = Math.min;

		var CORRECT_IS_REGEXP_LOGIC = correctIsRegExpLogic('startsWith');
		// https://github.com/zloirock/core-js/pull/702
		var MDN_POLYFILL_BUG = !IS_PURE && !CORRECT_IS_REGEXP_LOGIC && !!function () {
		  var descriptor = getOwnPropertyDescriptor(String.prototype, 'startsWith');
		  return descriptor && !descriptor.writable;
		}();

		// `String.prototype.startsWith` method
		// https://tc39.es/ecma262/#sec-string.prototype.startswith
		$({ target: 'String', proto: true, forced: !MDN_POLYFILL_BUG && !CORRECT_IS_REGEXP_LOGIC }, {
		  startsWith: function startsWith(searchString /* , position = 0 */) {
		    var that = toString(requireObjectCoercible(this));
		    notARegExp(searchString);
		    var search = toString(searchString);
		    var index = toLength(min(arguments.length > 1 ? arguments[1] : undefined, that.length));
		    return stringSlice(that, index, index + search.length) === search;
		  }
		});
		return es_string_startsWith;
	}

	requireEs_string_startsWith();

	var es_weakSet = {};

	var es_weakSet_constructor = {};

	var arraySpeciesConstructor;
	var hasRequiredArraySpeciesConstructor;

	function requireArraySpeciesConstructor () {
		if (hasRequiredArraySpeciesConstructor) return arraySpeciesConstructor;
		hasRequiredArraySpeciesConstructor = 1;
		var isArray = requireIsArray();
		var isConstructor = requireIsConstructor();
		var isObject = requireIsObject();
		var wellKnownSymbol = requireWellKnownSymbol();

		var SPECIES = wellKnownSymbol('species');
		var $Array = Array;

		// a part of `ArraySpeciesCreate` abstract operation
		// https://tc39.es/ecma262/#sec-arrayspeciescreate
		arraySpeciesConstructor = function (originalArray) {
		  var C;
		  if (isArray(originalArray)) {
		    C = originalArray.constructor;
		    // cross-realm fallback
		    if (isConstructor(C) && (C === $Array || isArray(C.prototype))) C = undefined;
		    else if (isObject(C)) {
		      C = C[SPECIES];
		      if (C === null) C = undefined;
		    }
		  } return C === undefined ? $Array : C;
		};
		return arraySpeciesConstructor;
	}

	var arraySpeciesCreate;
	var hasRequiredArraySpeciesCreate;

	function requireArraySpeciesCreate () {
		if (hasRequiredArraySpeciesCreate) return arraySpeciesCreate;
		hasRequiredArraySpeciesCreate = 1;
		var arraySpeciesConstructor = requireArraySpeciesConstructor();

		// `ArraySpeciesCreate` abstract operation
		// https://tc39.es/ecma262/#sec-arrayspeciescreate
		arraySpeciesCreate = function (originalArray, length) {
		  return new (arraySpeciesConstructor(originalArray))(length === 0 ? 0 : length);
		};
		return arraySpeciesCreate;
	}

	var arrayIteration;
	var hasRequiredArrayIteration;

	function requireArrayIteration () {
		if (hasRequiredArrayIteration) return arrayIteration;
		hasRequiredArrayIteration = 1;
		var bind = requireFunctionBindContext();
		var IndexedObject = requireIndexedObject();
		var toObject = requireToObject();
		var lengthOfArrayLike = requireLengthOfArrayLike();
		var arraySpeciesCreate = requireArraySpeciesCreate();
		var createProperty = requireCreateProperty();

		// `Array.prototype.{ forEach, map, filter, some, every, find, findIndex, filterReject }` methods implementation
		var createMethod = function (TYPE) {
		  var IS_MAP = TYPE === 1;
		  var IS_FILTER = TYPE === 2;
		  var IS_SOME = TYPE === 3;
		  var IS_EVERY = TYPE === 4;
		  var IS_FIND_INDEX = TYPE === 6;
		  var IS_FILTER_REJECT = TYPE === 7;
		  var NO_HOLES = TYPE === 5 || IS_FIND_INDEX;
		  return function ($this, callbackfn, that) {
		    var O = toObject($this);
		    var self = IndexedObject(O);
		    var length = lengthOfArrayLike(self);
		    var boundFunction = bind(callbackfn, that);
		    var index = 0;
		    var resIndex = 0;
		    var target = IS_MAP ? arraySpeciesCreate($this, length) : IS_FILTER || IS_FILTER_REJECT ? arraySpeciesCreate($this, 0) : undefined;
		    var value, result;
		    for (;length > index; index++) if (NO_HOLES || index in self) {
		      value = self[index];
		      result = boundFunction(value, index, O);
		      if (TYPE) {
		        if (IS_MAP) createProperty(target, index, result);    // map
		        else if (result) switch (TYPE) {
		          case 3: return true;                                // some
		          case 5: return value;                               // find
		          case 6: return index;                               // findIndex
		          case 2: createProperty(target, resIndex++, value);  // filter
		        } else switch (TYPE) {
		          case 4: return false;                               // every
		          case 7: createProperty(target, resIndex++, value);  // filterReject
		        }
		      }
		    }
		    return IS_FIND_INDEX ? -1 : IS_SOME || IS_EVERY ? IS_EVERY : target;
		  };
		};

		arrayIteration = {
		  // `Array.prototype.forEach` method
		  // https://tc39.es/ecma262/#sec-array.prototype.foreach
		  forEach: createMethod(0),
		  // `Array.prototype.map` method
		  // https://tc39.es/ecma262/#sec-array.prototype.map
		  map: createMethod(1),
		  // `Array.prototype.filter` method
		  // https://tc39.es/ecma262/#sec-array.prototype.filter
		  filter: createMethod(2),
		  // `Array.prototype.some` method
		  // https://tc39.es/ecma262/#sec-array.prototype.some
		  some: createMethod(3),
		  // `Array.prototype.every` method
		  // https://tc39.es/ecma262/#sec-array.prototype.every
		  every: createMethod(4),
		  // `Array.prototype.find` method
		  // https://tc39.es/ecma262/#sec-array.prototype.find
		  find: createMethod(5),
		  // `Array.prototype.findIndex` method
		  // https://tc39.es/ecma262/#sec-array.prototype.findIndex
		  findIndex: createMethod(6),
		  // `Array.prototype.filterReject` method
		  // https://github.com/tc39/proposal-array-filtering
		  filterReject: createMethod(7)
		};
		return arrayIteration;
	}

	var collectionWeak;
	var hasRequiredCollectionWeak;

	function requireCollectionWeak () {
		if (hasRequiredCollectionWeak) return collectionWeak;
		hasRequiredCollectionWeak = 1;
		var uncurryThis = requireFunctionUncurryThis();
		var defineBuiltIns = requireDefineBuiltIns();
		var getWeakData = requireInternalMetadata().getWeakData;
		var anInstance = requireAnInstance();
		var anObject = requireAnObject();
		var isNullOrUndefined = requireIsNullOrUndefined();
		var isObject = requireIsObject();
		var iterate = requireIterate();
		var ArrayIterationModule = requireArrayIteration();
		var hasOwn = requireHasOwnProperty();
		var InternalStateModule = requireInternalState();

		var setInternalState = InternalStateModule.set;
		var internalStateGetterFor = InternalStateModule.getterFor;
		var find = ArrayIterationModule.find;
		var findIndex = ArrayIterationModule.findIndex;
		var splice = uncurryThis([].splice);
		var id = 0;

		// fallback for uncaught frozen keys
		var uncaughtFrozenStore = function (state) {
		  return state.frozen || (state.frozen = new UncaughtFrozenStore());
		};

		var UncaughtFrozenStore = function () {
		  this.entries = [];
		};

		var findUncaughtFrozen = function (store, key) {
		  return find(store.entries, function (it) {
		    return it[0] === key;
		  });
		};

		UncaughtFrozenStore.prototype = {
		  get: function (key) {
		    var entry = findUncaughtFrozen(this, key);
		    if (entry) return entry[1];
		  },
		  has: function (key) {
		    return !!findUncaughtFrozen(this, key);
		  },
		  set: function (key, value) {
		    var entry = findUncaughtFrozen(this, key);
		    if (entry) entry[1] = value;
		    else this.entries.push([key, value]);
		  },
		  'delete': function (key) {
		    var index = findIndex(this.entries, function (it) {
		      return it[0] === key;
		    });
		    if (~index) splice(this.entries, index, 1);
		    return !!~index;
		  }
		};

		collectionWeak = {
		  getConstructor: function (wrapper, CONSTRUCTOR_NAME, IS_MAP, ADDER) {
		    var Constructor = wrapper(function (that, iterable) {
		      anInstance(that, Prototype);
		      setInternalState(that, {
		        type: CONSTRUCTOR_NAME,
		        id: id++,
		        frozen: null
		      });
		      if (!isNullOrUndefined(iterable)) iterate(iterable, that[ADDER], { that: that, AS_ENTRIES: IS_MAP });
		    });

		    var Prototype = Constructor.prototype;

		    var getInternalState = internalStateGetterFor(CONSTRUCTOR_NAME);

		    var define = function (that, key, value) {
		      var state = getInternalState(that);
		      var data = getWeakData(anObject(key), true);
		      if (data === true) uncaughtFrozenStore(state).set(key, value);
		      else data[state.id] = value;
		      return that;
		    };

		    defineBuiltIns(Prototype, {
		      // `{ WeakMap, WeakSet }.prototype.delete(key)` methods
		      // https://tc39.es/ecma262/#sec-weakmap.prototype.delete
		      // https://tc39.es/ecma262/#sec-weakset.prototype.delete
		      'delete': function (key) {
		        var state = getInternalState(this);
		        if (!isObject(key)) return false;
		        var data = getWeakData(key);
		        if (data === true) return uncaughtFrozenStore(state)['delete'](key);
		        return data && hasOwn(data, state.id) && delete data[state.id];
		      },
		      // `{ WeakMap, WeakSet }.prototype.has(key)` methods
		      // https://tc39.es/ecma262/#sec-weakmap.prototype.has
		      // https://tc39.es/ecma262/#sec-weakset.prototype.has
		      has: function has(key) {
		        var state = getInternalState(this);
		        if (!isObject(key)) return false;
		        var data = getWeakData(key);
		        if (data === true) return uncaughtFrozenStore(state).has(key);
		        return data && hasOwn(data, state.id);
		      }
		    });

		    defineBuiltIns(Prototype, IS_MAP ? {
		      // `WeakMap.prototype.get(key)` method
		      // https://tc39.es/ecma262/#sec-weakmap.prototype.get
		      get: function get(key) {
		        var state = getInternalState(this);
		        if (isObject(key)) {
		          var data = getWeakData(key);
		          if (data === true) return uncaughtFrozenStore(state).get(key);
		          if (data) return data[state.id];
		        }
		      },
		      // `WeakMap.prototype.set(key, value)` method
		      // https://tc39.es/ecma262/#sec-weakmap.prototype.set
		      set: function set(key, value) {
		        return define(this, key, value);
		      }
		    } : {
		      // `WeakSet.prototype.add(value)` method
		      // https://tc39.es/ecma262/#sec-weakset.prototype.add
		      add: function add(value) {
		        return define(this, value, true);
		      }
		    });

		    return Constructor;
		  }
		};
		return collectionWeak;
	}

	var hasRequiredEs_weakSet_constructor;

	function requireEs_weakSet_constructor () {
		if (hasRequiredEs_weakSet_constructor) return es_weakSet_constructor;
		hasRequiredEs_weakSet_constructor = 1;
		var collection = requireCollection();
		var collectionWeak = requireCollectionWeak();

		// `WeakSet` constructor
		// https://tc39.es/ecma262/#sec-weakset-constructor
		collection('WeakSet', function (init) {
		  return function WeakSet() { return init(this, arguments.length ? arguments[0] : undefined); };
		}, collectionWeak);
		return es_weakSet_constructor;
	}

	var hasRequiredEs_weakSet;

	function requireEs_weakSet () {
		if (hasRequiredEs_weakSet) return es_weakSet;
		hasRequiredEs_weakSet = 1;
		// TODO: Remove this module from `core-js@4` since it's replaced to module below
		requireEs_weakSet_constructor();
		return es_weakSet;
	}

	requireEs_weakSet();

	function _arrayLikeToArray(r, a) {
	  (null == a || a > r.length) && (a = r.length);
	  for (var e = 0, n = Array(a); e < a; e++) n[e] = r[e];
	  return n;
	}
	function _arrayWithHoles(r) {
	  if (Array.isArray(r)) return r;
	}
	function _arrayWithoutHoles(r) {
	  if (Array.isArray(r)) return _arrayLikeToArray(r);
	}
	function _classCallCheck(a, n) {
	  if (!(a instanceof n)) throw new TypeError("Cannot call a class as a function");
	}
	function _defineProperties(e, r) {
	  for (var t = 0; t < r.length; t++) {
	    var o = r[t];
	    o.enumerable = o.enumerable || false, o.configurable = true, "value" in o && (o.writable = true), Object.defineProperty(e, _toPropertyKey(o.key), o);
	  }
	}
	function _createClass(e, r, t) {
	  return r && _defineProperties(e.prototype, r), Object.defineProperty(e, "prototype", {
	    writable: false
	  }), e;
	}
	function _createForOfIteratorHelper(r, e) {
	  var t = "undefined" != typeof Symbol && r[Symbol.iterator] || r["@@iterator"];
	  if (!t) {
	    if (Array.isArray(r) || (t = _unsupportedIterableToArray(r)) || e) {
	      t && (r = t);
	      var n = 0,
	        F = function () {};
	      return {
	        s: F,
	        n: function () {
	          return n >= r.length ? {
	            done: true
	          } : {
	            done: false,
	            value: r[n++]
	          };
	        },
	        e: function (r) {
	          throw r;
	        },
	        f: F
	      };
	    }
	    throw new TypeError("Invalid attempt to iterate non-iterable instance.\nIn order to be iterable, non-array objects must have a [Symbol.iterator]() method.");
	  }
	  var o,
	    a = true,
	    u = false;
	  return {
	    s: function () {
	      t = t.call(r);
	    },
	    n: function () {
	      var r = t.next();
	      return a = r.done, r;
	    },
	    e: function (r) {
	      u = true, o = r;
	    },
	    f: function () {
	      try {
	        a || null == t.return || t.return();
	      } finally {
	        if (u) throw o;
	      }
	    }
	  };
	}
	function _defineProperty(e, r, t) {
	  return (r = _toPropertyKey(r)) in e ? Object.defineProperty(e, r, {
	    value: t,
	    enumerable: true,
	    configurable: true,
	    writable: true
	  }) : e[r] = t, e;
	}
	function _iterableToArray(r) {
	  if ("undefined" != typeof Symbol && null != r[Symbol.iterator] || null != r["@@iterator"]) return Array.from(r);
	}
	function _iterableToArrayLimit(r, l) {
	  var t = null == r ? null : "undefined" != typeof Symbol && r[Symbol.iterator] || r["@@iterator"];
	  if (null != t) {
	    var e,
	      n,
	      i,
	      u,
	      a = [],
	      f = true,
	      o = false;
	    try {
	      if (i = (t = t.call(r)).next, 0 === l) {
	        if (Object(t) !== t) return;
	        f = !1;
	      } else for (; !(f = (e = i.call(t)).done) && (a.push(e.value), a.length !== l); f = !0);
	    } catch (r) {
	      o = true, n = r;
	    } finally {
	      try {
	        if (!f && null != t.return && (u = t.return(), Object(u) !== u)) return;
	      } finally {
	        if (o) throw n;
	      }
	    }
	    return a;
	  }
	}
	function _nonIterableRest() {
	  throw new TypeError("Invalid attempt to destructure non-iterable instance.\nIn order to be iterable, non-array objects must have a [Symbol.iterator]() method.");
	}
	function _nonIterableSpread() {
	  throw new TypeError("Invalid attempt to spread non-iterable instance.\nIn order to be iterable, non-array objects must have a [Symbol.iterator]() method.");
	}
	function ownKeys(e, r) {
	  var t = Object.keys(e);
	  if (Object.getOwnPropertySymbols) {
	    var o = Object.getOwnPropertySymbols(e);
	    r && (o = o.filter(function (r) {
	      return Object.getOwnPropertyDescriptor(e, r).enumerable;
	    })), t.push.apply(t, o);
	  }
	  return t;
	}
	function _objectSpread2(e) {
	  for (var r = 1; r < arguments.length; r++) {
	    var t = null != arguments[r] ? arguments[r] : {};
	    r % 2 ? ownKeys(Object(t), true).forEach(function (r) {
	      _defineProperty(e, r, t[r]);
	    }) : Object.getOwnPropertyDescriptors ? Object.defineProperties(e, Object.getOwnPropertyDescriptors(t)) : ownKeys(Object(t)).forEach(function (r) {
	      Object.defineProperty(e, r, Object.getOwnPropertyDescriptor(t, r));
	    });
	  }
	  return e;
	}
	function _slicedToArray(r, e) {
	  return _arrayWithHoles(r) || _iterableToArrayLimit(r, e) || _unsupportedIterableToArray(r, e) || _nonIterableRest();
	}
	function _toConsumableArray(r) {
	  return _arrayWithoutHoles(r) || _iterableToArray(r) || _unsupportedIterableToArray(r) || _nonIterableSpread();
	}
	function _toPrimitive(t, r) {
	  if ("object" != typeof t || !t) return t;
	  var e = t[Symbol.toPrimitive];
	  if (void 0 !== e) {
	    var i = e.call(t, r);
	    if ("object" != typeof i) return i;
	    throw new TypeError("@@toPrimitive must return a primitive value.");
	  }
	  return ("string" === r ? String : Number)(t);
	}
	function _toPropertyKey(t) {
	  var i = _toPrimitive(t, "string");
	  return "symbol" == typeof i ? i : i + "";
	}
	function _typeof(o) {
	  "@babel/helpers - typeof";

	  return _typeof = "function" == typeof Symbol && "symbol" == typeof Symbol.iterator ? function (o) {
	    return typeof o;
	  } : function (o) {
	    return o && "function" == typeof Symbol && o.constructor === Symbol && o !== Symbol.prototype ? "symbol" : typeof o;
	  }, _typeof(o);
	}
	function _unsupportedIterableToArray(r, a) {
	  if (r) {
	    if ("string" == typeof r) return _arrayLikeToArray(r, a);
	    var t = {}.toString.call(r).slice(8, -1);
	    return "Object" === t && r.constructor && (t = r.constructor.name), "Map" === t || "Set" === t ? Array.from(r) : "Arguments" === t || /^(?:Ui|I)nt(?:8|16|32)(?:Clamped)?Array$/.test(t) ? _arrayLikeToArray(r, a) : void 0;
	  }
	}

	var main = {exports: {}};

	var hasRequiredMain;
	function requireMain() {
	  if (hasRequiredMain) return main.exports;
	  hasRequiredMain = 1;
	  (function (module) {
	    (function () {

	      var AhoCorasick = function AhoCorasick(keywords) {
	        this._buildTables(keywords);
	      };
	      AhoCorasick.prototype._buildTables = function (keywords) {
	        var gotoFn = {
	          0: {}
	        };
	        var output = {};
	        var state = 0;
	        keywords.forEach(function (word) {
	          var curr = 0;
	          for (var i = 0; i < word.length; i++) {
	            var l = word[i];
	            if (gotoFn[curr] && l in gotoFn[curr]) {
	              curr = gotoFn[curr][l];
	            } else {
	              state++;
	              gotoFn[curr][l] = state;
	              gotoFn[state] = {};
	              curr = state;
	              output[state] = [];
	            }
	          }
	          output[curr].push(word);
	        });
	        var failure = {};
	        var xs = [];

	        // f(s) = 0 for all states of depth 1 (the ones from which the 0 state can transition to)
	        for (var l in gotoFn[0]) {
	          var state = gotoFn[0][l];
	          failure[state] = 0;
	          xs.push(state);
	        }
	        while (xs.length) {
	          var r = xs.shift();
	          // for each symbol a such that g(r, a) = s
	          for (var l in gotoFn[r]) {
	            var s = gotoFn[r][l];
	            xs.push(s);

	            // set state = f(r)
	            var state = failure[r];
	            while (state > 0 && !(l in gotoFn[state])) {
	              state = failure[state];
	            }
	            if (l in gotoFn[state]) {
	              var fs = gotoFn[state][l];
	              failure[s] = fs;
	              output[s] = output[s].concat(output[fs]);
	            } else {
	              failure[s] = 0;
	            }
	          }
	        }
	        this.gotoFn = gotoFn;
	        this.output = output;
	        this.failure = failure;
	      };
	      AhoCorasick.prototype.search = function (string) {
	        var state = 0;
	        var results = [];
	        for (var i = 0; i < string.length; i++) {
	          var l = string[i];
	          while (state > 0 && !(l in this.gotoFn[state])) {
	            state = this.failure[state];
	          }
	          if (!(l in this.gotoFn[state])) {
	            continue;
	          }
	          state = this.gotoFn[state][l];
	          if (this.output[state].length) {
	            var foundStrs = this.output[state];
	            results.push([i, foundStrs]);
	          }
	        }
	        return results;
	      };
	      {
	        module.exports = AhoCorasick;
	      }
	    })();
	  })(main);
	  return main.exports;
	}

	var mainExports = requireMain();
	var AhoCorasick = /*@__PURE__*/getDefaultExportFromCjs(mainExports);

	var support = {};

	var hasRequiredSupport;

	function requireSupport () {
		if (hasRequiredSupport) return support;
		hasRequiredSupport = 1;
		support.ARRAY_BUFFER_SUPPORT = typeof ArrayBuffer !== 'undefined';
		support.SYMBOL_SUPPORT = typeof Symbol !== 'undefined';
		return support;
	}

	/**
	 * Obliterator ForEach Function
	 * =============================
	 *
	 * Helper function used to easily iterate over mixed values.
	 */

	var foreach;
	var hasRequiredForeach;

	function requireForeach () {
		if (hasRequiredForeach) return foreach;
		hasRequiredForeach = 1;
		var support = requireSupport();

		var ARRAY_BUFFER_SUPPORT = support.ARRAY_BUFFER_SUPPORT;
		var SYMBOL_SUPPORT = support.SYMBOL_SUPPORT;

		/**
		 * Function able to iterate over almost any iterable JS value.
		 *
		 * @param  {any}      iterable - Iterable value.
		 * @param  {function} callback - Callback function.
		 */
		foreach = function forEach(iterable, callback) {
		  var iterator, k, i, l, s;

		  if (!iterable) throw new Error('obliterator/forEach: invalid iterable.');

		  if (typeof callback !== 'function')
		    throw new Error('obliterator/forEach: expecting a callback.');

		  // The target is an array or a string or function arguments
		  if (
		    Array.isArray(iterable) ||
		    (ARRAY_BUFFER_SUPPORT && ArrayBuffer.isView(iterable)) ||
		    typeof iterable === 'string' ||
		    iterable.toString() === '[object Arguments]'
		  ) {
		    for (i = 0, l = iterable.length; i < l; i++) callback(iterable[i], i);
		    return;
		  }

		  // The target has a #.forEach method
		  if (typeof iterable.forEach === 'function') {
		    iterable.forEach(callback);
		    return;
		  }

		  // The target is iterable
		  if (
		    SYMBOL_SUPPORT &&
		    Symbol.iterator in iterable &&
		    typeof iterable.next !== 'function'
		  ) {
		    iterable = iterable[Symbol.iterator]();
		  }

		  // The target is an iterator
		  if (typeof iterable.next === 'function') {
		    iterator = iterable;
		    i = 0;

		    while (((s = iterator.next()), s.done !== true)) {
		      callback(s.value, i);
		      i++;
		    }

		    return;
		  }

		  // The target is a plain object
		  for (k in iterable) {
		    if (iterable.hasOwnProperty(k)) {
		      callback(iterable[k], k);
		    }
		  }

		  return;
		};
		return foreach;
	}

	var iterables = {};

	var typedArrays = {};

	/**
	 * Mnemonist Typed Array Helpers
	 * ==============================
	 *
	 * Miscellaneous helpers related to typed arrays.
	 */
	var hasRequiredTypedArrays;
	function requireTypedArrays() {
	  if (hasRequiredTypedArrays) return typedArrays;
	  hasRequiredTypedArrays = 1;
	  (function (exports) {
	    /**
	     * When using an unsigned integer array to store pointers, one might want to
	     * choose the optimal word size in regards to the actual numbers of pointers
	     * to store.
	     *
	     * This helpers does just that.
	     *
	     * @param  {number} size - Expected size of the array to map.
	     * @return {TypedArray}
	     */
	    var MAX_8BIT_INTEGER = Math.pow(2, 8) - 1,
	      MAX_16BIT_INTEGER = Math.pow(2, 16) - 1,
	      MAX_32BIT_INTEGER = Math.pow(2, 32) - 1;
	    var MAX_SIGNED_8BIT_INTEGER = Math.pow(2, 7) - 1,
	      MAX_SIGNED_16BIT_INTEGER = Math.pow(2, 15) - 1,
	      MAX_SIGNED_32BIT_INTEGER = Math.pow(2, 31) - 1;
	    exports.getPointerArray = function (size) {
	      var maxIndex = size - 1;
	      if (maxIndex <= MAX_8BIT_INTEGER) return Uint8Array;
	      if (maxIndex <= MAX_16BIT_INTEGER) return Uint16Array;
	      if (maxIndex <= MAX_32BIT_INTEGER) return Uint32Array;
	      throw new Error('mnemonist: Pointer Array of size > 4294967295 is not supported.');
	    };
	    exports.getSignedPointerArray = function (size) {
	      var maxIndex = size - 1;
	      if (maxIndex <= MAX_SIGNED_8BIT_INTEGER) return Int8Array;
	      if (maxIndex <= MAX_SIGNED_16BIT_INTEGER) return Int16Array;
	      if (maxIndex <= MAX_SIGNED_32BIT_INTEGER) return Int32Array;
	      return Float64Array;
	    };

	    /**
	     * Function returning the minimal type able to represent the given number.
	     *
	     * @param  {number} value - Value to test.
	     * @return {TypedArrayClass}
	     */
	    exports.getNumberType = function (value) {
	      // <= 32 bits itnteger?
	      if (value === (value | 0)) {
	        // Negative
	        if (Math.sign(value) === -1) {
	          if (value <= 127 && value >= -128) return Int8Array;
	          if (value <= 32767 && value >= -32768) return Int16Array;
	          return Int32Array;
	        } else {
	          if (value <= 255) return Uint8Array;
	          if (value <= 65535) return Uint16Array;
	          return Uint32Array;
	        }
	      }

	      // 53 bits integer & floats
	      // NOTE: it's kinda hard to tell whether we could use 32bits or not...
	      return Float64Array;
	    };

	    /**
	     * Function returning the minimal type able to represent the given array
	     * of JavaScript numbers.
	     *
	     * @param  {array}    array  - Array to represent.
	     * @param  {function} getter - Optional getter.
	     * @return {TypedArrayClass}
	     */
	    var TYPE_PRIORITY = {
	      Uint8Array: 1,
	      Int8Array: 2,
	      Uint16Array: 3,
	      Int16Array: 4,
	      Uint32Array: 5,
	      Int32Array: 6,
	      Float32Array: 7,
	      Float64Array: 8
	    };

	    // TODO: make this a one-shot for one value
	    exports.getMinimalRepresentation = function (array, getter) {
	      var maxType = null,
	        maxPriority = 0,
	        p,
	        t,
	        v,
	        i,
	        l;
	      for (i = 0, l = array.length; i < l; i++) {
	        v = getter ? getter(array[i]) : array[i];
	        t = exports.getNumberType(v);
	        p = TYPE_PRIORITY[t.name];
	        if (p > maxPriority) {
	          maxPriority = p;
	          maxType = t;
	        }
	      }
	      return maxType;
	    };

	    /**
	     * Function returning whether the given value is a typed array.
	     *
	     * @param  {any} value - Value to test.
	     * @return {boolean}
	     */
	    exports.isTypedArray = function (value) {
	      return typeof ArrayBuffer !== 'undefined' && ArrayBuffer.isView(value);
	    };

	    /**
	     * Function used to concat byte arrays.
	     *
	     * @param  {...ByteArray}
	     * @return {ByteArray}
	     */
	    exports.concat = function () {
	      var length = 0,
	        i,
	        o,
	        l;
	      for (i = 0, l = arguments.length; i < l; i++) length += arguments[i].length;
	      var array = new arguments[0].constructor(length);
	      for (i = 0, o = 0; i < l; i++) {
	        array.set(arguments[i], o);
	        o += arguments[i].length;
	      }
	      return array;
	    };

	    /**
	     * Function used to initialize a byte array of indices.
	     *
	     * @param  {number}    length - Length of target.
	     * @return {ByteArray}
	     */
	    exports.indices = function (length) {
	      var PointerArray = exports.getPointerArray(length);
	      var array = new PointerArray(length);
	      for (var i = 0; i < length; i++) array[i] = i;
	      return array;
	    };
	  })(typedArrays);
	  return typedArrays;
	}

	/**
	 * Mnemonist Iterable Function
	 * ============================
	 *
	 * Harmonized iteration helpers over mixed iterable targets.
	 */
	var hasRequiredIterables;
	function requireIterables() {
	  if (hasRequiredIterables) return iterables;
	  hasRequiredIterables = 1;
	  var forEach = requireForeach();
	  var typed = /*@__PURE__*/requireTypedArrays();

	  /**
	   * Function used to determine whether the given object supports array-like
	   * random access.
	   *
	   * @param  {any} target - Target object.
	   * @return {boolean}
	   */
	  function isArrayLike(target) {
	    return Array.isArray(target) || typed.isTypedArray(target);
	  }

	  /**
	   * Function used to guess the length of the structure over which we are going
	   * to iterate.
	   *
	   * @param  {any} target - Target object.
	   * @return {number|undefined}
	   */
	  function guessLength(target) {
	    if (typeof target.length === 'number') return target.length;
	    if (typeof target.size === 'number') return target.size;
	    return;
	  }

	  /**
	   * Function used to convert an iterable to an array.
	   *
	   * @param  {any}   target - Iteration target.
	   * @return {array}
	   */
	  function toArray(target) {
	    var l = guessLength(target);
	    var array = typeof l === 'number' ? new Array(l) : [];
	    var i = 0;

	    // TODO: we could optimize when given target is array like
	    forEach(target, function (value) {
	      array[i++] = value;
	    });
	    return array;
	  }

	  /**
	   * Same as above but returns a supplementary indices array.
	   *
	   * @param  {any}   target - Iteration target.
	   * @return {array}
	   */
	  function toArrayWithIndices(target) {
	    var l = guessLength(target);
	    var IndexArray = typeof l === 'number' ? typed.getPointerArray(l) : Array;
	    var array = typeof l === 'number' ? new Array(l) : [];
	    var indices = typeof l === 'number' ? new IndexArray(l) : [];
	    var i = 0;

	    // TODO: we could optimize when given target is array like
	    forEach(target, function (value) {
	      array[i] = value;
	      indices[i] = i++;
	    });
	    return [array, indices];
	  }

	  /**
	   * Exporting.
	   */
	  iterables.isArrayLike = isArrayLike;
	  iterables.guessLength = guessLength;
	  iterables.toArray = toArray;
	  iterables.toArrayWithIndices = toArrayWithIndices;
	  return iterables;
	}

	/**
	 * Obliterator Iterator Class
	 * ===========================
	 *
	 * Simple class representing the library's iterators.
	 */

	var iterator;
	var hasRequiredIterator;

	function requireIterator () {
		if (hasRequiredIterator) return iterator;
		hasRequiredIterator = 1;
		/**
		 * Iterator class.
		 *
		 * @constructor
		 * @param {function} next - Next function.
		 */
		function Iterator(next) {
		  if (typeof next !== 'function')
		    throw new Error('obliterator/iterator: expecting a function!');

		  this.next = next;
		}

		/**
		 * If symbols are supported, we add `next` to `Symbol.iterator`.
		 */
		if (typeof Symbol !== 'undefined')
		  Iterator.prototype[Symbol.iterator] = function () {
		    return this;
		  };

		/**
		 * Returning an iterator of the given values.
		 *
		 * @param  {any...} values - Values.
		 * @return {Iterator}
		 */
		Iterator.of = function () {
		  var args = arguments,
		    l = args.length,
		    i = 0;

		  return new Iterator(function () {
		    if (i >= l) return {done: true};

		    return {done: false, value: args[i++]};
		  });
		};

		/**
		 * Returning an empty iterator.
		 *
		 * @return {Iterator}
		 */
		Iterator.empty = function () {
		  var iterator = new Iterator(function () {
		    return {done: true};
		  });

		  return iterator;
		};

		/**
		 * Returning an iterator over the given indexed sequence.
		 *
		 * @param  {string|Array} sequence - Target sequence.
		 * @return {Iterator}
		 */
		Iterator.fromSequence = function (sequence) {
		  var i = 0,
		    l = sequence.length;

		  return new Iterator(function () {
		    if (i >= l) return {done: true};

		    return {done: false, value: sequence[i++]};
		  });
		};

		/**
		 * Returning whether the given value is an iterator.
		 *
		 * @param  {any} value - Value.
		 * @return {boolean}
		 */
		Iterator.is = function (value) {
		  if (value instanceof Iterator) return true;

		  return (
		    typeof value === 'object' &&
		    value !== null &&
		    typeof value.next === 'function'
		  );
		};

		/**
		 * Exporting.
		 */
		iterator = Iterator;
		return iterator;
	}

	/**
	 * Mnemonist LRUCache
	 * ===================
	 *
	 * JavaScript implementation of the LRU Cache data structure. To save up
	 * memory and allocations this implementation represents its underlying
	 * doubly-linked list as static arrays and pointers. Thus, memory is allocated
	 * only once at instantiation and JS objects are never created to serve as
	 * pointers. This also means this implementation does not trigger too many
	 * garbage collections.
	 *
	 * Note that to save up memory, a LRU Cache can be implemented using a singly
	 * linked list by storing predecessors' pointers as hashmap values.
	 * However, this means more hashmap lookups and would probably slow the whole
	 * thing down. What's more, pointers are not the things taking most space in
	 * memory.
	 */
	var lruCache;
	var hasRequiredLruCache;
	function requireLruCache() {
	  if (hasRequiredLruCache) return lruCache;
	  hasRequiredLruCache = 1;
	  var Iterator = requireIterator(),
	    forEach = requireForeach(),
	    typed = /*@__PURE__*/requireTypedArrays(),
	    iterables = /*@__PURE__*/requireIterables();

	  /**
	   * LRUCache.
	   *
	   * @constructor
	   * @param {function} Keys     - Array class for storing keys.
	   * @param {function} Values   - Array class for storing values.
	   * @param {number}   capacity - Desired capacity.
	   */
	  function LRUCache(Keys, Values, capacity) {
	    if (arguments.length < 2) {
	      capacity = Keys;
	      Keys = null;
	      Values = null;
	    }
	    this.capacity = capacity;
	    if (typeof this.capacity !== 'number' || this.capacity <= 0) throw new Error('mnemonist/lru-cache: capacity should be positive number.');else if (!isFinite(this.capacity) || Math.floor(this.capacity) !== this.capacity) throw new Error('mnemonist/lru-cache: capacity should be a finite positive integer.');
	    var PointerArray = typed.getPointerArray(capacity);
	    this.forward = new PointerArray(capacity);
	    this.backward = new PointerArray(capacity);
	    this.K = typeof Keys === 'function' ? new Keys(capacity) : new Array(capacity);
	    this.V = typeof Values === 'function' ? new Values(capacity) : new Array(capacity);

	    // Properties
	    this.size = 0;
	    this.head = 0;
	    this.tail = 0;
	    this.items = {};
	  }

	  /**
	   * Method used to clear the structure.
	   *
	   * @return {undefined}
	   */
	  LRUCache.prototype.clear = function () {
	    this.size = 0;
	    this.head = 0;
	    this.tail = 0;
	    this.items = {};
	  };

	  /**
	   * Method used to splay a value on top.
	   *
	   * @param  {number}   pointer - Pointer of the value to splay on top.
	   * @return {LRUCache}
	   */
	  LRUCache.prototype.splayOnTop = function (pointer) {
	    var oldHead = this.head;
	    if (this.head === pointer) return this;
	    var previous = this.backward[pointer],
	      next = this.forward[pointer];
	    if (this.tail === pointer) {
	      this.tail = previous;
	    } else {
	      this.backward[next] = previous;
	    }
	    this.forward[previous] = next;
	    this.backward[oldHead] = pointer;
	    this.head = pointer;
	    this.forward[pointer] = oldHead;
	    return this;
	  };

	  /**
	   * Method used to set the value for the given key in the cache.
	   *
	   * @param  {any} key   - Key.
	   * @param  {any} value - Value.
	   * @return {undefined}
	   */
	  LRUCache.prototype.set = function (key, value) {
	    var pointer = this.items[key];

	    // The key already exists, we just need to update the value and splay on top
	    if (typeof pointer !== 'undefined') {
	      this.splayOnTop(pointer);
	      this.V[pointer] = value;
	      return;
	    }

	    // The cache is not yet full
	    if (this.size < this.capacity) {
	      pointer = this.size++;
	    }

	    // Cache is full, we need to drop the last value
	    else {
	      pointer = this.tail;
	      this.tail = this.backward[pointer];
	      delete this.items[this.K[pointer]];
	    }

	    // Storing key & value
	    this.items[key] = pointer;
	    this.K[pointer] = key;
	    this.V[pointer] = value;

	    // Moving the item at the front of the list
	    this.forward[pointer] = this.head;
	    this.backward[this.head] = pointer;
	    this.head = pointer;
	  };

	  /**
	   * Method used to set the value for the given key in the cache
	   *
	   * @param  {any} key   - Key.
	   * @param  {any} value - Value.
	   * @return {{evicted: boolean, key: any, value: any}} An object containing the
	   * key and value of an item that was overwritten or evicted in the set
	   * operation, as well as a boolean indicating whether it was evicted due to
	   * limited capacity. Return value is null if nothing was evicted or overwritten
	   * during the set operation.
	   */
	  LRUCache.prototype.setpop = function (key, value) {
	    var oldValue = null;
	    var oldKey = null;
	    var pointer = this.items[key];

	    // The key already exists, we just need to update the value and splay on top
	    if (typeof pointer !== 'undefined') {
	      this.splayOnTop(pointer);
	      oldValue = this.V[pointer];
	      this.V[pointer] = value;
	      return {
	        evicted: false,
	        key: key,
	        value: oldValue
	      };
	    }

	    // The cache is not yet full
	    if (this.size < this.capacity) {
	      pointer = this.size++;
	    }

	    // Cache is full, we need to drop the last value
	    else {
	      pointer = this.tail;
	      this.tail = this.backward[pointer];
	      oldValue = this.V[pointer];
	      oldKey = this.K[pointer];
	      delete this.items[oldKey];
	    }

	    // Storing key & value
	    this.items[key] = pointer;
	    this.K[pointer] = key;
	    this.V[pointer] = value;

	    // Moving the item at the front of the list
	    this.forward[pointer] = this.head;
	    this.backward[this.head] = pointer;
	    this.head = pointer;

	    // Return object if eviction took place, otherwise return null
	    if (oldKey) {
	      return {
	        evicted: true,
	        key: oldKey,
	        value: oldValue
	      };
	    } else {
	      return null;
	    }
	  };

	  /**
	   * Method used to check whether the key exists in the cache.
	   *
	   * @param  {any} key   - Key.
	   * @return {boolean}
	   */
	  LRUCache.prototype.has = function (key) {
	    return key in this.items;
	  };

	  /**
	   * Method used to get the value attached to the given key. Will move the
	   * related key to the front of the underlying linked list.
	   *
	   * @param  {any} key   - Key.
	   * @return {any}
	   */
	  LRUCache.prototype.get = function (key) {
	    var pointer = this.items[key];
	    if (typeof pointer === 'undefined') return;
	    this.splayOnTop(pointer);
	    return this.V[pointer];
	  };

	  /**
	   * Method used to get the value attached to the given key. Does not modify
	   * the ordering of the underlying linked list.
	   *
	   * @param  {any} key   - Key.
	   * @return {any}
	   */
	  LRUCache.prototype.peek = function (key) {
	    var pointer = this.items[key];
	    if (typeof pointer === 'undefined') return;
	    return this.V[pointer];
	  };

	  /**
	   * Method used to iterate over the cache's entries using a callback.
	   *
	   * @param  {function}  callback - Function to call for each item.
	   * @param  {object}    scope    - Optional scope.
	   * @return {undefined}
	   */
	  LRUCache.prototype.forEach = function (callback, scope) {
	    scope = arguments.length > 1 ? scope : this;
	    var i = 0,
	      l = this.size;
	    var pointer = this.head,
	      keys = this.K,
	      values = this.V,
	      forward = this.forward;
	    while (i < l) {
	      callback.call(scope, values[pointer], keys[pointer], this);
	      pointer = forward[pointer];
	      i++;
	    }
	  };

	  /**
	   * Method used to create an iterator over the cache's keys from most
	   * recently used to least recently used.
	   *
	   * @return {Iterator}
	   */
	  LRUCache.prototype.keys = function () {
	    var i = 0,
	      l = this.size;
	    var pointer = this.head,
	      keys = this.K,
	      forward = this.forward;
	    return new Iterator(function () {
	      if (i >= l) return {
	        done: true
	      };
	      var key = keys[pointer];
	      i++;
	      if (i < l) pointer = forward[pointer];
	      return {
	        done: false,
	        value: key
	      };
	    });
	  };

	  /**
	   * Method used to create an iterator over the cache's values from most
	   * recently used to least recently used.
	   *
	   * @return {Iterator}
	   */
	  LRUCache.prototype.values = function () {
	    var i = 0,
	      l = this.size;
	    var pointer = this.head,
	      values = this.V,
	      forward = this.forward;
	    return new Iterator(function () {
	      if (i >= l) return {
	        done: true
	      };
	      var value = values[pointer];
	      i++;
	      if (i < l) pointer = forward[pointer];
	      return {
	        done: false,
	        value: value
	      };
	    });
	  };

	  /**
	   * Method used to create an iterator over the cache's entries from most
	   * recently used to least recently used.
	   *
	   * @return {Iterator}
	   */
	  LRUCache.prototype.entries = function () {
	    var i = 0,
	      l = this.size;
	    var pointer = this.head,
	      keys = this.K,
	      values = this.V,
	      forward = this.forward;
	    return new Iterator(function () {
	      if (i >= l) return {
	        done: true
	      };
	      var key = keys[pointer],
	        value = values[pointer];
	      i++;
	      if (i < l) pointer = forward[pointer];
	      return {
	        done: false,
	        value: [key, value]
	      };
	    });
	  };

	  /**
	   * Attaching the #.entries method to Symbol.iterator if possible.
	   */
	  if (typeof Symbol !== 'undefined') LRUCache.prototype[Symbol.iterator] = LRUCache.prototype.entries;

	  /**
	   * Convenience known methods.
	   */
	  LRUCache.prototype.inspect = function () {
	    var proxy = new Map();
	    var iterator = this.entries(),
	      step;
	    while (step = iterator.next(), !step.done) proxy.set(step.value[0], step.value[1]);

	    // Trick so that node displays the name of the constructor
	    Object.defineProperty(proxy, 'constructor', {
	      value: LRUCache,
	      enumerable: false
	    });
	    return proxy;
	  };
	  if (typeof Symbol !== 'undefined') LRUCache.prototype[Symbol.for('nodejs.util.inspect.custom')] = LRUCache.prototype.inspect;

	  /**
	   * Static @.from function taking an arbitrary iterable & converting it into
	   * a structure.
	   *
	   * @param  {Iterable} iterable - Target iterable.
	   * @param  {function} Keys     - Array class for storing keys.
	   * @param  {function} Values   - Array class for storing values.
	   * @param  {number}   capacity - Cache's capacity.
	   * @return {LRUCache}
	   */
	  LRUCache.from = function (iterable, Keys, Values, capacity) {
	    if (arguments.length < 2) {
	      capacity = iterables.guessLength(iterable);
	      if (typeof capacity !== 'number') throw new Error('mnemonist/lru-cache.from: could not guess iterable length. Please provide desired capacity as last argument.');
	    } else if (arguments.length === 2) {
	      capacity = Keys;
	      Keys = null;
	      Values = null;
	    }
	    var cache = new LRUCache(Keys, Values, capacity);
	    forEach(iterable, function (value, key) {
	      cache.set(key, value);
	    });
	    return cache;
	  };

	  /**
	   * Exporting.
	   */
	  lruCache = LRUCache;
	  return lruCache;
	}

	var lruCacheExports = /*@__PURE__*/ requireLruCache();
	var LRUCache = /*@__PURE__*/getDefaultExportFromCjs(lruCacheExports);

	// error.ts — Error handling utilities.
	// Uses alert() for explicit reporting to ensure maximum compatibility across
	// NW.js, JoiPlay, and browsers. Ported from translator_scratch.
	// =============================================================================
	// Error display
	// =============================================================================
	/**
	 * Attempt to gracefully stop the game's audio and scene before throwing.
	 */
	function tryStopGame() {
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
	function showError(name, message) {
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
	function showWarning(name, message) {
	  var fullMessage = "[NST Warning] " + name + "\n" + message;
	  console.warn(fullMessage);
	}
	// =============================================================================
	// Result unwrap
	// =============================================================================
	/**
	 * Unwrap a Result. If it is an error, display the error and abort.
	 */
	function unwrap(result, context) {
	  if (result.isErr()) {
	    showError(context, result.error);
	  }
	  return result.value;
	}

	// neverthrow.ts — Simplified Result type implementation.
	// ES5-compatible, following Suckless & KISS principles.
	// Ported from translator_scratch.
	function ok(value) {
	  var result = {
	    _tag: "Ok",
	    value: value,
	    error: undefined,
	    isOk: function isOk() {
	      return true;
	    },
	    isErr: function isErr() {
	      return false;
	    },
	    map: function map(fn) {
	      return ok(fn(value));
	    },
	    mapErr: function mapErr(_fn) {
	      return this;
	    }
	  };
	  return result;
	}
	function err(error) {
	  var result = {
	    _tag: "Err",
	    value: undefined,
	    error: error,
	    isOk: function isOk() {
	      return false;
	    },
	    isErr: function isErr() {
	      return true;
	    },
	    map: function map(_fn) {
	      return this;
	    },
	    mapErr: function mapErr(fn) {
	      return err(fn(error));
	    }
	  };
	  return result;
	}

	// zod.ts - 简化的 Schema 验证实现
	// ES5 兼容，遵循 Suckless & KISS 原则
	// =============================================================================
	// 辅助函数
	// =============================================================================
	function createError(message, path) {
	  return {
	    message: message,
	    issues: [{
	      path: [],
	      message: message
	    }]
	  };
	}
	function isObject(value) {
	  return _typeof(value) === "object" && value !== null && !Array.isArray(value);
	}
	// =============================================================================
	// Schema 实现
	// =============================================================================
	function createStringSchema(minLength) {
	  var schema = {
	    _type: "",
	    parse: function parse(data) {
	      if (typeof data !== "string") {
	        throw new TypeError("Expected string, got " + _typeof(data));
	      }
	      if (minLength !== undefined && data.length < minLength) {
	        throw new Error("String must be at least " + minLength + " characters");
	      }
	      return data;
	    },
	    safeParse: function safeParse(data) {
	      try {
	        return {
	          success: true,
	          data: this.parse(data)
	        };
	      } catch (e) {
	        var msg = e instanceof Error ? e.message : String(e);
	        return {
	          success: false,
	          error: createError(msg)
	        };
	      }
	    },
	    optional: function optional() {
	      return createOptionalSchema(this);
	    },
	    default: function _default(defaultValue) {
	      return createDefaultSchema(this, defaultValue);
	    },
	    min: function min(length) {
	      return createStringSchema(length);
	    }
	  };
	  return schema;
	}
	function createNumberSchema() {
	  var schema = {
	    _type: 0,
	    parse: function parse(data) {
	      if (typeof data !== "number") {
	        throw new Error("Expected number, got " + _typeof(data));
	      }
	      return data;
	    },
	    safeParse: function safeParse(data) {
	      try {
	        return {
	          success: true,
	          data: this.parse(data)
	        };
	      } catch (e) {
	        var msg = e instanceof Error ? e.message : String(e);
	        return {
	          success: false,
	          error: createError(msg)
	        };
	      }
	    },
	    optional: function optional() {
	      return createOptionalSchema(this);
	    },
	    default: function _default(defaultValue) {
	      return createDefaultSchema(this, defaultValue);
	    }
	  };
	  return schema;
	}
	function createBooleanSchema() {
	  var schema = {
	    _type: false,
	    parse: function parse(data) {
	      if (typeof data !== "boolean") {
	        throw new Error("Expected boolean, got " + _typeof(data));
	      }
	      return data;
	    },
	    safeParse: function safeParse(data) {
	      try {
	        return {
	          success: true,
	          data: this.parse(data)
	        };
	      } catch (e) {
	        var msg = e instanceof Error ? e.message : String(e);
	        return {
	          success: false,
	          error: createError(msg)
	        };
	      }
	    },
	    optional: function optional() {
	      return createOptionalSchema(this);
	    },
	    default: function _default(defaultValue) {
	      return createDefaultSchema(this, defaultValue);
	    }
	  };
	  return schema;
	}
	function createArraySchema(itemSchema) {
	  var schema = {
	    _type: [],
	    parse: function parse(data) {
	      if (!Array.isArray(data)) {
	        throw new Error("Expected array, got " + _typeof(data));
	      }
	      var result = [];
	      for (var i = 0; i < data.length; i++) {
	        result.push(itemSchema.parse(data[i]));
	      }
	      return result;
	    },
	    safeParse: function safeParse(data) {
	      try {
	        return {
	          success: true,
	          data: this.parse(data)
	        };
	      } catch (e) {
	        var msg = e instanceof Error ? e.message : String(e);
	        return {
	          success: false,
	          error: createError(msg)
	        };
	      }
	    },
	    optional: function optional() {
	      return createOptionalSchema(this);
	    },
	    default: function _default(defaultValue) {
	      return createDefaultSchema(this, defaultValue);
	    }
	  };
	  return schema;
	}
	function createCustomSchema(parser) {
	  var schema = {
	    _type: undefined,
	    parse: function parse(data) {
	      return parser(data);
	    },
	    safeParse: function safeParse(data) {
	      try {
	        return {
	          success: true,
	          data: this.parse(data)
	        };
	      } catch (e) {
	        var msg = e instanceof Error ? e.message : String(e);
	        return {
	          success: false,
	          error: createError(msg)
	        };
	      }
	    },
	    optional: function optional() {
	      return createOptionalSchema(this);
	    },
	    default: function _default(defaultValue) {
	      return createDefaultSchema(this, defaultValue);
	    }
	  };
	  return schema;
	}
	function createOptionalSchema(innerSchema) {
	  var schema = {
	    _type: undefined,
	    _isOptional: true,
	    parse: function parse(data) {
	      if (data === undefined || data === null) {
	        return undefined;
	      }
	      return innerSchema.parse(data);
	    },
	    safeParse: function safeParse(data) {
	      try {
	        return {
	          success: true,
	          data: this.parse(data)
	        };
	      } catch (e) {
	        var msg = e instanceof Error ? e.message : String(e);
	        return {
	          success: false,
	          error: createError(msg)
	        };
	      }
	    },
	    optional: function optional() {
	      return this;
	    },
	    default: function _default(defaultValue) {
	      return createDefaultSchema(this, defaultValue);
	    }
	  };
	  return schema;
	}
	function createDefaultSchema(innerSchema, defaultValue) {
	  var schema = {
	    _type: defaultValue,
	    _defaultValue: defaultValue,
	    parse: function parse(data) {
	      if (data === undefined || data === null) {
	        return defaultValue;
	      }
	      return innerSchema.parse(data);
	    },
	    safeParse: function safeParse(data) {
	      try {
	        return {
	          success: true,
	          data: this.parse(data)
	        };
	      } catch (e) {
	        var msg = e instanceof Error ? e.message : String(e);
	        return {
	          success: false,
	          error: createError(msg)
	        };
	      }
	    },
	    optional: function optional() {
	      return createOptionalSchema(this);
	    },
	    default: function _default(newDefault) {
	      return createDefaultSchema(innerSchema, newDefault);
	    }
	  };
	  return schema;
	}
	function createObjectSchema(shape) {
	  var schema = {
	    _type: {},
	    shape: shape,
	    parse: function parse(data) {
	      if (!isObject(data)) {
	        throw new Error("Expected object, got " + _typeof(data));
	      }
	      var result = {};
	      var keys = Object.keys(shape);
	      for (var i = 0; i < keys.length; i++) {
	        var key = keys[i];
	        if (key === undefined) continue;
	        var fieldSchema = shape[key];
	        if (fieldSchema === undefined) continue;
	        var value = data[key];
	        result[key] = fieldSchema.parse(value);
	      }
	      return result;
	    },
	    safeParse: function safeParse(data) {
	      try {
	        return {
	          success: true,
	          data: this.parse(data)
	        };
	      } catch (e) {
	        var msg = e instanceof Error ? e.message : String(e);
	        return {
	          success: false,
	          error: createError(msg)
	        };
	      }
	    },
	    optional: function optional() {
	      return createOptionalSchema(this);
	    },
	    default: function _default(defaultValue) {
	      return createDefaultSchema(this, defaultValue);
	    }
	  };
	  return schema;
	}
	function createRecordSchema(_keySchema, valueSchema) {
	  var schema = {
	    _type: {},
	    parse: function parse(data) {
	      if (!isObject(data)) {
	        throw new Error("Expected object, got " + _typeof(data));
	      }
	      var result = {};
	      var keys = Object.keys(data);
	      for (var i = 0; i < keys.length; i++) {
	        var key = keys[i];
	        if (key === undefined) continue;
	        result[key] = valueSchema.parse(data[key]);
	      }
	      return result;
	    },
	    safeParse: function safeParse(data) {
	      try {
	        return {
	          success: true,
	          data: this.parse(data)
	        };
	      } catch (e) {
	        var msg = e instanceof Error ? e.message : String(e);
	        return {
	          success: false,
	          error: createError(msg)
	        };
	      }
	    },
	    optional: function optional() {
	      return createOptionalSchema(this);
	    },
	    default: function _default(defaultValue) {
	      return createDefaultSchema(this, defaultValue);
	    }
	  };
	  return schema;
	}
	function createUnionSchema(schemas) {
	  var schema = {
	    _type: undefined,
	    parse: function parse(data) {
	      var errors = [];
	      for (var i = 0; i < schemas.length; i++) {
	        var _result$error;
	        var s = schemas[i];
	        if (s === undefined) continue;
	        var result = s.safeParse(data);
	        if (result.success) {
	          return result.data;
	        }
	        errors.push(((_result$error = result.error) == null ? void 0 : _result$error.message) || "Unknown error");
	      }
	      throw new Error("No matching schema: " + errors.join(", "));
	    },
	    safeParse: function safeParse(data) {
	      try {
	        return {
	          success: true,
	          data: this.parse(data)
	        };
	      } catch (e) {
	        var msg = e instanceof Error ? e.message : String(e);
	        return {
	          success: false,
	          error: createError(msg)
	        };
	      }
	    },
	    optional: function optional() {
	      return createOptionalSchema(this);
	    },
	    default: function _default(defaultValue) {
	      return createDefaultSchema(this, defaultValue);
	    }
	  };
	  return schema;
	}
	function createTupleSchema(schemas) {
	  var schema = {
	    _type: [],
	    parse: function parse(data) {
	      if (!Array.isArray(data)) {
	        throw new Error("Expected array, got " + _typeof(data));
	      }
	      if (data.length !== schemas.length) {
	        throw new Error("Expected exactly " + schemas.length + " elements, got " + data.length);
	      }
	      var result = [];
	      for (var i = 0; i < schemas.length; i++) {
	        var s = schemas[i];
	        if (s === undefined) continue;
	        result.push(s.parse(data[i]));
	      }
	      return result;
	    },
	    safeParse: function safeParse(data) {
	      try {
	        return {
	          success: true,
	          data: this.parse(data)
	        };
	      } catch (e) {
	        var msg = e instanceof Error ? e.message : String(e);
	        return {
	          success: false,
	          error: createError(msg)
	        };
	      }
	    },
	    optional: function optional() {
	      return createOptionalSchema(this);
	    },
	    default: function _default(defaultValue) {
	      return createDefaultSchema(this, defaultValue);
	    }
	  };
	  return schema;
	}
	// =============================================================================
	// Enum Schema
	// =============================================================================
	function createEnumSchema(values) {
	  var schema = {
	    _type: values[0],
	    options: values,
	    parse: function parse(data) {
	      if (typeof data !== "string") {
	        throw new Error("Expected string, got " + _typeof(data));
	      }
	      for (var i = 0; i < values.length; i++) {
	        if (values[i] === data) {
	          return data;
	        }
	      }
	      throw new Error("Invalid enum value: " + data + ". Expected one of: " + values.join(", "));
	    },
	    safeParse: function safeParse(data) {
	      try {
	        return {
	          success: true,
	          data: this.parse(data)
	        };
	      } catch (e) {
	        var msg = e instanceof Error ? e.message : String(e);
	        return {
	          success: false,
	          error: createError(msg)
	        };
	      }
	    },
	    optional: function optional() {
	      return createOptionalSchema(this);
	    },
	    default: function _default(defaultValue) {
	      return createDefaultSchema(this, defaultValue);
	    }
	  };
	  return schema;
	}
	// =============================================================================
	// 公共 API - 模拟 zod 的 z 命名空间
	// =============================================================================
	var z = {
	  string: function string() {
	    return createStringSchema();
	  },
	  number: function number() {
	    return createNumberSchema();
	  },
	  boolean: function boolean() {
	    return createBooleanSchema();
	  },
	  array: function array(itemSchema) {
	    return createArraySchema(itemSchema);
	  },
	  custom: function custom(parser) {
	    return createCustomSchema(parser);
	  },
	  object: function object(shape) {
	    return createObjectSchema(shape);
	  },
	  record: function record(keySchema, valueSchema) {
	    return createRecordSchema(keySchema, valueSchema);
	  },
	  union: function union(schemas) {
	    return createUnionSchema(schemas);
	  },
	  tuple: function tuple(schemas) {
	    return createTupleSchema(schemas);
	  },
	  enum: function _enum(values) {
	    return createEnumSchema(values);
	  }
	};

	// logger.ts — Simple browser-compatible logging module.
	// Replaces pino to avoid a Node.js dependency at runtime.
	// Ported from translator_scratch.
	var LOG_LEVELS = {
	  trace: 10,
	  debug: 20,
	  info: 30,
	  warn: 40,
	  error: 50,
	  fatal: 60,
	  silent: 100
	};
	function createLogger() {
	  var options = arguments.length > 0 && arguments[0] !== undefined ? arguments[0] : {};
	  var currentLevel = options.level || 'info';
	  var prefix = options.name ? `[${options.name}]` : '[NST]';
	  function shouldLog(level) {
	    return LOG_LEVELS[level] >= LOG_LEVELS[currentLevel];
	  }
	  function formatMessage(msg, args) {
	    if (_typeof(msg) === 'object') {
	      try {
	        return JSON.stringify(msg) + (args.length > 0 ? ' ' + args.join(' ') : '');
	      } catch (_unused) {
	        return String(msg) + (args.length > 0 ? ' ' + args.join(' ') : '');
	      }
	    }
	    return msg + (args.length > 0 ? ' ' + args.join(' ') : '');
	  }
	  var logger = {
	    get level() {
	      return currentLevel;
	    },
	    set level(newLevel) {
	      currentLevel = newLevel;
	    },
	    trace: function trace(msg) {
	      if (shouldLog('trace')) {
	        for (var _len = arguments.length, args = new Array(_len > 1 ? _len - 1 : 0), _key = 1; _key < _len; _key++) {
	          args[_key - 1] = arguments[_key];
	        }
	        console.log(`${prefix} [TRACE]`, formatMessage(msg, args));
	      }
	    },
	    debug: function debug(msg) {
	      if (shouldLog('debug')) {
	        for (var _len2 = arguments.length, args = new Array(_len2 > 1 ? _len2 - 1 : 0), _key2 = 1; _key2 < _len2; _key2++) {
	          args[_key2 - 1] = arguments[_key2];
	        }
	        console.log(`${prefix} [DEBUG]`, formatMessage(msg, args));
	      }
	    },
	    info: function info(msg) {
	      if (shouldLog('info')) {
	        for (var _len3 = arguments.length, args = new Array(_len3 > 1 ? _len3 - 1 : 0), _key3 = 1; _key3 < _len3; _key3++) {
	          args[_key3 - 1] = arguments[_key3];
	        }
	        console.log(`${prefix} [INFO]`, formatMessage(msg, args));
	      }
	    },
	    warn: function warn(msg) {
	      if (shouldLog('warn')) {
	        for (var _len4 = arguments.length, args = new Array(_len4 > 1 ? _len4 - 1 : 0), _key4 = 1; _key4 < _len4; _key4++) {
	          args[_key4 - 1] = arguments[_key4];
	        }
	        console.warn(`${prefix} [WARN]`, formatMessage(msg, args));
	      }
	    },
	    error: function error(msg) {
	      if (shouldLog('error')) {
	        for (var _len5 = arguments.length, args = new Array(_len5 > 1 ? _len5 - 1 : 0), _key5 = 1; _key5 < _len5; _key5++) {
	          args[_key5 - 1] = arguments[_key5];
	        }
	        console.error(`${prefix} [ERROR]`, formatMessage(msg, args));
	      }
	    },
	    fatal: function fatal(msg) {
	      if (shouldLog('fatal')) {
	        for (var _len6 = arguments.length, args = new Array(_len6 > 1 ? _len6 - 1 : 0), _key6 = 1; _key6 < _len6; _key6++) {
	          args[_key6 - 1] = arguments[_key6];
	        }
	        console.error(`${prefix} [FATAL]`, formatMessage(msg, args));
	      }
	    }
	  };
	  return logger;
	}
	// Default export: a factory function mimicking pino's API.
	function pino() {
	  var options = arguments.length > 0 && arguments[0] !== undefined ? arguments[0] : {};
	  return createLogger(options);
	}

	/// <reference path="../../typings/rmmv.d.ts" />
	// version-detection.ts — RPG Maker MV/MZ version detection.
	// Ported from translator_scratch.
	// ============================================
	// Version detection (RPG Maker MV/MZ)
	// ============================================
	function detectVersion() {
	  var utils = typeof Utils !== "undefined" && Utils !== null ? Utils : null;
	  var isMZ = (utils == null ? void 0 : utils.RPGMAKER_NAME) === "MZ";
	  var isMV = !isMZ && typeof (utils == null ? void 0 : utils.RPGMAKER_NAME) === "string";
	  return {
	    isMZ: isMZ,
	    isMV: isMV
	  };
	}
	function isMZ() {
	  return detectVersion().isMZ;
	}
	function isMV() {
	  return detectVersion().isMV;
	}

	var CUSTOM_FONT_NAME = "NotoSans";
	var customFontConfig = null;
	var fontLoadAttempted = false;
	var hooksInstalled = false;
	// =============================================================================
	// Logger (simplified)
	// =============================================================================
	var log$3 = pino({
	  level: "info"
	});
	// ============================================
	// Font config validation (simplified)
	// ============================================
	function validateFontConfig(config) {
	  if (!config || _typeof(config) !== "object") {
	    return null;
	  }
	  var cfg = config;
	  if (typeof cfg["fontName"] !== "string" || !cfg["fontName"]) {
	    return null;
	  }
	  if (typeof cfg["fontUrl"] !== "string" || !cfg["fontUrl"]) {
	    return null;
	  }
	  return {
	    fontName: cfg["fontName"],
	    fontUrl: cfg["fontUrl"],
	    offsetSize: typeof cfg["offsetSize"] === "number" ? cfg["offsetSize"] : undefined,
	    maxSizeOffset: typeof cfg["maxSizeOffset"] === "number" ? cfg["maxSizeOffset"] : undefined,
	    dir: typeof cfg["dir"] === "string" ? cfg["dir"] : undefined
	  };
	}
	function getConfiguredFontName() {
	  var _customFontConfig;
	  return ((_customFontConfig = customFontConfig) == null ? void 0 : _customFontConfig.fontName) || CUSTOM_FONT_NAME;
	}
	function hasFontFaceApi() {
	  return typeof FontFace !== "undefined" && typeof document !== "undefined" && document.fonts !== undefined;
	}
	function escapeCssString(value) {
	  return value.replace(/\\/g, "\\\\").replace(/"/g, '\\"');
	}
	function addFontFaceStyle(config, fontUrl) {
	  if (typeof document === "undefined" || !document.head) {
	    showError("字体加载失败", "当前环境不支持字体加载 API");
	  }
	  var style = document.createElement("style");
	  style.type = "text/css";
	  style.appendChild(document.createTextNode('@font-face { font-family: "' + escapeCssString(config.fontName) + '"; src: url("' + escapeCssString(fontUrl) + '"); }'));
	  document.head.appendChild(style);
	  log$3.info("[NST] Registered font via CSS @font-face:", config.fontName, fontUrl);
	  refreshAllWindows();
	}
	// ============================================
	// Font file existence check
	// ============================================
	function checkFontFileExists(fontUrl) {
	  return new Promise(function (resolve) {
	    var xhr = new XMLHttpRequest();
	    xhr.open("HEAD", fontUrl, true);
	    xhr.onreadystatechange = function () {
	      if (xhr.readyState === 4) {
	        // 200-299 indicates success; 0 indicates local file (file://).
	        resolve(xhr.status >= 200 && xhr.status < 300 || xhr.status === 0);
	      }
	    };
	    xhr.onerror = function () {
	      resolve(false);
	    };
	    try {
	      xhr.send();
	    } catch (_e) {
	      // Some environments may throw.
	      resolve(false);
	    }
	  });
	}
	// ============================================
	// Font loading functions
	// ============================================
	function loadFont(config) {
	  try {
	    var font = new FontFace(config.fontName, "url(" + config.fontUrl + ")");
	    return ok(font);
	  } catch (error) {
	    var errorMsg = error instanceof Error ? error.message : String(error);
	    // Explicit error: font creation failed (showError throws, does not return).
	    showError("字体创建失败", '无法创建字体 "' + config.fontName + '"\nURL: ' + config.fontUrl + "\n错误: " + errorMsg);
	  }
	}
	/**
	 * Extract the file name from a full path.
	 * e.g. "fonts/NotoSans-Regular.woff2" -> "NotoSans-Regular.woff2"
	 */
	function extractFileName(fontUrl) {
	  var parts = fontUrl.split("/");
	  return parts[parts.length - 1] || fontUrl;
	}
	/**
	 * MZ-specific font loader — uses both FontManager.load and FontFace API
	 * to maximize compatibility.
	 */
	function loadCustomFontMZ() {
	  if (fontLoadAttempted) return;
	  fontLoadAttempted = true;
	  var fontConfig = customFontConfig || {
	    fontName: CUSTOM_FONT_NAME,
	    fontUrl: "fonts/NotoSans-Regular.woff2",
	    offsetSize: 0,
	    maxSizeOffset: 0,
	    dir: "fonts"
	  };
	  var validatedConfig = validateFontConfig(fontConfig);
	  if (!validatedConfig) {
	    showWarning("字体配置验证失败", "常规或默认字体配置无效，跳过自定义字体加载。");
	    return;
	  }
	  var config = validatedConfig;
	  // MZ uses FontManager.load(name, fileName).
	  // fileName is relative to the fonts folder.
	  var fileName = extractFileName(config.fontUrl);
	  var FM = window.FontManager;
	  try {
	    // Method 1: FontManager.load (RPG Maker MZ built-in).
	    if (FM && FM.load) {
	      FM.load(config.fontName, fileName);
	      log$3.info("[NST] MZ FontManager.load invoked:", config.fontName, fileName);
	    }
	    // Method 2: FontFace API or CSS @font-face fallback.
	    var fontUrl = config.fontUrl;
	    if (hasFontFaceApi()) {
	      var font = new FontFace(config.fontName, 'url("' + fontUrl + '")');
	      document.fonts.add(font);
	      font.load().catch(function (error) {
	        var errorMsg = error instanceof Error ? error.message : String(error);
	        setTimeout(function () {
	          showWarning("字体加载失败", "无法加载字体\n" + "字体名称: " + config.fontName + "\n" + "文件名: " + fileName + "\n" + "错误信息: " + errorMsg);
	        }, 0);
	      });
	      log$3.info("[NST] MZ FontFace API added:", config.fontName, fontUrl);
	    } else {
	      addFontFaceStyle(config, fontUrl);
	    }
	  } catch (error) {
	    var errorMsg = error instanceof Error ? error.message : String(error);
	    showWarning("字体加载失败", "无法加载字体\n" + "字体名称: " + config.fontName + "\n" + "文件名: " + fileName + "\n" + "错误信息: " + errorMsg);
	  }
	}
	/**
	 * MV and fallback font loader — uses the FontFace API.
	 */
	function loadCustomFontFallback(config) {
	  if (!config) {
	    if (fontLoadAttempted) return;
	    fontLoadAttempted = true;
	    var fontConfig = customFontConfig || {
	      fontName: CUSTOM_FONT_NAME,
	      fontUrl: "fonts/NotoSans-Regular.woff2",
	      offsetSize: 0,
	      maxSizeOffset: 0,
	      dir: "fonts"
	    };
	    var validatedConfig = validateFontConfig(fontConfig);
	    if (!validatedConfig) {
	      showWarning("字体配置验证失败", "字体配置格式无效：缺少 fontName 或 fontUrl，跳过自定义字体加载。");
	      return;
	    }
	    config = validatedConfig;
	  }
	  // Check the font file exists first.
	  checkFontFileExists(config.fontUrl).then(function (exists) {
	    if (!exists) {
	      setTimeout(function () {
	        showWarning("字体文件不存在", "找不到字体文件，请检查路径是否正确\n" + "字体名称: " + config.fontName + "\n" + "字体路径: " + config.fontUrl);
	      }, 0);
	      return;
	    }
	    if (!hasFontFaceApi()) {
	      addFontFaceStyle(config, config.fontUrl);
	      return;
	    }
	    var fontResult = loadFont(config);
	    fontResult.map(function (font) {
	      font.load().then(function () {
	        document.fonts.add(font);
	        log$3.info("Custom font loaded:", font.family);
	        refreshAllWindows();
	      }).catch(function (error) {
	        var errorMsg = error instanceof Error ? error.message : String(error);
	        setTimeout(function () {
	          showWarning("字体文件加载失败", "无法加载字体文件\n" + "字体名称: " + config.fontName + "\n" + "字体路径: " + config.fontUrl + "\n" + "错误信息: " + errorMsg);
	        }, 0);
	      });
	    }).mapErr(function (error) {
	      var errorMsg = error instanceof Error ? error.message : String(error);
	      showWarning("字体创建失败", "无法创建字体对象\n错误信息: " + errorMsg);
	    });
	  });
	}
	/**
	 * Universal font loading entry — selects the method based on engine type.
	 */
	function loadCustomFont() {
	  if (isMZ()) {
	    loadCustomFontMZ();
	  } else {
	    loadCustomFontFallback();
	  }
	}
	function refreshAllWindows() {
	  var scene = SceneManager._scene;
	  if (!scene) return;
	  if (scene._windowLayer) {
	    scene._windowLayer.children.forEach(function (child) {
	      if (child && typeof child.refresh === "function") {
	        child.refresh();
	      }
	    });
	  }
	  if (scene._messageWindow && typeof scene._messageWindow.refresh === "function") {
	    scene._messageWindow.refresh();
	  }
	}
	// ============================================
	// Hook installation
	// ============================================
	function installHooks() {
	  if (hooksInstalled) return;
	  if (typeof Scene_Boot === "undefined" || Scene_Boot === null) {
	    console.warn("[NST] Scene_Boot undefined; cannot install font hooks");
	    return;
	  }
	  hooksInstalled = true;
	  // ============================================
	  // MZ-specific API hooks
	  // ============================================
	  if (isMZ()) {
	    log$3.info("[NST] Detected MZ; installing MZ font hooks");
	    // MZ: Game_System.prototype.mainFontFace
	    if (typeof Game_System !== "undefined" && Game_System.prototype.mainFontFace) {
	      var _Game_System_mainFontFace = Game_System.prototype.mainFontFace;
	      Game_System.prototype.mainFontFace = function () {
	        var original = _Game_System_mainFontFace.call(this);
	        return getConfiguredFontName() + ", " + original;
	      };
	    }
	    // MZ: Game_System.prototype.mainFontSize
	    if (typeof Game_System !== "undefined" && Game_System.prototype.mainFontSize) {
	      var _Game_System_mainFontSize = Game_System.prototype.mainFontSize;
	      Game_System.prototype.mainFontSize = function () {
	        var originalSize = _Game_System_mainFontSize.call(this);
	        var config = customFontConfig;
	        if (config && config.offsetSize !== undefined) {
	          var newSize = originalSize + config.offsetSize;
	          if (config.maxSizeOffset !== undefined) {
	            var maxSize = originalSize + config.maxSizeOffset;
	            return Math.max(8, Math.min(newSize, maxSize));
	          }
	          return Math.max(8, newSize);
	        }
	        return originalSize;
	      };
	    }
	    // MZ: font load hook (Scene_Boot.prototype.loadGameFonts)
	    var _Scene_Boot_loadGameFonts = Scene_Boot.prototype.loadGameFonts;
	    Scene_Boot.prototype.loadGameFonts = function () {
	      _Scene_Boot_loadGameFonts.call(this);
	      loadCustomFont();
	      log$3.info("[NST] MZ font load hook executed");
	    };
	    // Note: we no longer block isReady; font loading happens in the background.
	    // If font loading fails, showError reports it explicitly.
	    log$3.info("[NST] MZ font hooks installed");
	  }
	  // ============================================
	  // MV-specific API hooks
	  // ============================================
	  if (isMV()) {
	    log$3.info("[NST] Detected MV; installing MV font hooks");
	    // MV: Window_Base.prototype.standardFontFace
	    if (typeof Window_Base !== "undefined" && Window_Base.prototype.standardFontFace) {
	      var _Window_Base_standardFontFace = Window_Base.prototype.standardFontFace;
	      Window_Base.prototype.standardFontFace = function () {
	        var original = _Window_Base_standardFontFace.call(this);
	        return getConfiguredFontName() + ", " + original;
	      };
	    }
	    // MV: Window_Base.prototype.standardFontSize
	    if (typeof Window_Base !== "undefined" && Window_Base.prototype.standardFontSize) {
	      var _Window_Base_standardFontSize = Window_Base.prototype.standardFontSize;
	      Window_Base.prototype.standardFontSize = function () {
	        var originalSize = _Window_Base_standardFontSize.call(this);
	        var config = customFontConfig;
	        if (config && config.offsetSize !== undefined) {
	          var newSize = originalSize + config.offsetSize;
	          if (config.maxSizeOffset !== undefined) {
	            var maxSize = originalSize + config.maxSizeOffset;
	            return Math.max(8, Math.min(newSize, maxSize));
	          }
	          return Math.max(8, newSize);
	        }
	        return originalSize;
	      };
	    }
	    // MV: font load hook (Scene_Boot.prototype.create)
	    var _Scene_Boot_create = Scene_Boot.prototype.create;
	    Scene_Boot.prototype.create = function () {
	      _Scene_Boot_create.call(this);
	      loadCustomFont();
	      log$3.info("[NST] MV font load hook executed");
	    };
	    // Note: we no longer block isReady; font loading happens in the background.
	    log$3.info("[NST] MV font hooks installed");
	  }
	}
	// ============================================
	// Public API
	// ============================================
	/**
	 * Initialize font configuration.
	 * Font hooks are installed after the config is initialized to avoid the
	 * module-load-order issue that would cause a permanent skip.
	 */
	function initFonts(fontConfig) {
	  if (!fontConfig) {
	    // When no fontConfig is provided, attempt to load from the JSON config.
	    // json-loader re-exports the resolved config via getLoadedFontConfig().
	    try {
	      var loaded = getLoadedFontConfig();
	      if (loaded) {
	        fontConfig = {
	          fontName: loaded.fontName,
	          fontUrl: loaded.fontUrl,
	          offsetSize: loaded.offsetSize,
	          maxSizeOffset: loaded.maxSizeOffset
	        };
	      }
	    } catch (error) {
	      var errorMsg = error instanceof Error ? error.message : String(error);
	      showError("字体配置加载失败", "无法从配置文件加载字体设置\n错误信息: " + errorMsg);
	    }
	  }
	  if (fontConfig) {
	    customFontConfig = fontConfig;
	  }
	  log$3.info("[NST] Font config initialized");
	  installHooks();
	}
	// ============================================
	// Bridge to json-loader's resolved font config
	// ============================================
	// To avoid a circular import, json-loader publishes the resolved font config
	// onto this module via setLoadedFontConfig at init time.
	var _loadedFontConfig;
	function setLoadedFontConfig(cfg) {
	  _loadedFontConfig = cfg;
	}
	function getLoadedFontConfig() {
	  return _loadedFontConfig;
	}

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
	var DEFAULT_CUSTOM_HOOKS = [
	// --- 1. Main text pipeline: Window_Base.convertEscapeCharacters ---
	{
	  class: "Window_Base",
	  method: "convertEscapeCharacters",
	  type: "tr0",
	  paramIndex: 0,
	  enabled: true,
	  title: "Main text pipeline",
	  desc: "Translates text passing through the escape-character converter."
	},
	// --- 2. Dialogue messages: Game_Message.add ---
	{
	  class: "Game_Message",
	  method: "add",
	  type: "tr0",
	  paramIndex: 0,
	  enabled: true,
	  title: "Dialogue message text",
	  desc: "Translates dialogue message text as it is added."
	},
	// --- 3. Speaker name: Game_Message.setSpeakerName ---
	{
	  class: "Game_Message",
	  method: "setSpeakerName",
	  type: "tr0",
	  paramIndex: 0,
	  enabled: true,
	  title: "Speaker name",
	  desc: "Translates the speaker name displayed above dialogue."
	},
	// --- 4. Event command 101 (Show Text) ---
	{
	  class: "Game_Interpreter",
	  method: "command101",
	  type: "trNested",
	  paramIndex: 0,
	  nestedIndex: [4],
	  minParamLength: 5,
	  enabled: true,
	  title: "Show Text command",
	  desc: "Translates event command 101 (Show Text) text parameter."
	},
	// --- 5. Event command 102 (Show Choices) ---
	{
	  class: "Game_Interpreter",
	  method: "command102",
	  type: "trNestedArray",
	  paramIndex: 0,
	  nestedIndex: [0],
	  enabled: true,
	  title: "Show Choices command",
	  desc: "Translates event command 102 (Show Choices) choice text."
	},
	// --- 6. Data load translation ---
	{
	  class: "DataManager",
	  method: "onLoad",
	  type: "trData",
	  paramIndex: 0,
	  enabled: true,
	  title: "Data load translation",
	  desc: "Translates text content in game data objects on load."
	},
	// --- 7. Text rendering: Window_Base.drawText ---
	{
	  class: "Window_Base",
	  method: "drawText",
	  type: "tr0",
	  paramIndex: 0,
	  enabled: true,
	  title: "Window text draw",
	  desc: "Translates text rendered by Window_Base.drawText."
	},
	// --- 8. Extended text rendering: Window_Base.drawTextEx ---
	{
	  class: "Window_Base",
	  method: "drawTextEx",
	  type: "tr0",
	  paramIndex: 0,
	  enabled: true,
	  title: "Extended text draw",
	  desc: "Translates text rendered by Window_Base.drawTextEx."
	},
	// --- 9. Text width calculation: Window_Base.textWidth ---
	{
	  class: "Window_Base",
	  method: "textWidth",
	  type: "tr0",
	  paramIndex: 0,
	  enabled: true,
	  title: "Text width calc",
	  desc: "Translates text during width measurement for correct layout."
	},
	// --- 10. Bitmap rendering: Bitmap.drawText ---
	{
	  class: "Bitmap",
	  method: "drawText",
	  type: "tr0",
	  paramIndex: 0,
	  enabled: true,
	  title: "Bitmap text draw",
	  desc: "Translates text rendered by Bitmap.drawText."
	},
	// --- 11. Bitmap measurement: Bitmap.measureTextWidth ---
	{
	  class: "Bitmap",
	  method: "measureTextWidth",
	  type: "tr0",
	  paramIndex: 0,
	  enabled: true,
	  title: "Bitmap text measure",
	  desc: "Translates text during Bitmap.measureTextWidth."
	},
	// --- 12. Text state creation ---
	{
	  class: "Window_Base",
	  method: "createTextState",
	  type: "tr0",
	  paramIndex: 0,
	  enabled: true,
	  title: "Text state creation",
	  desc: "Translates text in Window_Base.createTextState."
	}];
	// =============================================================================
	// Default Config Object
	// =============================================================================
	/**
	 * The full default configuration. This is merged with (or used in place of)
	 * the user's nst_translations/config.json when fields are missing.
	 */
	var DEFAULT_CONFIG = {
	  __customHooks__: DEFAULT_CUSTOM_HOOKS,
	  __textFields__: ["name", "description", "displayName", "nickname", "profile", "message1", "message2", "message3", "message4", "gameTitle", "terms", "messages"],
	  __textCommands__: {
	    "101": [4],
	    "102": [0],
	    "320": [1],
	    "324": [1],
	    "325": [1],
	    "402": [1],
	    "405": [0]
	  },
	  __controlCharPatterns__: ["\\\\[VNP]\\[\\d+\\]", "\\\\I\\[\\d+\\]", "\\\\C\\[\\d+\\]", "\\\\G", "\\\\[{}]", "\\\\\\$", "\\\\[.\\|]", "\\\\!", "\\\\[><]", "\\\\\\^", "\\\\\\\\", "\\\\FS\\[\\d+\\]", "\\\\P[XY]\\[-?\\d+\\]", "\\\\[OT]C\\[\\d+\\]", "\\\\(?:MSGCORE|MSGSND)\\[[^\\]]*\\]"],
	  __ignorePatterns__: ["^.$", "^\\s*$", "^[\\d\\s.,\\-+%$/\\\\:;()\\[\\]{}=*#@!?<>~`'\"^&|_]+$", "^\\d+$"],
	  __fontConfig__: {
	    offsetSize: 0,
	    maxSizeOffset: 0,
	    fontName: "NotoSans",
	    fontUrl: "fonts/NotoSans-Regular.woff2"
	  },
	  __sourceLocale__: "ja"
	};

	// =============================================================================
	// Schemas
	// =============================================================================
	var CUSTOM_HOOK_TYPES = ["tr0", "trRet", "trArray", "trNestedArray", "trObject", "trObjectAfter", "trRetObject", "trThis", "trThisAfter", "trCommandList", "trData", "trNested"];
	z.enum(CUSTOM_HOOK_TYPES);
	var HOOK_TYPES_REQUIRING_FIELDS = {
	  trObject: true,
	  trObjectAfter: true,
	  trRetObject: true,
	  trThis: true,
	  trThisAfter: true
	};
	function isRecord(value) {
	  return _typeof(value) === "object" && value !== null && !Array.isArray(value);
	}
	function isNonNegativeInteger(value) {
	  return typeof value === "number" && isFinite(value) && Math.floor(value) === value && value >= 0;
	}
	function requireNonEmptyString(value, path) {
	  if (typeof value !== "string" || value.length === 0) {
	    throw new Error(path + " must be a non-empty string");
	  }
	  return value;
	}
	function readOptionalString(value, path) {
	  if (value === undefined) return undefined;
	  if (typeof value !== "string") {
	    throw new Error(path + " must be a string");
	  }
	  return value;
	}
	function readOptionalBoolean(value, path) {
	  if (value === undefined) return undefined;
	  if (typeof value !== "boolean") {
	    throw new Error(path + " must be a boolean");
	  }
	  return value;
	}
	function readOptionalIndex(value, path) {
	  if (value === undefined) return undefined;
	  if (!isNonNegativeInteger(value)) {
	    throw new Error(path + " must be a non-negative integer");
	  }
	  return value;
	}
	function readOptionalStringArray(value, path) {
	  if (value === undefined) return undefined;
	  if (!Array.isArray(value)) {
	    throw new Error(path + " must be an array");
	  }
	  return value.map(function (item, index) {
	    return requireNonEmptyString(item, path + "[" + index + "]");
	  });
	}
	function readOptionalIndexArray(value, path) {
	  if (value === undefined) return undefined;
	  if (!Array.isArray(value)) {
	    throw new Error(path + " must be an array");
	  }
	  return value.map(function (item, index) {
	    if (!isNonNegativeInteger(item)) {
	      throw new Error(path + "[" + index + "] must be a non-negative integer");
	    }
	    return item;
	  });
	}
	function isCustomHookType(value) {
	  return CUSTOM_HOOK_TYPES.indexOf(value) !== -1;
	}
	function parseCustomHookConfig(data) {
	  var _result$type;
	  if (!isRecord(data)) {
	    throw new Error("custom hook must be an object");
	  }
	  var typeValue = readOptionalString(data["type"], "custom hook type");
	  if (typeValue !== undefined && !isCustomHookType(typeValue)) {
	    throw new Error("custom hook type is invalid: " + typeValue);
	  }
	  var result = {
	    class: requireNonEmptyString(data["class"], "custom hook class"),
	    method: requireNonEmptyString(data["method"], "custom hook method")
	  };
	  if (typeValue !== undefined) result.type = typeValue;
	  var fields = readOptionalStringArray(data["fields"], "custom hook fields");
	  if (fields !== undefined) result.fields = fields;
	  var paramIndex = readOptionalIndex(data["paramIndex"], "custom hook paramIndex");
	  if (paramIndex !== undefined) result.paramIndex = paramIndex;
	  var nestedIndex = readOptionalIndexArray(data["nestedIndex"], "custom hook nestedIndex");
	  if (nestedIndex !== undefined) result.nestedIndex = nestedIndex;
	  var minParamLength = readOptionalIndex(data["minParamLength"], "custom hook minParamLength");
	  if (minParamLength !== undefined) result.minParamLength = minParamLength;
	  var enabled = readOptionalBoolean(data["enabled"], "custom hook enabled");
	  if (enabled !== undefined) result.enabled = enabled;
	  var isStatic = readOptionalBoolean(data["static"], "custom hook static");
	  if (isStatic !== undefined) result.static = isStatic;
	  var title = readOptionalString(data["title"], "custom hook title");
	  if (title !== undefined) result.title = title;
	  var desc = readOptionalString(data["desc"], "custom hook desc");
	  if (desc !== undefined) result.desc = desc;
	  var effectiveType = (_result$type = result.type) != null ? _result$type : "tr0";
	  if (HOOK_TYPES_REQUIRING_FIELDS[effectiveType] && (!result.fields || result.fields.length === 0)) {
	    throw new Error("custom hook type " + effectiveType + " requires fields");
	  }
	  return result;
	}
	function parseTextCommands(data) {
	  if (!isRecord(data)) {
	    throw new Error("__textCommands__ must be an object");
	  }
	  var result = {};
	  for (var _i = 0, _Object$keys = Object.keys(data); _i < _Object$keys.length; _i++) {
	    var key = _Object$keys[_i];
	    if (!/^\d+$/.test(key)) {
	      throw new Error("__textCommands__ key must be a numeric command code: " + key);
	    }
	    var indices = readOptionalIndexArray(data[key], "__textCommands__." + key);
	    result[key] = indices != null ? indices : [];
	  }
	  return result;
	}
	function parseTranslationsData(data) {
	  if (!isRecord(data)) {
	    throw new Error("translations must be an object");
	  }
	  var result = {};
	  for (var _i2 = 0, _Object$keys2 = Object.keys(data); _i2 < _Object$keys2.length; _i2++) {
	    var key = _Object$keys2[_i2];
	    var value = data[key];
	    // Keys starting with "_" are comments / disabled entries — skip them.
	    // Meta keys (__x__) are NOT skipped here; they are handled by the caller.
	    if (key.charAt(0) === "_" && !(key.startsWith("__") && key.endsWith("__"))) {
	      continue;
	    }
	    if (typeof value === "string") {
	      result[key] = value;
	      continue;
	    }
	    if (isRecord(value) && typeof value["translation"] === "string") {
	      result[key] = {
	        translation: value["translation"]
	      };
	      continue;
	    }
	    throw new Error("translation entry " + key + " must be a string or { translation: string }");
	  }
	  return result;
	}
	var CustomHookConfigSchema = z.custom(parseCustomHookConfig);
	var TextCommandsSchema = z.custom(parseTextCommands);
	var FontConfigSchema = z.object({
	  offsetSize: z.number().default(0),
	  maxSizeOffset: z.number().default(0),
	  fontName: z.string().min(1).default("NotoSans"),
	  fontUrl: z.string().min(1).default("NotoSans-Regular.woff2")
	});
	var ConfigSchema = z.object({
	  __textFields__: z.array(z.string().min(1)).default(["name", "description", "displayName", "nickname", "profile", "message1", "message2", "message3", "message4", "gameTitle"]),
	  __textCommands__: TextCommandsSchema.default({
	    "101": [4],
	    "102": [0],
	    "401": [0],
	    "405": [0]
	  }),
	  __controlCharPatterns__: z.array(z.string().min(1)).default(["\\\\[A-Za-z]+(?:\\\\[\\\\d+\\\\])?"]),
	  __ignorePatterns__: z.array(z.string().min(1)).default([String.raw`^(?:\s*|\d+|[\s\d.,]+)$`]),
	  __fontConfig__: FontConfigSchema.optional(),
	  __customHooks__: z.array(CustomHookConfigSchema).optional(),
	  __sourceLocale__: z.string().min(1).default(String.raw`ja`)
	});
	var TranslationsSchema = z.custom(parseTranslationsData);
	// =============================================================================
	// Path Utilities
	// =============================================================================
	/**
	 * Detect the game root directory.
	 * RPG Maker MV/MZ supports two directory structures:
	 *
	 * Structure 1 (standard):
	 *   Game root/
	 *   ├── data/          <- data files
	 *   ├── js/
	 *   │   └── plugins/   <- plugins directory
	 *   └── index.html
	 *
	 * Structure 2 (www subdirectory):
	 *   Game root/
	 *   ├── www/
	 *   │   ├── data/      <- data files
	 *   │   ├── js/
	 *   │   │   └── plugins/
	 *   │   └── index.html
	 *   └── Game.exe
	 *
	 * We look for nst_translations/ relative to the game root.
	 */
	function getBasePath() {
	  // Try Node.js path resolution if running inside NW.js or Electron
	  var glob = typeof globalThis !== "undefined" ? globalThis : typeof window !== "undefined" ? window : {};
	  if (typeof glob.require !== "undefined" && typeof glob.process !== "undefined" && glob.process.mainModule) {
	    try {
	      var path = glob.require("path");
	      var base = path.dirname(glob.process.mainModule.filename);
	      if (base) return base;
	    } catch (_) {}
	  }
	  if (typeof window !== "undefined") {
	    var href = window.location.href;
	    var basePath = getDirectoryPath(href);
	    if (basePath && basePath !== ".") return basePath;
	    var currentScript = document.currentScript;
	    if (currentScript && currentScript.src) {
	      // Go up 2 levels from js/plugins/<this>.js -> game root.
	      return getDirectoryPath(getDirectoryPath(getDirectoryPath(currentScript.src)));
	    }
	    var scripts = document.getElementsByTagName("script");
	    if (scripts.length > 0) {
	      var lastScript = scripts[scripts.length - 1];
	      if (lastScript && lastScript.src) {
	        return getDirectoryPath(getDirectoryPath(getDirectoryPath(lastScript.src)));
	      }
	    }
	  }
	  return ".";
	}
	function getDirectoryPath(filePath) {
	  var _split$, _filePath$split$;
	  var cleanPath = (_split$ = ((_filePath$split$ = filePath.split("?")[0]) != null ? _filePath$split$ : filePath).split("#")[0]) != null ? _split$ : filePath;
	  var lastSlash = cleanPath.lastIndexOf("/");
	  if (lastSlash === -1) lastSlash = cleanPath.lastIndexOf("\\");
	  return lastSlash > 0 ? cleanPath.substring(0, lastSlash) : ".";
	}
	// =============================================================================
	// Cross-Platform File Reading
	// =============================================================================
	function toLocalPath(path) {
	  if (path.indexOf("file://") === 0) {
	    var decoded = decodeURIComponent(path.substring(7));
	    if (decoded.match(/^\/[a-zA-Z]:/)) {
	      return decoded.substring(1);
	    }
	    return decoded;
	  }
	  return path;
	}
	/**
	 * Read a file as text. Works on NW.js, JoiPlay, Electron, and browsers.
	 * @param filePath - path relative to game root
	 * @returns file content or null
	 */
	function readFileSync(filePath) {
	  var full = getBasePath() + "/" + filePath;
	  // Node.js fs (NW.js desktop).
	  if (typeof require !== "undefined") {
	    try {
	      var fs = require("fs");
	      var localPath = toLocalPath(full);
	      if (fs.existsSync(localPath)) {
	        return fs.readFileSync(localPath, "utf8");
	      }
	      return null;
	    } catch (_e) {/* fall through to XHR */}
	  }
	  // Synchronous XHR — works on JoiPlay, browsers, and everything else.
	  try {
	    var x = new XMLHttpRequest();
	    x.open("GET", full, false);
	    x.overrideMimeType("application/json; charset=utf-8");
	    x.send(null);
	    return x.status === 200 || x.status === 0 ? x.responseText : null;
	  } catch (_e) {
	    return null;
	  }
	}
	// =============================================================================
	// JSON Loader
	// =============================================================================
	var log$2 = pino({
	  level: "info"
	});
	function loadJsonSync(filePath, schema) {
	  try {
	    var fileContent = readFileSync(filePath);
	    if (!fileContent) return err("File not found or unreadable: " + filePath);
	    var jsonData;
	    try {
	      jsonData = JSON.parse(fileContent);
	    } catch (parseError) {
	      var parseMsg = parseError instanceof Error ? parseError.message : String(parseError);
	      return err("JSON parse failed: " + filePath + "\nError: " + parseMsg);
	    }
	    var parsed = schema.safeParse(jsonData);
	    if (!parsed.success) {
	      var errorMsg = parsed.error ? parsed.error.message : "Unknown validation error";
	      return err("Validation failed: " + filePath + "\nError: " + errorMsg);
	    }
	    return ok(parsed.data);
	  } catch (e) {
	    var message = e instanceof Error ? e.message : String(e);
	    return err("Unknown error loading: " + filePath + "\nError: " + message);
	  }
	}
	// =============================================================================
	// Translation Parsing
	// =============================================================================
	/**
	 * Parse a translation data object into a source→translated Map.
	 * Supports both simple { source: translation } and extended { source: { translation } } formats.
	 * Entries with keys starting with "_" (but not "__x__") are treated as comments and skipped.
	 */
	function parseTranslations(data) {
	  var map = new Map();
	  for (var _i3 = 0, _Object$keys3 = Object.keys(data); _i3 < _Object$keys3.length; _i3++) {
	    var key = _Object$keys3[_i3];
	    var value = data[key];
	    // Skip comment/disabled entries (keys starting with "_" but not meta-keys).
	    if (key.charAt(0) === "_" && !(key.startsWith("__") && key.endsWith("__"))) {
	      continue;
	    }
	    var text = null;
	    if (typeof value === "string") {
	      text = value;
	    } else if (!Array.isArray(value) && value && _typeof(value) === "object") {
	      text = value.translation;
	    }
	    // Skip entries where source equals translation (no-op).
	    if (text && text !== key) {
	      map.set(key, text);
	    }
	  }
	  // Sort entries by key length (longest first) for optimal partial matching.
	  var entries = Array.from(map.entries()).sort(function (a, b) {
	    return b[0].length - a[0].length;
	  });
	  return new Map(entries);
	}
	// =============================================================================
	// Lazy-Load State
	// =============================================================================
	var _loadedFiles = {};
	var _baseTranslations = new Map();
	/** Database files that are always loaded at init. */
	var DB_FILES = ["Actors", "Classes", "Skills", "Items", "Weapons", "Armors", "Enemies", "Troops", "States", "CommonEvents", "System", "_inline"];
	/**
	 * Load a single translation JSON file and merge its entries into the
	 * shared translation map. Called both at init (for DB files) and lazily
	 * (for map files on map change).
	 */
	function loadTranslationFile(fileName) {
	  if (_loadedFiles[fileName]) return 0;
	  var txt = readFileSync("nst_translations/" + fileName + ".json");
	  if (txt === null) return 0;
	  var parsed = loadJsonSync("nst_translations/" + fileName + ".json", TranslationsSchema);
	  if (parsed.isErr()) {
	    log$2.warn("Failed to load " + fileName + ".json: " + parsed.error);
	    return 0;
	  }
	  var translations = parseTranslations(parsed.value);
	  translations.forEach(function (value, key) {
	    _baseTranslations.set(key, value);
	  });
	  _loadedFiles[fileName] = true;
	  log$2.info("Loaded " + fileName + ".json (" + translations.size + " entries)");
	  return translations.size;
	}
	// =============================================================================
	// Public API
	// =============================================================================
	/**
	 * Load all database translation files and the config.
	 * This is the primary initialization entry point.
	 */
	function loadAll() {
	  var loadedConfigResult = loadJsonSync("nst_translations/config.json", z.custom(function (val) {
	    return val;
	  }));
	  var loadedConfig = unwrap(loadedConfigResult, "Config Load Failed");
	  var merged = _objectSpread2(_objectSpread2(_objectSpread2({}, DEFAULT_CONFIG), loadedConfig), {}, {
	    __textCommands__: loadedConfig != null && loadedConfig.__textCommands__ ? _objectSpread2(_objectSpread2({}, DEFAULT_CONFIG.__textCommands__), loadedConfig.__textCommands__) : DEFAULT_CONFIG.__textCommands__,
	    __fontConfig__: loadedConfig != null && loadedConfig.__fontConfig__ ? _objectSpread2(_objectSpread2({}, DEFAULT_CONFIG.__fontConfig__), loadedConfig.__fontConfig__) : DEFAULT_CONFIG.__fontConfig__
	  });
	  var parsed = ConfigSchema.safeParse(merged);
	  if (!parsed.success) {
	    throw new Error(parsed.error ? parsed.error.message : "Unknown validation error");
	  }
	  var config = parsed.data;
	  // Publish font config to the font-set module.
	  setLoadedFontConfig(config.__fontConfig__);
	  // Load all database files.
	  _baseTranslations = new Map();
	  _loadedFiles = {};
	  var totalEntries = 0;
	  for (var i = 0; i < DB_FILES.length; i++) {
	    var dbFile = DB_FILES[i];
	    if (dbFile !== undefined) {
	      totalEntries += loadTranslationFile(dbFile);
	    }
	  }
	  // On NW.js, scan the nst_translations/ directory for any additional .json files.
	  if (typeof require !== "undefined") {
	    try {
	      var fs = require("fs");
	      var dir = toLocalPath(getBasePath() + "/nst_translations/");
	      if (fs.existsSync(dir)) {
	        var all = fs.readdirSync(dir);
	        for (var j = 0; j < all.length; j++) {
	          var fname = all[j];
	          if (fname !== undefined && /\.json$/i.test(fname) && fname !== "config.json") {
	            var baseName = fname.replace(/\.json$/i, "");
	            totalEntries += loadTranslationFile(baseName);
	          }
	        }
	      }
	    } catch (_e) {/* ignore */}
	  }
	  log$2.info({
	    configEntries: Object.keys(config).length,
	    translationEntries: _baseTranslations.size,
	    filesLoaded: Object.keys(_loadedFiles).length
	  }, "Load complete");
	  return {
	    config: config,
	    translations: _baseTranslations,
	    fontConfig: config.__fontConfig__,
	    customHooks: config.__customHooks__,
	    sourceLocale: config.__sourceLocale__
	  };
	}
	/**
	 * Load a map-specific translation file. Call this when the game changes maps.
	 * @param mapId - the map number (e.g. 1, 2, 3)
	 */
	function loadMap(mapId) {
	  if (mapId <= 0) return;
	  var s = String(mapId);
	  while (s.length < 3) s = "0" + s;
	  loadTranslationFile("Map" + s);
	}
	/**
	 * Reload everything from disk. Useful for the F12 debug console.
	 */
	function reloadAll() {
	  _baseTranslations = new Map();
	  _loadedFiles = {};
	  return loadAll();
	}

	var HOOK_ORIGINAL_KEY = "__translatorHookOriginal__";
	var HOOK_FULL_NAME_KEY = "__translatorHookFullName__";
	// ============== 常量 ==============
	var MESSAGES = {
	  classNotExist: "类不存在",
	  methodNotExist: "方法不存在",
	  hookInstallSuccess: "Hook安装成功:",
	  hookInstallError: "Hook安装错误:",
	  hookExecError: "Hook执行错误:"
	};
	function createSilentLogger() {
	  return function () {};
	}
	function getOriginalHookMethod(method) {
	  var _method$HOOK_ORIGINAL;
	  return (_method$HOOK_ORIGINAL = method[HOOK_ORIGINAL_KEY]) != null ? _method$HOOK_ORIGINAL : method;
	}
	// ============== Hook 管理器 ==============
	var CustomHookManager = /*#__PURE__*/function () {
	  function CustomHookManager(tr, trData, logger) {
	    _classCallCheck(this, CustomHookManager);
	    this._tr = void 0;
	    this._trData = void 0;
	    this._log = void 0;
	    this._stats = {
	      total: 0,
	      installed: 0,
	      failed: 0
	    };
	    /** 处理器映射表 */
	    this._handlers = new Map([["tr0", this._handleTr0.bind(this)], ["trRet", this._handleTrRet.bind(this)], ["trArray", this._handleTrArray.bind(this)], ["trNestedArray", this._handleTrNestedArray.bind(this)], ["trObject", this._handleTrObject.bind(this)], ["trObjectAfter", this._handleTrObjectAfter.bind(this)], ["trRetObject", this._handleTrRetObject.bind(this)], ["trThis", this._handleTrThis.bind(this)], ["trThisAfter", this._handleTrThisAfter.bind(this)], ["trCommandList", this._handleTrCommandList.bind(this)], ["trData", this._handleTrData.bind(this)], ["trNested", this._handleTrNested.bind(this)]]);
	    this._tr = tr;
	    this._trData = trData;
	    this._log = logger != null ? logger : createSilentLogger();
	  }
	  return _createClass(CustomHookManager, [{
	    key: "stats",
	    get: function get() {
	      return _objectSpread2({}, this._stats);
	    }
	    /**
	     * 通过点号分隔的路径获取嵌套对象/函数（支持 class/function）
	     * @param path 对象路径，如 "SceneManager" 或 "Window_Base"
	     */
	  }, {
	    key: "_getObjectByPath",
	    value: function _getObjectByPath(path) {
	      if (!path || typeof path !== "string") return undefined;
	      var parts = path.split(".").filter(Boolean);
	      // 比 window 更通用：支持 module/不同运行环境下的全局对象
	      var root = typeof globalThis !== "undefined" ? globalThis : window;
	      var current = root;
	      var _iterator = _createForOfIteratorHelper(parts),
	        _step;
	      try {
	        for (_iterator.s(); !(_step = _iterator.n()).done;) {
	          var part = _step.value;
	          if (current === null || current === undefined) return undefined;
	          var _t = _typeof(current);
	          // 允许 function（class/constructor 在 JS 里就是 function）
	          if (_t !== "object" && _t !== "function") return undefined;
	          current = current[part];
	        }
	      } catch (err) {
	        _iterator.e(err);
	      } finally {
	        _iterator.f();
	      }
	      if (current === null || current === undefined) return undefined;
	      var t = _typeof(current);
	      return t === "object" || t === "function" ? current : undefined;
	    }
	    /**
	     * 检测 Hook 兼容性（支持静态方法和实例方法）
	     * 支持嵌套路径访问
	     */
	  }, {
	    key: "_checkCompatibility",
	    value: function _checkCompatibility(h) {
	      // 使用路径解析获取目标对象
	      var cls = this._getObjectByPath(h.class);
	      if (!cls) {
	        return {
	          compatible: false,
	          reason: `${MESSAGES.classNotExist}: ${h.class}`,
	          target: null,
	          method: null,
	          isStatic: false
	        };
	      }
	      // 优先检查配置中指定的类型
	      if (h.static === true) {
	        // 明确指定为静态方法（直接属性）
	        var method = cls[h.method];
	        if (typeof method !== "function") {
	          return {
	            compatible: false,
	            reason: `静态 ${MESSAGES.methodNotExist}: ${h.method}`,
	            target: null,
	            method: null,
	            isStatic: true
	          };
	        }
	        return {
	          compatible: true,
	          reason: "",
	          target: cls,
	          method: method,
	          isStatic: true
	        };
	      }
	      if (h.static === false) {
	        // 明确指定为实例方法（prototype 上的方法）
	        var _prototype = cls["prototype"];
	        if (!_prototype) {
	          return {
	            compatible: false,
	            reason: `${MESSAGES.classNotExist}，无 prototype`,
	            target: null,
	            method: null,
	            isStatic: false
	          };
	        }
	        var _method = _prototype[h.method];
	        if (typeof _method !== "function") {
	          return {
	            compatible: false,
	            reason: `实例 ${MESSAGES.methodNotExist}: ${h.method}`,
	            target: null,
	            method: null,
	            isStatic: false
	          };
	        }
	        return {
	          compatible: true,
	          reason: "",
	          target: _prototype,
	          method: _method,
	          isStatic: false
	        };
	      }
	      // 未指定时自动检测：先检查直接方法（静态），再检查 prototype（实例）
	      var staticMethod = cls[h.method];
	      if (typeof staticMethod === "function") {
	        return {
	          compatible: true,
	          reason: "",
	          target: cls,
	          method: staticMethod,
	          isStatic: true
	        };
	      }
	      var prototype = cls["prototype"];
	      if (prototype) {
	        var instanceMethod = prototype[h.method];
	        if (typeof instanceMethod === "function") {
	          return {
	            compatible: true,
	            reason: "",
	            target: prototype,
	            method: instanceMethod,
	            isStatic: false
	          };
	        }
	      }
	      return {
	        compatible: false,
	        reason: `${MESSAGES.methodNotExist}: ${h.method}`,
	        target: null,
	        method: null,
	        isStatic: false
	      };
	    }
	    /** 创建内部 Hook 配置 */
	  }, {
	    key: "_createConfig",
	    value: function _createConfig(h, isStatic) {
	      var _h$paramIndex, _h$minParamLength;
	      return Object.freeze({
	        paramIdx: (_h$paramIndex = h.paramIndex) != null ? _h$paramIndex : 0,
	        nestedIdx: Object.freeze(h.nestedIndex ? _toConsumableArray(h.nestedIndex) : []),
	        fields: Object.freeze(h.fields ? _toConsumableArray(h.fields) : []),
	        fullName: `${h.class}.${h.method}`,
	        type: h.type || "tr0",
	        minParamLen: (_h$minParamLength = h.minParamLength) != null ? _h$minParamLength : 0,
	        isStatic: isStatic
	      });
	    }
	    /** 验证数组索引是否有效 */
	  }, {
	    key: "_isValidIndex",
	    value: function _isValidIndex(arr, idx) {
	      return Array.isArray(arr) && typeof idx === "number" && Number.isInteger(idx) && idx >= 0 && idx < arr.length;
	    }
	    /** 获取嵌套值 */
	  }, {
	    key: "_getNestedValue",
	    value: function _getNestedValue(root, nestedIdx) {
	      var value = root;
	      var _iterator2 = _createForOfIteratorHelper(nestedIdx),
	        _step2;
	      try {
	        for (_iterator2.s(); !(_step2 = _iterator2.n()).done;) {
	          var idx = _step2.value;
	          if (!this._isValidIndex(value, idx)) return undefined;
	          value = value[idx];
	        }
	      } catch (err) {
	        _iterator2.e(err);
	      } finally {
	        _iterator2.f();
	      }
	      return value;
	    }
	    /** 设置嵌套值 */
	  }, {
	    key: "_setNestedValue",
	    value: function _setNestedValue(root, nestedIdx, newValue) {
	      if (nestedIdx.length === 0) return;
	      var target = root;
	      for (var i = 0; i < nestedIdx.length - 1; i++) {
	        var idx = nestedIdx[i];
	        if (!this._isValidIndex(target, idx)) return;
	        target = target[idx];
	      }
	      var lastIdx = nestedIdx[nestedIdx.length - 1];
	      if (this._isValidIndex(target, lastIdx)) {
	        target[lastIdx] = newValue;
	      }
	    }
	    // ============== 处理器方法 ==============
	    /** tr0: 翻译指定索引的字符串参数 */
	  }, {
	    key: "_handleTr0",
	    value: function _handleTr0(hctx) {
	      var orig = hctx.orig,
	        args = hctx.args,
	        cfg = hctx.cfg,
	        ctx = hctx.ctx,
	        tr = hctx.tr;
	      var idx = cfg.paramIdx;
	      if (typeof args[idx] === "string") {
	        args[idx] = tr(args[idx]);
	      }
	      return orig.apply(ctx, args);
	    }
	    /** trRet: 翻译返回值 */
	  }, {
	    key: "_handleTrRet",
	    value: function _handleTrRet(hctx) {
	      var orig = hctx.orig,
	        args = hctx.args,
	        ctx = hctx.ctx,
	        tr = hctx.tr;
	      var ret = orig.apply(ctx, args);
	      return typeof ret === "string" ? tr(ret) : ret;
	    }
	    /** trArray: 翻译数组参数中的所有字符串 */
	  }, {
	    key: "_handleTrArray",
	    value: function _handleTrArray(hctx) {
	      var orig = hctx.orig,
	        args = hctx.args,
	        cfg = hctx.cfg,
	        ctx = hctx.ctx,
	        tr = hctx.tr;
	      var arr = args[cfg.paramIdx];
	      if (Array.isArray(arr)) {
	        for (var i = 0; i < arr.length; i++) {
	          if (typeof arr[i] === "string") {
	            arr[i] = tr(arr[i]);
	          }
	        }
	      }
	      return orig.apply(ctx, args);
	    }
	    /** trNestedArray: 翻译嵌套数组中的所有字符串 */
	  }, {
	    key: "_handleTrNestedArray",
	    value: function _handleTrNestedArray(hctx) {
	      var orig = hctx.orig,
	        args = hctx.args,
	        cfg = hctx.cfg,
	        ctx = hctx.ctx,
	        tr = hctx.tr,
	        getNestedValue = hctx.getNestedValue;
	      var root = args[cfg.paramIdx];
	      if (cfg.minParamLen > 0 && (!Array.isArray(root) || root.length < cfg.minParamLen)) {
	        return orig.apply(ctx, args);
	      }
	      var nestedArr = getNestedValue(root, cfg.nestedIdx);
	      if (Array.isArray(nestedArr)) {
	        for (var i = 0; i < nestedArr.length; i++) {
	          if (typeof nestedArr[i] === "string") {
	            nestedArr[i] = tr(nestedArr[i]);
	          }
	        }
	      }
	      return orig.apply(ctx, args);
	    }
	    /** trObject: 翻译对象参数的指定字段 */
	  }, {
	    key: "_handleTrObject",
	    value: function _handleTrObject(hctx) {
	      var orig = hctx.orig,
	        args = hctx.args,
	        cfg = hctx.cfg,
	        ctx = hctx.ctx,
	        tr = hctx.tr;
	      var obj = args[cfg.paramIdx];
	      if (obj) {
	        var _iterator3 = _createForOfIteratorHelper(cfg.fields),
	          _step3;
	        try {
	          for (_iterator3.s(); !(_step3 = _iterator3.n()).done;) {
	            var f = _step3.value;
	            if (typeof obj[f] === "string") {
	              obj[f] = tr(obj[f]);
	            }
	          }
	        } catch (err) {
	          _iterator3.e(err);
	        } finally {
	          _iterator3.f();
	        }
	      }
	      return orig.apply(ctx, args);
	    }
	    /** trObjectAfter: 调用原方法后翻译对象参数的指定字段 */
	  }, {
	    key: "_handleTrObjectAfter",
	    value: function _handleTrObjectAfter(hctx) {
	      var orig = hctx.orig,
	        args = hctx.args,
	        cfg = hctx.cfg,
	        ctx = hctx.ctx,
	        tr = hctx.tr;
	      var result = orig.apply(ctx, args);
	      var obj = args[cfg.paramIdx];
	      if (obj) {
	        var _iterator4 = _createForOfIteratorHelper(cfg.fields),
	          _step4;
	        try {
	          for (_iterator4.s(); !(_step4 = _iterator4.n()).done;) {
	            var f = _step4.value;
	            if (typeof obj[f] === "string") {
	              obj[f] = tr(obj[f]);
	            }
	          }
	        } catch (err) {
	          _iterator4.e(err);
	        } finally {
	          _iterator4.f();
	        }
	      }
	      return result;
	    }
	    /** trRetObject: 翻译返回对象的指定字段 */
	  }, {
	    key: "_handleTrRetObject",
	    value: function _handleTrRetObject(hctx) {
	      var orig = hctx.orig,
	        args = hctx.args,
	        cfg = hctx.cfg,
	        ctx = hctx.ctx,
	        tr = hctx.tr;
	      var result = orig.apply(ctx, args);
	      if (result && _typeof(result) === "object") {
	        var obj = result;
	        var _iterator5 = _createForOfIteratorHelper(cfg.fields),
	          _step5;
	        try {
	          for (_iterator5.s(); !(_step5 = _iterator5.n()).done;) {
	            var f = _step5.value;
	            if (typeof obj[f] === "string") {
	              obj[f] = tr(obj[f]);
	            }
	          }
	        } catch (err) {
	          _iterator5.e(err);
	        } finally {
	          _iterator5.f();
	        }
	      }
	      return result;
	    }
	    /** trThis: 翻译 this 上下文的指定字段 */
	  }, {
	    key: "_handleTrThis",
	    value: function _handleTrThis(hctx) {
	      var orig = hctx.orig,
	        args = hctx.args,
	        cfg = hctx.cfg,
	        ctx = hctx.ctx,
	        tr = hctx.tr;
	      var context = ctx;
	      var _iterator6 = _createForOfIteratorHelper(cfg.fields),
	        _step6;
	      try {
	        for (_iterator6.s(); !(_step6 = _iterator6.n()).done;) {
	          var f = _step6.value;
	          if (typeof context[f] === "string") {
	            context[f] = tr(context[f]);
	          }
	        }
	      } catch (err) {
	        _iterator6.e(err);
	      } finally {
	        _iterator6.f();
	      }
	      return orig.apply(ctx, args);
	    }
	    /** trThisAfter: 调用原方法后翻译 this 上下文的指定字段 */
	  }, {
	    key: "_handleTrThisAfter",
	    value: function _handleTrThisAfter(hctx) {
	      var orig = hctx.orig,
	        args = hctx.args,
	        cfg = hctx.cfg,
	        ctx = hctx.ctx,
	        tr = hctx.tr;
	      var result = orig.apply(ctx, args);
	      var context = ctx;
	      var _iterator7 = _createForOfIteratorHelper(cfg.fields),
	        _step7;
	      try {
	        for (_iterator7.s(); !(_step7 = _iterator7.n()).done;) {
	          var f = _step7.value;
	          if (typeof context[f] === "string") {
	            context[f] = tr(context[f]);
	          }
	        }
	      } catch (err) {
	        _iterator7.e(err);
	      } finally {
	        _iterator7.f();
	      }
	      return result;
	    }
	    /** trCommandList: 翻译命令列表中的 name 字段（RPG Maker 专用） */
	  }, {
	    key: "_handleTrCommandList",
	    value: function _handleTrCommandList(hctx) {
	      var orig = hctx.orig,
	        args = hctx.args,
	        ctx = hctx.ctx,
	        tr = hctx.tr;
	      var result = orig.apply(ctx, args);
	      var context = ctx;
	      var list = context["_list"];
	      if (Array.isArray(list)) {
	        var _iterator8 = _createForOfIteratorHelper(list),
	          _step8;
	        try {
	          for (_iterator8.s(); !(_step8 = _iterator8.n()).done;) {
	            var item = _step8.value;
	            if (item && typeof item["name"] === "string") {
	              item["name"] = tr(item["name"]);
	            }
	          }
	        } catch (err) {
	          _iterator8.e(err);
	        } finally {
	          _iterator8.f();
	        }
	      }
	      return result;
	    }
	    /** trData: 调用原方法后使用 translateData 翻译数据对象 */
	  }, {
	    key: "_handleTrData",
	    value: function _handleTrData(hctx) {
	      var orig = hctx.orig,
	        args = hctx.args,
	        cfg = hctx.cfg,
	        ctx = hctx.ctx,
	        trData = hctx.trData;
	      var result = orig.apply(ctx, args);
	      var dataObj = args[cfg.paramIdx];
	      if (dataObj !== null && dataObj !== undefined) {
	        trData(dataObj);
	      }
	      return result;
	    }
	    /** trNested: 翻译嵌套路径指向的字符串值 */
	  }, {
	    key: "_handleTrNested",
	    value: function _handleTrNested(hctx) {
	      var orig = hctx.orig,
	        args = hctx.args,
	        cfg = hctx.cfg,
	        ctx = hctx.ctx,
	        tr = hctx.tr,
	        getNestedValue = hctx.getNestedValue,
	        setNestedValue = hctx.setNestedValue;
	      var root = args[cfg.paramIdx];
	      if (cfg.minParamLen > 0 && (!Array.isArray(root) || root.length < cfg.minParamLen)) {
	        return orig.apply(ctx, args);
	      }
	      var nestedValue = getNestedValue(root, cfg.nestedIdx);
	      if (typeof nestedValue === "string") {
	        if (cfg.nestedIdx.length === 0) {
	          args[cfg.paramIdx] = tr(nestedValue);
	        } else {
	          setNestedValue(root, cfg.nestedIdx, tr(nestedValue));
	        }
	      }
	      return orig.apply(ctx, args);
	    }
	    /** 默认处理器: 直接调用原方法 */
	  }, {
	    key: "_handleDefault",
	    value: function _handleDefault(hctx) {
	      var orig = hctx.orig,
	        args = hctx.args,
	        ctx = hctx.ctx;
	      return orig.apply(ctx, args);
	    }
	  }, {
	    key: "_executeHandler",
	    value: /** 执行翻译处理 */
	    function _executeHandler(type, orig, args, cfg, ctx) {
	      var _this$_handlers$get;
	      var hctx = {
	        orig: orig,
	        args: args,
	        cfg: cfg,
	        ctx: ctx,
	        tr: this._tr,
	        trData: this._trData,
	        getNestedValue: this._getNestedValue.bind(this),
	        setNestedValue: this._setNestedValue.bind(this)
	      };
	      var handler = (_this$_handlers$get = this._handlers.get(type)) != null ? _this$_handlers$get : this._handleDefault.bind(this);
	      return handler(hctx);
	    }
	    /** 创建包装函数 */
	  }, {
	    key: "_createWrapper",
	    value: function _createWrapper(origMethod, cfg) {
	      var executeHandler = this._executeHandler.bind(this);
	      var fullName = cfg.fullName;
	      var hookType = cfg.type;
	      var wrapper = function wrapper() {
	        for (var _len = arguments.length, args = new Array(_len), _key = 0; _key < _len; _key++) {
	          args[_key] = arguments[_key];
	        }
	        try {
	          return executeHandler(cfg.type, origMethod, [].concat(args), cfg, this);
	        } catch (e) {
	          var errorMsg = e instanceof Error ? e.message : String(e);
	          // 使用 console.error 而非 showError，Hook 运行时错误不应崩溃游戏
	          console.error(`[Translator] Hook 执行错误 "${fullName}" [${hookType}]: ${errorMsg}`);
	          // 降级：调用原始方法
	          try {
	            return origMethod.apply(this, args);
	          } catch (_) {
	            return undefined;
	          }
	        }
	      };
	      Object.defineProperty(wrapper, HOOK_ORIGINAL_KEY, {
	        value: origMethod,
	        configurable: true
	      });
	      Object.defineProperty(wrapper, HOOK_FULL_NAME_KEY, {
	        value: fullName,
	        configurable: true
	      });
	      return wrapper;
	    }
	    /** 批量安装 Hook */
	  }, {
	    key: "install",
	    value: function install(hooks) {
	      if (!(hooks != null && hooks.length)) {
	        this._log("No hooks to install");
	        return;
	      }
	      this._log(`Installing ${hooks.length} hooks...`);
	      var _iterator9 = _createForOfIteratorHelper(hooks),
	        _step9;
	      try {
	        for (_iterator9.s(); !(_step9 = _iterator9.n()).done;) {
	          var h = _step9.value;
	          if (!h.class || !h.method) continue;
	          if (h.enabled === false) {
	            this._log(`Hook被禁用: ${h.class}.${h.method}`);
	            continue;
	          }
	          this._stats.total++;
	          var fullName = `${h.class}.${h.method}`;
	          // 1. 兼容性检查
	          var compat = this._checkCompatibility(h);
	          if (!compat.compatible) {
	            this._stats.failed++;
	            this._log(`${MESSAGES.hookInstallError} ${fullName} (${compat.reason})`);
	            continue;
	          }
	          // 2. 安装 Hook
	          try {
	            var cfg = this._createConfig(h, compat.isStatic);
	            var originalMethod = getOriginalHookMethod(compat.method);
	            var wrapper = this._createWrapper(originalMethod, cfg);
	            // 替换方法（静态方法或实例方法）
	            compat.target[h.method] = wrapper;
	            var methodType = compat.isStatic ? "静态" : "实例";
	            this._stats.installed++;
	            this._log(`${MESSAGES.hookInstallSuccess} ${fullName} [${cfg.type}] (${methodType}方法)`);
	          } catch (e) {
	            this._stats.failed++;
	            var errorMsg = e instanceof Error ? e.message : String(e);
	            showError("Hook 安装失败", `无法安装 Hook "${fullName}"\n错误: ${errorMsg}`);
	          }
	        }
	      } catch (err) {
	        _iterator9.e(err);
	      } finally {
	        _iterator9.f();
	      }
	      this._log(`Hook安装执行完毕: ${this._stats.installed}/${this._stats.total} installed, ${this._stats.failed} failed`);
	    }
	  }]);
	}();
	// ============== 公共 API ==============
	function applyHooks$1(options) {
	  var translate = options.translate,
	    translateData = options.translateData,
	    hooks = options.hooks,
	    logger = options.logger,
	    _options$verbose = options.verbose,
	    verbose = _options$verbose === void 0 ? true : _options$verbose;
	  var effectiveLogger = verbose ? logger : createSilentLogger();
	  var manager = new CustomHookManager(translate, translateData, effectiveLogger);
	  manager.install(hooks);
	  return {
	    manager: manager,
	    stats: manager.stats
	  };
	}

	// =============================================================================
	// 常量配置
	// =============================================================================
	var REGEX_KEY = "__regex__";
	var DEFAULT_MISSED_LIMIT = 500;
	var DEFAULT_MAX_DEPTH = 6;
	var DEFAULT_CACHE_SIZE = 3000;
	var MAX_LOOP_COUNT = 100;
	var MAX_MARKERS_SIZE = 5000;
	var PLACEHOLDER_BRACE = /\{(\d+)\}/g;
	/**
	 * 验证正则表达式标志是否有效
	 * @param flags 标志字符串
	 * @returns 如果标志有效返回 true
	 */
	function isValidRegexFlags(flags) {
	  // 只允许有效的正则表达式标志
	  return /^[gimsuy]*$/.test(flags);
	}
	// =============================================================================
	// 日志配置（简化版）
	// =============================================================================
	var log$1 = pino({
	  level: "info"
	});
	// =============================================================================
	// 状态管理
	// =============================================================================
	var translatedObjects = new WeakSet();
	var state = {
	  dict: new Map(),
	  sortedKeys: [],
	  cache: new LRUCache(DEFAULT_CACHE_SIZE),
	  missed: new Map(),
	  config: null,
	  controlRe: null,
	  controlPattern: "",
	  // 存储控制符模式字符串，用于创建新正则实例
	  ignoreRe: null,
	  regexPatterns: [],
	  ready: false,
	  hooked: false,
	  acAutomaton: null,
	  acPatterns: [],
	  missedLimit: DEFAULT_MISSED_LIMIT,
	  maxDepth: DEFAULT_MAX_DEPTH,
	  // 用 LRUCache 替换 Set，避免 FIFO 淘汰时的 O(n) 遍历
	  translatedMarkers: new LRUCache(MAX_MARKERS_SIZE),
	  sourceLocaleCharsRe: null
	};
	// =============================================================================
	// 初始化
	// =============================================================================
	function rebuildAhoCorasick() {
	  var patterns = state.sortedKeys;
	  if (patterns.length === 0) {
	    state.acAutomaton = null;
	    state.acPatterns = [];
	    return;
	  }
	  state.acPatterns = patterns;
	  state.acAutomaton = new AhoCorasick(patterns);
	}
	function invalidateAhoCorasick() {
	  state.acAutomaton = null;
	  state.acPatterns = [];
	}
	function rebuildSortedKeys() {
	  state.sortedKeys = Array.from(state.dict.keys()).filter(function (key) {
	    return !(key.startsWith("__") && key.endsWith("__"));
	  }).sort(function (a, b) {
	    return b.length - a.length;
	  });
	}
	/**
	 * 根据源语言代码构建字符检测正则表达式
	 * @param sourceLocale 源语言代码（如 'ja', 'ko', 'en' 等）
	 * @returns 用于检测源语言字符的正则表达式，如果无法识别则返回 null
	 */
	function buildSourceLocaleRegex(sourceLocale) {
	  var locale = sourceLocale.toLowerCase();
	  // 日语：平假名 + 片假名（不包含CJK汉字以避免误判中文）
	  if (locale === "ja" || locale === "jp" || locale === "jpn") {
	    return /[\u3040-\u309F\u30A0-\u30FF]/;
	  }
	  // 韩语：谚文（韩文字母）
	  if (locale === "ko" || locale === "kor" || locale === "kr") {
	    return /[\uAC00-\uD7AF\u1100-\u11FF\u3130-\u318F]/;
	  }
	  // 英语：基本拉丁字母
	  if (locale === "en" || locale === "eng" || locale.startsWith("en-")) {
	    return /[a-zA-Z]/;
	  }
	  showError("不支持的源语言", `无法识别的语言代码: "${sourceLocale}"\n支持的语言代码:\n  - 日语: ja, jp, jpn\n  - 韩语: ko, kor, kr\n  - 英语: en, eng, en-*`);
	}
	function init(preloaded) {
	  var _config$__controlChar, _config$__ignorePatte;
	  if (state.ready) return;
	  var _ref = preloaded != null ? preloaded : loadAll(),
	    config = _ref.config,
	    translations = _ref.translations,
	    sourceLocale = _ref.sourceLocale;
	  state.config = config;
	  state.sourceLocaleCharsRe = buildSourceLocaleRegex(sourceLocale);
	  var controlPatterns = (_config$__controlChar = config == null ? void 0 : config.__controlCharPatterns__) != null ? _config$__controlChar : [];
	  var ignorePatterns = (_config$__ignorePatte = config == null ? void 0 : config.__ignorePatterns__) != null ? _config$__ignorePatte : [];
	  var controlPattern = controlPatterns.length > 0 ? controlPatterns.join("|") : "";
	  state.controlRe = safeCompileRegex(controlPattern, "gi");
	  // 存储控制符模式字符串，用于在extractControls 中创建新的正则实例
	  state.controlPattern = controlPattern;
	  state.ignoreRe = safeCompileRegex(ignorePatterns.length > 0 ? ignorePatterns.join("|") : "");
	  state.dict = translations;
	  rebuildSortedKeys();
	  var regexData = state.dict.get(REGEX_KEY);
	  if (regexData) {
	    loadRegexPatterns(regexData);
	    state.dict.delete(REGEX_KEY);
	    rebuildSortedKeys();
	  }
	  rebuildAhoCorasick();
	  state.ready = true;
	  log$1.info({
	    count: state.dict.size
	  }, "Translator ready");
	}
	function safeCompileRegex(pattern, flags) {
	  if (!pattern) return null;
	  if (flags && !isValidRegexFlags(flags)) {
	    showError("正则表达式标志无效", `无效的正则表达式标志: "${flags}"，只允许 g, i, m, s, u, y`);
	  }
	  try {
	    return new RegExp(pattern, flags);
	  } catch (e) {
	    var errorMsg = e instanceof Error ? e.message : String(e);
	    return showError("正则表达式编译失败", `无法编译正则表达式: "${pattern}"\n错误: ${errorMsg}`);
	  }
	}
	function loadRegexPatterns(regexData) {
	  var regexArray;
	  try {
	    regexArray = JSON.parse(regexData);
	  } catch (e) {
	    var errorMsg = e instanceof Error ? e.message : String(e);
	    showError("正则模式解析失败", `无法解析 __regex__ 配置的JSON 数据\n错误: ${errorMsg}`);
	  }
	  if (!Array.isArray(regexArray)) {
	    showError("正则模式格式错误", `__regex__ 配置必须是数组格式，当前类型: ${_typeof(regexArray)}`);
	  }
	  state.regexPatterns = regexArray.map(function (entry) {
	    // 验证元素数量
	    if (entry.length < 2 || entry.length > 3) {
	      return showError("正则模式格式错误", `__regex__ 配置的每个模式必须包含2或3个元素，当前元素数量: ${entry.length}`);
	    }
	    var _entry = _slicedToArray(entry, 3),
	      patternStr = _entry[0],
	      replacement = _entry[1],
	      _entry$ = _entry[2],
	      flags = _entry$ === void 0 ? "" : _entry$;
	    // 验证 flags 是否有效
	    if (flags && !isValidRegexFlags(flags)) {
	      return showError("正则模式标志无效", `__regex__ 配置的标志必须是有效的正则表达式标志（g, i, m, s, u, y），当前标志: "${flags}"`);
	    }
	    try {
	      return {
	        pattern: new RegExp(patternStr, flags),
	        replacement: replacement
	      };
	    } catch (e) {
	      var _errorMsg = e instanceof Error ? e.message : String(e);
	      return showError("正则模式编译失败", `无法编译正则表达式: "${patternStr}"\n标志: "${flags}"\n错误: ${_errorMsg}`);
	    }
	  }).filter(function (x) {
	    return x !== null;
	  });
	  log$1.info({
	    count: state.regexPatterns.length
	  }, "Regex patterns loaded");
	}
	// =============================================================================
	// 控制符处理
	// =============================================================================
	/**
	 * 从文本中提取控制符信息
	 * 改进：每次创建新的正则实例避免状态污染
	 */
	function _extractControls3(text) {
	  // 如果没有配置控制符模式，直接返回原文
	  if (!state.controlPattern) {
	    return {
	      plain: text,
	      controls: []
	    };
	  }
	  // 每次创建新的正则实例，避免全局状态污染（并发安全）
	  var extractRe = new RegExp(state.controlPattern, "gi");
	  var replaceRe = new RegExp(state.controlPattern, "gi");
	  var controls = [];
	  var match;
	  var loopCount = 0;
	  match = extractRe.exec(text);
	  while (match !== null && loopCount++ < MAX_LOOP_COUNT) {
	    controls.push({
	      text: match[0],
	      index: match.index
	    });
	    // 防止零宽匹配导致无限循环
	    if (match.index === extractRe.lastIndex) {
	      extractRe.lastIndex++;
	    }
	    match = extractRe.exec(text);
	  }
	  var plain = text.replace(replaceRe, "");
	  return {
	    plain: plain,
	    controls: controls
	  };
	}
	/**
	 * 将控制符应用到翻译后的文本
	 * 改进：合并 test + replace 为单次操作，避免重复匹配
	 */
	function applyControlPlaceholders(translation, controls) {
	  if (controls.length === 0) return translation;
	  // 单次 replace 操作，同时检测是否有占位符被替换
	  var hasReplacement = false;
	  var result = translation.replace(PLACEHOLDER_BRACE, function (match, indexStr) {
	    var index = Number.parseInt(indexStr, 10);
	    if (index >= 0 && index < controls.length) {
	      var _controls$index$text, _controls$index;
	      hasReplacement = true;
	      return (_controls$index$text = (_controls$index = controls[index]) == null ? void 0 : _controls$index.text) != null ? _controls$index$text : match;
	    }
	    return match;
	  });
	  // 如果有占位符被替换，返回替换结果；否则使用重建
	  return hasReplacement ? result : reconstructWithControls(translation, controls);
	}
	/**
	 * 重建控制符：将前缀控制符放在开头，其余控制符追加到末尾
	 */
	function reconstructWithControls(translation, controls) {
	  if (controls.length === 0) return translation;
	  var prefixControls = [];
	  var suffixControls = [];
	  // 识别连续出现在文本开头的控制符作为前缀
	  var lastEnd = 0;
	  var _iterator = _createForOfIteratorHelper(controls),
	    _step;
	  try {
	    for (_iterator.s(); !(_step = _iterator.n()).done;) {
	      var ctrl = _step.value;
	      if (ctrl.index === lastEnd) {
	        prefixControls.push(ctrl.text);
	        lastEnd = ctrl.index + ctrl.text.length;
	      } else {
	        break;
	      }
	    }
	    // 剩余的控制符作为后缀
	  } catch (err) {
	    _iterator.e(err);
	  } finally {
	    _iterator.f();
	  }
	  for (var i = prefixControls.length; i < controls.length; i++) {
	    var _controls$i$text, _controls$i;
	    suffixControls.push((_controls$i$text = (_controls$i = controls[i]) == null ? void 0 : _controls$i.text) != null ? _controls$i$text : "");
	  }
	  return prefixControls.join("") + translation + suffixControls.join("");
	}
	// =============================================================================
	// 核心翻译功能
	// =============================================================================
	/**
	 * 检测文本是否包含源语言字符
	 * 根据配置的 sourceLocale 动态判断
	 */
	function hasSourceLocaleChars(text) {
	  if (!state.sourceLocaleCharsRe) {
	    return false;
	  }
	  return state.sourceLocaleCharsRe.test(text);
	}
	function markTranslated(text) {
	  state.translatedMarkers.set(text, true);
	}
	function translate(text) {
	  var _state$ignoreRe;
	  if (!state.ready || !text || typeof text !== "string") {
	    return text;
	  }
	  var cached = state.cache.get(text);
	  if (cached !== undefined) {
	    return cached;
	  }
	  if ((_state$ignoreRe = state.ignoreRe) != null && _state$ignoreRe.test(text)) {
	    state.cache.set(text, text);
	    return text;
	  }
	  var result = translateCore(text);
	  // 只记录包含源语言字符的未翻译文本
	  if (result === text && hasSourceLocaleChars(text)) {
	    var count = state.missed.get(text);
	    if (count !== undefined) {
	      state.missed.set(text, count + 1);
	    } else if (state.missed.size < state.missedLimit) {
	      state.missed.set(text, 1);
	    }
	  }
	  state.cache.set(text, result);
	  return result;
	}
	function translateCore(text) {
	  // 1. 直接精确匹配 - O(1)
	  var directMatch = state.dict.get(text);
	  if (directMatch !== undefined) {
	    markTranslated(directMatch);
	    return directMatch;
	  }
	  if (state.translatedMarkers.get(text) === true) {
	    return text;
	  }
	  // 2. 提取控制符
	  var _extractControls = _extractControls3(text),
	    plain = _extractControls.plain,
	    controls = _extractControls.controls;
	  var hasControls = controls.length > 0 && plain.length > 0 && plain !== text;
	  // 3. 去除控制符后精确匹配 - O(1)
	  if (hasControls) {
	    var plainMatch = state.dict.get(plain);
	    if (plainMatch !== undefined) {
	      var _result = applyControlPlaceholders(plainMatch, controls);
	      markTranslated(_result);
	      return _result;
	    }
	  }
	  // 4. 正则模式匹配
	  var regexResult = applyRegexPatterns(text);
	  if (regexResult !== null) {
	    markTranslated(regexResult);
	    return regexResult;
	  }
	  // 5. 正则模式匹配 - 去除控制符后
	  if (hasControls) {
	    var plainRegexResult = applyRegexPatterns(plain);
	    if (plainRegexResult !== null) {
	      var _result2 = applyControlPlaceholders(plainRegexResult, controls);
	      markTranslated(_result2);
	      return _result2;
	    }
	  }
	  // 6. 部分匹配
	  var result = partialMatchTranslate(text);
	  if (result !== text) {
	    markTranslated(result);
	  }
	  return result;
	}
	function applyRegexPatterns(text) {
	  var _iterator2 = _createForOfIteratorHelper(state.regexPatterns),
	    _step2;
	  try {
	    for (_iterator2.s(); !(_step2 = _iterator2.n()).done;) {
	      var _step2$value = _step2.value,
	        pattern = _step2$value.pattern,
	        replacement = _step2$value.replacement;
	      pattern.lastIndex = 0;
	      var result = text.replace(pattern, replacement);
	      if (result !== text) {
	        return result;
	      }
	    }
	  } catch (err) {
	    _iterator2.e(err);
	  } finally {
	    _iterator2.f();
	  }
	  return null;
	}
	function partialMatchTranslate(text) {
	  if (!state.acAutomaton) {
	    rebuildAhoCorasick();
	    if (!state.acAutomaton) return text;
	  }
	  var rawMatches = state.acAutomaton.search(text);
	  if (!rawMatches || rawMatches.length === 0) {
	    return text;
	  }
	  var processedMatches = [];
	  var _iterator3 = _createForOfIteratorHelper(rawMatches),
	    _step3;
	  try {
	    for (_iterator3.s(); !(_step3 = _iterator3.n()).done;) {
	      var _step3$value = _slicedToArray(_step3.value, 2),
	        endIndex = _step3$value[0],
	        patterns = _step3$value[1];
	      var longestPattern = patterns.reduce(function (longest, p) {
	        return p.length > longest.length ? p : longest;
	      }, "");
	      if (longestPattern) {
	        var translation = state.dict.get(longestPattern);
	        if (translation) {
	          var start = endIndex - longestPattern.length + 1;
	          processedMatches.push({
	            start: start,
	            end: endIndex + 1,
	            pattern: longestPattern,
	            translation: translation
	          });
	        }
	      }
	    }
	  } catch (err) {
	    _iterator3.e(err);
	  } finally {
	    _iterator3.f();
	  }
	  if (processedMatches.length === 0) {
	    return text;
	  }
	  var selectedMatches = selectNonOverlappingMatches(processedMatches);
	  if (selectedMatches.length === 0) {
	    return text;
	  }
	  var result = text;
	  for (var i = selectedMatches.length - 1; i >= 0; i--) {
	    var match = selectedMatches[i];
	    if (match) {
	      result = result.slice(0, match.start) + match.translation + result.slice(match.end);
	    }
	  }
	  return result;
	}
	function selectNonOverlappingMatches(matches) {
	  if (matches.length === 0) return [];
	  if (matches.length === 1) return matches;
	  var sorted = _toConsumableArray(matches).sort(function (a, b) {
	    var lenDiff = b.end - b.start - (a.end - a.start);
	    return lenDiff === 0 ? a.start - b.start : lenDiff;
	  });
	  var selected = [];
	  var _iterator4 = _createForOfIteratorHelper(sorted),
	    _step4;
	  try {
	    for (_iterator4.s(); !(_step4 = _iterator4.n()).done;) {
	      var match = _step4.value;
	      if (!hasOverlapBinarySearch(selected, match)) {
	        insertSorted(selected, match);
	      }
	    }
	  } catch (err) {
	    _iterator4.e(err);
	  } finally {
	    _iterator4.f();
	  }
	  return selected;
	}
	function hasOverlapBinarySearch(selected, newMatch) {
	  if (selected.length === 0) return false;
	  var start = newMatch.start,
	    end = newMatch.end;
	  var left = 0;
	  var right = selected.length;
	  while (left < right) {
	    var mid = left + right >>> 1;
	    if (selected[mid].end <= start) {
	      left = mid + 1;
	    } else {
	      right = mid;
	    }
	  }
	  if (left < selected.length && selected[left].start < end) {
	    return true;
	  }
	  return false;
	}
	function insertSorted(selected, match) {
	  var left = 0;
	  var right = selected.length;
	  while (left < right) {
	    var mid = left + right >>> 1;
	    if (selected[mid].start < match.start) {
	      left = mid + 1;
	    } else {
	      right = mid;
	    }
	  }
	  selected.splice(left, 0, match);
	}
	// =============================================================================
	// 数据翻译
	// =============================================================================
	function translateData(data) {
	  if (!state.ready || !data) return;
	  try {
	    if (Array.isArray(data)) {
	      var _iterator5 = _createForOfIteratorHelper(data),
	        _step5;
	      try {
	        for (_iterator5.s(); !(_step5 = _iterator5.n()).done;) {
	          var item = _step5.value;
	          if (item && _typeof(item) === "object") {
	            translateObject(item, 0);
	          }
	        }
	      } catch (err) {
	        _iterator5.e(err);
	      } finally {
	        _iterator5.f();
	      }
	    } else if (_typeof(data) === "object") {
	      translateObject(data, 0);
	    }
	  } catch (e) {
	    var errorMsg = e instanceof Error ? e.message : String(e);
	    showError("数据翻译错误", `翻译游戏数据时发生错误\n错误: ${errorMsg}`);
	  }
	}
	function translateObject(obj, depth) {
	  var _cfg$__textFields__;
	  if (!obj || _typeof(obj) !== "object") return;
	  if (translatedObjects.has(obj)) return;
	  translatedObjects.add(obj);
	  var cfg = state.config;
	  if (!cfg) return;
	  var textFields = (_cfg$__textFields__ = cfg.__textFields__) != null ? _cfg$__textFields__ : [];
	  var _iterator6 = _createForOfIteratorHelper(textFields),
	    _step6;
	  try {
	    for (_iterator6.s(); !(_step6 = _iterator6.n()).done;) {
	      var field = _step6.value;
	      var val = obj[field];
	      if (typeof val === "string") {
	        obj[field] = translate(val);
	      } else if (val && _typeof(val) === "object" && !Array.isArray(val)) {
	        translateNested(val, depth + 1);
	      }
	    }
	  } catch (err) {
	    _iterator6.e(err);
	  } finally {
	    _iterator6.f();
	  }
	  var list = obj.list;
	  if (Array.isArray(list)) {
	    translateCommands(list);
	  }
	  var pages = obj.pages;
	  if (Array.isArray(pages)) {
	    var _iterator7 = _createForOfIteratorHelper(pages),
	      _step7;
	    try {
	      for (_iterator7.s(); !(_step7 = _iterator7.n()).done;) {
	        var page = _step7.value;
	        if (page && _typeof(page) === "object") {
	          var pageObj = page;
	          if (Array.isArray(pageObj.list)) {
	            translateCommands(pageObj.list);
	          }
	        }
	      }
	    } catch (err) {
	      _iterator7.e(err);
	    } finally {
	      _iterator7.f();
	    }
	  }
	}
	function translateNested(obj, depth) {
	  if (depth > state.maxDepth || !obj || _typeof(obj) !== "object") return;
	  if (translatedObjects.has(obj)) return;
	  translatedObjects.add(obj);
	  for (var _i = 0, _Object$keys = Object.keys(obj); _i < _Object$keys.length; _i++) {
	    var key = _Object$keys[_i];
	    var val = obj[key];
	    if (typeof val === "string") {
	      obj[key] = translate(val);
	    } else if (val && _typeof(val) === "object" && !Array.isArray(val)) {
	      translateNested(val, depth + 1);
	    }
	  }
	}
	function translateCommands(list) {
	  var _state$config$__textC;
	  if (!Array.isArray(list) || !state.config) return;
	  var cmds = (_state$config$__textC = state.config.__textCommands__) != null ? _state$config$__textC : {};
	  var _iterator8 = _createForOfIteratorHelper(list),
	    _step8;
	  try {
	    for (_iterator8.s(); !(_step8 = _iterator8.n()).done;) {
	      var cmd = _step8.value;
	      if (!cmd || _typeof(cmd) !== "object") continue;
	      var cmdObj = cmd;
	      var code = cmdObj.code;
	      var params = cmdObj.parameters;
	      if (typeof code !== "number" || !Array.isArray(params)) continue;
	      var indices = cmds[String(code)];
	      if (indices) {
	        var _iterator9 = _createForOfIteratorHelper(indices),
	          _step9;
	        try {
	          for (_iterator9.s(); !(_step9 = _iterator9.n()).done;) {
	            var idx = _step9.value;
	            if (idx >= params.length) continue;
	            var p = params[idx];
	            if (typeof p === "string") {
	              params[idx] = translate(p);
	            } else if (Array.isArray(p)) {
	              for (var j = 0; j < p.length; j++) {
	                if (typeof p[j] === "string") {
	                  p[j] = translate(p[j]);
	                }
	              }
	            }
	          }
	        } catch (err) {
	          _iterator9.e(err);
	        } finally {
	          _iterator9.f();
	        }
	      }
	    }
	  } catch (err) {
	    _iterator8.e(err);
	  } finally {
	    _iterator8.f();
	  }
	}
	// =============================================================================
	// 钩子函数
	// =============================================================================
	/**
	 * 创建用于 custom-hooks 的日志函数
	 */
	function createHookLogger() {
	  return function () {
	    for (var _len = arguments.length, args = new Array(_len), _key = 0; _key < _len; _key++) {
	      args[_key] = arguments[_key];
	    }
	    var message = args.map(function (arg) {
	      if (typeof arg === "string") return arg;
	      if (arg instanceof Error) return arg.message;
	      try {
	        return JSON.stringify(arg);
	      } catch (_unused) {
	        return String(arg);
	      }
	    }).join(" ");
	    log$1.info(message);
	  };
	}
	function applyHooks() {
	  var _state$config$__custo, _state$config;
	  if (state.hooked) return;
	  var customHooks = (_state$config$__custo = (_state$config = state.config) == null ? void 0 : _state$config.__customHooks__) != null ? _state$config$__custo : [];
	  var _applyCustomHooks = applyHooks$1({
	      translate: translate,
	      translateData: translateData,
	      hooks: customHooks,
	      logger: createHookLogger()
	    }),
	    stats = _applyCustomHooks.stats;
	  state.hooked = true;
	  log$1.info({
	    stats: stats
	  }, "Custom hooks applied");
	}
	// =============================================================================
	// 辅助函数
	// =============================================================================
	function clearTranslationState() {
	  state.cache.clear();
	  state.translatedMarkers.clear();
	  translatedObjects = new WeakSet();
	}
	// =============================================================================
	// 公共 API
	// =============================================================================
	var Translator = {
	  translate: translate,
	  init: init,
	  hook: applyHooks,
	  add: function add(key, value) {
	    state.dict.set(key, value);
	    var isMetaKey = key.startsWith("__") && key.endsWith("__");
	    if (!isMetaKey) {
	      rebuildSortedKeys();
	    }
	    clearTranslationState();
	    invalidateAhoCorasick();
	  },
	  addBatch: function addBatch(entries) {
	    var _iterator0 = _createForOfIteratorHelper(entries),
	      _step0;
	    try {
	      for (_iterator0.s(); !(_step0 = _iterator0.n()).done;) {
	        var _step0$value = _slicedToArray(_step0.value, 2),
	          key = _step0$value[0],
	          value = _step0$value[1];
	        state.dict.set(key, value);
	      }
	    } catch (err) {
	      _iterator0.e(err);
	    } finally {
	      _iterator0.f();
	    }
	    rebuildSortedKeys();
	    clearTranslationState();
	    invalidateAhoCorasick();
	  },
	  remove: function remove(key) {
	    state.dict.delete(key);
	    rebuildSortedKeys();
	    clearTranslationState();
	    invalidateAhoCorasick();
	  },
	  has: function has(key) {
	    return state.dict.has(key);
	  },
	  reload: function reload() {
	    var shouldRehook = state.hooked;
	    state.dict.clear();
	    state.missed.clear();
	    state.regexPatterns = [];
	    clearTranslationState();
	    invalidateAhoCorasick();
	    state.ready = false;
	    state.hooked = false;
	    init();
	    if (shouldRehook) {
	      applyHooks();
	    }
	  },
	  getMissed: function getMissed() {
	    return Array.from(state.missed.keys());
	  },
	  exportMissed: function exportMissed() {
	    var includeCount = arguments.length > 0 && arguments[0] !== undefined ? arguments[0] : false;
	    var entries = Array.from(state.missed.entries()).sort(function (a, b) {
	      return b[1] - a[1];
	    });
	    var obj = {};
	    var _iterator1 = _createForOfIteratorHelper(entries),
	      _step1;
	    try {
	      for (_iterator1.s(); !(_step1 = _iterator1.n()).done;) {
	        var _step1$value = _slicedToArray(_step1.value, 2),
	          key = _step1$value[0],
	          count = _step1$value[1];
	        obj[key] = includeCount ? count : "";
	      }
	    } catch (err) {
	      _iterator1.e(err);
	    } finally {
	      _iterator1.f();
	    }
	    return JSON.stringify(obj, null, 2);
	  },
	  clearMissed: function clearMissed() {
	    state.missed.clear();
	  },
	  stats: function stats() {
	    return {
	      translations: state.dict.size,
	      cached: state.cache.size,
	      missed: state.missed.size,
	      missedLimit: state.missedLimit,
	      regexPatterns: state.regexPatterns.length,
	      acBuilt: state.acAutomaton !== null,
	      acPatterns: state.acPatterns.length,
	      maxDepth: state.maxDepth,
	      ready: state.ready,
	      hooked: state.hooked,
	      translatedMarkers: state.translatedMarkers.size
	    };
	  },
	  isReady: function isReady() {
	    return state.ready;
	  },
	  /**
	   * 设置日志级别
	   */
	  setLogLevel: function setLogLevel(level) {
	    log$1.level = level;
	  },
	  debug: {
	    getConfig: function getConfig() {
	      return state.config;
	    },
	    getRegexPatterns: function getRegexPatterns() {
	      return _toConsumableArray(state.regexPatterns);
	    },
	    testTranslate: function testTranslate(text) {
	      var cached = state.cache.get(text);
	      var _extractControls2 = _extractControls3(text),
	        controls = _extractControls2.controls;
	      var result = translate(text);
	      return {
	        result: result,
	        cached: cached !== undefined,
	        matched: result !== text,
	        controls: controls
	      };
	    },
	    extractControls: function extractControls(text) {
	      return _extractControls3(text);
	    },
	    applyPlaceholders: function applyPlaceholders(translation, controls) {
	      return applyControlPlaceholders(translation, controls);
	    },
	    getDict: function getDict() {
	      return new Map(state.dict);
	    },
	    getLogLevel: function getLogLevel() {
	      return log$1.level;
	    }
	  }
	};
	// eslint-disable-next-line @typescript-eslint/no-implied-eval
	var _global = typeof window !== "undefined" ? window : typeof self !== "undefined" ? self : Function("return this")();
	_global["Translator"] = Translator;

	// main.ts — NST Translation Layer plugin entry.
	// Supports RPG Maker MV/MZ.
	//
	// This file is the IIFE entry that Rollup bundles into a single
	// NST_TranslationLayer.js file for dropping into js/plugins/.
	var log = pino({
	  level: "info"
	});
	// =============================================================================
	// Initialization
	// =============================================================================
	var _preloaded = loadAll();
	initFonts(_preloaded.fontConfig);
	var translatorInitialized = false;
	function initializeTranslator() {
	  if (translatorInitialized) return;
	  Translator.init(_preloaded);
	  Translator.hook();
	  translatorInitialized = true;
	  log.info("Translator initialized");
	}
	// Initialize immediately on plugin load.
	initializeTranslator();
	// RPG Maker MV/MZ boot hook — re-initialize after the engine is ready.
	if (typeof Scene_Boot !== "undefined" && Scene_Boot !== null) {
	  var origSceneBootStart = Scene_Boot.prototype.start;
	  Scene_Boot.prototype.start = function () {
	    initializeTranslator();
	    origSceneBootStart.call(this);
	  };
	  log.info("RPG Maker boot hook installed");
	}
	// =============================================================================
	// Map-change lazy load hook
	// =============================================================================
	if (typeof Game_Map !== "undefined" && Game_Map !== null) {
	  var origGameMapSetup = Game_Map.prototype.setup;
	  Game_Map.prototype.setup = function (mapId) {
	    origGameMapSetup.call(this, mapId);
	    loadMap(mapId);
	  };
	  log.info("Map lazy-load hook installed");
	}
	log.info("Plugin loaded");
	// =============================================================================
	// Public Debug API (F12 console)
	// =============================================================================
	window.NST = window.NST || {};
	window.NST.TL = {
	  /**
	   * Translate a string (same as the internal pipeline).
	   */
	  translate: Translator.translate,
	  /**
	   * Reload all translations and config from disk.
	   */
	  reload: function reload() {
	    var result = reloadAll();
	    Translator.init(result);
	    log.info("Translations reloaded. " + result.translations.size + " entries.");
	  },
	  /**
	   * Show statistics about loaded translations.
	   */
	  stats: function stats() {
	    var s = Translator.stats();
	    console.log("[NST] === Translation Stats ===");
	    console.log("  Source entries: " + s.translations);
	    console.log("  Cached: " + s.cached);
	    console.log("  Missed: " + s.missed);
	    console.log("  Regex patterns: " + s.regexPatterns);
	    console.log("  AC patterns: " + s.acPatterns);
	    console.log("  Ready: " + s.ready);
	    console.log("  Hooked: " + s.hooked);
	  },
	  /**
	   * List the top 100 missed (untranslated) source-locale strings.
	   */
	  missed: function missed() {
	    var missed = Translator.getMissed();
	    console.log("[NST] === Missed Translations (top 100) ===");
	    for (var i = 0; i < Math.min(missed.length, 100); i++) {
	      console.log("  [" + i + "] " + missed[i]);
	    }
	    console.log("  Total missed: " + missed.length);
	  },
	  /**
	   * Export missed translations as a JSON string (for feeding back to the tool).
	   */
	  exportMissed: function exportMissed() {
	    var includeCount = arguments.length > 0 && arguments[0] !== undefined ? arguments[0] : false;
	    console.log(Translator.exportMissed(includeCount));
	  }
	};

})();
//# sourceMappingURL=NST_TranslationLayer.js.map
