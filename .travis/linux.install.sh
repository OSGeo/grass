#!/usr/bin/env bash
# Author: Ivan Mincik, ivan.mincik@gmail.com

set -e
set -x
dpkg -l
dpkg -l --selected-only
sudo apt-get update -y
    sudo apt-get install -y wget git gawk findutils pkg-config libpdal-plugins
    xargs -a <(awk '! /^ *(#|$)/' ".github/workflows/apt.txt") -r -- \
        sudo apt-get install -y --no-install-recommends --no-install-suggests

# https://github.com/qgis/QGIS/issues/56285
dpkg -l | grep pdal
apt rdepends libpdal-util13 || echo "not found"
apt depends libpdal-base13 || echo "not found"
apt depends libpdal16 || echo "not found"
apt depends libpdal-dev || echo "not found"
apt policy libpdal-dev || echo "not found"
apt policy pdal || echo "not found"
dpkg -l
dpkg -l --selected-only
