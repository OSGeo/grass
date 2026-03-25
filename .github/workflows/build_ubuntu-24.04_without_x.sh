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

# Adding -Werror to make's CFLAGS is a workaround for configuring with
# an old version of configure, which issues compiler warnings and
# errors out. This may be removed with upgraded configure.in file.
makecmd="make"
if [[ "$#" -eq 2 ]]; then
    makecmd="make CFLAGS='$CFLAGS $2' CXXFLAGS='$CXXFLAGS $2'"
fi

# non-existent variables as an errors
set -u

export INSTALL_PREFIX=$1

./configure \
    --enable-largefile \
    --prefix="$INSTALL_PREFIX/" \
    --without-blas \
    --without-lapack \
    --with-bzlib \
    --with-cxx \
    --with-fftw \
    --with-freetype \
    --with-freetype-includes="/usr/include/freetype2/" \
    --with-geos \
    --with-netcdf \
    --with-pdal \
    --with-proj-share=/usr/share/proj \
    --with-readline \
    --with-sqlite \
    --with-tiff \
    --with-zstd \
    --without-openmp \
    --without-pthread

eval $makecmd
make install
