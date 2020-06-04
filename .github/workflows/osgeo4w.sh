#!/bin/sh
#
# Standalone script for building a portable package of GRASS GIS for OSGeo4W
#
# Written by Huidae Cho
#

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

make clean default

# package

opt_path=$osgeo4w_path/opt
grass_path=$opt_path/grass
version=`sed -n '/^INST_DIR[ \t]*=/{s/^.*grass//; p}' include/Make/Platform.make`
date=`date +%Y%m%d`
grass_zip=$grass_src/grass$version-$arch-osgeo4w64-$date.zip

test -d $grass_path && rm -rf $grass_path
test -d $opt_path || mkdir -p $opt_path
cp -a dist.$arch $grass_path
rm -f $grass_path/grass$version.tmp $grass_path/etc/fontcap
cp -a bin.$arch/grass$version.py $grass_path/etc
cp -a `ldd dist.$arch/lib/*.dll | awk '/mingw64/{print $3}' |
	sort -u | grep -v 'lib\(crypto\|ssl\)'` $grass_path/lib

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
) > $grass_path/etc/env.bat
unix2dos $grass_path/etc/env.bat

(
sed -e 's/^\(call "%~dp0\)\(.*\)$/\1\\..\\..\\bin\2/' \
    -e 's/^\(call "%OSGEO4W_ROOT%\\\).*\(\\etc\\env\.bat"\)$/\1opt\\grass\2/' \
    -e 's/@POSTFIX@/'$version'/g' \
    mswindows/osgeo4w/grass.bat.tmpl
) > $grass_path/grass$version.bat
unix2dos $grass_path/grass$version.bat

cd $osgeo4w_path/..
osgeo4w_basename=`basename $osgeo4w_path`
zip -r $grass_zip $osgeo4w_basename -x "$osgeo4w_basename/var/*" "*/__pycache__/*"

echo
echo "Completed compilation: `date`"
