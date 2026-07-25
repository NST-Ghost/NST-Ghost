#!/bin/bash
# =============================================================================
# NST Code Quality & Static Analysis Suite
# =============================================================================
# Automated toolsuite running:
#  1. Code Formatting (clang-format)
#  2. Static Analysis & Linting (clang-tidy)
#  3. Cyclomatic Complexity Inspection (lizard)
#  4. Architecture & Dependency Analysis (CppDepend)
#  5. Automated Unit Testing (gtest)
# =============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
NST_ROOT="$(dirname "$SCRIPT_DIR")"

cd "$NST_ROOT"

echo "======================================================================"
echo "[INFO] Running NST Code Quality Suite..."
echo "======================================================================"

# 1. Clang-Format Verification
if command -v clang-format &>/dev/null; then
    echo "[QUALITY 1/5] Checking Code Formatting (clang-format)..."
    find src QtLingo BGA -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) \
        -exec clang-format --dry-run --Werror {} + && echo "[OK] Formatting passed." || echo "[WARN] Code formatting issues detected."
else
    echo "[SKIP 1/5] clang-format not installed."
fi

# 2. Lizard Complexity Inspection (Threshold: <= 10)
if command -v lizard &>/dev/null; then
    echo ""
    echo "[QUALITY 2/5] Checking Cyclomatic Complexity (lizard threshold: 10)..."
    lizard -C 10 -w src QtLingo BGA || echo "[WARN] High complexity functions detected."
elif command -v python3 &>/dev/null && python3 -m lizard --version &>/dev/null; then
    echo ""
    echo "[QUALITY 2/5] Checking Cyclomatic Complexity (lizard threshold: 10)..."
    python3 -m lizard -C 10 -w src QtLingo BGA || echo "[WARN] High complexity functions detected."
else
    echo "[SKIP 2/5] lizard tool not installed (install via: pip install lizard)."
fi

# 3. Clang-Tidy Static Analysis
if command -v clang-tidy &>/dev/null; then
    echo ""
    echo "[QUALITY 3/5] Running Static Analysis (clang-tidy)..."
    if [ -f "build/compile_commands.json" ]; then
        run-clang-tidy -p build "src/.*" || echo "[WARN] clang-tidy warnings found."
    else
        echo "[INFO] compile_commands.json missing. Run cmake with -DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
    fi
else
    echo "[SKIP 3/5] clang-tidy not installed."
fi

# 4. CppDepend Architecture & Dependency Analyzer
CPPDEPEND_BIN="/home/jop/Downloads/CppDepend_linux_x64_2026.1/CppDependConsole.sh"
if [ -f "$CPPDEPEND_BIN" ]; then
    echo ""
    echo "[QUALITY 4/5] CppDepend detected at: $CPPDEPEND_BIN"
    if [ -f "NST.cdproj" ]; then
        echo "[INFO] Running CppDepend architecture analysis..."
        "$CPPDEPEND_BIN" "$NST_ROOT/NST.cdproj" /Silent || echo "[WARN] CppDepend analysis completed with notes."
    else
        echo "[INFO] CppDepend is available. Create NST.cdproj or launch VisualCppDepend.sh to inspect Architecture Graphs & Metrics."
    fi
else
    echo "[SKIP 4/5] CppDepend path not found."
fi

# 5. GoogleTest Suite Execution
echo ""
echo "[QUALITY 5/5] Executing Unit Test Suite (GoogleTest)..."
if [ -d "build" ]; then
    ctest --test-dir build --output-on-failure || echo "[FAIL] Unit tests failed."
else
    echo "[SKIP 5/5] Build directory missing. Build project first."
fi

echo ""
echo "======================================================================"
echo "[OK] Quality Suite Execution Complete."
echo "======================================================================"
