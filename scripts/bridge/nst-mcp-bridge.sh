#!/usr/bin/env bash
# ──────────────────────────────────────────────────────────────────
# nst-mcp-bridge.sh — stdio-to-socket bridge for NST MCP Server
#
# This script bridges stdin/stdout ↔ Unix Domain Socket so that
# MCP clients that only support stdio transport (e.g., Claude Desktop,
# Cline, Cursor) can connect to the NST TUI's embedded MCP server.
#
# Usage:
#   1. Start NST TUI:  ./NST -t
#   2. In your MCP client config (e.g., claude_desktop_config.json):
#      {
#        "mcpServers": {
#          "nst": {
#            "command": "/path/to/nst-mcp-bridge.sh"
#          }
#        }
#      }
#
# The bridge will auto-detect the socket path from XDG_RUNTIME_DIR.
# Override with: NST_MCP_SOCKET=/path/to/socket nst-mcp-bridge.sh
# ──────────────────────────────────────────────────────────────────

set -euo pipefail

# Determine socket path
if [ -n "${NST_MCP_SOCKET:-}" ]; then
    SOCKET_PATH="$NST_MCP_SOCKET"
elif [ -n "${XDG_RUNTIME_DIR:-}" ]; then
    SOCKET_PATH="${XDG_RUNTIME_DIR}/nst-mcp.sock"
else
    SOCKET_PATH="/tmp/nst-mcp.sock"
fi

# Verify socket exists
if [ ! -S "$SOCKET_PATH" ]; then
    echo "Error: NST MCP socket not found at: $SOCKET_PATH" >&2
    echo "Make sure NST TUI is running (./NST -t)" >&2
    exit 1
fi

# Check for socat
if command -v socat &>/dev/null; then
    exec socat STDIO "UNIX-CONNECT:${SOCKET_PATH}"
fi

# Fallback: nc (netcat) — some versions support Unix sockets
if command -v nc &>/dev/null; then
    # Try GNU netcat first
    if nc --help 2>&1 | grep -q '\-U'; then
        exec nc -U "$SOCKET_PATH"
    fi
fi

# Fallback: ncat (from nmap)
if command -v ncat &>/dev/null; then
    exec ncat -U "$SOCKET_PATH"
fi

echo "Error: No suitable tool found. Install one of: socat, nc (netcat), ncat" >&2
echo "  Ubuntu/Debian: sudo apt install socat" >&2
echo "  Fedora/RHEL:   sudo dnf install socat" >&2
echo "  Arch:          sudo pacman -S socat" >&2
echo "  macOS:         brew install socat" >&2
exit 1
