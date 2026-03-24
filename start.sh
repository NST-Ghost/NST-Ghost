#!/bin/bash

# Get the directory of the script
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# Set PYTHONPATH to include the pylib directory
export PYTHONPATH="$SCRIPT_DIR:$PYTHONPATH"

# Path to the executable
EXECUTABLE="./build/NST"

# Check if the executable exists
if [ ! -f "$EXECUTABLE" ]; then
    echo "Error: Executable not found at $EXECUTABLE"
    echo "Please build the project first using CMake."
    exit 1
fi

# Run the executable with any arguments passed to this script
echo "Starting NST..."
"$EXECUTABLE" "$@"
