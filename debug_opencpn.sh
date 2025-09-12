#!/bin/bash

# Debug script for OpenCPN with ropeless plugin
# Usage: ./debug_opencpn.sh

echo "=== OpenCPN Debug Mode ==="
echo "This will run OpenCPN with the debug-enabled ropeless plugin"
echo "When it crashes, gdb will show the stack trace"
echo ""

# Check if debug plugin is installed
echo "Checking for debug plugin..."
if [ -f ~/.local/lib/opencpn/libropeless_pi.so ]; then
    echo "Using installed debug plugin:"
    file ~/.local/lib/opencpn/libropeless_pi.so
else
    echo "Installing debug plugin from build..."
    mkdir -p ~/.local/lib/opencpn
    cp build/libropeless_pi.so ~/.local/lib/opencpn/
fi

# Set up core dumps
echo "Enabling core dumps..."
ulimit -c unlimited

# Set debugging environment
export MALLOC_CHECK_=2
export G_DEBUG=gc-friendly
export G_SLICE=always-malloc

# Run with gdb
echo "Starting OpenCPN with gdb..."
echo "Commands you can use in gdb:"
echo "  'run' - start OpenCPN"
echo "  'bt' or 'backtrace' - show stack trace when it crashes"
echo "  'bt full' - detailed stack trace with variable values"
echo "  'info registers' - show CPU registers"
echo "  'quit' - exit gdb"
echo ""

gdb --args opencpn