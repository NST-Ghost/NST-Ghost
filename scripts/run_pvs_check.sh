#!/bin/bash
# =============================================================================
# NST PVS-Studio Post-Build Check & Summary Script
# =============================================================================

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
NST_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${1:-$NST_ROOT/build}"

cd "$NST_ROOT"

if ! command -v pvs-studio-analyzer &>/dev/null; then
    echo "[PVS-Studio] pvs-studio-analyzer not installed. Skipping analysis."
    exit 0
fi

COMPILE_COMMANDS="$BUILD_DIR/compile_commands.json"
if [ ! -f "$COMPILE_COMMANDS" ]; then
    echo "[PVS-Studio] $COMPILE_COMMANDS not found. Build with -DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
    exit 0
fi

echo ""
echo "======================================================================"
echo "[PVS-Studio] Running Automatic Static Analysis..."
echo "======================================================================"

LOG_FILE="$BUILD_DIR/pvs-studio.log"
TASKS_FILE="$BUILD_DIR/pvs-studio.tasks"

# Fast Incremental Analysis: only analyze modified source files on build
pvs-studio-analyzer analyze -i -f "$COMPILE_COMMANDS" -e "$BUILD_DIR/_deps" -o "$LOG_FILE" -j$(nproc) >/dev/null 2>&1 || true

if command -v plog-converter &>/dev/null && [ -f "$LOG_FILE" ]; then
    # Fast Post-Build Check: Filter only High Severity (GA:1) critical issues
    plog-converter -a GA:1 -t errorfile -o "$TASKS_FILE" "$LOG_FILE" >/dev/null 2>&1 || true
    echo ""
    echo "[PVS-Studio] Quick Post-Build Report ($TASKS_FILE):"
    echo "======================================================================"
    if [ -s "$TASKS_FILE" ]; then
        cat "$TASKS_FILE"
    else
        echo "[OK] No critical High-Severity issues detected in modified files."
    fi
    echo "======================================================================"
fi
