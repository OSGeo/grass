#!/bin/bash

### inspired by https://github.com/OSGeo/gdal/blob/master/gdal/scripts/vagrant/gdal.sh

# abort install if any errors occur and enable tracing
set -o errexit
set -o xtrace

NUMTHREADS=2
if [[ -f /sys/devices/system/cpu/online ]]; then
	# Calculates 1.5 times physical threads
	NUMTHREADS=$(( ( $(cut -f 2 -d '-' /sys/devices/system/cpu/online) + 1 ) * 15 / 10  ))
fi
#NUMTHREADS=1 # disable MP
export NUMTHREADS

cd /vagrant

if [ ! -f "include/Make/Platform.make" ] ; then
    ./configure \
        --bindir=/usr/bin \
        --enable-largefile \
        --enable-shared \
        --prefix=/usr/lib \
        --srcdir=/vagrant \
        --with-blas \
        --with-bzlib \
        --with-cairo \
        --with-cxx \
        --with-freetype \
        --with-freetype-includes=/usr/include/freetype2 \
        --with-gdal \
        --with-geos \
        --with-lapack \
        --with-mysql \
        --with-mysql-includes=`mysql_config --include | sed -e 's/-I//'` \
        --with-netcdf \
        --with-nls \
        --with-odbc \
        --with-postgres \
        --with-postgres-includes=`pg_config --includedir` \
        --with-proj-share=/usr/share/proj \
        --with-pthread \
        --with-readline \
        --with-sqlite \
        --with-x \
        --without-pdal
fi

make -j $NUMTHREADS

sudo make install
sudo ldconfig

exit 0
