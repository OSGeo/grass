#!/bin/sh
#
# Usage: build_osgeo4w.sh [path]
#
# By default, the script will look for the source code in the current directory
# and create bin.x86_64-w64-mingw32\grass$ver.bat (run this batch file to start
# GRASS GIS) and dist.x86_64-w64-mingw32\etc\env.bat.
#
# path	specify a path to the source code
#

# stop on errors
set -e

test -d "$1" && cd "$1"

export OSGEO4W_ROOT_MSYS=/c/OSGeo4W
export SRC=$(pwd)
export UNITTEST=1

# Build according to OSGeo4W recipe
${SRC}/mswindows/osgeo4w/build_osgeo4W.sh
