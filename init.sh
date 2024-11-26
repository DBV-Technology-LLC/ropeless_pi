#!/bin/bash

sudo apt install cmake
git submodule update --init opencpn-libs
sudo apt install devscripts equivs software-properties-common
mk-build-deps --root-cmd=sudo -ir build-deps/control