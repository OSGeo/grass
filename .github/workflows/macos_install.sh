#!/bin/sh

set -e

if [ -z "$1" ]; then
    echo "Usage: $0 PREFIX"
    exit 1
fi

CONDA_ARCH=$(uname -m)
INSTALL_PREFIX=$1

CONFIGURE_FLAGS="\
  --prefix=${INSTALL_PREFIX} \
  --with-blas=openblas \
  --with-bzlib \
  --with-bzlib-includes=${CONDA_PREFIX}/include \
  --with-bzlib-libs=${CONDA_PREFIX}/lib \
  --with-cairo \
  --with-cairo-includes=${CONDA_PREFIX}/include/cairo \
  --with-cairo-ldflags="-lcairo" \
  --with-cairo-libs=${CONDA_PREFIX}/lib \
  --with-cxx \
  --with-fftw-includes=${CONDA_PREFIX}/include \
  --with-fftw-libs=${CONDA_PREFIX}/lib \
  --with-freetype \
  --with-freetype-includes=${CONDA_PREFIX}/include/freetype2 \
  --with-freetype-libs=${CONDA_PREFIX}/lib \
  --with-gdal=${CONDA_PREFIX}/bin/gdal-config \
  --with-geos=${CONDA_PREFIX}/bin/geos-config \
  --with-includes=${CONDA_PREFIX}/include \
  --with-lapack=openblas \
  --with-libpng=${CONDA_PREFIX}/bin/libpng-config \
  --with-libs=${CONDA_PREFIX}/lib \
  --with-netcdf=${CONDA_PREFIX}/bin/nc-config \
  --with-netcdf=${CONDA_PREFIX}/bin/nc-config \
  --with-nls \
  --with-opengl=aqua \
  --with-openmp \
  --with-pdal \
  --with-postgres-includes=${CONDA_PREFIX}/include \
  --with-postgres-libs=${CONDA_PREFIX}/lib \
  --with-postgres=yes \
  --with-proj-includes=${CONDA_PREFIX}/include \
  --with-proj-libs=${CONDA_PREFIX}/lib \
  --with-proj-share=${CONDA_PREFIX}/share/proj \
  --with-readline \
  --with-readline-includes=${CONDA_PREFIX}/include/readline \
  --with-readline-libs=${CONDA_PREFIX}/lib
  --with-sqlite \
  --with-sqlite-includes=${CONDA_PREFIX}/include \
  --with-sqlite-libs=${CONDA_PREFIX}/lib \
  --with-tiff-includes=${CONDA_PREFIX}/include \
  --with-tiff-libs=${CONDA_PREFIX}/lib \
  --with-zstd \
  --with-zstd-includes=${CONDA_PREFIX}/include \
  --with-zstd-libs=${CONDA_PREFIX}/lib \
  --without-mysql \
  --without-x \
"

export CFLAGS="-O2 -pipe -ffp-contract=off -arch ${CONDA_ARCH} -DGL_SILENCE_DEPRECATION -Wall -Wextra -Wpedantic -Wvla"
export CXXFLAGS="-O2 -pipe -ffp-contract=off -stdlib=libc++ -arch ${CONDA_ARCH} -Wall -Wextra -Wpedantic"
export CPPFLAGS="-isystem${CONDA_PREFIX}/include"

./configure $CONFIGURE_FLAGS

EXEMPT=""
make -j$(sysctl -n hw.ncpu) CFLAGS="$CFLAGS -Werror $EXEMPT" \
  CXXFLAGS="$CXXFLAGS -Werror $EXEMPT"

make install
