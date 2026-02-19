#!/bin/bash
# =============================================================================
# NST AI Features Installer (PaddleX)
# =============================================================================
# This script installs the AI-powered features for NST using PaddleX.
#
# Usage:
#   ./install-ai-features.sh          # Install CPU version (default)
#   ./install-ai-features.sh --gpu    # Install GPU version (requires CUDA 11.8)
#
# Note for AMD Users:
#   Currently, this script supports automatic installation for CPU and NVIDIA GPU (CUDA).
#   For AMD GPUs (ROCm), please install the specific paddlepaddle-rocm version manually
#   before running this script, or run this script in CPU mode first.
# =============================================================================

set -e

# Parse arguments
USE_GPU=false
if [[ "$1" == "--gpu" ]] || [[ "$1" == "-g" ]]; then
    USE_GPU=true
fi

echo "╔══════════════════════════════════════════════════════════════════╗"
echo "║           NST AI Features Installer (PaddleX)                    ║"
echo "╠══════════════════════════════════════════════════════════════════╣"
echo "║  This will install:                                              ║"
echo "║  • PaddleX         - OCR pipeline (PP-OCRv4)                    ║"
if [ "$USE_GPU" = true ]; then
echo "║  • PaddlePaddle-GPU - AI framework with CUDA 11.8 support       ║"
else
echo "║  • PaddlePaddle     - AI framework (CPU only)                    ║"
fi
echo "╚══════════════════════════════════════════════════════════════════╝"
echo ""

# Find NST root directory from script location
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
NST_ROOT="$(dirname "$(dirname "$(dirname "$SCRIPT_DIR")")")"

if [ ! -d "$NST_ROOT/usr/lib" ]; then
    NST_ROOT="$(dirname "$SCRIPT_DIR")"
    if [ ! -d "$NST_ROOT/usr/lib" ]; then
        echo "❌ Error: Cannot find NST installation directory."
        exit 1
    fi
fi

# Find bundled Python version
PY_SITE_PACKAGES=""
BUNDLED_PY_VER=""
for pydir in "$NST_ROOT/usr/lib"/python3.*; do
    if [ -d "$pydir/site-packages" ]; then
        PY_SITE_PACKAGES="$pydir/site-packages"
        BUNDLED_PY_VER=$(basename "$pydir" | sed 's/python//')
        break
    fi
done

if [ -z "$PY_SITE_PACKAGES" ]; then
    echo "❌ Error: Cannot find bundled Python in NST."
    exit 1
fi

echo "✓ NST bundled Python: $BUNDLED_PY_VER"
echo "✓ Install target: $PY_SITE_PACKAGES"

# Check if uv is installed
UV_CMD=""
if command -v uv &>/dev/null; then
    UV_CMD="uv"
elif [ -f "$HOME/.local/bin/uv" ]; then
    UV_CMD="$HOME/.local/bin/uv"
elif [ -f "$HOME/.cargo/bin/uv" ]; then
    UV_CMD="$HOME/.cargo/bin/uv"
fi

if [ -z "$UV_CMD" ]; then
    echo ""
    echo "📦 Installing uv (fast Python package manager)..."
    curl -LsSf https://astral.sh/uv/install.sh | sh
    
    # Add to path for this session
    export PATH="$HOME/.local/bin:$PATH"
    UV_CMD="$HOME/.local/bin/uv"
    
    if [ ! -f "$UV_CMD" ]; then
        UV_CMD="$HOME/.cargo/bin/uv"
    fi
    
    if [ ! -f "$UV_CMD" ]; then
        echo "❌ Error: Failed to install uv."
        exit 1
    fi
fi

echo "✓ Using uv: $UV_CMD"

# Create temp venv with correct Python version
TEMP_VENV="/tmp/nst-install-venv"
echo ""
echo "📦 Setting up Python $BUNDLED_PY_VER environment..."

# Clean up any previous venv
rm -rf "$TEMP_VENV"

# uv can download Python if needed
$UV_CMD venv --python "$BUNDLED_PY_VER" "$TEMP_VENV" 2>/dev/null || {
    echo "Installing Python $BUNDLED_PY_VER..."
    $UV_CMD python install "$BUNDLED_PY_VER"
    $UV_CMD venv --python "$BUNDLED_PY_VER" "$TEMP_VENV"
}

# Install pip in the venv (uv venv doesn't include pip by default)
echo "Installing pip in temporary environment..."
$UV_CMD pip install --python "$TEMP_VENV/bin/python" pip

echo "✓ Python $BUNDLED_PY_VER environment ready"

# Ask for confirmation
echo ""
if [ "$USE_GPU" = true ]; then
    echo "This will download PaddlePaddle-GPU (~1.5GB) and PaddleX."
else
    echo "This will download PaddlePaddle (~200MB) and PaddleX."
fi
echo "Estimated time: 5-15 minutes depending on internet speed."
echo ""
read -p "Install AI features now? [Y/n] " -n 1 -r
echo ""
if [[ $REPLY =~ ^[Nn]$ ]]; then
    echo "Installation cancelled."
    rm -rf "$TEMP_VENV"
    exit 0
fi

echo ""
echo "📦 Installing packages..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Install PaddlePaddle
echo ""
if [ "$USE_GPU" = true ]; then
    echo "[1/2] Installing PaddlePaddle-GPU (CUDA 11.8)..."
    "$TEMP_VENV/bin/pip" install \
        --target="$PY_SITE_PACKAGES" \
        --upgrade --no-user \
        paddlepaddle-gpu==3.0.0 \
        -i https://www.paddlepaddle.org.cn/packages/stable/cu118/
else
    echo "[1/2] Installing PaddlePaddle (CPU)..."
    "$TEMP_VENV/bin/pip" install \
        --target="$PY_SITE_PACKAGES" \
        --upgrade --no-user \
        paddlepaddle==3.0.0
fi

# Install PaddleX and dependencies
echo ""
echo "[2/2] Installing PaddleX and OCR dependencies..."
# Install paddlex[ocr] to get all OCR-related dependencies
"$TEMP_VENV/bin/pip" install \
    --target="$PY_SITE_PACKAGES" \
    --upgrade --no-user \
    "paddlex[ocr]" opencv-python-headless numpy Pillow

# Cleanup
rm -rf "$TEMP_VENV"

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "╔══════════════════════════════════════════════════════════════════╗"
echo "║  ✅ Installation Complete!                                       ║"
echo "╠══════════════════════════════════════════════════════════════════╣"
echo "║  Please restart NST to enable AI features.                       ║"
echo "║                                                                  ║"
echo "║  Note: First OCR run will download PP-OCRv4 models (~150MB).    ║"
echo "╚══════════════════════════════════════════════════════════════════╝"
echo ""
