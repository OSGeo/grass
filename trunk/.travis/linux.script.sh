#!/usr/bin/env sh
# Author: Ivan Mincik, ivan.mincik@gmail.com

set -e

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
            --enable-largefile \
            --with-freetype-includes=/usr/include/freetype2/ \
            --with-postgres-includes=/usr/include/postgresql/ \
            --with-proj-share=/usr/share/proj \
            --with-wxwidgets=/usr/bin/wx-config \
            --with-python \
            --with-cairo

make -j2
