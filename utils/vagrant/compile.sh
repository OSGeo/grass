#!/bin/sh

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
        --srcdir=/vagrant \
        --prefix=/usr/lib \
        --enable-socket \
        --enable-shared \
        --with-postgres \
        --with-mysql \
        --with-cxx \
        --with-x \
        --with-gdal \
        --with-geos \
        --with-freetype \
        --with-motif \
        --with-readline \
        --with-nls \
        --with-odbc \
        --with-netcdf \
        --with-blas \
        --with-lapack \
        --with-sqlite \
        --enable-largefile \
        --with-freetype-includes=/usr/include/freetype2 \
        --with-postgres-includes=`pg_config --includedir` \
        --with-mysql-includes=`mysql_config --include | sed -e 's/-I//'` \
        --with-proj-share=/usr/share/proj \
        --with-wxwidgets=/usr/bin/wx-config \
        --with-python \
        --with-cairo \
        --with-liblas
fi

make -j $NUMTHREADS

sudo make install
sudo ldconfig

exit 0
