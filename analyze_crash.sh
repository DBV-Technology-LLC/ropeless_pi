#!/bin/bash

# Crash analysis script
# Usage: ./analyze_crash.sh [core_file]

echo "=== Crash Analysis Tool ==="

if [ "$1" ]; then
    CORE_FILE="$1"
elif [ -f "core" ]; then
    CORE_FILE="core"
else
    echo "Looking for core files..."
    CORE_FILE=$(find /tmp /var/tmp ~ -name "core*" -type f 2>/dev/null | head -1)
    if [ -z "$CORE_FILE" ]; then
        echo "No core file found. Run with: ./analyze_crash.sh path/to/core"
        exit 1
    fi
fi

echo "Analyzing core file: $CORE_FILE"
echo ""

# Make sure we have the debug plugin
if [ ! -f "build/libropeless_pi.so" ]; then
    echo "Debug plugin not found. Run the build first."
    exit 1
fi

echo "Starting analysis with gdb..."
echo "Useful gdb commands:"
echo "  bt full    - full backtrace with variables"
echo "  info threads - show all threads"
echo "  thread N   - switch to thread N"
echo "  frame N    - switch to frame N in backtrace"
echo "  p variable - print variable value"
echo "  quit       - exit"
echo ""

gdb --batch \
    --ex "set debug-file-directory build" \
    --ex "file /usr/bin/opencpn" \
    --ex "core-file $CORE_FILE" \
    --ex "bt full" \
    --ex "info registers" \
    --ex "info threads" \
    --ex "quit"