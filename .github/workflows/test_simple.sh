#!/usr/bin/env bash

# fail on non-zero return code from a subprocess
set -e

echo $PATH
# Test if grass executable is found
which grass
grass_exe=$(which grass)

which python3
python3 -c "import sys; print(sys.executable, sys.version)"

echo $grass_exe

# Test execution of binary command
grass --tmp-location EPSG:4326 --exec g.region res=0.1 -p
# Test if python modules without extension are found
grass --tmp-location EPSG:4326 --exec which t.create
# Test if python modules can be called
grass --tmp-location EPSG:4326 --exec t.create --help
