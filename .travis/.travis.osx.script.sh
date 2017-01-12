#!/usr/bin/env sh
# Author: Rainer M. Krug, Rainer@krugs.de

## some tests for the recipe 
# brew audit -v $RECIPE

## install GRASS trunk
travis_wait brew install --HEAD \
             --enable-shared \
             --with-postgresql \
             --with-cxx \
             --with-gdal-1 \
             --with-freetype \
             --with-readline \
             --with-nls \
             --with-odbc \
             --with-geos \
             --with-lapack \
             --with-netcdf \
             --with-blas \
             --with-sqlite \
             --with-freetype \
             --with-python \
             grass-trunk

## Not implementyed yet:
             # --enable-largefile
             # --with-ffmpeg \
             # --with-mysql \
             # --with-netcdf \
             # --with-openblas \
             # --with-liblas \

brew test -v grass-trunk
# brew info grass-trunk

## run tests
## grass73 ./nc_basic_spm_grass7/PERMANENT --exec python -m grass.gunittest.main --location './nc_basic_spm_grass7' --location-type nc

## uninstall grass-trunk
# brew uninstall grass-trunk
