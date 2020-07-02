#!/bin/sh
#
# Usage: build_osgeo4w.sh [-p] [path]
#
# By default, the script will look for the source code in the current directory
# and create bin.x86_64-w64-mingw32\grass$ver.bat (run this batch file to start
# GRASS GIS) and dist.x86_64-w64-mingw32\etc\env.bat.
#
# -p	optionally install GRASS GIS to C:\OSGeo4W64\opt\grass (run
#	C:\OSGeo4W64\opt\grass\grass$ver.bat) and create an unzippable package
#	grass$ver-x86_64-w64-mingw32-osgeo4w64-$date.zip
#
# path	specify a path to the source code
#

# stop on errors
set -e

if [ "$1" = "-p" ]; then
	package=1
	shift
else
	package=0
fi

test -d "$1" && cd "$1"

osgeo4w_path=/c/OSGeo4W64
arch=x86_64-w64-mingw32
src=`pwd`

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

# install

bin=bin.$arch
dist=dist.$arch
ver=`sed -n '/^INST_DIR[ \t]*=/{s/^.*grass//; p}' include/Make/Platform.make`

rm -f $dist/grass$ver.tmp $dist/etc/fontcap

# create batch files

src_esc=`echo $src | sed 's#^/\([a-z]\)#\1:#; s#/#\\\\\\\\#g'`
dist_esc="$src_esc\\\\$dist"

(
sed 's/^\(set GISBASE=\).*/\1'$dist_esc'/' \
    mswindows/osgeo4w/env.bat.tmpl
cat<<EOT

set PATH=C:\\msys64\\mingw64\\bin;%OSGEO4W_ROOT%\\apps\\msys\\bin;%PATH%

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
unix2dos $dist/etc/env.bat

(
sed -e 's/^\(call "\)%~dp0\(.*\)$/\1C:\\OSGeo4W64\\bin\2/' \
    -e 's/^\(call "\).*\(\\etc\\env\.bat"\)$/\1'$dist_esc'\2/' \
    -e 's/^\(.* "\)%GISBASE%\\etc\(\\grass.*\)$/\1%GISBASE%\\..\\'$bin'\2/' \
    -e 's/@POSTFIX@/'$ver'/g' \
    mswindows/osgeo4w/grass.bat.tmpl
) > $bin/grass$ver.bat
unix2dos $bin/grass$ver.bat

test $package -eq 0 && exit

# package

opt_path=$osgeo4w_path/opt
grass_path=$opt_path/grass

mkdir -p $opt_path
cp -a $dist $grass_path
cp -a $bin/grass$ver.py $grass_path/etc
cp -a `ldd $dist/lib/*.dll | awk '/mingw64/{print $3}' |
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
    -e 's/@POSTFIX@/'$ver'/g' \
    mswindows/osgeo4w/grass.bat.tmpl
) > $grass_path/grass$ver.bat
unix2dos $grass_path/grass$ver.bat

exit

# don't package for GitHub workflow; unnecessary

osgeo4w_basename=`basename $osgeo4w_path`
date=`date +%Y%m%d`
zip=$src/grass$ver-$arch-osgeo4w64-$date.zip

cd $osgeo4w_path/..
zip -r $zip $osgeo4w_basename -x "$osgeo4w_basename/var/*" "*/__pycache__/*"
