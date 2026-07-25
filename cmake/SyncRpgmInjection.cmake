# cmake/SyncRpgmInjection.cmake
#
# Conditionally copies the Injection/rpgm/ directory into the build tree.
# If the generated NST_TranslationLayer.js plugin file is missing inside the
# source Injection/rpgm/ folder, the copy still proceeds (so the README and
# main.js boot shims are synced), but a warning is emitted telling the user
# how to build the plugin via the TypeScript toolchain.
#
# Variables expected:
#   SRC         — source Injection/rpgm directory
#   DST         — destination Injection/rpgm directory in the build tree
#   PLUGIN_FILE — the generated JS plugin filename (NST_TranslationLayer.js)

if(NOT EXISTS "${SRC}/${PLUGIN_FILE}")
    message(WARNING
        "NST Translation Layer plugin not found at ${SRC}/${PLUGIN_FILE}.\n"
        "  The RPGM injection layer will NOT be functional until it is built.\n"
        "  Run: scripts/build-translator.sh   (or scripts/build-translator.bat on Windows)\n"
        "  Requires Node.js >= 18. See Injection/rpgm_ts/README.md for details."
    )
endif()

# Always copy the directory; missing plugin is non-fatal.
file(COPY "${SRC}/" DESTINATION "${DST}")
