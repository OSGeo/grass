#!/usr/bin/env bash
# Author: Ivan Mincik, ivan.mincik@gmail.com

set -e

sudo apt-get update -y
    sudo apt-get install -y wget git gawk findutils
    xargs -a <(awk '! /^ *(#|$)/' ".github/workflows/apt.txt") -r -- \
        sudo apt-get install -y --no-install-recommends --no-install-suggests
