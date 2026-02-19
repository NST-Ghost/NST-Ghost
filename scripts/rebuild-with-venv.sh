#!/bin/bash
# Reconfigure CMake to use the venv Python 3.10 and venv pybind11
# Usage: ./scripts/rebuild-with-venv.sh

set -e

# Path to the paddle-venv Python executable
VENV_PYTHON="/home/jop/paddle-venv/bin/python3"
VENV_PIP="/home/jop/paddle-venv/bin/pip"

echo "Using Python: $VENV_PYTHON"

if [ ! -f "$VENV_PYTHON" ]; then
    echo "Python executable not found at $VENV_PYTHON"
    echo "Please ensure the venv is created at /home/jop/paddle-venv"
    exit 1
fi

# Ensure pybind11 is installed in venv to avoid system pybind11 mismatch
echo "Ensuring pybind11 is installed in venv..."
"$VENV_PIP" install pybind11

# Get pybind11 cmake directory from venv
PYBIND11_CMAKE_DIR=$("$VENV_PYTHON" -m pybind11 --cmakedir)
echo "Using pybind11 from: $PYBIND11_CMAKE_DIR"

echo "Cleaning CMake cache..."
rm -f build/CMakeCache.txt

echo "Reconfiguring CMake..."
cmake -S . -B build \
    -DPython3_EXECUTABLE="$VENV_PYTHON" \
    -Dpybind11_DIR="$PYBIND11_CMAKE_DIR" \
    -DCMAKE_BUILD_TYPE=Release

echo "Building..."
cmake --build build -j$(nproc)

echo "Done! Run ./build/NST to start the app."
