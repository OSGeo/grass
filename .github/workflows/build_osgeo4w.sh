#!/bin/sh

# stop on errors
set -e

test -d "$1" && cd "$1"

osgeo4w_path=/c/OSGeo4W64
arch=x86_64-w64-mingw32
src=`pwd`

# start
echo "Started compilation: `date`"
echo

# compile

export PATH=/mingw64/bin:/c/OSGeo4W64/bin:/usr/bin
export PROJ_LIB=$osgeo4w_path/share/proj

OSGEO4W_ROOT_MSYS=$osgeo4w_path \
./configure \
--host=$arch \
--with-includes=$osgeo4w_path/include \
--with-libs="$osgeo4w_path/lib $osgeo4w_path/bin" \
--with-nls \
--with-freetype-includes=$osgeo4w_path/include/freetype2 \
--with-bzlib \
--with-geos=$src/mswindows/osgeo4w/geos-config \
--with-netcdf=$osgeo4w_path/bin/nc-config \
--with-gdal=$src/mswindows/osgeo4w/gdal-config \
--with-liblas=$src/mswindows/osgeo4w/liblas-config \
--with-opengl=windows

make

echo
echo "Completed compilation: `date`"

# install

bin=bin.$arch
dist=dist.$arch
version=`sed -n '/^INST_DIR[ \t]*=/{s/^.*grass//; p}' include/Make/Platform.make`
src_win_esc=`echo $src | sed 's#^/\([a-z]\)#\1:#; s#/#\\\\\\\\#g'`
bin_win_esc="$src_win_esc\\$bin"
dist_win_esc="$src_win_esc\\$dist"

(
sed 's/^\(set GISBASE=\).*/\1'$dist_win_esc'/' \
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
) > $dist/etc/env.bat

(
sed -e 's/^\(call "%~dp0\)\(.*\)$/\1\\..\\..\\bin\2/' \
    -e 's/^\(call "\).*\(\\etc\\env\.bat"\)$/\1'$dist_win_esc'\2/' \
    -e 's/@POSTFIX@/'$version'/g' \
    mswindows/osgeo4w/grass.bat.tmpl
) > $bin/grass$version.bat

echo env.bat
cat $dist/etc/env.bat

echo grass.bat
cat $bin/grass$version.bat
