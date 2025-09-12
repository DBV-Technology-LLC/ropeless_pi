#!/bin/bash

# Define the directory
DIR="build/"

# Check if the directory exists
if [ ! -d "$DIR" ]; then
    echo "Directory $DIR does not exist. Creating it now..."
    mkdir "$DIR"
    cd "$DIR"
    cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DOCPN_TARGET_TUPLE='ubuntu-wx32-x86_64;22.04;x86_64' ..
else
    echo "Directory $DIR already exists."
    cd "$DIR"
fi

make tarball
mkdir -p ~/.local/lib/opencpn/
cp libropeless_pi.so ~/.local/lib/opencpn/
echo "Debug-enabled plugin installed to ~/.local/lib/opencpn/"
file ~/.local/lib/opencpn/libropeless_pi.so