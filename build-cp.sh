#!/bin/bash

# Define the directory
DIR="build/"

# Check if the directory exists
if [ ! -d "$DIR" ]; then
    echo "Directory $DIR does not exist. Creating it now..."
    mkdir "$DIR"
    cd "$DIR"
    cmake -DOCPN_TARGET_TUPLE='ubuntu-wx32;22.04;x86_64' ..
else
    echo "Directory $DIR already exists."
    cd "$DIR"
fi

make tarball
cp libropeless_pi.so ~/.local/lib/opencpn/