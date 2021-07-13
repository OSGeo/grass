#!/usr/bin/env bash

# The make step requires something like:
# export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$PREFIX/lib"
# further steps additionally require:
# export PATH="$PATH:$PREFIX/bin"

# fail on non-zero return code from a subprocess
set -e

# print commands
set -x

if [ -z "$1" ]; then
    echo "Usage: $0 PREFIX"
    exit 1
fi

# non-existent variables as an errors
set -u

export INSTALL_PREFIX=$1

./configure \
    --prefix="$INSTALL_PREFIX/" \
    --without-freetype \
    --with-sqlite \
    --with-cairo --with-cairo-ldflags=-lfontconfig \
    --with-freetype --with-freetype-includes=/usr/include/freetype2 \
    --with-proj --with-proj-share=/usr/share/proj \
    --with-gdal=/usr/bin/gdal-config \
    --without-zstd \
    --without-tiff \
    --without-ffmpeg \
    --without-mysql \
    --without-postgres \
    --without-odbc \
    --without-fftw

make
make install
