#!/usr/bin/env bash

set -e
set -x

$GRASS_PYTHON g.extension.py --doctest
