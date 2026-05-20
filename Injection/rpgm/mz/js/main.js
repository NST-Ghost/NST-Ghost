//=============================================================================
// main.js — NST Patched (RPG Maker MZ)
//=============================================================================

const scriptUrls = [
    "js/rmmz_core.js",
    "js/rmmz_managers.js",
    "js/rmmz_objects.js",
    "js/rmmz_scenes.js",
    "js/rmmz_sprites.js",
    "js/rmmz_windows.js",
    "js/plugins.js"
];

const effekseerWasmUrl = "js/libs/effekseer.wasm";

class Main {
    constructor() {
        this.xhrSucceeded = false;
        this.loadCount = 0;
        this.error = null;
    }

    run() {
        this.showLoadingSpinner();
        this.testXhr();
        this.loadMainScripts();
    }

    showLoadingSpinner() {
        const loadingSpinner = document.createElement("div");
        loadingSpinner.id = "loadingSpinner";
        const loadingImage = document.createElement("div");
        loadingImage.id = "loadingImage";
        loadingSpinner.appendChild(loadingImage);
        document.body.appendChild(loadingSpinner);
    }

    eraseLoadingSpinner() {
        const loadingSpinner = document.getElementById("loadingSpinner");
        if (loadingSpinner) {
            document.body.removeChild(loadingSpinner);
        }
    }

    testXhr() {
        const xhr = new XMLHttpRequest();
        xhr.open("GET", document.currentScript.src);
        xhr.onload = () => (this.xhrSucceeded = true);
        xhr.send();
    }

    hookNWjsClose() {
        if (typeof nw === "object") {
            nw.Window.get().on("close", () => nw.App.quit());
        }
    }

    loadMainScripts() {
        for (const url of scriptUrls) {
            const script = document.createElement("script");
            script.type = "text/javascript";
            script.src = url;
            script.async = false;
            script.defer = true;
            script.onload = this.onScriptLoad.bind(this);
            script.onerror = this.onScriptError.bind(this);
            script._url = url;
            document.body.appendChild(script);
        }
    }

    onScriptLoad() {
        if (++this.loadCount === scriptUrls.length) {
            // NST: Auto-register translation plugin
            if (typeof $plugins !== 'undefined') {
                var _nst = false;
                for (var i = 0; i < $plugins.length; i++) {
                    if ($plugins[i].name === 'NST_TranslationLayer') { _nst = true; break; }
                }
                if (!_nst) {
                    $plugins.push({"name":"NST_TranslationLayer","status":true,"description":"NST Translation Layer","parameters":{}});
                }
            }

            PluginManager.setup($plugins);
            this.loadEffekseer();
        }
    }

    onScriptError(e) {
        this.printError("Failed to load", e.target._url);
    }

    printError(name, message) {
        this.eraseLoadingSpinner();
        this.error = { name, message };
        const body = document.body;
        body.style.color = "white";
        body.style.textAlign = "center";
        body.style.fontSize = "2rem";
        body.style.padding = "2rem";
        body.innerHTML =
            '<font color="yellow"><b>' + name + "</b></font><br>" + message;
    }

    makeErrorHtml(name, message) {
        return (
            '<font color="yellow"><b>' + name + "</b></font><br>" + message
        );
    }

    loadEffekseer() {
        if (typeof effekseer !== "undefined") {
            effekseer.initRuntime(effekseerWasmUrl, () => {
                this.onEffekseerLoad();
            }, () => {
                this.onEffekseerError();
            });
        } else {
            this.onEffekseerLoad();
        }
    }

    onEffekseerLoad() {
        this.eraseLoadingSpinner();
        this.hookNWjsClose();
        window.onload = this.onWindowLoad.bind(this);
    }

    onEffekseerError() {
        this.printError("Failed to load", effekseerWasmUrl);
    }

    onWindowLoad() {
        if (this.error) return;
        if (Utils.isOptionValid("btest")) {
            SceneManager.run(Scene_Battle);
        } else {
            SceneManager.run(Scene_Boot);
        }
    }
}

const main = new Main();
main.run();
