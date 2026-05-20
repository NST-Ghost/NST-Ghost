// NST: Auto-register translation plugin before game boot
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

window.onload = function() {
    SceneManager.run(Scene_Boot);
};
