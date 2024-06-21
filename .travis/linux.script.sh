#!/usr/bin/env bash
# Author: Ivan Mincik, ivan.mincik@gmail.com

set -e

export CC="ccache $CC"
export CXX="ccache $CXX"
export MAKEFLAGS="-j $(nproc) --no-keep-going"

echo "MAKEFLAGS is '$MAKEFLAGS'"

./configure --host=x86_64-linux-gnu \
            --build=x86_64-linux-gnu \
            --prefix=/usr/lib \
            --sysconfdir=/etc \
            --sharedstatedir=/var \
            --enable-shared \
            --with-postgres \
            --with-cxx \
            --with-gdal \
            --with-freetype \
            --with-readline \
            --with-nls \
            --with-odbc \
            --with-geos \
            --with-lapack \
            --with-netcdf \
            --with-blas \
            --with-sqlite \
            --with-zstd \
            --enable-largefile \
            --with-freetype-includes=/usr/include/freetype2/ \
            --with-postgres-includes=/usr/include/postgresql/ \
            --with-proj-share=/usr/share/proj \
            --with-cairo \
            --with-pdal

make CFLAGS="$CFLAGS $GRASS_EXTRA_CFLAGS" CXXFLAGS="$CXXFLAGS $GRASS_EXTRA_CXXFLAGS"
