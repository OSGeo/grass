#!/usr/bin/env bash
# Author: Ivan Mincik, ivan.mincik@gmail.com

set -e

export CC="ccache $CC"
export CXX="ccache $CXX"
export MAKEFLAGS="-j $(nproc) --no-keep-going"

echo "MAKEFLAGS is '$MAKEFLAGS'"

./configure --host=x86_64-linux-gnu \
            --build=x86_64-linux-gnu \
            --enable-largefile \
            --enable-shared \
            --prefix=/usr/lib \
            --sharedstatedir=/var \
            --sysconfdir=/etc \
            --with-blas \
            --with-cairo \
            --with-cxx \
            --with-freetype \
            --with-freetype-includes=/usr/include/freetype2/ \
            --with-gdal \
            --with-geos \
            --with-lapack \
            --with-netcdf \
            --with-nls \
            --with-odbc \
            --with-pdal \
            --with-postgres \
            --with-postgres-includes=/usr/include/postgresql/ \
            --with-proj-share=/usr/share/proj \
            --with-readline \
            --with-sqlite \
            --with-zstd

make CFLAGS="$CFLAGS $GRASS_EXTRA_CFLAGS" CXXFLAGS="$CXXFLAGS $GRASS_EXTRA_CXXFLAGS"
