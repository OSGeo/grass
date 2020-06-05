#!/bin/sh
package=0

# stop on errors
set -e

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

if [ $package -eq 1 ]; then
	# package

	opt_path=$osgeo4w_path/opt
	grass_path=$opt_path/grass
	date=`date +%Y%m%d`
	zip=$src/grass$ver-$arch-osgeo4w64-$date.zip

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

	cd $osgeo4w_path/..
	osgeo4w_basename=`basename $osgeo4w_path`
	zip -r $zip $osgeo4w_basename \
	    -x "$osgeo4w_basename/var/*" "*/__pycache__/*"
else
	# create batch files

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
	unix2dos $dist/etc/env.bat

	(
	sed -e 's/^\(call "%~dp0\)\(.*\)$/\1\\..\\..\\bin\2/' \
	    -e 's/^\(call "\).*\(\\etc\\env\.bat"\)$/\1'$dist_win_esc'\2/' \
	    -e 's/@POSTFIX@/'$ver'/g' \
	    mswindows/osgeo4w/grass.bat.tmpl
	) > $bin/grass$ver.bat
	unix2dos $bin/grass$ver.bat
fi
