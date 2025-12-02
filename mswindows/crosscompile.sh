#!/bin/sh
#
################################################################################
#
# MODULE:	crosscompile.sh
# AUTHOR(S):	Huidae Cho <grass4u gmail.com>
# PURPOSE:	Builds a cross-compiled portable package of GRASS
# COPYRIGHT:	(C) 2019-2021 by Huidae Cho and the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
################################################################################
#
# This script requires MXE <https://mxe.cc/> for cross-compilation and was
# tested on Slackware 14.2 x86_64 with up-to-date packages from slackpkg and
# sbopkg. It was also tested on WSLackware
# <https://github.com/Mohsens22/WSLackware> in WSL
# <https://docs.microsoft.com/en-us/windows/wsl/>.
#
# Basic steps:
#
# mkdir -p ~/usr/src
# cd ~/usr/src
# git clone https://github.com/mxe/mxe.git
# cd mxe
# echo MXE_TARGETS=x86_64-w64-mingw32.shared > settings.mk
# make cc blas bzip2 cairo fftw freetype gdal geos lapack netcdf libpng \
#      pthreads readline libgnurx sqlite tiff zstd proj
#
# cd ~/usr/src
# git clone https://github.com/OSGeo/grass.git
# git clone https://github.com/OSGeo/grass-addons.git
# cd grass
# mswindows/crosscompile.sh --mxe-path=$HOME/usr/src/mxe --update --package \
#      > crosscompile.log 2>&1
#

# stop on errors
set -e

# default paths, but can be overridden from the command line
mxe_path=${MXE_PATH-$HOME/usr/local/src/mxe}
addons_path=${ADDONS_PATH-../grass-addons}
freetype_include=${FREETYPE_INCLUDE-/usr/include/freetype2}

# process options
update=0
package=0
for opt; do
	case "$opt" in
	-h|--help)
		cat<<'EOT'
Usage: crosscompile.sh [OPTIONS]

-h, --help                   display this help message
    --mxe-path=PATH          MXE path (default: $HOME/usr/local/src/mxe)
    --addons-path=PATH       grass-addons path (default: ../grass-addons)
    --freetype-include=PATH  FreeType include path
                             (default: /usr/include/freetype2)
    --update                 update the current branch
    --package                package the cross-compiled build as
                             grassVV-x86_64-w64-mingw32-YYYYMMDD.zip
EOT
		exit
		;;
	--mxe-path=*)
		mxe_path=`echo $opt | sed 's/^[^=]*=//'`
		;;
	--addons-path=*)
		addons_path=`echo $opt | sed 's/^[^=]*=//'`
		;;
	--freetype-include=*)
		freetype_include=`echo $opt | sed 's/^[^=]*=//'`
		;;
	--update)
		update=1
		;;
	--package)
		package=1
		;;
	*)
		echo "$opt: unknown option"
		exit 1
		;;
	esac
done

# see if we're inside the root of the GRASS source code
if [ ! -f grass.pc.in ]; then
	echo "Please run this script from the root of the GRASS source code"
	exit 1
fi

# check paths
errors=0
if [ ! -d $mxe_path ]; then
	echo "$mxe_path: not found"
	errors=1
fi
if [ ! -d $freetype_include ]; then
	echo "$freetype_include: not found"
	errors=1
fi
if [ $update -eq 1 -a ! -d .git ]; then
	echo "not a git repository"
	errors=1
fi
if [ $errors -eq 1 ]; then
	exit 1
fi

################################################################################
# Start

echo "Started cross-compilation: `date`"
echo

# update the current branch if requested
if [ $update -eq 1 -a -d .git ]; then
	git pull
fi

################################################################################
# Compile the native architecture for generating document files

CFLAGS="-g -O2 -Wall" \
CXXFLAGS="-g -O2 -Wall" \
LDFLAGS="-lcurses" \
./configure \
--with-blas \
--with-bzlib \
--with-freetype-includes=$freetype_include \
--with-geos \
--with-lapack \
--with-netcdf \
--with-nls \
--with-openmp \
--with-postgres \
--with-pthread \
--with-readline \
>> /dev/stdout

make clean default

if [ -d $addons_path ]; then
	MODULE_TOPDIR=`pwd`
	(
	cd $addons_path
	if [ $update -eq 1 -a -d .git ]; then
		git pull
	fi
	cd src
	make MODULE_TOPDIR=$MODULE_TOPDIR clean default
	)
fi

build_arch=`sed -n '/^ARCH[ \t]*=/{s/^.*=[ \t]*//; p}' include/Make/Platform.make`
for i in \
	config.log \
	error.log \
	include/Make/Doxyfile_arch_html \
	include/Make/Doxyfile_arch_latex \
	include/Make/Platform.make \
; do
	cp -a $i $i.$build_arch
done

################################################################################
# Cross-compile the target architecture

arch=x86_64-w64-mingw32
shared=$arch.shared
mxe_bin=$mxe_path/usr/bin/$shared
mxe_shared=$mxe_path/usr/$shared

CC=$mxe_bin-gcc \
CXX=$mxe_bin-g++ \
CFLAGS="-g -O2 -Wall" \
CXXFLAGS="-g -O2 -Wall" \
AR=$mxe_bin-ar \
RANLIB=$mxe_bin-ranlib \
WINDRES=$mxe_bin-windres \
PKG_CONFIG=$mxe_bin-pkg-config \
./configure \
--build=$build_arch \
--host=$arch \
--with-blas \
--with-bzlib \
--with-freetype-includes=$mxe_shared/include/freetype2 \
--with-gdal=$mxe_shared/bin/gdal-config \
--with-geos=$mxe_shared/bin/geos-config \
--with-lapack \
--with-netcdf=$mxe_shared/bin/nc-config \
--with-nls \
--with-opengl=windows \
--with-openmp \
--with-postgres \
--with-pthread \
--with-readline \
>> /dev/stdout

make clean default

if [ -d $addons_path ]; then
	(
	cd $addons_path
	if [ $update -eq 1 -a -d .git ]; then
		git pull
	fi
	cd src
	make MODULE_TOPDIR=$MODULE_TOPDIR clean default
	)
fi

arch=`sed -n '/^ARCH[ \t]*=/{s/^.*=[ \t]*//; p}' include/Make/Platform.make`
for i in \
	config.log \
	error.log \
	include/Make/Doxyfile_arch_html \
	include/Make/Doxyfile_arch_latex \
	include/Make/Platform.make \
; do
	cp -a $i $i.$arch
done

################################################################################
# Copy document files from the native build

build_dist=dist.$build_arch
dist=dist.$arch

for i in \
	docs \
	gui/wxpython/xml \
; do
	rm -rf $dist/$i
	cp -a $build_dist/$i $dist/$i
done

################################################################################
# Copy MXE files

for i in \
	libblas.dll \
	libbz2.dll \
	libcairo-2.dll \
	libcrypto-3-x64.dll \
	libcurl-4.dll \
	libdf-0.dll \
	libexpat-1.dll \
	libfftw3-3.dll \
	libfontconfig-1.dll \
	libfreetype-6.dll \
	libfreexl-1.dll \
	libgcc_s_seh-1.dll \
	libgcrypt-20.dll \
	libgdal-20.dll \
	libgeos-3-6-2.dll \
	libgeos_c-1.dll \
	libgeotiff-2.dll \
	libgfortran-3.dll \
	libgif-7.dll \
	libglib-2.0-0.dll \
	libgnurx-0.dll \
	libgomp-1.dll \
	libgpg-error-0.dll \
	libgta-0.dll \
	libharfbuzz-0.dll \
	libhdf5-8.dll \
	libhdf5_hl-8.dll \
	libiconv-2.dll \
	libidn2-0.dll \
	libintl-8.dll \
	libjpeg-9.dll \
	libjson-c-4.dll \
	liblapack.dll \
	liblzma-5.dll \
	libmfhdf-0.dll \
	libmysqlclient.dll \
	libnetcdf.dll \
	libopenjp2.dll \
	libpcre-1.dll \
	libpixman-1-0.dll \
	libpng16-16.dll \
	libportablexdr-0.dll \
	libpq.dll \
	libproj-13.dll \
	libquadmath-0.dll \
	libreadline8.dll \
	libspatialite-7.dll \
	libsqlite3-0.dll \
	libssh2-1.dll \
	libssl-3-x64.dll \
	libstdc++-6.dll \
	libtermcap.dll \
	libtiff-5.dll \
	libunistring-2.dll \
	libwebp-7.dll \
	libwinpthread-1.dll \
	libxml2-2.dll \
	libzstd.dll \
	zlib1.dll \
; do
	cp -a $mxe_shared/bin/$i $dist/lib
done

for i in \
	gdal \
	proj \
; do
	rm -rf $dist/share/$i
	cp -a $mxe_shared/share/$i $dist/share/$i
done

################################################################################
# Post-compile process

version=`sed -n '/^INST_DIR[ \t]*=/{s/^.*grass//; p}' include/Make/Platform.make`

rm -f $dist/grass.tmp
cp -a bin.$arch/grass.py $dist/etc/grass$version.py

cat<<'EOT' | sed "s/\$version/$version/g" > $dist/grass.bat
@echo off
setlocal EnableDelayedExpansion

set GISBASE=%~dp0
set GISBASE=%GISBASE:~0,-1%

set GRASS_PROJSHARE=%GISBASE%\share\proj

set PROJ_LIB=%GISBASE%\share\proj
set GDAL_DATA=%GISBASE%\share\gdal

rem XXX: Do we need these variables?
rem set GEOTIFF_CSV=%GISBASE%\share\epsg_csv
rem set FONTCONFIG_FILE=%GISBASE%\etc\fonts.conf

if defined GRASS_PYTHON (
	if not exist "%GRASS_PYTHON%" (
		echo.
		echo %GRASS_PYTHON% not found
		echo Please fix GRASS_PYTHON
		echo.
		pause
		goto :eof
	)
) else (
	rem Change this variable to override auto-detection of python.exe in
	rem PATH
	set GRASS_PYTHON=C:\Python312\python.exe

	rem For portable installation, use %~d0 for the changing drive letter
	rem set GRASS_PYTHON=%~d0\Python312\python.exe

	if not exist "%GRASS_PYTHON%" (
		set GRASS_PYTHON=
		for /f usebackq %%i in (`where python.exe`) do if "!GRASS_PYTHON!" == "" set GRASS_PYTHON=%%i
	)
	if not defined GRASS_PYTHON (
		echo.
		echo python.exe not found in PATH
		echo Please set GRASS_PYTHON
		echo.
		pause
		goto :eof
	)
)
rem XXX: Do we need PYTHONHOME?
rem for %%i in (%GRASS_PYTHON%) do set PYTHONHOME=%%~dpi

rem If GRASS_SH is externally defined, that shell will be used; Otherwise,
rem GISBASE\etc\sh.bat will be used if it exists; If not, cmd.exe will be used;
rem This check is mainly for supporting BusyBox for Windows (busybox64.exe)
rem (https://frippery.org/busybox/)
if not defined GRASS_SH (
	set GRASS_SH=%GISBASE%\etc\sh.bat
	if not exist "!GRASS_SH!" set GRASS_SH=
)

rem With busybox64.exe and Firefox as the default browser, g.manual fails with
rem "Your Firefox profile cannot be loaded. It may be missing or inaccessible";
rem I tried to set GRASS_HTML_BROWSER to the full path of chrome.exe, but it
rem didn't work; Setting BROWSER to its full path according to the webbrowser
rem manual worked
if "%GRASS_SH%" == "%GISBASE%\etc\sh.bat" if not defined BROWSER (
	for %%i in ("%ProgramFiles%" "%ProgramFiles(x86)%") do (
		if not defined BROWSER (
			set BROWSER=%%i
			set BROWSER=!BROWSER:"=!
			if exist "!BROWSER!\Google\Chrome\Application\chrome.exe" (
				set BROWSER=!BROWSER!\Google\Chrome\Application\chrome.exe
			) else (
				set BROWSER=
			)
		)
	)
)

if not exist "%GISBASE%\etc\fontcap" (
	pushd .
	%~d0
	cd %GISBASE%\lib
	set GISRC=dummy
	"%GISBASE%\bin\g.mkfontcap.exe"
	popd
)

"%GRASS_PYTHON%" "%GISBASE%\etc\grass$version.py" %*
EOT
unix2dos $dist/grass.bat

# package if requested
if [ $package -eq 1 ]; then
	date=`date +%Y%m%d`
	rm -f grass
	ln -s $dist grass
	rm -f grass*-$arch-*.zip
	zip -r grass$version-$arch-$date.zip grass
	rm -f grass
fi

echo
echo "Completed cross-compilation: `date`"
