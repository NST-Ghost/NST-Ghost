#!/bin/sh
set -e

# Set executable permissions
chmod +x /usr/bin/NST 2>/dev/null || true
chmod +x /usr/bin/libBGACore.so* 2>/dev/null || true
chmod +x /usr/bin/libQtLingo.so* 2>/dev/null || true

# Update desktop database
if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database /usr/share/applications 2>/dev/null || true
fi

# Update icon cache
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -f -t /usr/share/icons/hicolor 2>/dev/null || true
fi

exit 0
