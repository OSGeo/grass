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
if [[ "$#" -ge 2 ]]; then
    ARGS=("$@")
    makecmd="make CFLAGS='$CFLAGS ${ARGS[@]:1}' CXXFLAGS='$CXXFLAGS ${ARGS[@]:1}'"
fi

# non-existent variables as an errors
set -u

export INSTALL_PREFIX=$1

./configure \
    --enable-largefile \
    --prefix="$INSTALL_PREFIX/" \
    --with-blas \
    --with-bzlib \
    --with-cxx \
    --with-fftw \
    --with-freetype \
    --with-freetype-includes="/usr/include/freetype2/" \
    --with-geos \
    --with-lapack \
    --with-libsvm \
    --with-netcdf \
    --with-openmp \
    --with-pdal \
    --with-proj-share=/usr/share/proj \
    --with-pthread \
    --with-readline \
    --with-sqlite \
    --with-tiff \
    --with-zstd

eval $makecmd
make install
