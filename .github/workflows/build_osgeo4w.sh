#!/bin/sh

# stop on errors
set -e

test -d "$1" && cd "$1"

# start
echo "Started compilation: `date`"
echo

# compile

osgeo4w_path=/c/OSGeo4W64
arch=x86_64-w64-mingw32
grass_src=`pwd`

export PATH=/mingw64/bin:/c/OSGeo4W64/bin:/usr/bin
export PROJ_LIB=/$osgeo4w_path/share/proj

OSGEO4W_ROOT_MSYS=$osgeo4w_path \
./configure \
--host=$arch \
--with-includes=$osgeo4w_path/include \
--with-libs="$osgeo4w_path/lib $osgeo4w_path/bin" \
--with-nls \
--with-freetype-includes=$osgeo4w_path/include/freetype2 \
--with-bzlib \
--with-geos=$grass_src/mswindows/osgeo4w/geos-config \
--with-netcdf=$osgeo4w_path/bin/nc-config \
--with-gdal=$grass_src/mswindows/osgeo4w/gdal-config \
--with-liblas=$grass_src/mswindows/osgeo4w/liblas-config \
--with-opengl=windows

make

echo
echo "Completed compilation: `date`"

# install

grass_bin=bin.$arch
version=`sed -n '/^INST_DIR[ \t]*=/{s/^.*grass//; p}' include/Make/Platform.make`

(
sed -e 's/^\(set GISBASE=\).*/\1%OSGEO4W_ROOT%\\opt\\grass/' \
    mswindows/osgeo4w/env.bat.tmpl
cat<<EOT

set PATH=%OSGEO4W_ROOT%\\apps\\msys\\bin;%PATH%

if not exist %GISBASE%\etc\fontcap (
	pushd .
	%~d0
	cd %GISBASE%\lib
	set GISRC=dummy
	%GISBASE%\bin\g.mkfontcap.exe
	popd
)
EOT
) > $grass_bin/env.bat

(
sed -e 's/^\(call "%~dp0\)\(.*\)$/\1\\..\\..\\bin\2/' \
    -e 's/^\(call "%OSGEO4W_ROOT%\\\).*\(\\etc\\env\.bat"\)$/\1opt\\grass\2/' \
    -e 's/@POSTFIX@/'$version'/g' \
    mswindows/osgeo4w/grass.bat.tmpl
) > $grass_bin/grass$version.bat
